/*******************************************************************************
 * File  name	 : Osdchar.c
 * Description    : 实现yuv上的字幕叠加，同时支持滚动，颜色变换，逐渐消隐，大小可调等功能.
 该文件为纯C文件，不依赖第三方库及其他系统调用。可实现跨平台功能。
 * Wrote by/Date  : gaoc@devison.com/2010.02.27
 * Modify/Date    :
 * Project		 : V30E
 *******************************************************************************/
#include "Osdchar.h"
#include <android/log.h>

#define LOG_TAG_OSD "osd"

//#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,LOG_TAG_OSD,__VA_ARGS__)
//缓冲分配
unsigned char srcRGBBuf[IMAGEWIDTH * IMAGEHEIGHT * 3];
unsigned char dstRGBBuf[IMAGEWIDTH * IMAGEHEIGHT * 3];
unsigned char srcYUVBuf[IMAGEWIDTH * IMAGEHEIGHT * 3 / 2];
unsigned char dstYUVBuf[IMAGEWIDTH * IMAGEHEIGHT * 3 / 2];
unsigned char subYUVBuf[IMAGEWIDTH * IMAGEHEIGHT * 3 / 2];

//转换表空间
long int crv_tab[256];
long int cbu_tab[256];
long int cgu_tab[256];
long int cgv_tab[256];
long int tab_76309[256];
unsigned char clp[1024];

//全局量
char g_szDefault[128] = "请在此处设置缺省叠加文字汇智";
O_OBJCHAR *g_pAllObjCharObj[64];
char *g_szAllCharObj[64];
FILE *g_fpHZKLIB = NULL;
FILE *g_fpASCII = NULL;
unsigned int g_frameCounter = 0;

//转换矩阵
#define MY(a,b,c) (( a*  0.2989  + b*  0.5866  + c*  0.1145))
#define MU(a,b,c) (( a*(-0.1688) + b*(-0.3312) + c*  0.5000 + 128))
#define MV(a,b,c) (( a*  0.5000  + b*(-0.4184) + c*(-0.0816) + 128))
//大小判断
#define DY(a,b,c) (MY(a,b,c) > 255 ? 255 : (MY(a,b,c) < 0 ? 0 : MY(a,b,c)))
#define DU(a,b,c) (MU(a,b,c) > 255 ? 255 : (MU(a,b,c) < 0 ? 0 : MU(a,b,c)))
#define DV(a,b,c) (MV(a,b,c) > 255 ? 255 : (MV(a,b,c) < 0 ? 0 : MV(a,b,c)))
//转换矩阵
double YuvToRgb[3][3] = { 1, 0, 1.4022, 1, -0.3456, -0.7145, 1, 1.771, 0 };
/*******************************************************************************
 说明:
 1.所有函数前向申明
 2.以OSD_XXX(XX,XX,...)形式出现的函数均为接口函数
 3.以_XXX(XX,XX,...)形式出现的函数均为私有函数
 4.以p_XXX形式的均为指针量，以g_XXX形式出现的均为全局量

 WroteBy/Date:
 gaoc@Dvision.com/2010.03.05

 Modify:
 *******************************************************************************/
//接口函数
char OSD_CreateObjCharObj(int strID, char *szStr, O_STRINGATTR OAttrCharObj);
char OSD_DeleteObjCharObj(int strID);
char OSD_SetContentCharObj(int strID, char *szStr);
char OSD_SetPositionCharObj(int strID, int x, int y);
char OSD_SetAttrCharObj(int strID, O_STRINGATTR OAttrCharObj);
void OSD_FeedFrameYUV420(char* pYUV420Frame, int iSrcWidth, int iSrcHeight);
char OSD_Init(char *szPathHZK, char *szPathASCII);
void OSD_Release();
//内部函数
void _InitDitherTab();
void _RGB24ToYUV420(unsigned char *RGB, int nWidth, int nHeight, //源
		unsigned char *YUV, unsigned long nLen); //目标
void _YUV420ToRGB24(unsigned char *src_yuv, //源
		unsigned char *dst_rgb, int width, int height); //目标
void _YUV420ToYUV422(char* pYUV420Buf, int iSrcWidth, int iSrcHeight, //源
		char* pYUV422Buf); //目标
