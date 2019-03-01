//
// Created by mao on 19-3-1.
//

#ifndef RAR_FILE_H
#define RAR_FILE_H
#ifdef __cplusplus
extern "C" {
#endif

#define  LOG_TAG    "libunrar-jni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void initIDs(JNIEnv *env);

jboolean registerNativeMethods(JNIEnv *env);

#ifdef __cplusplus
}
#endif
#endif //RAR_FILE_H
