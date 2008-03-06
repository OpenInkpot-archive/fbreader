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
#include "../../../../../fbreader/src/fbreader/BookTextView.h""

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "Viewer.h"
#include <sys/types.h>

#include <sys/time.h>

int page;
bool init;


#define ALLOW_RUN_EXE 1

extern ZLApplication *mainApplication;

unsigned char *buf;

int InitDoc(char *fileName)
{
	printf("InitDoc\n");

	init = true;

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
    char *file;

	if(strstr(fileName, ".pdb.fb2")) {         
		char buf[200];
		sprintf(buf, "cp -f %s /root/abook/tmp.pdb", fileName);
		system(buf);
		char *pdbFile = "/root/abook/tmp.pdb";
		file = pdbFile;
	} else if(strstr(fileName, ".prc.fb2")) {         
		char buf[200];
		sprintf(buf, "cp -f %s /root/abook/tmp.prc", fileName);
		system(buf);
		char *prcFile = "/root/abook/tmp.prc";
		file = prcFile;
	} else
		file = fileName;


	printf("plugin init\n");

	page = 0;

	buf = (unsigned char *)malloc(800*600/4);

	int argc = 1;
	char **argv = NULL;
	if (!ZLibrary::init(argc, argv)) {
		return 0;
	}
	ZLibrary::run(new FBReader(file));


	printf("plugin init end\n");

	return 1;
}

void GetPageData(void **data)
{
	*data = (void *)buf;
}

void vSetDisplayState(Apollo_State *state) {printf("1\n");}
extern "C"{
void vSetCurPage(int p) { 
	if(init)
		return;
	((FBReader *)mainApplication)->bookTextView().gotoPage(p + 1);
	mainApplication->refreshWindow();	
}
int bGetRotate() {printf("GetRotate\n"); return 0;}
void vSetRotate(int rot) { printf("vSetRotate: %d\n", rot); }
void vGetTotalPage(int *iTotalPage) {  *iTotalPage = 0;}

int GetPageIndex() {
	return ((FBReader *)mainApplication)->bookTextView().pageNumber();
}

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

	return 1;
}
int Next()
{
	printf("next\n");
	page++;
	std::string x;
	x = "<PageDown>";


	struct timeval tv1, tv2;
	int sec, usec;
	gettimeofday(&tv1, NULL);


	mainApplication->doActionByKey(x);	

	gettimeofday(&tv2, NULL);
	sec = tv2.tv_sec - tv1.tv_sec;
	usec = tv2.tv_usec - tv1.tv_usec;
	if(usec < 0) {
		sec--;
		usec = -usec;
	}
	printf("Next: %d.%06d\n", sec, usec);
		
	return 1;
}

int IsStandardStatusBarVisible() { return 0; }
int GotoPage(int index) { 
	printf("GotoPage: %d\n", index);
}
void Release() {printf("13\n");}
void GetPageDimension(int *width, int *height) { *width = 600; *height = 800; }
void SetPageDimension(int width, int height) {printf("setpagedimension: %dx%d\n", width, height);}
double dGetResizePro() {printf("15\n");}
void vSetResizePro(double dSetPro) {printf("16\n");}
int GetPageNum() { return page; }
void bGetUserData(void **vUserData, int *iUserDataLength) {
/*    static int testData[] = {1, 2, 3, 4};

    *vUserData = testData;
    *iUserDataLength = 1;
*/	

// WTF?
	vEndDoc();
}
void vSetUserData(void *vUserData, int iUserDataLength){printf("18\n");}
int iGetDocPageWidth(){ return 600;}
int iGetDocPageHeight(){ return 800;}
unsigned short usGetLeftBarFlag(){printf("21\n");}
void   vEndInit(int iEndStyle) { printf("vEndInit: %d\n", iEndStyle); }
void   vEndDoc()
{
	free(buf);

	delete mainApplication;
	ZLibrary::shutdown();
}
int  iInitDocF(char *filename,int pageNo, int flag) { init = false; printf("iInitDocF: %c %d %d\n", filename, pageNo, flag); return 0;}
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
