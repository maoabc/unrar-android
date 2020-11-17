//
// Created by mao on 17-6-4.
//
#include "jni.h"
#include "rar.hpp"
#include "dll.hpp"

#include "rar_file.h"
#include "ScopedLocalRef.h"

#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

extern JavaVM *javaVM;

static JNIEnv *getJNIEnv() {
    JNIEnv *env;
    if (javaVM->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return nullptr;
    }
    return env;
}

static jmethodID callbackProcessDataMethod;
static jmethodID callbackNeedPasswordMethod;

static jclass entryClass;
static jmethodID entryCtor;

static void ThrowExceptionByName(JNIEnv *env, const char *name, const char *msg) {
    ScopedLocalRef<jclass> cls(env, env->FindClass(name));
    if (cls.get() != nullptr)
        env->ThrowNew(cls.get(), msg);
}
static void ThrowIOException(JNIEnv *env, const char *msg) {
    ThrowExceptionByName(env, "java/io/IOException", msg);
}

void initIDs(JNIEnv *env) {

    entryClass = static_cast<jclass>(
            env->NewGlobalRef(env->FindClass("com/github/maoabc/unrar/RarEntry")));
    if (entryClass == nullptr) {
        return;
    }

    ScopedLocalRef<jclass> callback_cls(env,
                                        env->FindClass("com/github/maoabc/unrar/UnrarCallback"));
    if (callback_cls.get() == nullptr) {
        return;
    }

    callbackProcessDataMethod = env->GetMethodID(callback_cls.get(), "processData",
                                                 "(Ljava/nio/ByteBuffer;I)V");

    callbackNeedPasswordMethod = env->GetMethodID(callback_cls.get(), "needPassword",
                                                  "()Ljava/lang/String;");


    entryCtor = env->GetMethodID(entryClass, "<init>", "(Ljava/lang/String;JJJJI)V");

}

enum {
    MIN_HIGH_SURROGATE = 0xD800U,
    MAX_HIGH_SURROGATE = 0xDBFFU,
    MIN_LOW_SURROGATE = 0xDC00U,
    MAX_LOW_SURROGATE = 0xDFFFU,
    MIN_SUPPLEMENTARY_CODE_POINT = 0x010000U,
    MAX_CODE_POINT = 0X10FFFFU
};


constexpr static bool IsHighSurrogate(jchar ch) {
    return ch >= MIN_HIGH_SURROGATE && ch <= MAX_HIGH_SURROGATE;
}

constexpr static bool IsLowSurrogate(jchar ch) {
    return ch >= MIN_LOW_SURROGATE && ch <= MAX_LOW_SURROGATE;
}

constexpr static jchar HighSurrogate(wchar_t wc) {
    return static_cast<jchar>((wc >> 10)
                              + (MIN_HIGH_SURROGATE - (MIN_SUPPLEMENTARY_CODE_POINT >> 10)));
}

constexpr static jchar LowSurrogate(wchar_t wc) {
    return static_cast<jchar>((wc & 0x3ff) + MIN_LOW_SURROGATE);
}

inline size_t Utf16ToUtf32(wchar_t *dest, const jchar *src, size_t len) {
    if (dest == nullptr || src == nullptr)
        return 0;
    size_t i = 0;
    size_t j = 0;
    for (; i < len && src[i] != '\0'; i++, j++) {
        jchar jc = src[i];
        if (IsHighSurrogate(jc) && i + 1 < len) {
            jchar jc2 = src[i + 1];
            if (IsLowSurrogate(jc2)) {
                dest[j] = ((jc << 10) + jc2) + (MIN_SUPPLEMENTARY_CODE_POINT
                                                - (MIN_HIGH_SURROGATE << 10)
                                                - MIN_LOW_SURROGATE);
                i++;
            } else {
                dest[j] = jc;
            }
        } else {
            dest[j] = jc;
        }

    }
    dest[j] = '\0';

    return j;
}
inline size_t Utf32ToUtf16(jchar *dest, const wchar_t *src, size_t len) {
    if (dest == nullptr || src == nullptr)
        return 0;

    size_t i = 0;
    size_t j = 0;
    for (; i < len && src[i] != '\0'; i++, j++) {
        wchar_t cp = src[i];
        if ((cp >> 16) == 0) {
            dest[j] = (jchar) cp;
        } else if ((cp >> 16) < ((MAX_CODE_POINT + 1) >> 16)) {
            dest[j] = HighSurrogate(cp);
            dest[j + 1] = LowSurrogate(cp);
            j++;
        }
    }
    dest[j] = '\0';
    return j;
}

