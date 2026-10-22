//go:build android

#include <jni.h>
#include <stdlib.h>
#include "_cgo_export.h"

JNIEXPORT jstring JNICALL Java_io_github_wszqkzqk_pvzportable_LanHostService_nativeStart(
    JNIEnv* env, jclass type, jint port) {
    char* error = PVZHostStart(port);
    if (!error) return NULL;
    jstring result = (*env)->NewStringUTF(env, error);
    free(error);
    return result;
}
JNIEXPORT void JNICALL Java_io_github_wszqkzqk_pvzportable_LanHostService_nativeStop(
    JNIEnv* env, jclass type) { PVZHostStop(); }
JNIEXPORT jint JNICALL Java_io_github_wszqkzqk_pvzportable_LanHostService_nativePort(
    JNIEnv* env, jclass type) { return PVZHostPort(); }
