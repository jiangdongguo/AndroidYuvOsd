/**  YUV图像处理
 *
 * @author Created by jiangdongguo on 2017-5-31 上午10:42:55
 */
#include <jni.h>
#include "YuvOsd.h"

jint cacheFrameSize = 0;
unsigned char *nameTable;
unsigned char count = 0;

/**
 * 叠加时间水印
 * */
JNIEXPORT void JNICALL Java_com_jiangdg_yuvosd_YuvUtils_AddYuvOsd
  (JNIEnv *env, jclass jcls, jbyteArray j_srcArr, jint width, jint height, jbyteArray j_destArr, jstring j_data, jint colorFormat, jboolean isHorizontalTake){
	jint wh = width * height;
	jint frameSize = wh * 3 / 2;
	if(j_srcArr == NULL || j_destArr == NULL){
		return;
	}
	// j_srcArr转jbyte *
	 jbyte* c_srcArr =  (*env)->GetByteArrayElements(env,j_srcArr,JNI_FALSE);
	 jbyte* c_destArr =  (*env)->GetByteArrayElements(env,j_destArr,JNI_FALSE);
	 jbyte* pTmpOut = c_destArr;
	 jbyte* yuv420 = (jbyte*) malloc(frameSize);

	 if(cacheFrameSize != frameSize){
		 // 将NV21转换为编码器支持的YUV420sp或YUV420p格式
		 swapNv21ColorFormat(c_srcArr,yuv420,wh,frameSize,colorFormat);
		 // 叠加时间水印，需区别拍摄方向
		 if(isHorizontalTake){
			 draw_Font_Func(yuv420, width,height,25,25, 5, jstringTostring(env, j_data),nameTable,count);
		 }else{
			 draw_Font_Func(yuv420,height,width,25,25, 5, jstringTostring(env, j_data),nameTable,count);
		 }
		memcpy(pTmpOut, yuv420, frameSize);
	 }else{
		 cacheFrameSize = frameSize;
	 }
	// 释放资源
	(*env)->ReleaseByteArrayElements(env,j_srcArr, c_srcArr,JNI_FALSE);
	(*env)->ReleaseByteArrayElements(env,j_destArr, c_destArr,JNI_FALSE);
	free(yuv420);
}

/**
 * 颜色格式转换(摄像头支持的格式->编码器支持的格式)
 *  NV21--->yuv420sp(I420)
 *  NV21--->yuv420p(YUV21)
 * */
JNIEXPORT void JNICALL Java_com_jiangdg_yuvosd_YuvUtils_transferColorFormat
  (JNIEnv *env, jclass jcls, jbyteArray j_srcArr, jint width, jint height, jbyteArray j_destArr, jint colorFormat){
	jint wh = width * height;
	jint frameSize = wh * 3 / 2;
	if(j_srcArr == NULL || j_destArr == NULL){
		return;
	}
	// j_srcArr转jbyte *
	 jbyte* c_srcArr =  (*env)->GetByteArrayElements(env,j_srcArr,JNI_FALSE);
	 jbyte* c_destArr =  (*env)->GetByteArrayElements(env,j_destArr,JNI_FALSE);

	 // 将NV21转换为编码器支持的YUV420sp或YUV420p格式
	 swapNv21ColorFormat(c_srcArr,c_destArr,wh,frameSize,colorFormat);

	// 释放内存，是否同步到Java层
	(*env)->ReleaseByteArrayElements(env,j_srcArr, c_srcArr,JNI_FALSE);
	(*env)->ReleaseByteArrayElements(env,j_destArr, c_destArr,JNI_FALSE);
}

/**
 *旋转后置摄像头采集的NV21图像
 * */
JNIEXPORT void JNICALL Java_com_jiangdg_yuvosd_YuvUtils_YUV420spRotateOfBack
  (JNIEnv *env, jclass jcls, jbyteArray j_srcArr, jbyteArray j_destArr, jint width, jint height, jint rotateDegree){
	jbyte * c_srcArr = (jbyte*) (*env)->GetByteArrayElements(env, j_srcArr, JNI_FALSE);
	jbyte * c_destArr = (jbyte*) (*env)->GetByteArrayElements(env, j_destArr, JNI_FALSE);
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
	(*env)->ReleaseByteArrayElements(env, j_srcArr, c_srcArr, JNI_FALSE);
	(*env)->ReleaseByteArrayElements(env, j_destArr, c_destArr, JNI_FALSE);
}

/**
 *旋转前置摄像头采集的NV21图像
 * */
JNIEXPORT void JNICALL Java_com_jiangdg_yuvosd_YuvUtils_Yuv420spRotateOfFront
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
	jbyte* c_srcArr = (*env)->GetByteArrayElements(env,j_srcArr,JNI_FALSE);
	jbyte* c_destArr = (*env)->GetByteArrayElements(env,j_destArr,JNI_FALSE);
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
	(*env)->ReleaseByteArrayElements(env,j_srcArr, c_srcArr,JNI_FALSE);
	(*env)->ReleaseByteArrayElements(env,j_destArr, c_destArr,JNI_FALSE);
}

/**
 *YV12转NV21(一帧图像大小为w*h*3/2)
 *YYYYYYYY UUVV->YYYYYYYY UV UV
 * */