void _YUV422ToYUV420(char* pYUV422Buf, int iSrcWidth, int iSrcHeight, //源
		char* pYUV420Buf); //目标
void _GetSubReginFromYUV420(unsigned char *src_yuv, int srcW, int srcH, //yuv源图
		unsigned char *sub_yuv, int x, int y, int subW, int subH); //yuv子区域
void _SetSubReginToYUV420(unsigned char *src_yuv, int srcW, int srcH, //yuv源图
		unsigned char *sub_yuv, int x, int y, int subW, int subH); //yuv子区域
void _OverlapCaptionOnRGB(unsigned char* srcRgbBuf, int nWidth, int nHeight, //rgb源图
		char* pCaption, O_STRINGATTR *pOAttrCharObj); //要叠加的文字，叠加属性及输出
char _OverLapCaptionOnYUV420(unsigned char *src_yuv, int srcW, int srcH, //源图及宽高
		int xStart, int yStart, int dstW, int dstH, //要叠加的文字及区域
		char* pCaption, O_STRINGATTR *pOAttrCharObj); //要叠加的文字及叠加属性
void _OverLapCaptionOnYUV422Raw(char* pCharcode, int column, int row,
		int imageWidth, int imageHeight, char *pYUVbuffer, char OsdY, char OsdU,
		char OsdV);

/*******************************************************************************
 说明:
 以下以OSD_XXX(XX,XX,...)形式出现的函数均为接口函数

 WroteBy/Date:
 gaoc@Dvision.com/2010.03.05

 Modify:
 *******************************************************************************/
