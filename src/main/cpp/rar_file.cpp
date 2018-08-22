//
// Created by mao on 17-6-4.
//
#include "jni.h"
#include <string.h>
#include "rar.hpp"
#include "dll.hpp"
#include "jni_util.h"

#include <android/log.h>

#define  LOG_TAG    "libunrar-jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif
struct user_data {
    JNIEnv *env;
    jobject callback;
    jbyteArray readbuf;
};
static jfieldID entry_field_name;
static jfieldID entry_field_size;
static jfieldID entry_field_csize;
static jfieldID entry_field_mtime;
static jfieldID entry_field_crc;
static jfieldID entry_field_flags;
static jmethodID entry_method_constructor;

static jmethodID callback_process_data;
static jmethodID callback_need_password;

JNIEXPORT void JNICALL Java_mao_archive_unrar_RarFile_initIDs
        (JNIEnv *env, jclass jcls) {
    jclass entry_cls = NULL;
    jclass callback_cls = NULL;

    if (env->EnsureLocalCapacity(2) < 0)
        goto done;

    entry_cls = env->FindClass("mao/archive/unrar/RarEntry");
    if (entry_cls == NULL)
        goto done;


    callback_cls = env->FindClass("mao/archive/unrar/UnrarCallback");
    if (callback_cls == NULL)
        goto done;

    callback_process_data = env->GetMethodID(callback_cls, "processData", "([BII)Z");

    callback_need_password = env->GetMethodID(callback_cls, "needPassword", "()Ljava/lang/String;");

    entry_field_name = env->GetFieldID(entry_cls, "name", "Ljava/lang/String;");
    entry_field_mtime = env->GetFieldID(entry_cls, "mtime", "J");
    entry_field_size = env->GetFieldID(entry_cls, "size", "J");
    entry_field_csize = env->GetFieldID(entry_cls, "csize", "J");
    entry_field_crc = env->GetFieldID(entry_cls, "crc", "J");
    entry_field_flags = env->GetFieldID(entry_cls, "flags", "I");

    entry_method_constructor = env->GetMethodID(entry_cls, "<init>", "()V");
    done:
    env->DeleteLocalRef(entry_cls);
    env->DeleteLocalRef(callback_cls);
}
inline wchar_t *jcharntowcs(wchar *dest, const jchar *src, size_t len) {
    if (dest == NULL || src == NULL)
        return NULL;
    size_t i = 0;
    for (; i < len && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i <= len; i++)
        dest[i] = '\0';

    return dest;
}
inline jchar *wcsntojchar(jchar *dest, const wchar *src, size_t len) {
    if (dest == NULL || src == NULL)
        return NULL;
    size_t i = 0;
    for (; i < len && src[i] != '\0'; i++) {
        dest[i] = (jchar) src[i];
    }
    for (; i <= len; i++)
        dest[i] = '\0';
    return dest;
}

int CALLBACK callbackFunc(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2) {
    struct user_data *data = (struct user_data *) UserData;
    switch (msg) {
        case UCM_PROCESSDATA: {
            JNIEnv *env = data->env;
            const jbyte *buf = (const jbyte *) P1;

            for (jsize readed = 0, rem = (jsize) P2, len = env->GetArrayLength(data->readbuf);
                 rem > 0; readed += len, rem -= len) {
                if (len > rem) {
                    len = rem;
                }
                env->SetByteArrayRegion(data->readbuf, 0, len, buf + readed);

                if (!env->CallBooleanMethod(data->callback, callback_process_data,
                                            data->readbuf, 0, len)) {
                    return -1;

                }

                if (env->ExceptionCheck()) {
                    return -1;
                }
            }
            return 1;
        }
        case UCM_NEEDPASSWORDW: {
            LOGI("need password");
            JNIEnv *env = data->env;
            jstring jpassword = (jstring) env->CallObjectMethod(data->callback,
                                                                callback_need_password);
            if (jpassword != NULL) {
                wchar *password = (wchar *) P1;
                const jchar *chars = env->GetStringChars(jpassword, 0);
                jcharntowcs(password, chars, Min(env->GetStringLength(jpassword), P2));
                //保证字符串正确终止
                password[P2 - 1] = '\0';
                env->ReleaseStringChars(jpassword, chars);
                return 1;
            }
            return -1;
        }
        case UCM_CHANGEVOLUMEW: {
            //TODO 缺少卷调用回调java层
            LOGI("volume %d", P2);
            return -1;
        }
        default:
            break;
    }
    return 1;
}