JNIEXPORT void JNICALL Java_com_jiangdg_yuvosd_YuvUtils_swapYV12ToNV21
  (JNIEnv *env, jclass jcls, jbyteArray j_srcArr,jint srcWidth, jint srcHeight){
	if(j_srcArr == NULL) {
		return;
	}
	jint i,wh = 0;
	jint mWidth = 0;
	jint mHeight = 0;
	jint uWh = 0;
	jint vWh = 0;
	if (srcWidth != mWidth || srcHeight != mHeight) {
		mWidth = srcWidth;
		mHeight = srcHeight;
	}
	wh = mWidth * mHeight;
	uWh = wh / 4;
	vWh = wh * 5 / 4;
	jsize len_srcArr = (*env)->GetArrayLength(env,j_srcArr);
	jbyte* c_srcArr =  (*env)->GetByteArrayElements(env,j_srcArr,JNI_FALSE);
	char *c_tmp = (char *)malloc(len_srcArr);
	// YUV数据转换
	memcpy(c_tmp,c_srcArr, wh);//Y分量，占wh字节
	for(i = 0; i<uWh; i++) {
		c_tmp[wh + i*2] = c_srcArr[wh + i];	// U分量
		c_tmp[wh+i*2 +1] = c_srcArr[vWh+i];//V分量
	}
	(*env)->SetByteArrayRegion(env,j_srcArr,0,len_srcArr,c_tmp);
	(*env)->ReleaseByteArrayElements(env,j_srcArr, c_srcArr,JNI_COMMIT);
	free(c_tmp);
}

JNIEXPORT void JNICALL Java_com_jiangdg_yuvosd_YuvUtils_swYV12ToNV21
(JNIEnv *env, jclass jcls, jbyteArray j_srcArr, jbyteArray j_destArr, jint srcWidth, jint srcHeight) {
	if(j_srcArr == NULL || j_destArr == NULL) {
		return;
	}
	jint i,width=0,height=0;
	jint wh = 0;
	jint uWh = 0;
	jint vWh = 0;
	if(srcWidth != width || srcHeight != height) {
		width = srcWidth;
		height = srcHeight;
	}
	wh = width * height;
	uWh = wh / 4;
	vWh = wh * 5 / 4;
	// j_srcArr转jbyte *
	jbyte* c_srcArr = (*env)->GetByteArrayElements(env,j_srcArr,JNI_FALSE);// YV12数据
	jbyte* c_destArr = (*env)->GetByteArrayElements(env,j_destArr,JNI_FALSE);// NV21数据
	// YUV数据转换
	memcpy(c_destArr,c_srcArr, wh);//Y分量，占wh字节
	for(i = 0; i<uWh; i++) {
		c_destArr[wh + i*2] = c_srcArr[wh + i];	// U分量
		c_destArr[wh+i*2 +1] = c_srcArr[vWh+i];//V分量
	}
	// 释放内存，是否同步到Java层
	(*env)->ReleaseByteArrayElements(env,j_srcArr, c_srcArr,JNI_FALSE);
	(*env)->ReleaseByteArrayElements(env,j_destArr, c_destArr,JNI_FALSE);
}

/**
 * 	 NV21转I420(YYYYYYYY VUVU -> YYYYYYYY UU VV)
 *	 NV21转YUV21(YYYYYYYY VUVU -> YYYYYYYY VV UU)
 * */
void swapNv21ColorFormat(char* c_srcArr,char* c_destArr,long wh,long frameSize,int colorFormat){
	int i=0 , j=0;
	// 转YUV420p (Yuv21)
	if(colorFormat == colorFormatYuv420sp){
		LOGI("开始将NV21转换为YUV420sp(I420)");
		memcpy(c_destArr, c_srcArr, wh);				// Y分量
		for (i = wh; i < frameSize; i += 2) {
			c_destArr[i] = c_srcArr[i + 1]; 					// U分量
			c_destArr[i + 1] = c_srcArr[i];					// V分量
		}
	} else {
		LOGD("开始将NV21转换为YUV420p及其他");
		memcpy(c_destArr, c_srcArr, wh);            // Y分量
		for (i = 0,j = 0; j < wh / 2; j += 2) {          // U分量
			c_destArr[i + wh * 5 / 4] = c_srcArr[j + wh];
			i++;
		}
		for (i = 0,j = 1; j < wh / 2; j += 2) {           // V分量
			c_destArr[i + wh] = c_srcArr[j + wh];
			i++;
		}
	}
}

JNIEXPORT void JNICALL
Java_com_jiangdg_natives_YuvUtils_convertColorFormat(JNIEnv *env, jclass type) {

	// TODO

}

char* jstringTostring(JNIEnv* env, jstring jstr) {
	char* rtn = NULL;
	jclass clsstring = (*env)->FindClass(env, "java/lang/String");
	jstring strencode = (*env)->NewStringUTF(env, "utf-8");
	jmethodID mid = (*env)->GetMethodID(env, clsstring, "getBytes","(Ljava/lang/String;)[B");
	jbyteArray barr = (jbyteArray)(*env)->CallObjectMethod(env, jstr, mid,strencode);
	jsize alen = (*env)->GetArrayLength(env, barr);
	jbyte* ba = (*env)->GetByteArrayElements(env, barr, JNI_FALSE);
	if (alen > 0) {
		rtn = (char*) malloc(alen + 1);
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}
	(*env)->ReleaseByteArrayElements(env, barr, ba, 0);
	return rtn;
}

JNIEXPORT void JNICALL
Java_com_jiangdg_natives_YuvUtils_nativeYV12ToNV21(JNIEnv *env, jclass type, jbyteArray src_,
                                                   jint width, jint height, jbyteArray dst_) {
    jbyte *src = (*env)->GetByteArrayElements(env, src_, NULL);
    jbyte *dst = (*env)->GetByteArrayElements(env, dst_, NULL);

    // TODO

    (*env)->ReleaseByteArrayElements(env, src_, src, 0);
    (*env)->ReleaseByteArrayElements(env, dst_, dst, 0);
}