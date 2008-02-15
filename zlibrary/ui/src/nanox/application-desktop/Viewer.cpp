/*
 * Copyright (C) 2008 Alexander Egorov <lunohod@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <ZLibrary.h>
#include <ZLApplication.h>
#include "../../../../../fbreader/src/fbreader/FBReader.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "Viewer.h"
#include <sys/types.h>


int page;


#define ALLOW_RUN_EXE 1

extern ZLApplication *mainApplication;

extern GR_WINDOW_ID win;
extern GR_GC_ID gc;
extern GR_FONT_ID fontid;
GR_PIXELVAL *pix;

unsigned char *buf;


int InitDoc(char *fileName)
{
	printf("InitDoc\n");
#ifdef ALLOW_RUN_EXE
    __pid_t pid;
    if( strstr(fileName, ".exe.txt") || strstr(fileName, ".exe.fb2")) {
		pid = fork();
		if(!pid) {
			execve(fileName, NULL, NULL);
			exit(0);
		} else {
//			waitpid(pid, NULL, 0);
			exit(0);
			//return 0;
		}
    }
#endif

	printf("plugin init\n");
	printf("pid: %d\n", getpid());

	int i = 1;
	page = 0;
//	while(i) { sleep(1); }


	buf = (unsigned char *)malloc(800*600/4);
	pix = (GR_PIXELVAL*)malloc(600*800*4); 

	int argc = 1;
	char **argv = NULL;
	if (!ZLibrary::init(argc, argv)) {
		return 0;
	}
	ZLibrary::run(new FBReader(fileName));


	printf("plugin init end\n");

	return 1;
}

void GetPageData(void **data)
{
	printf("GetPageData\n");
	
//	sleep(1);
	
	GrReadArea(win, 0, 0, 600, 800, (GR_PIXELVAL*)pix); 

	unsigned char *h = (unsigned char*)pix;
	unsigned char *d = buf;
	int val, f;

#ifdef ARM
	f = 6;
	*d = 0;
	for(int i=0; i < 800*600; i++) {
//		*d &= ~(0x3 << f);
		if(f == 6) 
			*d = 0;
		
		*d |= (h[i] << f);

		f -= 2;
		if(f < 0) {
			f = 6;
			d++;
		}
	}
#else	

	f = 6;
	for(int i=0; i < 800*600; i++) {
		val = 0.222 * (int)*h + 0.707 * (int)*(h+1) + 0.071 * (int)*(h+2);
		if(f == 6) 
			*d = 0;
//		*d &= ~(0x3 << f);
		if(val < 0x2a)
			;							
		else if(val < 0x7f) 
			*d |= (0x1 << f);
		else if(val < 0xd4)
			*d |= (0x2 << f);
		else if(val <= 0xff)
			*d |= (0x3 << f);

		h += 4;
		f -= 2;		
		if(f < 0) {
			f = 6;
			d++;			
		}
	}
	
#endif

	//GrClearShareMem_Apollo(wid, gc, 0xff);	 


	*data = (void *)buf;
	printf("GetPageData3\n");

}

void vSetDisplayState(Apollo_State *state) {printf("1\n");}
extern "C"{
void vSetCurPage(int p) {printf("2\n");}
int bGetRotate() {printf("3\n"); return 0;}
void vSetRotate(int rot) {printf("4\n");}
void vGetTotalPage(int *iTotalPage) { *iTotalPage = 5; }
int GetPageIndex() { return 1; }
int Origin() {printf("6\n");}
void vFontBigger() {printf("7\n");}
int Bigger() {printf("8\n");}
int Smaller() {printf("9\n");}
int Rotate() {printf("10\n");}
int Fit() {printf("11\n");}
int Prev()
{ 
	printf("prev\n");
	page--;
	std::string x;
	x = "<PageUp>";
	mainApplication->doActionByKey(x);	
}
int Next()
{
	printf("next\n");
	page++;
	std::string x;
	x = "<PageDown>";
	mainApplication->doActionByKey(x);	
}

int IsStandardStatusBarVisible() { return 0; }
int GotoPage(int index) {printf("12\n");}
void Release() {printf("13\n");}
void GetPageDimension(int *width, int *height) { *width = 600; *height = 800; }
void SetPageDimension(int width, int height) {printf("14\n"); printf("setpagedimension: %dx%d\n", width, height);}
double dGetResizePro() {printf("15\n");}
void vSetResizePro(double dSetPro) {printf("16\n");}
int GetPageNum() { return page; }
void bGetUserData(void **vUserData, int *iUserDataLength) {printf("17\n");}
void vSetUserData(void *vUserData, int iUserDataLength){printf("18\n");}
int iGetDocPageWidth(){printf("19\n"); return 600;}
int iGetDocPageHeight(){printf("20\n"); return 800;}
unsigned short usGetLeftBarFlag(){printf("21\n");}
void   vEndInit(int iEndStyle){printf("22\n");}
void   vEndDoc()
{
	printf("vEndDoc\n");
	free(buf);
	free(pix);
	delete mainApplication;
	ZLibrary::shutdown();
}
int  iInitDocF(char *filename,int pageNo, int flag){printf("24\n"); return 0;}
void   vFirstBmp(char *fileName, int pageNo){printf("25\n");}
int	iGetCurDirPage(int idx, int level){printf("26\n");}
int	iCreateDirList() { printf("huj\n"); return 0;}
int	iGetDirNumber(){printf("27\n");}
unsigned short* usGetCurDirNameAndLen(int pos, int * len){printf("28\n");}
unsigned short* usGetCurDirName(int level, int index){printf("29\n");}
int iGetCurDirLen(int level, int index){printf("30\n");}
void   vClearAllDirList(){printf("31\n");}
int OpenLeaf( int pos ){printf("32\n");}
int  bCurItemIsLeaf(int pos){printf("33\n");}
void  vEnterChildDir(int pos){printf("34\n");}
void   vReturnParentDir(){printf("35\n");}
void   vFreeDir(){printf("36\n");}
}; //extern "C"
