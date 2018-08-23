//
// Created by jianddongguo on 2018/8/23.
//
#include <jni.h>

extern "C" {
JNIEXPORT void JNICALL
Java_com_jiangdg_natives_YuvUtils_nativeNV21ToYV12(JNIEnv *env, jclass type, jbyteArray srcNV21_,
                                                   jint width, jint height, jbyteArray dstYV12_) {
    jbyte *srcNV21 = env->GetByteArrayElements(srcNV21_, NULL);
    jbyte *dstYV12 = env->GetByteArrayElements(dstYV12_, NULL);

// TODO

    env->ReleaseByteArrayElements(srcNV21_, srcNV21, 0);
    env->ReleaseByteArrayElements(dstYV12_, dstYV12, 0);
}
}.