//创建叠加的字符串对象，用于字符串叠加
char OSD_CreateObjCharObj(int strID, char *szStr, O_STRINGATTR OAttrCharObj) {
	szStr = strlen(szStr) == 0 ? g_szDefault : szStr;

	if (g_pAllObjCharObj[strID] == NULL) {
		g_pAllObjCharObj[strID] = (O_OBJCHAR *) malloc(sizeof(O_OBJCHAR));
		assert(g_pAllObjCharObj[strID]);
		strcpy(g_pAllObjCharObj[strID]->szStr, szStr);
		g_pAllObjCharObj[strID]->oAttrChar = OAttrCharObj;
		g_pAllObjCharObj[strID]->w = g_pAllObjCharObj[strID]->oAttrChar.sizeW;
		g_pAllObjCharObj[strID]->h = g_pAllObjCharObj[strID]->oAttrChar.sizeH;
	} else {
		return ERR_OBJEXIST; //返回错误码，对象已经存在
	}

	return ERR_NONE;
}
//删除叠加的字符串对象
char OSD_DeleteObjCharObj(int strID) {
	if (g_pAllObjCharObj[strID] == NULL)
		return ERR_OBJNOTEXIST; //返回错误码，对象不存在
	else {
		free(g_pAllObjCharObj[strID]);
		g_pAllObjCharObj[strID] = NULL;
	}

	return ERR_NONE;
}
//改变该字符串的属性
char OSD_SetAttrCharObj(int strID, O_STRINGATTR OAttrCharObj) {
	if (g_pAllObjCharObj[strID] == NULL)
		return ERR_OBJNOTEXIST; //返回错误码，对象不存在
	else {
		g_pAllObjCharObj[strID]->oAttrChar = OAttrCharObj;
	}

	return ERR_NONE;
}
//改变字符串的内容
char OSD_SetContentCharObj(int strID, char *szStr) {
	if (g_pAllObjCharObj[strID] == NULL)
		return ERR_OBJNOTEXIST; //返回错误码，对象不存在
	else {
		strcpy(g_pAllObjCharObj[strID]->szStr, szStr);
	}

	return ERR_NONE;
}
//改变字符串叠加位置
char OSD_SetPositionCharObj(int strID, int x, int y) {
	if (g_pAllObjCharObj[strID] == NULL)
		return ERR_OBJNOTEXIST; //返回错误码，对象不存在
	else {
		g_pAllObjCharObj[strID]->x = x;
		g_pAllObjCharObj[strID]->y = y;
	}

	return ERR_NONE;
}
//当设置完要叠加的字符后通过该函数装填图像的连续帧
void OSD_FeedFrameYUV420(char* pYUV420Frame, int iSrcWidth, int iSrcHeight) {
	int i = 0;
	g_frameCounter++;

	//遍历列表叠加所有字符串对象
	for (i = 0; i < 64; i++) {
		//如果空项，则检查下一个
		if (g_pAllObjCharObj[i] == NULL)
			continue;

		switch (g_pAllObjCharObj[i]->oAttrChar.eActionType) {
		case e_SCROLL_LEFT: //如果是滚动字幕,则需要修改属性中叠加位置参数
			g_pAllObjCharObj[i]->x =
					g_frameCounter
							% (g_pAllObjCharObj[i]->oAttrChar.actionValue1)
							== 0 ?
							g_pAllObjCharObj[i]->x
									- g_pAllObjCharObj[i]->oAttrChar.actionValue2 :
							g_pAllObjCharObj[i]->x;
			break;
		case e_SCROLL_RIGHT: //如果是滚动字幕,则需要修改属性中叠加位置参数
			g_pAllObjCharObj[i]->x =
					g_frameCounter
							% (g_pAllObjCharObj[i]->oAttrChar.actionValue1)
							== 0 ?
							g_pAllObjCharObj[i]->x
									+ g_pAllObjCharObj[i]->oAttrChar.actionValue2 :
							g_pAllObjCharObj[i]->x;
			break;
		case e_SCROLL_UP: //如果是滚动字幕,则需要修改属性中叠加位置参数
			g_pAllObjCharObj[i]->y =
					g_frameCounter
							% (g_pAllObjCharObj[i]->oAttrChar.actionValue1)
							== 0 ?
							g_pAllObjCharObj[i]->y
									- g_pAllObjCharObj[i]->oAttrChar.actionValue2 :
							g_pAllObjCharObj[i]->y;
			break;
		case e_SCROLL_DOWN: //如果是滚动字幕,则需要修改属性中叠加位置参数
			g_pAllObjCharObj[i]->y =
					g_frameCounter
							% (g_pAllObjCharObj[i]->oAttrChar.actionValue1)
							== 0 ?
							g_pAllObjCharObj[i]->y
									+ g_pAllObjCharObj[i]->oAttrChar.actionValue2 :
							g_pAllObjCharObj[i]->y;
			break;
		case e_STATIC: //如果是静态字幕
			break;
		case e_SLOWHIDE: //如果是逐渐消隐的字幕
			break;
		case e_SPARK: //如果是闪烁字幕
			break;
		default:
			break;
		}
		_OverLapCaptionOnYUV420(pYUV420Frame, iSrcWidth,
				iSrcHeight, //源图及宽高
				g_pAllObjCharObj[i]->x, g_pAllObjCharObj[i]->y,
				g_pAllObjCharObj[i]->w, g_pAllObjCharObj[i]->h, //要叠加的位置
				g_pAllObjCharObj[i]->szStr, &(g_pAllObjCharObj[i]->oAttrChar)); //要叠加的文字及叠加属性
	}
}
//初始化
char OSD_Init(char *szPathHZK, char *szPathASCII) {
	//加载汉字点阵字库
	if ((g_fpHZKLIB = fopen(szPathHZK, "rb")) == NULL) {
		return ERR_FILEOPENFAIL;
	}

//	LOGE("OSD_Init 1");
	//加载ascii点阵字库
	if ((g_fpASCII = fopen(szPathASCII, "rb")) == NULL) {
		return ERR_FILEOPENFAIL;
	}
//	LOGE("OSD_Init 2");
	//初始化转换表
	_InitDitherTab();

	return ERR_NONE;
}
//析构
void OSD_Release() {
	//关闭字库文件
	if (g_fpHZKLIB)
		fclose(g_fpHZKLIB);
	if (g_fpASCII)
		fclose(g_fpASCII);
}

/*******************************************************************************
 说明:
 以下以_XXX(XX,XX,...)形式出现的函数均为私有函数

 WroteBy/Date:
 gaoc@Dvision.com/2010.03.05

 Modify:
 *******************************************************************************/
