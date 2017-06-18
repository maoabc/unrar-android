//
// Created by mao on 17-6-4.
//
#include <jni.h>
#include <string.h>
#include "libunrar/rar.hpp"
#include "libunrar/dll.hpp"
#include "jni_util.h"
#include "jlong.h"

#ifdef __cplusplus
extern "C" {
#endif
struct user_data {
    JNIEnv *env;
    jobject output;
};
static jfieldID entry_field_name;
static jfieldID entry_field_size;
static jfieldID entry_field_csize;
static jfieldID entry_field_mtime;
static jfieldID entry_field_crc;
static jfieldID entry_field_flags;
static jmethodID entry_method_constructor;

static jmethodID output_write_method;

JNIEXPORT void JNICALL Java_mao_archive_unrar_RarFile_initIDs
        (JNIEnv *env, jclass jcls) {
    jclass entry_cls = 0;
    jclass output_cls = 0;

    if (env->EnsureLocalCapacity(2) < 0)
        goto done;

    entry_cls = env->FindClass("mao/archive/unrar/RarEntry");
    if (entry_cls == 0)
        goto done;


    output_cls = env->FindClass("java/io/OutputStream");
    if (output_cls == 0)
        goto done;

    output_write_method = env->GetMethodID(output_cls, "write", "([BII)V");

    entry_field_name = env->GetFieldID(entry_cls, "name", "Ljava/lang/String;");
    entry_field_mtime = env->GetFieldID(entry_cls, "mtime", "J");
    entry_field_size = env->GetFieldID(entry_cls, "size", "J");
    entry_field_csize = env->GetFieldID(entry_cls, "csize", "J");
    entry_field_crc = env->GetFieldID(entry_cls, "crc", "J");
    entry_field_flags = env->GetFieldID(entry_cls, "flags", "I");

    entry_method_constructor = env->GetMethodID(entry_cls, "<init>", "()V");
    done:
    env->DeleteLocalRef(entry_cls);
    env->DeleteLocalRef(output_cls);
}
wchar_t *jcharntowcs(wchar_t *dest, const jchar *src, size_t len) {
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
jchar *wcsntojchar(jchar *dest, const wchar_t *src, size_t len) {
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

JNIEXPORT jlong JNICALL Java_mao_archive_unrar_RarFile_openArchive
        (JNIEnv *env, jclass jcls, jstring jfilePath, jint mode, jbooleanArray jencrypted) {
    struct RAROpenArchiveDataEx data;
    memset(&data, 0, sizeof(data));

    const jchar *filePath = env->GetStringChars(jfilePath, 0);
    jsize len = env->GetStringLength(jfilePath);

    wchar_t nameW[NM];
    jcharntowcs(nameW, filePath, (size_t) len);
    env->ReleaseStringChars(jfilePath, filePath);

    data.ArcNameW = nameW;
    data.OpenMode = (unsigned int) mode;
    HANDLE handle = RAROpenArchiveEx(&data);

    if (handle == NULL || data.OpenResult) {
        if (handle) {
            RARCloseArchive(handle);
        }
        char err_str[128];
        sprintf(err_str, "RAROpenArchiveEx ErrorCode: %d", data.OpenResult);
        JNU_ThrowByName(env, "mao/archive/unrar/RarException", err_str);
        return -1;
    }
    if (jencrypted != NULL) {
        //检查文件名是否被加密
        jboolean encrypted = (jboolean) ((data.Flags & 0x80) == 0x80);
        env->SetBooleanArrayRegion(jencrypted, 0, 1, &encrypted);
    }
    return ptr_to_jlong(handle);
}
JNIEXPORT jobject JNICALL Java_mao_archive_unrar_RarFile_readHeader
        (JNIEnv *env, jclass jcls, jlong jhandle) {
    jobject entry = NULL;
    jclass entry_cls = NULL;

    HANDLE handle = jlong_to_ptr(jhandle);

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

    env->SetLongField(entry, entry_field_size, (((int64) header.UnpSizeHigh) << 32) | header.UnpSize);
    env->SetLongField(entry, entry_field_csize, (((int64) header.PackSizeHigh) << 32) | header.PackSize);
    env->SetLongField(entry, entry_field_mtime, ((int64) header.FileTime));
    env->SetLongField(entry, entry_field_crc, header.FileCRC);
    env->SetIntField(entry, entry_field_flags, header.Flags);

    return entry;
}

JNIEXPORT jobject JNICALL Java_mao_archive_unrar_RarFile_readHeaderSkipData
        (JNIEnv *env, jclass jcls, jlong jhandle) {
    jobject header = Java_mao_archive_unrar_RarFile_readHeader(env, jcls, jhandle);
    if (header == NULL) {
        return NULL;
    }
    HANDLE handle = jlong_to_ptr(jhandle);
    RARProcessFileW(handle, RAR_SKIP, NULL, NULL);
    return header;

}

int CALLBACK callbackData(UINT msg, LPARAM UserData, LPARAM P1, LPARAM P2) {
    user_data *data = (user_data *) UserData;
    JNIEnv *env = data->env;
    switch (msg) {
        case UCM_PROCESSDATA: {
            jsize len = (jsize) P2;
            jbyteArray array = env->NewByteArray(len);
            env->SetByteArrayRegion(array, 0, len, (const jbyte *) P1);
            env->CallVoidMethod(data->output, output_write_method, array, 0, len);
            env->DeleteLocalRef(array);
            if (env->ExceptionCheck()) {
                return -1;
            }
            return 1;
        }
        default:
            break;
    }
    return 1;
}

JNIEXPORT jint JNICALL Java_mao_archive_unrar_RarFile_readData
        (JNIEnv *env, jclass jcls, jlong jhandle, jstring jdestPath,
         jobject output) {
    HANDLE handle = jlong_to_ptr(jhandle);
    if (jdestPath != NULL) {
        wchar_t destPath[NM];
        const jchar *chars = env->GetStringChars(jdestPath, 0);
        jsize len = env->GetStringLength(jdestPath);
        jcharntowcs(destPath, chars, (size_t) len);
        env->ReleaseStringChars(jdestPath, chars);
        return RARProcessFileW(handle, RAR_EXTRACT, destPath, NULL);
    }
    if (output != NULL) {
        struct user_data userData;
        userData.env = env;
        userData.output = output;
        RARSetCallback(handle, callbackData, (LPARAM) &userData);
        return RARProcessFileW(handle, RAR_TEST, NULL, NULL);
    }
    return RARProcessFileW(handle, RAR_SKIP, NULL, NULL);

}


JNIEXPORT void JNICALL Java_mao_archive_unrar_RarFile_setPassword
        (JNIEnv *env, jclass jcls, jlong jhandle, jstring jpasswd) {
    if (jpasswd == NULL) {
        return;
    }
    HANDLE handle = jlong_to_ptr(jhandle);
    const char *passwd = env->GetStringUTFChars(jpasswd, 0);
    RARSetPassword(handle, (char *) passwd);
    env->ReleaseStringUTFChars(jpasswd, passwd);
}

JNIEXPORT void JNICALL Java_mao_archive_unrar_RarFile_closeArchive
        (JNIEnv *env, jclass jcls, jlong jhandle) {
    HANDLE handle = jlong_to_ptr(jhandle);
    RARCloseArchive(handle);
}

#ifdef __cplusplus
}
#endif




