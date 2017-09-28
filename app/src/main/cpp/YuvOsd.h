#include <string.h>
#include <stdlib.h>
#include "yuv/yuv.h"
#include <android/log.h>
#define colorFormatYuv420sp 21
#define colorFormatYuv420p 19

#define LOG_TAG "YuvOsd"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,"%s",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,"%s",__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,"%s",__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,"%s",__VA_ARGS__)

#define LOG_I(format,...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,format,__VA_ARGS__)
#define LOG_D(format,...) __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,format,__VA_ARGS__)
#define LOG_W(format,...) __android_log_print(ANDROID_LOG_WARN,LOG_TAG,format,__VA_ARGS__)
#define LOG_E(format,...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,format, __VA_ARGS__)

void swapNv21ColorFormat(char* c_srcArr,char* c_destArr,long wh,long frameSize,int colorFormat);
char* jstringTostring(JNIEnv* env, jstring jstr);