//初始化转换表
void _InitDitherTab() {
	long int crv, cbu, cgu, cgv;
	int i, ind;

	crv = 104597;
	cbu = 132201;
	cgu = 25675;
	cgv = 53279;

	for (i = 0; i < 256; i++) {
		crv_tab[i] = (i - 128) * crv;
		cbu_tab[i] = (i - 128) * cbu;
		cgu_tab[i] = (i - 128) * cgu;
		cgv_tab[i] = (i - 128) * cgv;
		tab_76309[i] = 76309 * (i - 16);
	}

	for (i = 0; i < 384; i++) {
		clp[i] = 0;
	}

	ind = 384;
	for (i = 0; i < 256; i++) {
		clp[ind++] = i;
	}

	ind = 640;
	for (i = 0; i < 384; i++) {
		clp[ind++] = 255;
	}
}
//工具函数:RGB24转YUV420
void _RGB24ToYUV420(unsigned char *RGB, int nWidth, int nHeight,
		unsigned char *YUV, unsigned long nLen) {
	//变量声明
	int i, x, y, j;
	unsigned char *Y = NULL;
	unsigned char *U = NULL;
	unsigned char *V = NULL;

	Y = YUV;
	U = YUV + nWidth * nHeight;
	V = U + ((nWidth * nHeight) >> 2);

	for (y = 0; y < nHeight; y++) {
		for (x = 0; x < nWidth; x++) {
			j = y * nWidth + x;
			i = j * 3;

			Y[j] = (unsigned char) (DY(RGB[i+2], RGB[i+1], RGB[i]));

			if (x % 2 == 1 && y % 2 == 1) {
				j = (nWidth >> 1) * (y >> 1) + (x >> 1);
				//上面i仍有效
				U[j] =
						(unsigned char) ((DU(RGB[i +2 ], RGB[i+1], RGB[i])
								+ DU(RGB[i-1], RGB[i-2], RGB[i-3])
								+ DU(RGB[i+2 -nWidth*3], RGB[i+1-nWidth*3], RGB[i-nWidth*3])
								+ DU(RGB[i-1-nWidth*3], RGB[i-2-nWidth*3], RGB[i-3-nWidth*3]))/4);

				V[j] =
						(unsigned char) ((DV(RGB[i+2 ], RGB[i+1], RGB[i])
								+ DV(RGB[i-1], RGB[i-2], RGB[i-3])
								+ DV(RGB[i+2 -nWidth*3], RGB[i+1-nWidth*3], RGB[i-nWidth*3])
								+ DV(RGB[i-1-nWidth*3], RGB[i-2-nWidth*3], RGB[i-3-nWidth*3]))/4);
			}
		}
	}

	nLen = nWidth * nHeight + (nWidth * nHeight) / 2;
}
//工具函数:YUV420转RGB
void _YUV420ToRGB24(unsigned char *src_yuv, unsigned char *dst_rgb, int width,
		int height) {
	int y1, y2, u, v;
	unsigned char *py1, *py2;
	int i, j, c1, c2, c3, c4;
	unsigned char *d1, *d2;

	unsigned char *srcY = src_yuv;
	unsigned char *srcU = src_yuv + width * height;
	unsigned char *srcV = src_yuv + width * height + (width / 2) * (height / 2);

	py1 = srcY;
	py2 = py1 + width;
	d1 = dst_rgb;
	d2 = d1 + 3 * width;
	for (j = 0; j < height; j += 2) {
		for (i = 0; i < width; i += 2) {
			u = *srcU++;
			v = *srcV++;

			c1 = crv_tab[v];
			c2 = cgu_tab[u];
			c3 = cgv_tab[v];
			c4 = cbu_tab[u];

			//up-left
			y1 = tab_76309[*py1++];
			*d1++ = clp[384 + ((y1 + c4) >> 16)];
			*d1++ = clp[384 + ((y1 - c2 - c3) >> 16)];
			*d1++ = clp[384 + ((y1 + c1) >> 16)];

			//down-left
			y2 = tab_76309[*py2++];
			*d2++ = clp[384 + ((y2 + c4) >> 16)];
			*d2++ = clp[384 + ((y2 - c2 - c3) >> 16)];
			*d2++ = clp[384 + ((y2 + c1) >> 16)];

			//up-right
			y1 = tab_76309[*py1++];
			*d1++ = clp[384 + ((y1 + c4) >> 16)];
			*d1++ = clp[384 + ((y1 - c2 - c3) >> 16)];
			*d1++ = clp[384 + ((y1 + c1) >> 16)];

			//down-right
			y2 = tab_76309[*py2++];
			*d2++ = clp[384 + ((y2 + c4) >> 16)];
			*d2++ = clp[384 + ((y2 - c2 - c3) >> 16)];
			*d2++ = clp[384 + ((y2 + c1) >> 16)];
		}
		d1 += 3 * width;
		d2 += 3 * width;
		py1 += width;
		py2 += width;
	}
}
//工具函数:YUV420转YUV422的函数
void _YUV420ToYUV422(char* pYUV420Buf, int iSrcWidth, int iSrcHeight,
		char* pYUV422Buf) {
	unsigned int nIamgeSize = iSrcWidth * iSrcHeight;
	int i;
	if ((pYUV420Buf == NULL) || (pYUV422Buf == NULL)) {
		return;
	}
	//Copy Y
	for (i = 0; i < iSrcHeight; i++) {
		memcpy(pYUV422Buf + i * iSrcWidth, pYUV420Buf + i * iSrcWidth,
				iSrcWidth);
	}
	//Copy U
	for (i = 0; i < iSrcHeight / 2; i++) {
		memcpy(pYUV422Buf + nIamgeSize + (2 * i) * iSrcWidth / 2,
				pYUV420Buf + nIamgeSize + i * iSrcWidth / 2, iSrcWidth / 2);
		memcpy(pYUV422Buf + nIamgeSize + (2 * i + 1) * iSrcWidth / 2,
				pYUV420Buf + nIamgeSize + i * iSrcWidth / 2, iSrcWidth / 2);
	}
	//Copy V
	for (i = 0; i < iSrcHeight / 2; i++) {
		memcpy(pYUV422Buf + nIamgeSize * 3 / 2 + (2 * i) * iSrcWidth / 2,
				pYUV420Buf + nIamgeSize * 5 / 4 + i * iSrcWidth / 2,
				iSrcWidth / 2);
		memcpy(pYUV422Buf + nIamgeSize * 3 / 2 + (2 * i + 1) * iSrcWidth / 2,
				pYUV420Buf + nIamgeSize * 5 / 4 + i * iSrcWidth / 2,
				iSrcWidth / 2);
	}
}
//工具函数:YUV422转YUV420的函数
void _YUV422ToYUV420(char* pYUV422Buf, int iSrcWidth, int iSrcHeight,
		char* pYUV420Buf) {
	unsigned int nIamgeSize = iSrcWidth * iSrcHeight;
	int i;

	if ((pYUV422Buf == NULL) || (pYUV420Buf == NULL)) {
		return;
	}
	//Copy Y
	for (i = 0; i < iSrcHeight; i++) {
		memcpy(pYUV420Buf + i * iSrcWidth, pYUV422Buf + i * iSrcWidth,
				iSrcWidth);
	}
	//Copy U
	for (i = 0; i < iSrcHeight / 2; i++) {
		memcpy(pYUV420Buf + nIamgeSize + i * iSrcWidth / 2,
				pYUV422Buf + nIamgeSize + 2 * i * iSrcWidth / 2, iSrcWidth / 2);
	}
	//Copy V
	for (i = 0; i < iSrcHeight / 2; i++) {
		memcpy(pYUV420Buf + nIamgeSize * 5 / 4 + i * iSrcWidth / 2,
				pYUV422Buf + nIamgeSize * 3 / 2 + 2 * i * iSrcWidth / 2,
				iSrcWidth / 2);
	}
}
//在YUV源图上从起始位置为(x,y)的地方截取指定大小的子区域
void _GetSubReginFromYUV420(unsigned char *src_yuv, int srcW, int srcH, //yuv源图
		unsigned char *sub_yuv, int x, int y, int subW, int subH) //yuv子区域的xy坐标及宽高
{
	unsigned char *pSrcY, *pSrcU, *pSrcV;
	unsigned char *pSubY, *pSubU, *pSubV;
	int i;
	int srcTmp = srcW * srcH;
	int subTmp = subW * subH;

	if (src_yuv == NULL || sub_yuv == NULL)
		return;

	if (subW > srcW || (x + subW) > srcW || subH > srcH || (y + subH) > srcH) {
		return;
	}

	//拷贝Y数据
	for (i = 0; i < subH; i++) {
		pSrcY = src_yuv + srcW * (y + i) + x;
		pSubY = sub_yuv + subW * i;
		memcpy(pSubY, pSrcY, subW);
	}

	//拷贝U数据
	for (i = 0; i < (subH / 2); i++) {
		pSrcU = (src_yuv + srcTmp) + (srcW / 2) * (y / 2 + i) + x / 2;
		pSubU = (sub_yuv + subTmp) + (subW / 2) * i;
		memcpy(pSubU, pSrcU, subW / 2);
	}

	//拷贝V数据
	for (i = 0; i < (subH / 2); i++) {
		pSrcV = (src_yuv + srcTmp + srcTmp / 4) + (srcW / 2) * (y / 2 + i)
				+ x / 2;
		pSubV = (sub_yuv + subTmp + subTmp / 4) + (subW / 2) * i;
		memcpy(pSubV, pSrcV, subW / 2);
	}
}
//修改yuv
void _SetSubReginToYUV420(unsigned char *src_yuv, int srcW, int srcH, //yuv源图
		unsigned char *sub_yuv, int x, int y, int subW, int subH) //yuv子区域
{
	unsigned char *pSrcY, *pSrcU, *pSrcV;
	unsigned char *pSubY, *pSubU, *pSubV;
	int i;
	int srcTmp = srcW * srcH;
	int subTmp = subW * subH;

	if (src_yuv == NULL || sub_yuv == NULL)
		return;

	if (subW > srcW || (x + subW) > srcW || subH > srcH || (y + subH) > srcH) {
		return;
	}

	//拷贝Y数据
	for (i = 0; i < subH; i++) {
		pSrcY = src_yuv + srcW * (y + i) + x;
		pSubY = sub_yuv + subW * i;
		memcpy(pSrcY, pSubY, subW);
	}

	//拷贝U数据
	for (i = 0; i < (subH / 2); i++) {
		pSrcU = (src_yuv + srcTmp) + (srcW / 2) * (y / 2 + i) + x / 2;
		pSubU = (sub_yuv + subTmp) + (subW / 2) * i;
		memcpy(pSrcU, pSubU, subW / 2);
	}

	//拷贝V数据
	for (i = 0; i < (subH / 2); i++) {
		pSrcV = (src_yuv + srcTmp + srcTmp / 4) + (srcW / 2) * (y / 2 + i)
				+ x / 2;
		pSubV = (sub_yuv + subTmp + subTmp / 4) + (subW / 2) * i;
		memcpy(pSrcV, pSubV, subW / 2);
	}
}
//在RGB上叠加文字
void _OverlapCaptionOnRGB(unsigned char* srcRgbBuf, int nWidth, int nHeight,
		char* pCharcode, O_STRINGATTR *pOAttrCharObj) {
	int i, j, k, m;
	unsigned char qh, wh;
	unsigned long offset;
	unsigned int pixelCount;
	char frontBuffer[32] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xF0,
			0x18, 0x3C, 0x18, 0x0E, 0x18, 0x0E, 0x18, 0x0E, 0x18, 0x0F, 0x18,
			0x0E, 0x18, 0x0E, 0x18, 0x0C, 0x18, 0x38, 0x7F, 0xE0, 0x00, 0x00,
			0x00, 0x00 };
	int iStartXpos = 0;
	int iStartYpos = 0;
	unsigned int nStrlen = strlen(pCharcode);
	unsigned char bIsChar = 0;

	//断言保护
	if ((srcRgbBuf == NULL) || (pCharcode == NULL)) {
		return;
	}

	//逐个读取字符，逐个叠加
	for (m = 0; m < nStrlen;) {

		memset(frontBuffer, 0, sizeof(frontBuffer));

		//取得点阵信息
		if (pCharcode[m] & 0x80) { //汉字
			qh = pCharcode[m] - 0xa0;
			wh = pCharcode[m + 1] - 0xa0;
			offset = (94 * (qh - 1) + (wh - 1)) * 32;
			fseek(g_fpHZKLIB, offset, SEEK_SET);
			fread(frontBuffer, 32, 1, g_fpHZKLIB);
			m += 2;
			bIsChar = 0;
		} else { //字符
			offset = pCharcode[m] * 32;
			fseek(g_fpASCII, offset, SEEK_SET);
			fread(frontBuffer, 32, 1, g_fpASCII);
			m++;
			bIsChar = 1;
		}

		//叠加
		for (j = 0; j < 16; j++) {
			pixelCount = 0;
			for (i = 0; i < 2; i++) {
				for (k = 0; k < 8; k++) {
					if (((frontBuffer[j * 2 + i] >> (7 - k)) & 0x1) != 0) {
						srcRgbBuf[nWidth * 3 * j + (i * 8 + k + iStartXpos) * 3] =
								pOAttrCharObj->osdB;
						srcRgbBuf[nWidth * 3 * j + (i * 8 + k + iStartXpos) * 3
								+ 1] = pOAttrCharObj->osdG;
						srcRgbBuf[nWidth * 3 * j + (i * 8 + k + iStartXpos) * 3
								+ 2] = pOAttrCharObj->osdR;
					}
					//if (k%2==0){
					pixelCount++;
					//}
				}
			}
		}

		//区分汉字及字符所占宽度
		if (bIsChar == 0) {
			iStartXpos += 16;
		} else {
			iStartXpos += 16;
		}

		if (iStartXpos > nWidth)
			return;

	}
}
//间接在YUV420上叠加字幕
char _OverLapCaptionOnYUV420(unsigned char *src_yuv, int srcW, int srcH, //源图及宽高
		int xStart, int yStart, int dstW, int dstH, //要叠加的字符串的位置及宽高
		char* pCaption, O_STRINGATTR *pOAttrCharObj) //要叠加的字符串及叠加属性
{
	/*
	 FILE *pSubYuv = fopen("AsubYUV.yuv","wb");
	 FILE *pSubRGB = fopen("AsubRGB.rgb","wb");
	 FILE *pDstYuv = fopen("AdstYUV.yuv","wb");
	 FILE *pDstRGB = fopen("AdstRGB.rgb","wb");
	 */

	int sub_W = 0;
	int sub_H = pOAttrCharObj->sizeH;
	int m = 0;
	int nStrlen = strlen(pCaption);
	//计算要截取的区域宽度
	for (m = 0; m < nStrlen;) {
		if (pCaption[m] & 0x80)
			m += 2;
		else
			//字符
			m += 1;

		sub_W += pOAttrCharObj->sizeW;
	}

	sub_W = (sub_W + xStart) > srcW ? (srcW - xStart) : sub_W;
	sub_H = (sub_H + yStart) > srcH ? (srcH - yStart) : sub_H;

	assert(src_yuv && pCaption && pOAttrCharObj);

	//抠像
	_GetSubReginFromYUV420(src_yuv, srcW, srcH, subYUVBuf, xStart, yStart,
			sub_W, sub_H);

//	LOGE("_GetSubReginFromYUV420 success");
	//fwrite(subYUVBuf,1,sub_W*sub_H*3/2,pSubYuv);
	//fflush(pSubYuv);
	//fclose(pSubYuv);

	//yuv转换成RGB
	_YUV420ToRGB24(subYUVBuf, srcRGBBuf, sub_W, sub_H);
//	LOGE("_YUV420ToRGB24 success");
	//fwrite(srcRGBBuf,1,sub_W*sub_H*3,pSubRGB);
	//fflush(pSubRGB);
	//fclose(pSubRGB);

	//在RGB上完成叠加
	_OverlapCaptionOnRGB(srcRGBBuf, sub_W, sub_H, pCaption, pOAttrCharObj);
//	LOGE("_OverlapCaptionOnRGB success");
	//fwrite(dstRGBBuf,1,sub_W*sub_H*3,pDstRGB);
	//fflush(pDstRGB);
	//fclose(pDstRGB);

	//rgb转换成YUV
	_RGB24ToYUV420(srcRGBBuf, sub_W, sub_H, dstYUVBuf, sub_W * sub_H * 3 / 2);
//	LOGE("_RGB24ToYUV420 success");
	//fwrite(dstYUVBuf,1,sub_W*sub_H*3/2,pDstYuv);
	//fflush(pDstYuv);
	//fclose(pDstYuv);

	//抠像叠加
	_SetSubReginToYUV420(src_yuv, srcW, srcH, dstYUVBuf, xStart, yStart, sub_W,
			sub_H);
//	LOGE("_SetSubReginToYUV420 success");
	return ERR_NONE;
}
//直接在YUV422上叠加字符
void _OverLapCaptionOnYUV422Raw(char* pCharcode, int column, int row,
		int imageWidth, int imageHeight, char *pYUVbuffer, char OsdY, char OsdU,
		char OsdV) {
	int i, j, k, m;
	unsigned char qh, wh;
	unsigned long offset;
	unsigned int pixelCount;
	char frontBuffer[32] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xF0,
			0x18, 0x3C, 0x18, 0x0E, 0x18, 0x0E, 0x18, 0x0E, 0x18, 0x0F, 0x18,
			0x0E, 0x18, 0x0E, 0x18, 0x0C, 0x18, 0x38, 0x7F, 0xE0, 0x00, 0x00,
			0x00, 0x00 };
	int iStartXpos = 100;
	int iStartYpos = 100;
	unsigned int nStrlen = strlen(pCharcode);
	unsigned char bIsChar = 0;
	int temp1 = 0;
	int temp2 = 0;

	//断言保护
	if ((pCharcode == NULL) || (pYUVbuffer == NULL)) {
		return;
	}

	//逐个读取字符，逐个叠加
	for (m = 0; m < nStrlen;) {

		memset(frontBuffer, 0, sizeof(frontBuffer));

		//取得点阵信息
		if (pCharcode[m] & 0x80) {	//汉字
			qh = pCharcode[m] - 0xa0;
			wh = pCharcode[m + 1] - 0xa0;
			offset = (94 * (qh - 1) + (wh - 1)) * 32;
			fseek(g_fpHZKLIB, offset, SEEK_SET);
			fread(frontBuffer, 32, 1, g_fpHZKLIB);
			m += 2;
			bIsChar = 0;
		} else {	//字符
			offset = pCharcode[m] * 32;
			fseek(g_fpASCII, offset, SEEK_SET);
			fread(frontBuffer, 32, 1, g_fpASCII);
			m++;
			bIsChar = 1;
		}

		//叠加
		for (j = 0; j < 16; j++) {
			pixelCount = 0;
			for (i = 0; i < 2; i++) {
				for (k = 0; k < 8; k++) {
					if (((frontBuffer[j * 2 + i] >> (7 - k)) & 0x1) != 0) {
						pYUVbuffer[((j + iStartYpos) * imageWidth + iStartXpos
								+ pixelCount)] = OsdU;
						pYUVbuffer[((j + iStartYpos) * imageWidth + iStartXpos
								+ pixelCount) + 1] = OsdY;
						//pYUVbuffer[((j+iStartYpos)*imageWidth+iStartXpos+pixelCount)+2] = OsdY;
						//pYUVbuffer[((j+iStartYpos)*imageWidth+iStartXpos+pixelCount)+3] = OsdU;

						//temp1=imageWidth*imageHeight*5/4+((j+iStartYpos)*imageWidth+iStartXpos+pixelCount)/2;//色度修改
						//temp2=((j+iStartYpos)*imageWidth+iStartXpos+pixelCount)*2;
						//temp1=imageWidth*imageHeight*5/4+(j+iStartYpos)/2*352+(iStartXpos+pixelCount)/2;
						//pYUVbuffer[(int)(((j+iStartYpos)*imageWidth+iStartXpos+pixelCount)/4)+imageWidth*imageHeight] = 100;
						//pYUVbuffer[temp1] = 250;
						//pYUVbuffer[temp2] = 200;
						//pYUVbuffer[((j+iStartYpos)*imageWidth+iStartXpos+pixelCount)+2] = OsdV;
						//buffer[((index+row)*imageWidth+column+pixelCount)*2] = OsdU;
						//buffer[((index+row)*imageWidth+column+pixelCount)*2+1] = OsdY;
					}
					//if (k%2==0){
					pixelCount++;
					//}
				}
			}
		}

		//区分汉字及字符所占宽度
		if (bIsChar == 0) {
			iStartXpos += 32;
		} else {
			iStartXpos += 16;
		}

	}
}
