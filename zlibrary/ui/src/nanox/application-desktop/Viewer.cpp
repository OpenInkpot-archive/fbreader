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
#include "../../../../../fbreader/src/fbreader/BookTextView.h"
#include "../../../../../fbreader/src/fbreader/FBReaderActions.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "Viewer.h"
#include <sys/types.h>

#include <map>

#include <sys/time.h>

//#define _DEBUG 1

int glb_page;
bool init;

//std::map<int, std::string> xxx_myTOC;

struct xxx_toc_entry {
	int paragraph;
	std::string text;
};

std::vector<xxx_toc_entry> xxx_myTOC;

bool toc_jump;

std::vector<std::string> xxx_notes;

#define ALLOW_RUN_EXE 1

extern ZLApplication *mainApplication;

unsigned char *buf;

int InitDoc(char *fileName)
{
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

	//.FBReader/
	system("mv -f /home/.FBReader/UI.XML /home/.FBReader/ui.xml_ ; \
			mv -f /home/.FBReader/ui.xml_ /home/.FBReader/ui.xml ; \
			mv -f /home/.FBReader/BOOKS.XML /home/.FBReader/books.xml_; \
			mv -f /home/.FBReader/books.xml_ /home/.FBReader/books.xml; \
			mv -f /home/.FBReader/OPTIONS.XML /home/.FBReader/options.xml_; \
			mv -f /home/.FBReader/options.xml_ /home/.FBReader/options.xml; \
			mv -f /home/.FBReader/STATE.XML /home/.FBReader/state.xml_; \
			mv -f /home/.FBReader/state.xml_ /home/.FBReader/state.xml; \
			mv -f /home/.FBReader/SYSTEM.XML /home/.FBReader/system.xml_; \
			mv -f /home/.FBReader/system.xml_ /home/.FBReader/system.xml");

	glb_page = 0;
	toc_jump = false;

	buf = (unsigned char *)malloc(800*600/4);

	int argc = 1;
	char **argv = NULL;
	if (!ZLibrary::init(argc, argv)) {
		return 0;
	}
	ZLibrary::run(new FBReader(file));

	return 1;
}

void GetPageData(void **data)
{
#ifdef _DEBUG
	printf("getpagedata\n");
#endif
	*data = (void *)buf;
}

