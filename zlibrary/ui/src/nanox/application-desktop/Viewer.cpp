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

enum state_t {
	ST_NORMAL = 0,
	ST_LINK_NAV = 1
};

state_t cur_state;

std::vector<std::string> xxx_notes;

struct xxx_link {
	int x1, y1, x2, y2;
	std::string id;
	bool next;
};

std::vector<struct xxx_link> xxx_page_links;

void updateCoord(int &x, int &y, int &w, int&h);
void invert(int x, int y, int h, int w);

int cur_link_idx;

#define ALLOW_RUN_EXE 1

extern ZLApplication *mainApplication;

unsigned char *buf;

static CallbackFunction * v3_cb = NULL;

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
			waitpid(pid, NULL, 0);
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

	cur_state = ST_NORMAL;

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

	
	if(((FBReader *)mainApplication)->getMode() != FBReader::FOOTNOTE_MODE)
		xxx_notes.clear();

	//xxx_page_links.clear();

	if(toc_jump) {
		((FBReader *)mainApplication)->bookTextView().gotoParagraph(p);
		mainApplication->refreshWindow();	
		
		toc_jump = false;
		return;
	}

	if(p <= 0) {
		((FBReader *)mainApplication)->bookTextView().scrollToStartOfText();
	} else if((p + 1)  >= ((FBReader *)mainApplication)->bookTextView().pageNumber())
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
	//printf("vFontBigger\n");
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
	//printf("Bigger\n");
}
int Smaller() {//printf("9\n");
}
int Rotate() {//printf("10\n");
}
int Fit() {//printf("11\n");
}

void turnPage(int cnt)
{
	int l_cnt = cnt;

	ZLViewWidget::Angle rot = mainApplication->myViewWidget->rotation();
	
	if((rot == ZLViewWidget::DEGREES90) || (rot == ZLViewWidget::DEGREES180))	
		l_cnt = -l_cnt;

	ZLTextWordCursor endC = ((FBReader *)mainApplication)->bookTextView().endCursor();

	if((l_cnt > 0) &&			
			endC.paragraphCursor().isLast() &&
			endC.isEndOfParagraph()) {
		mainApplication->doAction(ActionCode::GOTO_NEXT_TOC_SECTION);	
		return;	
	}
	
	ZLTextWordCursor startC = ((FBReader *)mainApplication)->bookTextView().startCursor();
	if((l_cnt < 0) &&
			!startC.isNull() &&
			startC.isStartOfParagraph() &&
			startC.paragraphCursor().isFirst()
			) {
		mainApplication->doAction(ActionCode::GOTO_PREVIOUS_TOC_SECTION);	
		((FBReader *)mainApplication)->bookTextView().scrollToEndOfText();
		return;
	}

	switch(l_cnt) {
		case 1:
			mainApplication->doAction(ActionCode::LARGE_SCROLL_FORWARD);	
			break;
		case -1:
			mainApplication->doAction(ActionCode::LARGE_SCROLL_BACKWARD);
			break;
		case 10:
		case -10:
			vSetCurPage(GetPageIndex() + l_cnt);
			break;
		default:
			break;
	}
}

int Prev()
{  
	if(((FBReader *)mainApplication)->getMode() != FBReader::FOOTNOTE_MODE)
		xxx_notes.clear();
	//xxx_page_links.clear();
	
	turnPage(-1);

	return 1;
}
int Next()
{
	if(((FBReader *)mainApplication)->getMode() != FBReader::FOOTNOTE_MODE)
		xxx_notes.clear();
	//xxx_page_links.clear();

	turnPage(1);

	return 1;
}


