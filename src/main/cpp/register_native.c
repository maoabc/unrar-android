/*
    This file was generated automatically by RarFile.class
*/




#include <jni.h>
#include "mao_archive_unrar_RarFile.h"


#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

#define CLASS_NAME "mao/archive/unrar/RarFile"


static JNINativeMethod methods[]={
        {"initIDs","()V",(void *) Java_mao_archive_unrar_RarFile_initIDs},

        {"openArchive","(Ljava/lang/String;I)J",(void *) Java_mao_archive_unrar_RarFile_openArchive},

        {"readHeader0","(JLmao/archive/unrar/UnrarCallback;)Lmao/archive/unrar/RarEntry;",(void *) Java_mao_archive_unrar_RarFile_readHeader0},

        {"processFile0","(JILjava/lang/String;Ljava/lang/String;Lmao/archive/unrar/UnrarCallback;)V",(void *) Java_mao_archive_unrar_RarFile_processFile0},

        {"closeArchive","(J)V",(void *) Java_mao_archive_unrar_RarFile_closeArchive},

};


static jboolean registerNativeMethods(JNIEnv* env) {
    jclass clazz = (*env)->FindClass(env, "mao/archive/unrar/RarFile");
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if ((*env)->RegisterNatives(env, clazz, methods, NELEM(methods)) < 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    if (!registerNativeMethods(env)) {
        return -1;
    }

    return JNI_VERSION_1_4;
}