void vSetDisplayState(Apollo_State *state) {
#ifdef _DEBUG	
	printf("1\n");
#endif
}
extern "C"{
void vSetCurPage(int p) { 
#ifdef _DEBUG
	printf("vSetCurPage: %d\n", p);
#endif
	if(init)
		return;

	xxx_notes.clear();

	if(toc_jump) {
		((FBReader *)mainApplication)->bookTextView().gotoParagraph(p);
		mainApplication->refreshWindow();	
		
		toc_jump = false;
		return;
	}

	if(p <= 0) {
		((FBReader *)mainApplication)->bookTextView().scrollToStartOfText();
	} else if((p + 1)  == ((FBReader *)mainApplication)->bookTextView().pageNumber())
		 ((FBReader *)mainApplication)->bookTextView().scrollToEndOfText();
	else {
		((FBReader *)mainApplication)->bookTextView().gotoPage(p + 1);
		mainApplication->refreshWindow();	
	}
}
int bGetRotate() {
#ifdef _DEBUG	
	printf("GetRotate\n");
#endif
	
	return 0;
}

void vSetRotate(int rot) { 
#ifdef _DEBUG	
	printf("vSetRotate: %d\n", rot); 
#endif
}
void vGetTotalPage(int *iTotalPage) {  
	*iTotalPage = ((FBReader *)mainApplication)->bookTextView().pageNumber();
}

int GetPageIndex() {
#ifdef _DEBUG
	printf("getpageindex >%d\n", glb_page-1);
#endif
	return glb_page - 1;
}

int Origin() {
	//printf("6\n");
}
void vFontBigger() {
	ZLIntegerRangeOption &option = ZLTextStyleCollection::instance().baseStyle().FontSizeOption;

	int size = option.value();
	if(size % 2)
		size += 3;
	else
		size += 2;

	if(size > 14)
		size = 8;

	option.setValue(size);
	((FBReader *)mainApplication)->clearTextCaches();
	mainApplication->refreshWindow();
}
int Bigger() {
	//printf("8\n");
}
int Smaller() {//printf("9\n");
}
int Rotate() {//printf("10\n");
}
int Fit() {//printf("11\n");
}
int Prev()
{  
	xxx_notes.clear();
	mainApplication->doAction(ActionCode::LARGE_SCROLL_BACKWARD);
	return 1;
}
int Next()
{
	xxx_notes.clear();
	mainApplication->doAction(ActionCode::LARGE_SCROLL_FORWARD);
	return 1;
}

int IsStandardStatusBarVisible() { return 0; }
int GotoPage(int index) { 
#ifdef _DEBUG	
	printf("GotoPage: %d\n", index);
#endif
	if(index < 0 || index > ((FBReader *)mainApplication)->bookTextView().pageNumber())
		return 0;

	return 1;
}
void Release() {
#ifdef _DEBUG	
	printf("13\n");
#endif
}
void GetPageDimension(int *width, int *height) { *width = 600; *height = 800; }
void SetPageDimension(int width, int height) {
#ifdef _DEBUG		
	printf("setpagedimension: %dx%d\n", width, height);
#endif
}
double dGetResizePro() {
#ifdef _DEBUG		
	printf("15\n");
#endif
}
void vSetResizePro(double dSetPro) {
#ifdef _DEBUG		
	printf("16\n");
#endif
}
int GetPageNum() { 
	return ((FBReader *)mainApplication)->bookTextView().pageNumber();	
}
void bGetUserData(void **vUserData, int *iUserDataLength) {
/*    static int testData[] = {1, 2, 3, 4};

    *vUserData = testData;
    *iUserDataLength = 1;
*/	

// WTF?
	vEndDoc();
}
void vSetUserData(void *vUserData, int iUserDataLength){
#ifdef _DEBUG		
	printf("18\n");
#endif
}
int iGetDocPageWidth(){ return 600;}
int iGetDocPageHeight(){ return 800;}
unsigned short usGetLeftBarFlag(){
#ifdef _DEBUG		
	printf("21\n");
#endif
}
void   vEndInit(int iEndStyle) { 
#ifdef _DEBUG		
	printf("vEndInit: %d\n", iEndStyle); 
#endif
}
void   vEndDoc()
{
	if(((FBReader *)mainApplication)->getMode() == FBReader::FOOTNOTE_MODE)
		mainApplication->doAction(ActionCode::CANCEL);

	free(buf);

	delete mainApplication;
	ZLibrary::shutdown();
}
int  iInitDocF(char *filename,int pageNo, int flag) { 	
	init = false; 
#ifdef _DEBUG			
	printf("iInitDocF: %c %d %d\n", filename, pageNo, flag); 
#endif
	return 0;
}
void   vFirstBmp(char *fileName, int pageNo){
#ifdef _DEBUG			
	printf("25\n");
#endif
}
int	iGetCurDirPage(int idx, int level){
//	printf("26\n");

    int index = level * 8 + idx;

	toc_jump = true;
	return xxx_myTOC[index].paragraph;

}
int	iCreateDirList() { 
	if(xxx_myTOC.empty())
		return 0;
	else
		return 1;
}
int	iGetDirNumber(){
	return xxx_myTOC.size();
}
unsigned short* usGetCurDirNameAndLen(int pos, int * len){
    unsigned short         codepoint;
    unsigned char         in_code;
    int                   expect;

	int index = pos & 7;
    static unsigned short str[8][64] = {0};

	const char *p = xxx_myTOC[pos].text.c_str();
	int plen = xxx_myTOC[pos].text.size();

    int j=0;

	while (j < 60 && *p && plen)
	{
		in_code = *p++ ;

		if ( in_code >= 0xC0 )
		{
			if ( in_code < 0xE0 )           /*  U+0080 - U+07FF   */
			{
				expect = 1;
				codepoint = in_code & 0x1F;
			}
			else if ( in_code < 0xF0 )      /*  U+0800 - U+FFFF   */
			{
				expect = 2;
				codepoint = in_code & 0x0F;
			}
			else if ( in_code < 0xF8 )      /* U+10000 - U+10FFFF */
			{
				expect = 3;
				codepoint = in_code & 0x07;
			}
			continue;
		}
		else if ( in_code >= 0x80 )
		{
			--expect;

			if ( expect >= 0 )
			{
				codepoint <<= 6;
				codepoint  += in_code & 0x3F;
			}
			if ( expect >  0 )
				continue;

			expect = 0;
		}
		else                              /* ASCII, U+0000 - U+007F */
			codepoint = in_code;

		plen--;
        str[index][j++] = codepoint;
    }
    str[index][j] = 0;

    *len = j;
	return str[index];
}
unsigned short* usGetCurDirName(int level, int index){
#ifdef _DEBUG			
	printf("29\n");
#endif
}
int iGetCurDirLen(int level, int index){
#ifdef _DEBUG			
	printf("30\n"); 
#endif
	return 11;
}
void   vClearAllDirList(){
#ifdef _DEBUG			
	printf("31\n");
#endif
}
int OpenLeaf( int pos ){
//	((FBReader *)mainApplication)->bookTextView().gotoParagraph(xxx_myTOC[pos-1].paragraph);
//	mainApplication->refreshWindow();	

	return 1;
}

int  bCurItemIsLeaf(int pos){
#ifdef _DEBUG			
	printf("33\n"); 
#endif
	return 1;
}
void  vEnterChildDir(int pos){
#ifdef _DEBUG			
	printf("34\n");
#endif
}
void   vReturnParentDir(){
#ifdef _DEBUG			
	printf("35\n");
#endif
}
void   vFreeDir(){
#ifdef _DEBUG			
	printf("36\n");
#endif
}


int OnKeyPressed(int keyId, int state)
{
	static std::vector<std::string>::iterator it;
	switch ( keyId ) {
		case KEY_7:
			if(xxx_notes.size() != 0) {
				it = xxx_notes.begin();
				((FBReader *)mainApplication)->tryShowFootnoteView(*it, false);
				return 1;
			}
			break;

		case KEY_CANCEL:
			if(((FBReader *)mainApplication)->getMode() == FBReader::FOOTNOTE_MODE) {
				mainApplication->doAction(ActionCode::CANCEL);

				return 1;
			}
			break;

		case KEY_OK:
			if(((FBReader *)mainApplication)->getMode() == FBReader::FOOTNOTE_MODE) {				
				if((it == xxx_notes.end()) || (++it == xxx_notes.end()))
					it = xxx_notes.begin();

				//mainApplication->doAction(ActionCode::CANCEL);
				((FBReader *)mainApplication)->tryShowFootnoteView(*it, false);

				return 1;
			}
			break;
	}
	return 0;
}


}; //extern "C"
