//
// Created by jianddongguo on 2018/8/23.
//
#include <jni.h>
#include <malloc.h>
#include "yuv/yuv.h"


extern "C"
JNIEXPORT jint JNICALL
Java_com_jiangdg_natives_YuvUtils_nativeYV12ToNV21(JNIEnv *env, jclass type, jbyteArray jarray_,
                                                   jint width, jint height) {
    jbyte *srcData = env->GetByteArrayElements(jarray_, NULL);
    jsize srcLen = env->GetArrayLength(jarray_);
    int yLength = width * height;
    int vLength = yLength / 4;
    // 开辟一段临时内存空间
    char *c_tmp = (char *)malloc(srcLen);
    // YYYYYYYY VV UU --> YYYYYYYY VUVU
    // 拷贝Y分量
    memcpy(c_tmp,srcData,yLength);
    int i = 0;
    for(i=0; i<yLength/4; i++) {
        // U分量
        c_tmp[yLength + 2*i + 1] = srcData[yLength + vLength + i];
        // V分量
        c_tmp[yLength + 2*i] = srcData[yLength + i];
    }
    // 将c_tmp的数据覆盖到jarray_
    env->SetByteArrayRegion(jarray_,0,srcLen,(jbyte *)c_tmp);
    env->ReleaseByteArrayElements(jarray_, srcData, 0);
    // 释放临时内存
    free(c_tmp);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_jiangdg_natives_YuvUtils_nativeNV21ToYUV420sp(JNIEnv *env, jclass type, jbyteArray jarray_,
                                                   jint width, jint height) {
    jbyte *srcData = env->GetByteArrayElements(jarray_, NULL);
    jsize srcLen = env->GetArrayLength(jarray_);
    int yLength = width * height;
    int uLength = yLength / 4;
    // 开辟一段临时内存空间
    char *c_tmp = (char *)malloc(srcLen);
    // 拷贝Y分量
    memcpy(c_tmp,srcData,yLength);
    int i = 0;
    for(i=0; i<yLength/4; i++) {
        // U分量
        c_tmp[yLength + 2 * i] = srcData[yLength + 2*i+1];
        // V分量
        c_tmp[yLength + 2*i+1] = srcData[yLength + 2*i];
    }
    // 将c_tmp的数据覆盖到jarray_
    env->SetByteArrayRegion(jarray_,0,srcLen,(jbyte *)c_tmp);
    env->ReleaseByteArrayElements(jarray_, srcData, 0);
    // 释放临时内存
    free(c_tmp);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_jiangdg_natives_YuvUtils_nativeNV21ToYUV420p(JNIEnv *env, jclass type, jbyteArray jarray_,
                                                   jint width, jint height)
{
    jbyte *srcData = env->GetByteArrayElements(jarray_, NULL);
    jsize srcLen = env->GetArrayLength(jarray_);
    int yLength = width * height;
    int uLength = yLength / 4;
    // 开辟一段临时内存空间
    char *c_tmp = (char *)malloc(srcLen);
    // 拷贝Y分量
    memcpy(c_tmp,srcData,yLength);
    int i = 0;
    for(i=0; i<yLength/4; i++) {
        // U分量
        c_tmp[yLength + i] = srcData[yLength + 2*i + 1];
        // V分量
        c_tmp[yLength + uLength + i] = srcData[yLength + 2*i];
    }
    // 将c_tmp的数据覆盖到jarray_
    env->SetByteArrayRegion(jarray_,0,srcLen,(jbyte *)c_tmp);
    env->ReleaseByteArrayElements(jarray_, srcData, 0);
    // 释放临时内存
    free(c_tmp);
}

JNIEXPORT void JNICALL Java_com_jiangdg_natives_YuvUtils_nativeRotateNV21Flip
        (JNIEnv *env, jclass jcls, jbyteArray j_srcArr, jbyteArray j_destArr, jint srcWidth, jint srcHeight, jint rotateDegree) {
    if(j_srcArr == NULL || j_destArr == NULL) {
        return;
    }
    jint wh = 0;
    jint mWidth = 0;
    jint mHeight = 0;
    jint uvHeight = 0;
    if (srcWidth != mWidth || srcHeight != mHeight) {
        mWidth = srcWidth;
        mHeight = srcHeight;
        wh = srcWidth * srcHeight;
        uvHeight = srcHeight >> 1; // uvHeight=height/2
    }
    // j_srcArr转jbyte *
    jbyte* c_srcArr = env->GetByteArrayElements(j_srcArr,JNI_FALSE);
    jbyte* c_destArr = env->GetByteArrayElements(j_destArr,JNI_FALSE);
    int k = 0,i=0,j=0;
    if(rotateDegree == 270) {
        // 旋转Y
        for (i = 0; i < srcWidth; i++) {
            int nPos = srcWidth - 1;
            for (j = 0; j < srcHeight; j++) {
                c_destArr[k] = c_srcArr[nPos - i];
                k++;
                nPos += srcWidth;
            }
        }

        // 旋转UV
        for (i = 0; i < srcWidth; i += 2) {
            int nPos = wh + srcWidth - 2;
            for (j = 0; j < uvHeight; j++) {
                c_destArr[k] = c_srcArr[nPos - i];
                c_destArr[k + 1] = c_srcArr[nPos - i + 1];
                k += 2;
                nPos += srcWidth;
            }
        }
    }else if(rotateDegree == 180){
        // 旋转Y分量
        for (i = wh - 1; i >= 0; i--) {
            c_destArr[k] = c_srcArr[i];
            k++;
        }
        // 旋转U、V分量
        for (j = wh * 3 / 2 - 1; j >= wh; j -= 2) {
            c_destArr[k] = c_srcArr[j - 1];
            c_destArr[k + 1] = c_srcArr[j];
            k += 2;
        }
    }
    // 释放内存，是否同步到Java层
    env->ReleaseByteArrayElements(j_srcArr, c_srcArr,JNI_FALSE);
    env->ReleaseByteArrayElements(j_destArr, c_destArr,JNI_FALSE);
}

extern "C"
JNIEXPORT void JNICALL Java_com_jiangdg_natives_YuvUtils_nativeRotateNV21
        (JNIEnv *env, jclass jcls, jbyteArray j_srcArr, jbyteArray j_destArr, jint width, jint height, jint rotateDegree){
    jbyte * c_srcArr = (jbyte*) env->GetByteArrayElements(j_srcArr, JNI_FALSE);
    jbyte * c_destArr = (jbyte*) env->GetByteArrayElements(j_destArr, JNI_FALSE);
    jint wh = width * height;
    jint frameSize =wh * 3 / 2;
    int k = 0,i=0,j=0;
    if(rotateDegree == 90){
        // 旋转Y
        for (i = 0; i < width; i++) {
            for (j = height - 1; j >= 0; j--) {
                c_destArr[k] = c_srcArr[width * j + i];
                k++;
            }
        }
        // 旋转U、V分量
        for (i = 0; i < width; i += 2) {
            for (j = height / 2 - 1; j >= 0; j--) {
                c_destArr[k] = c_srcArr[wh + width * j + i];
                c_destArr[k + 1] = c_srcArr[wh + width * j + i + 1];
                k += 2;
            }
        }
    }else if(rotateDegree == 180){
        // 旋转Y分量
        for (i = wh - 1; i >= 0; i--) {
            c_destArr[k] = c_srcArr[i];
            k++;
        }
        // 旋转U、V分量
        for (j = wh * 3 / 2 - 1; j >= wh; j -= 2) {
            c_destArr[k] = c_srcArr[j - 1];
            c_destArr[k + 1] = c_srcArr[j];
            k += 2;
        }
    }else if(rotateDegree == 270){
        // 旋转Y分量
        for(i=width-1 ; i>=0 ; i--){
            for(j=height-1 ; j>=0 ; j--){
                c_destArr[k] = c_srcArr[width*j + i];
                k++;
            }
        }
        // 旋转U、V分量
        for(i=width-1 ; i>=0 ; i-=2){
            for(j=height/2-1 ; j>=0 ; j--){
                c_destArr[k] = c_srcArr[wh + width*j + i-1];
                c_destArr[k+1] = c_srcArr[wh + width*j + i];
                k +=2;
            }
        }
    }
    // 释放数组资源
    env->ReleaseByteArrayElements(j_srcArr, c_srcArr, JNI_FALSE);
    env->ReleaseByteArrayElements(j_destArr, c_destArr, JNI_FALSE);
}

extern "C"
char* jstringTostring(JNIEnv* env, jstring jstr) {
    char* rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("utf-8");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes","(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray)env->CallObjectMethod(jstr, mid,strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte* ba = env->GetByteArrayElements( barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char*) malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}





