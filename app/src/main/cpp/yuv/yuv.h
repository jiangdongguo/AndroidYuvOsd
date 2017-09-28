#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

//#define FRAME_WIDTH             (1280)
//#define FRAME_HEIGHT            (720)
//#define FRAME_SIZE              (FRAME_WIDTH*FRAME_HEIGHT*3/2)

char *draw_Font_Func(char *ptr_frame, int FRAME_WIDTH, int FRAME_HEIGHT,
		int startx, int starty, int color, char *str,char * nameTable,int count);
