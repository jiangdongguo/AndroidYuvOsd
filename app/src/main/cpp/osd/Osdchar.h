#ifndef _OSDCHAR_H_
#define _OSDCHAR_H_

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>

//错误码
#define ERR_NONE 0x00
#define ERR_OBJEXIST 0x01
#define ERR_OBJNOTEXIST 0x02
#define ERR_FILEOPENFAIL 0x03
#define ERR_PARA 0x04

//字符的行为类型
typedef enum {
	e_SCROLL_LEFT,
	e_SCROLL_RIGHT,
	e_SCROLL_UP,
	e_SCROLL_DOWN,
	e_MOVE_RAND,
	e_STATIC,
	e_SLOWHIDE,
	e_SPARK
} E_ACTIONTYPE;
//行为码表
/*
 e_ROLL:
 -val：向左滚动及滚动速度
 +val：向右滚动及滚动速度
 e_STATIC:
 无行为码
 e_SLOWHIDE:
 停留时间
 e_SPARK:
 闪烁间隔
 */
//叠加字符串对象属性
typedef struct _O_STRINGATTR {
	char osdR, osdG, osdB; //字符颜色
	char font; //字体类型
	char sizeW, sizeH; //字符点阵大小，只可为16*16或者32*32
	E_ACTIONTYPE eActionType; //行为类型
	int actionValue1; //行为码
	int actionValue2; //行为码

	/*
	 _O_STRINGATTR()
	 {
	 osdR=255;
	 osdG=255;
	 osdB=255;
	 }
	 */

} O_STRINGATTR, *PO_STRINGATTR, *LPO_STRINGATTR;

//叠加的字符串对象
typedef struct _O_OBJCHAR {
	int x, y, w, h; //叠加的位置
	char szStr[128]; //要叠加的字符
	O_STRINGATTR oAttrChar; //属性

} O_OBJCHAR, *PO_OBJCHAR, *LPO_OBJCHAR;

//要叠加的图像源的分辨率
#define  IMAGEWIDTH     1280
#define  IMAGEHEIGHT    720
//接口函数
extern char OSD_CreateObjCharObj(int strID, char *szStr,
		O_STRINGATTR OAttrCharObj);
extern char OSD_DeleteObjCharObj(int strID);
extern char OSD_SetContentCharObj(int strID, char *szStr);
extern char OSD_SetPositionCharObj(int strID, int x, int y);
extern char OSD_SetAttrCharObj(int strID, O_STRINGATTR OAttrCharObj);
extern void OSD_FeedFrameYUV420(char* pYUV420Frame, int iSrcWidth,
		int iSrcHeight);
extern char OSD_Init(char *szPathHZK, char *szPathASCII);
extern void OSD_Release();
#endif