void end_link_state()
{
	int x, y, w, h;	

	if(cur_link_idx < 0)
		return;

	v3_cb->BeginDialog();
	for(int tmp_idx = cur_link_idx; 
			(tmp_idx >= 0) && (tmp_idx < xxx_page_links.size());
			tmp_idx++) {					

		xxx_link cur_link = xxx_page_links.at(tmp_idx);
/*		for(int i = cur_link.x1/4; i <= (cur_link.x2 + 3)/4; i++) 
			for(int j = cur_link.y1; j <= cur_link.y2; j++)
				buf[i+j*150] = ~buf[i+j*150];
*/				

		x = cur_link.x1;
		y = cur_link.y1;
		w = cur_link.x2 - cur_link.x1;
		h = cur_link.y2-cur_link.y1, buf;
		updateCoord(x, y, w, h);
		invert(x, y, w, h);
		v3_cb->BlitBitmap(x, y, w, h, x, y, 600, 800, buf);

					if(!cur_link.next)
						break;
	}
	v3_cb->PartialPrint();
	v3_cb->EndDialog();
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
//	vEndDoc();
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
	sync();
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

	if(cur_state == ST_LINK_NAV) {		
		switch ( keyId ) {
			case KEY_OK: {
				xxx_link cur_link = xxx_page_links.at(cur_link_idx);

				end_link_state();
				cur_state = ST_NORMAL;
//				((FBReader *)mainApplication)->bookTextView()._onStylusPress((cur_link.x1 + cur_link.x2)/2, (cur_link.y1 + cur_link.y2)/2);
				((FBReader *)mainApplication)->tryShowFootnoteView(cur_link.id, false);
				return 1;
				break;
						 }

			case KEY_CANCEL:			
				cur_state = ST_NORMAL;
				end_link_state();
				return 2;
				break;

			case KEY_UP:
			case KEY_DOWN:
			case KEY_0:
			case KEY_9: {
NEXT:			int x, y, w, h;	
				xxx_link cur_link;

				v3_cb->BeginDialog();

				for(int tmp_idx = cur_link_idx; 
						(tmp_idx >= 0) && (tmp_idx < xxx_page_links.size());
						tmp_idx++) {					

					cur_link = xxx_page_links.at(tmp_idx);
/*					for(int i = cur_link.x1/4; i <= (cur_link.x2+3)/4; i++) 
						for(int j = cur_link.y1; j <= cur_link.y2; j++)
							buf[i+j*150] = ~buf[i+j*150];
*/							

					x = cur_link.x1;
					y = cur_link.y1;
					w = cur_link.x2 - cur_link.x1;
					h = cur_link.y2-cur_link.y1;

					updateCoord(x, y, w, h);
					invert(x, y, w, h);

					v3_cb->BlitBitmap(x, y, w, h, x, y, 600, 800, buf);
					//v3_cb->PartialPrint();

					if(!cur_link.next)
						break;
				}

				ZLViewWidget::Angle rot = mainApplication->myViewWidget->rotation();
				bool swapkeys = (rot == ZLViewWidget::DEGREES90) || (rot == ZLViewWidget::DEGREES180);

				if((keyId == KEY_UP) 
						|| (cur_link_idx == -1) 
						|| ((keyId == KEY_0) && !swapkeys)
						|| ((keyId == KEY_9) && swapkeys)) {

					if((cur_link_idx >= 0) && xxx_page_links.at(cur_link_idx).next) 
						while(++cur_link_idx && (cur_link_idx < xxx_page_links.size()) && xxx_page_links.at(cur_link_idx).next);
					cur_link_idx++;

					if(cur_link_idx >= xxx_page_links.size())
						cur_link_idx = 0;
				} else {
					--cur_link_idx;
					while(--cur_link_idx && (cur_link_idx >= 0) && xxx_page_links.at(cur_link_idx).next);

					if(cur_link_idx == -1)
						cur_link_idx = 0;
					else if(cur_link_idx < 0)
						cur_link_idx = xxx_page_links.size() - 1;
					else
						++cur_link_idx;
				}

				for(int tmp_idx = cur_link_idx; 
						(tmp_idx >= 0) && (tmp_idx < xxx_page_links.size());
						tmp_idx++) {					

					cur_link = xxx_page_links.at(tmp_idx);
/*					for(int i = cur_link.x1/4; i <= (cur_link.x2+3)/4; i++) 
						for(int j = cur_link.y1; j <= cur_link.y2; j++)
							buf[i+j*150] = ~buf[i+j*150];					
*/							

					x = cur_link.x1;
					y = cur_link.y1;
					w = cur_link.x2 - cur_link.x1;					
					h = cur_link.y2 - cur_link.y1;			

					updateCoord(x, y, w, h);
					invert(x, y, w, h);

					//v3_cb->BlitBitmap(0, 0, 600, 800, 0, 0, 600, 800, buf);
					v3_cb->BlitBitmap(x, y, w, h, x, y, 600, 800, buf);

					if(!cur_link.next)
						break;
				}
				v3_cb->PartialPrint();
				v3_cb->EndDialog();

				return 2;
				break;
						}
		}
		return 2;		
	}

	static std::vector<std::string>::iterator it;
	switch ( keyId ) {
		case KEY_8:
			vFontBigger();
			return 1;

		case LONG_KEY_8:
			mainApplication->doAction(ActionCode::ROTATE_SCREEN);
			return 1;

		case LONG_KEY_7:
			if(!xxx_page_links.empty()) {
				cur_state = ST_LINK_NAV;
				cur_link_idx = -1;
				goto NEXT;
				return 2;
			}
			break;

		case KEY_7:			
			if(((FBReader *)mainApplication)->getMode() == FBReader::FOOTNOTE_MODE)
				return 2;
			else if(xxx_notes.size() != 0) {
				it = xxx_notes.begin();
				((FBReader *)mainApplication)->setMode(FBReader::FOOTNOTE_MODE);
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

		case LONG_KEY_DOWN:
			turnPage(10);
			return 1;
			break;

		case LONG_KEY_UP:
			turnPage(-10);
			return 1;
			break;
			

		case LONG_KEY_NEXT: {
			ZLViewWidget::Angle rot = mainApplication->myViewWidget->rotation();
			bool swapkeys = (rot == ZLViewWidget::DEGREES90) || (rot == ZLViewWidget::DEGREES180);

			if(swapkeys)
				mainApplication->doAction(ActionCode::UNDO);
			else
				mainApplication->doAction(ActionCode::REDO);
			return 1;

/*			if(((FBReader *)mainApplication)->getMode() == FBReader::FOOTNOTE_MODE) {				
				if((it == xxx_notes.end()) || (++it == xxx_notes.end()))
					it = xxx_notes.begin();

				((FBReader *)mainApplication)->tryShowFootnoteView(*it, false);

				return 1;
			}
*/
			break;
							}



		case LONG_KEY_PREV: {
			ZLViewWidget::Angle rot = mainApplication->myViewWidget->rotation();
			bool swapkeys = (rot == ZLViewWidget::DEGREES90) || (rot == ZLViewWidget::DEGREES180);

			if(swapkeys)
				mainApplication->doAction(ActionCode::REDO);
			else
				mainApplication->doAction(ActionCode::UNDO);
			return 1;
/*			if(((FBReader *)mainApplication)->getMode() == FBReader::FOOTNOTE_MODE) {				
				if(it == xxx_notes.begin())
					it = xxx_notes.end();
				it--;

				((FBReader *)mainApplication)->tryShowFootnoteView(*it, false);

				return 1;
			}
*/
			break;
							}
			

		case LONG_KEY_CANCEL:
			mainApplication->doAction(ActionCode::CANCEL);
			return 1;
			break;
			

	}
	return 0;
}


void SetCallbackFunction(struct CallbackFunction *cb)
{
    v3_cb = cb;
}

}; //extern "C"

void updateCoord(int &x, int &y, int &w, int&h)
{
	ZLViewWidget::Angle angle = mainApplication->myViewWidget->rotation();
	switch(angle) {
		case ZLViewWidget::DEGREES90:
			{
				int tmp = x;
				x = 600 - y - h;
				y = tmp;

				int tmp2 = w;
				w = h;
				h = tmp2;
				break;
			}
		case ZLViewWidget::DEGREES180:
			{
				x = 600 - x - w;
				y = 800 - y - h;
				break;
			}
		case ZLViewWidget::DEGREES270:
			{
				int tmp = x;
				x = y;
				y = 800 - tmp - w;

				int tmp2 = w;
				w = h;
				h = tmp2;
				break;
			}
		default: break;
	}
}

void invert(int x, int y, int w, int h)
{
	for(int i = x / 4; i <= (x + w + 3) / 4; i++) 
		for(int j = y; j <= y + h; j++)
			buf[i+j*150] = ~buf[i+j*150];
}