JNIEXPORT jlong JNICALL Java_mao_archive_unrar_RarFile_openArchive
        (JNIEnv *env, jclass jcls, jstring jfilePath, jint mode) {
    wchar nameW[NM];
    struct RAROpenArchiveDataEx data;

    memset(&data, 0, sizeof(data));

    const jchar *path = env->GetStringChars(jfilePath, 0);
    jcharntowcs(nameW, path, (size_t) env->GetStringLength(jfilePath));
    env->ReleaseStringChars(jfilePath, path);

    data.ArcNameW = nameW;
    data.OpenMode = (unsigned int) mode;

    HANDLE handle = RAROpenArchiveEx(&data);

    if (handle == NULL || data.OpenResult) {
        if (handle) {
            RARCloseArchive(handle);
        }
        char err_str[128];
        sprintf(err_str, "ErrorCode: %d", data.OpenResult);
        JNU_ThrowByName(env, "mao/archive/unrar/RarException", err_str);
        return 0;
    }
    return ptr_to_jlong(handle);
}


JNIEXPORT jobject JNICALL Java_mao_archive_unrar_RarFile_readHeader0
        (JNIEnv *env, jclass jcls, jlong jhandle, jobject callback) {
    jobject entry = NULL;
    jclass entry_cls = NULL;

    HANDLE handle = jlong_to_ptr(jhandle);

    if (callback != NULL) {
        struct user_data userData;
        userData.env = env;
        userData.callback = callback;
        //需要密码回调
        RARSetCallback(handle, callbackFunc, (LPARAM) &userData);
    }


    struct RARHeaderDataEx header;
    memset(&header, 0, sizeof(struct RARHeaderDataEx));
    if (RARReadHeaderEx(handle, &header)) {
        return NULL;
    }

    if (env->EnsureLocalCapacity(2) < 0)
        return NULL;
    entry_cls = env->FindClass("mao/archive/unrar/RarEntry");
    if (entry_cls == NULL) {
        return NULL;
    }

    entry = env->NewObject(entry_cls, entry_method_constructor);
    env->DeleteLocalRef(entry_cls);
    if (entry == NULL) {
        return NULL;
    }

    jchar name[1024];
    size_t len = wcslen(header.FileNameW);
    wcsntojchar(name, header.FileNameW, len);

    jstring string = env->NewString(name, (jsize) len);
    env->SetObjectField(entry, entry_field_name, string);

    env->SetLongField(entry, entry_field_size,
                      (((int64) header.UnpSizeHigh) << 32) | header.UnpSize);
    env->SetLongField(entry, entry_field_csize,
                      (((int64) header.PackSizeHigh) << 32) | header.PackSize);
    env->SetLongField(entry, entry_field_mtime, ((int64) header.FileTime));
    env->SetLongField(entry, entry_field_crc, header.FileCRC);
    env->SetIntField(entry, entry_field_flags, header.Flags);

    return entry;
}

JNIEXPORT void JNICALL Java_mao_archive_unrar_RarFile_processFile0
        (JNIEnv *env, jclass jcls, jlong jhandle, jint operation, jstring jdestPath,
         jstring jdestName, jobject callback) {
#define MAXBUF 1024*16
    HANDLE handle = jlong_to_ptr(jhandle);
    jbyteArray readbuf = NULL;

    wchar destPath[NM], destName[NM];

    memset(destPath, 0, sizeof(wchar) * NM);
    memset(destName, 0, sizeof(wchar) * NM);

    if (jdestPath != NULL) {
        const jchar *chars = env->GetStringChars(jdestPath, 0);
        jcharntowcs(destPath, chars, (size_t) env->GetStringLength(jdestPath));
        env->ReleaseStringChars(jdestPath, chars);
    }

    if (jdestName != NULL) {
        const jchar *chars = env->GetStringChars(jdestName, 0);
        jcharntowcs(destName, chars, (size_t) env->GetStringLength(jdestName));
        env->ReleaseStringChars(jdestName, chars);
    }

    if (callback != NULL) {//UCM_PROCESSDATA
        readbuf = env->NewByteArray(MAXBUF);

        struct user_data userData;
        userData.env = env;
        userData.callback = callback;
        userData.readbuf = readbuf;
        RARSetCallback(handle, callbackFunc, (LPARAM) &userData);
    }
    int code = RARProcessFileW(handle, operation, destPath, destName);

    if (readbuf != NULL) {
        env->DeleteLocalRef(readbuf);
    }
    LOGI("operation %d,process result %d", operation, code);
    //检查异常
    switch (code) {
        case ERAR_SUCCESS:
            break;
        case ERAR_BAD_PASSWORD:
            JNU_ThrowIOException(env, "Bad password");
            break;
        case ERAR_MISSING_PASSWORD:
            JNU_ThrowIOException(env, "Missing password");
            break;
        default:
            JNU_ThrowIOException(env, "");
    }
}

JNIEXPORT void JNICALL Java_mao_archive_unrar_RarFile_closeArchive
        (JNIEnv *env, jclass jcls, jlong jhandle) {
    HANDLE handle = jlong_to_ptr(jhandle);
    if (RARCloseArchive(handle) != ERAR_SUCCESS) {
        JNU_ThrowIOExceptionWithLastError(env, "close error");
    }
}

#ifdef __cplusplus
}
#endif