int CALLBACK callbackFunc(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2) {
    auto callback = (jobject) UserData;
    switch (msg) {
        case UCM_PROCESSDATA: {
            JNIEnv *env = getJNIEnv();
            auto size = (jsize) P2;
            jobject byteBuffer = env->NewDirectByteBuffer(reinterpret_cast<void *>(P1), size);

            env->CallVoidMethod(callback, callbackProcessDataMethod, byteBuffer, size);
            if (env->ExceptionCheck()) {
                env->ExceptionClear();
                return -1;
            }

            return 1;
        }
        case UCM_NEEDPASSWORDW: {
            JNIEnv *env = getJNIEnv();
            ScopedLocalRef<jstring> jpassword(env,
                                              (jstring) env->CallObjectMethod(callback,
                                                                              callbackNeedPasswordMethod));
            if (jpassword.get() != nullptr) {
                auto *password = (wchar_t *) P1;
                const jchar *chars = env->GetStringChars(jpassword.get(), nullptr);
                Utf16ToUtf32(password, chars,
                             static_cast<size_t >(Min(env->GetStringLength(jpassword.get()),
                                                      P2)));
                //保证字符串正确终止
                password[P2 - 1] = '\0';
                env->ReleaseStringChars(jpassword.get(), chars);
                return 1;
            }
            return -1;
        }
        case UCM_CHANGEVOLUMEW: {
            if (P2 == RAR_VOL_NOTIFY) {
                return 0;
            }
            //TODO 缺少卷调用,回调java层
//            LOGI("volume %d", P2);
            return -1;
        }
        default:
            break;
    }
    return 1;
}

static jlong Java_com_github_maoabc_unrar_RarFile_openArchive0
        (JNIEnv *env, jclass jcls, jstring jfilePath, jint mode, jintArray flags) {
    wchar_t nameW[NM];
    struct RAROpenArchiveDataEx data{};

    memset(nameW, 0, sizeof(wchar_t) * NM);

    const jchar *path = env->GetStringChars(jfilePath, nullptr);

    Utf16ToUtf32(nameW, path, (size_t) env->GetStringLength(jfilePath));

    env->ReleaseStringChars(jfilePath, path);

    data.ArcNameW = &nameW[0];
    data.OpenMode = (unsigned int) mode;

    HANDLE handle = RAROpenArchiveEx(&data);

    if (handle == nullptr || data.OpenResult) {
        if (handle) {
            RARCloseArchive(handle);
        }
        char err_str[128];
        sprintf(err_str, "ErrorCode: %d", data.OpenResult);
        ThrowExceptionByName(env, "com/github/maoabc/unrar/RarException", err_str);
        return 0;
    }
    if (flags) {
        env->SetIntArrayRegion(flags, 0, 1, reinterpret_cast<const jint *>(&data.Flags));
    }
    return reinterpret_cast<jlong>(handle);
}


static jobject Java_com_github_maoabc_unrar_RarFile_readHeader0
        (JNIEnv *env, jclass jcls, jlong jhandle, jobject callback) {


    HANDLE handle = reinterpret_cast<void *>(jhandle);
    jobject globalCallback;
    if (callback != nullptr) {
        globalCallback = env->NewGlobalRef(callback);
        //需要密码回调
        RARSetCallback(handle, callbackFunc, (LPARAM) globalCallback);
    } else {
        globalCallback = nullptr;
        RARSetCallback(handle, nullptr, (LPARAM) nullptr);
    }


    struct RARHeaderDataEx header{};
    if (RARReadHeaderEx(handle, &header)) {
        return nullptr;
    }

    if (globalCallback) {
        env->DeleteGlobalRef(globalCallback);
    }

    jchar name[2048];
    size_t len = Utf32ToUtf16(name, header.FileNameW, wcslen(header.FileNameW));

    ScopedLocalRef<jstring> jname(env, env->NewString(name, (jsize) len));


    return env->NewObject(entryClass, entryCtor, jname.get(),
                          (((uint64_t) header.UnpSizeHigh) << 32) | header.UnpSize,
                          (((uint64_t) header.PackSizeHigh) << 32) | header.PackSize,
                          header.FileCRC, (header.FileTime), header.Flags);
}

