


#include <jni.h>
#include "rar_file.h"

JavaVM *javaVM;

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVM = vm;
    JNIEnv *env = NULL;

    if ((*vm)->GetEnv(vm, (void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }
    if (!registerNativeMethods(env)) {
        return -1;
    }
    initIDs(env);

    return JNI_VERSION_1_6;
}