#define MAXBUF (1024*64)
static void Java_com_github_maoabc_unrar_RarFile_processFile0
        (JNIEnv *env, jclass jcls, jlong jhandle, jint operation, jstring jdestPath,
         jstring jdestName, jobject callback) {
    HANDLE handle = reinterpret_cast<void *>(jhandle);

    wchar_t destPath[NM];
    wchar_t destName[NM];

    memset(destPath, 0, sizeof(wchar_t) * NM);
    memset(destName, 0, sizeof(wchar_t) * NM);

    if (jdestPath != nullptr) {
        const jchar *chars = env->GetStringChars(jdestPath, nullptr);
        Utf16ToUtf32(destPath, chars, static_cast<size_t>(env->GetStringLength(jdestPath)));
        env->ReleaseStringChars(jdestPath, chars);
    }

    if (jdestName != nullptr) {
        const jchar *chars = env->GetStringChars(jdestName, nullptr);
        Utf16ToUtf32(destName, chars, static_cast<size_t>(env->GetStringLength(jdestName)));
        env->ReleaseStringChars(jdestName, chars);
    }

    jobject globalCallback;
    if (callback != nullptr) {//UCM_PROCESSDATA
        globalCallback = env->NewGlobalRef(callback);

        RARSetCallback(handle, callbackFunc, (LPARAM) globalCallback);
    } else {
        globalCallback = nullptr;
        RARSetCallback(handle, nullptr, (LPARAM) nullptr);
    }
    int code = RARProcessFileW(handle, operation, destPath, destName);

    if (globalCallback) {
        env->DeleteGlobalRef(globalCallback);
    }
    //检查异常
    switch (code) {
        case ERAR_SUCCESS:
            break;
        case ERAR_BAD_PASSWORD:
            ThrowIOException(env, "Bad password");
            break;
        case ERAR_MISSING_PASSWORD:
            ThrowIOException(env, "Missing password");
            break;
        default:
            LOGI("operation %d,process result %d", operation, code);
            ThrowIOException(env, "");
    }
}

static void Java_com_github_maoabc_unrar_RarFile_closeArchive0
        (JNIEnv *env, jclass jcls, jlong jhandle) {
    HANDLE handle = reinterpret_cast<void *>(jhandle);
    if (RARCloseArchive(handle) != ERAR_SUCCESS) {
        ThrowIOException(env, "close error");
    }
}


static JNINativeMethod methods[] = {
        {"openArchive0",  "(Ljava/lang/String;I[I)J",                                                         (void *) Java_com_github_maoabc_unrar_RarFile_openArchive0},

        {"readHeader0",   "(JLcom/github/maoabc/unrar/UnrarCallback;)Lcom/github/maoabc/unrar/RarEntry;",     (void *) Java_com_github_maoabc_unrar_RarFile_readHeader0},

        {"processFile0",  "(JILjava/lang/String;Ljava/lang/String;Lcom/github/maoabc/unrar/UnrarCallback;)V", (void *) Java_com_github_maoabc_unrar_RarFile_processFile0},

        {"closeArchive0", "(J)V",                                                                             (void *) Java_com_github_maoabc_unrar_RarFile_closeArchive0},

};

#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

jboolean registerNativeMethods(JNIEnv *env) {
    ScopedLocalRef<jclass> clazz(env, env->FindClass("com/github/maoabc/unrar/RarFile"));
    if (clazz.get() == nullptr) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz.get(), methods, NELEM(methods)) < 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


#ifdef __cplusplus
}
#endif




