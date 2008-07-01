/*
 * Copyright (C) 2008 Alexander Kerner <lunohod@gmx.de>
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
#include "../../../../../fbreader/src/description/BookDescriptionUtil.h"
#include "../view/ZLNXPaintContext.h"
#include "../../../../../fbreader/src/formats/FormatPlugin.h"
#include "../../../../../fbreader/src/bookmodel/BookModel.h"
#include <ZLFile.h>
#include <ZLLanguageList.h>

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
	ST_LINK_NAV = 1,
	ST_CLOCK = 2,
	ST_FONT_SEL = 3,
	ST_FONT_SIZE = 4,
	ST_INTERLINE = 5,
	ST_MENU_ENC = 6,
	ST_ENC = 7,
	ST_LANG = 8,
	ST_MENU_FONT = 9
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

std::vector<std::string> encodings;

#define ALLOW_RUN_EXE 1

extern ZLApplication *mainApplication;

unsigned char *buf;

char *fileName_InitDoc;

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

	fileName_InitDoc = fileName;

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

int hour, min, pos;

void drawClock(bool init = false)
{
	shared_ptr<ZLPaintContext> pc = mainApplication->context();

	ZLColor c;
	c.Red = 0;
	c.Green = 0;
	c.Blue = 0;
	pc->setFillColor(c);
	pc->fillRectangle(235, 350, 368, 463);
	c.Red = 0xff;
	c.Green = 0xff;
	c.Blue = 0xff;
	pc->setFillColor(c);
	pc->fillRectangle(238, 353, 365, 460);

	pc->setFont("Arial", 14, false, false);
	char *clock = v3_cb->GetString("VIEWER_CLOCK");
	if(clock != NULL)
		pc->drawString(245, 385, clock, strlen(clock));
	else
		pc->drawString(245, 385, "Clock", 5);
	pc->drawLine(238, 395, 368, 395);
	pc->drawLine(238, 396, 368, 396);

	char t[10];
	sprintf(t, "%02u:%02u", hour, min);

	int x = 245;
	int y = 445;

	for(int i = 0; i < 5; i++) {
		bool bold;
		if(pos == i)
			bold = true;
		else
			bold = false;
		pc->setFont("Arial", 20, bold, false);
		pc->drawString(x, y, t + i, 1);
		x += pc->stringWidth(t + i, 1);
	}

	v3_cb->BeginDialog();
	if(!init) {
		v3_cb->BlitBitmap(238, 353, 130, 110, 238, 353, 600, 800, buf);
		v3_cb->PartialPrint();
	}
	v3_cb->EndDialog();
}

int setClock(int keyId)
{
	if(keyId == NULL) {
		drawClock(true);
		return 3;
	}

	int key;

	switch(keyId) {
		case KEY_OK:
			//set_clock
			struct timeval t;
			t.tv_sec = hour * 3600 + min * 60;
			settimeofday(&t, NULL);
		case KEY_CANCEL:
			cur_state = ST_NORMAL;
			mainApplication->refreshWindow();
			return 1;
			break;
		case KEY_0: key = 0; break;
		case KEY_1: key = 1; break;
		case KEY_2: key = 2; break;
		case KEY_3: key = 3; break;
		case KEY_4: key = 4; break;
		case KEY_5: key = 5; break;
		case KEY_6: key = 6; break;
		case KEY_7: key = 7; break;
		case KEY_8: key = 8; break;
		case KEY_9: key = 9; break;
		default: return 2;
	}

	if((pos == 0) && (key < 3)) {
		hour = hour % 10 + key * 10;
		if(hour > 24)
			hour = 24;
		pos++;
		drawClock();
	} else if((pos == 1)  && ((hour < 20) || (key <= 4))) {
		hour = (hour / 10) * 10 + key;
		pos += 2;
		drawClock();
	} else if((pos == 3) && (key < 6)) {
		min = min % 10 + key * 10;
		if(min > 59)
			min = 59;
		pos++;
		drawClock();
	} else if(pos == 4) {
		min = (min / 10) * 10 + key;
		pos = 0;
		drawClock();
	}

	return 2;
}

std::vector<std::string> ffamilies;
int fontsel_pg;
int enc_pg;
int lang_pg;

void drawMenuFont(bool init = false);
void drawFontSel(bool init = false)
{
	int x0 = 100 , y0 = 175, x1 = 500, y1 = 625;


	shared_ptr<ZLPaintContext> pc = mainApplication->context();
	if(init) {
		fontsel_pg = 0;
	}

	ZLColor c;
	c.Red = 0;
	c.Green = 0;
	c.Blue = 0;
	pc->setFillColor(c);
	pc->fillRectangle(x0, y0, x1 + 3, y1 + 3);
	c.Red = 0xff;
	c.Green = 0xff;
	c.Blue = 0xff;
	pc->setFillColor(c);
	pc->fillRectangle(x0 + 2, y0 + 3, x1, y1);

	pc->setFont("Arial", 14, false, false);
	char *bf = v3_cb->GetString("VIEWER_FONT");
	if(bf != NULL)
		pc->drawString(x0 + 5, y0 + 35, bf, strlen(bf));
	else
		pc->drawString(x0 + 5, y0 + 35, "Base font family", 16);
	pc->drawLine(x0, y0 + 45, x1, y0 + 45);
	pc->drawLine(x0, y0 + 46, x1, y0 + 46);

	char l[100];
	int i = 0;
	while((i < 8) && ((fontsel_pg * 8) + i) < ffamilies.size()) {
		int idx = fontsel_pg * 8 + i;
		sprintf(l, "%d. %s", i + 1, ffamilies.at(idx).c_str());
		pc->setFont(ffamilies.at(idx).c_str(), 14, false, false);
		pc->drawString(x0 + 10, y0 + 90 + 40 * i, l, strlen(l));
		i++;
	}

	pc->drawLine(x0, y1 - 45, x1, y1 - 45);
	pc->drawLine(x0, y1 - 46, x1, y1 - 46);

	int pgs = ffamilies.size() / 8; 
	if((ffamilies.size() % 8) > 0)
		pgs++;
	sprintf(l, "%d / %d", fontsel_pg + 1, pgs);
	pc->setFont("Arial", 14, false, false);
	pc->drawString(x0 + 5, y1 - 10, l, strlen(l));

	pc->drawLine(x0 + 95, y1 - 45, x0 + 95, y1);
	pc->drawLine(x0 + 96, y1 - 45, x0 + 96, y1);
	ZLStringOption &option = ZLTextStyleCollection::instance().baseStyle().FontFamilyOption;
	sprintf(l, "%s", option.value().c_str());
	pc->drawString(x0 + 106, y1 - 10, l, strlen(l));

	v3_cb->BeginDialog();
	if(!init) {
		v3_cb->BlitBitmap(x0, y0, x1 - x0, y1 - y0, x0, y0, 600, 800, buf);
		v3_cb->PartialPrint();
	}
	v3_cb->EndDialog();
}

int setFont(int keyId)
{
	if(keyId == NULL) {
		drawFontSel();		
		return 2;
	}

	int key;

	switch(keyId) {
		case KEY_OK:
		case KEY_CANCEL:
			cur_state = ST_MENU_FONT;
			mainApplication->refreshWindow();
			drawMenuFont();
			return 2;
			break;
		case KEY_DOWN:
		case KEY_0: key = 0; break;
		case KEY_1: key = 1; break;
		case KEY_2: key = 2; break;
		case KEY_3: key = 3; break;
		case KEY_4: key = 4; break;
		case KEY_5: key = 5; break;
		case KEY_6: key = 6; break;
		case KEY_7: key = 7; break;
		case KEY_8: key = 8; break;
		case KEY_UP:
		case KEY_9: key = 9; break;
		default: return 2;
	}

	if(key == 0) {
		int pgs = ffamilies.size() / 8; 
		if((ffamilies.size() % 8) > 0)
			pgs++;
		if(fontsel_pg < (pgs - 1)) {
			fontsel_pg++;
			drawFontSel();
		}
	} else if(key == 9) {
		if(fontsel_pg > 0) {
			fontsel_pg--;
			drawFontSel();
		}
	} else {
		int idx = key - 1 + fontsel_pg * 8;
		if(idx < ffamilies.size()) {
			// set font
			ZLStringOption &option = ZLTextStyleCollection::instance().baseStyle().FontFamilyOption;
			option.setValue(ffamilies.at(idx));
			((FBReader *)mainApplication)->clearTextCaches();
			cur_state = ST_MENU_FONT;
			mainApplication->refreshWindow();
			drawMenuFont();
			return 1;
		}
	}

	return 2;
}

void drawFontSize(bool init = false)
{
	int x0 = 100 , y0 = 175, x1 = 500, y1 = 625;

	shared_ptr<ZLPaintContext> pc = mainApplication->context();

	ZLColor c;
	c.Red = 0;
	c.Green = 0;
	c.Blue = 0;
	pc->setFillColor(c);
	pc->fillRectangle(x0, y0, x1 + 3, y1 + 3);
	c.Red = 0xff;
	c.Green = 0xff;
	c.Blue = 0xff;
	pc->setFillColor(c);
	pc->fillRectangle(x0 + 2, y0 + 3, x1, y1);

	pc->setFont("Arial", 14, false, false);
	char *bf = v3_cb->GetString("VIEWER_FONT_SIZE");
	if(bf != NULL)
		pc->drawString(x0 + 5, y0 + 35, bf, strlen(bf));
	else
		pc->drawString(x0 + 5, y0 + 35, "Base font size", 14);
	pc->drawLine(x0, y0 + 45, x1, y0 + 45);
	pc->drawLine(x0, y0 + 46, x1, y0 + 46);

	char l[100];
	int i = 0;
	while(i < 8) {
		int idx = fontsel_pg * 8 + i;
		sprintf(l, "%d. %d pt", i + 1, 2 * i + 6);
		pc->drawString(x0 + 10, y0 + 90 + 40 * i, l, strlen(l));
		i++;
	}

	pc->drawLine(x0, y1 - 45, x1, y1 - 45);
	pc->drawLine(x0, y1 - 46, x1, y1 - 46);

	sprintf(l, "%d / %d", 1, 1);
	pc->setFont("Arial", 14, false, false);
	pc->drawString(x0 + 5, y1 - 10, l, strlen(l));

	pc->drawLine(x0 + 95, y1 - 45, x0 + 95, y1);
	pc->drawLine(x0 + 96, y1 - 45, x0 + 96, y1);
	ZLIntegerRangeOption &option = ZLTextStyleCollection::instance().baseStyle().FontSizeOption;
	sprintf(l, "%d pt", option.value());
	pc->drawString(x0 + 106, y1 - 10, l, strlen(l));

	v3_cb->BeginDialog();
	if(!init) {
		v3_cb->BlitBitmap(x0, y0, x1 - x0, y1 - y0, x0, y0, 600, 800, buf);
		v3_cb->PartialPrint();
	}
	v3_cb->EndDialog();
}

int setFontSize(int keyId)
{
	if(keyId == NULL) {
		drawFontSize();
		return 2;
	}

	int key;

	switch(keyId) {
		case KEY_OK:
		case KEY_CANCEL:
			cur_state = ST_MENU_FONT;
			mainApplication->refreshWindow();
			drawMenuFont();
			return 2;
			break;
//		case KEY_0: key = 0; break;
		case KEY_1: key = 1; break;
		case KEY_2: key = 2; break;
		case KEY_3: key = 3; break;
		case KEY_4: key = 4; break;
		case KEY_5: key = 5; break;
		case KEY_6: key = 6; break;
		case KEY_7: key = 7; break;
		case KEY_8: key = 8; break;
//		case KEY_9: key = 9; break;
		default: return 2;
	}

	int size = 2 * key + 4;
	// set size 
	ZLIntegerRangeOption &option = ZLTextStyleCollection::instance().baseStyle().FontSizeOption;
	option.setValue(size);
	((FBReader *)mainApplication)->clearTextCaches();
	cur_state = ST_MENU_FONT;
	mainApplication->refreshWindow();
	drawMenuFont();
	return 1;
}

void drawInterLine(bool init = false)
{
	int x0 = 100 , y0 = 175, x1 = 500, y1 = 625;

	shared_ptr<ZLPaintContext> pc = mainApplication->context();

	ZLColor c;
	c.Red = 0;
	c.Green = 0;
	c.Blue = 0;
	pc->setFillColor(c);
	pc->fillRectangle(x0, y0, x1 + 3, y1 + 3);
	c.Red = 0xff;
	c.Green = 0xff;
	c.Blue = 0xff;
	pc->setFillColor(c);
	pc->fillRectangle(x0 + 2, y0 + 3, x1, y1);

	pc->setFont("Arial", 14, false, false);
	char *bf = v3_cb->GetString("VIEWER_INTERLINE");
	if(bf != NULL)
		pc->drawString(x0 + 5, y0 + 35, bf, strlen(bf));
	else
		pc->drawString(x0 + 5, y0 + 35, "Interline space", 15);
	pc->drawLine(x0, y0 + 45, x1, y0 + 45);
	pc->drawLine(x0, y0 + 46, x1, y0 + 46);

	char l[100];
	int i = 0;
	while(i < 8) {
		int idx = fontsel_pg * 8 + i;
		sprintf(l, "%d. %d %%", i + 1, 10 * i + 80);
		pc->drawString(x0 + 10, y0 + 90 + 40 * i, l, strlen(l));
		i++;
	}

	pc->drawLine(x0, y1 - 45, x1, y1 - 45);
	pc->drawLine(x0, y1 - 46, x1, y1 - 46);

	sprintf(l, "%d / %d", 1, 1);
	pc->setFont("Arial", 14, false, false);
	pc->drawString(x0 + 5, y1 - 10, l, strlen(l));

	pc->drawLine(x0 + 95, y1 - 45, x0 + 95, y1);
	pc->drawLine(x0 + 96, y1 - 45, x0 + 96, y1);
	ZLIntegerOption &option = ZLTextStyleCollection::instance().baseStyle().LineSpacePercentOption;
	sprintf(l, "%d %%", option.value());
	pc->drawString(x0 + 106, y1 - 10, l, strlen(l));

	v3_cb->BeginDialog();
	if(!init) {
		v3_cb->BlitBitmap(x0, y0, x1 - x0, y1 - y0, x0, y0, 600, 800, buf);
		v3_cb->PartialPrint();
	}
	v3_cb->EndDialog();
}

int setInterLine(int keyId)
{
	if(keyId == NULL) {
		drawInterLine();
		return 2;
	}

	int key;

	switch(keyId) {
		case KEY_OK:
		case KEY_CANCEL:
			mainApplication->refreshWindow();
			cur_state = ST_MENU_FONT;
			drawMenuFont();
			return 2;
			break;
//		case KEY_0: key = 0; break;
		case KEY_1: key = 1; break;
		case KEY_2: key = 2; break;
		case KEY_3: key = 3; break;
		case KEY_4: key = 4; break;
		case KEY_5: key = 5; break;
		case KEY_6: key = 6; break;
		case KEY_7: key = 7; break;
		case KEY_8: key = 8; break;
//		case KEY_9: key = 9; break;
		default: return 2;
	}

	int val = 10 * key + 70;
	// set size 
	ZLIntegerOption &option = ZLTextStyleCollection::instance().baseStyle().LineSpacePercentOption;
	option.setValue(val);
	((FBReader *)mainApplication)->clearTextCaches();
	cur_state = ST_MENU_FONT;
	mainApplication->refreshWindow();
	drawMenuFont();
	return 1;
}

BookInfo *myBookInfo;
void drawMenuEnc(bool init = false)
{
	int x0 = 100 , y0 = 175, x1 = 500, y1 = 625;

	shared_ptr<ZLPaintContext> pc = mainApplication->context();

	ZLColor c;
	c.Red = 0;
	c.Green = 0;
	c.Blue = 0;
	pc->setFillColor(c);
	pc->fillRectangle(x0, y0, x1 + 3, y1 + 3);
	c.Red = 0xff;
	c.Green = 0xff;
	c.Blue = 0xff;
	pc->setFillColor(c);
	pc->fillRectangle(x0 + 2, y0 + 3, x1, y1);

	if(myBookInfo == NULL)
		myBookInfo = new BookInfo(fileName_InitDoc);

	pc->setFont("Arial", 12, false, false);
	char *bf = v3_cb->GetString("VIEWER_MENU_ENC");
	if(bf != NULL)
		pc->drawString(x0 + 5, y0 + 35, bf, strlen(bf));
	else
		pc->drawString(x0 + 5, y0 + 35, "Encoding", 8);
	pc->drawLine(x0, y0 + 45, x1, y0 + 45);
	pc->drawLine(x0, y0 + 46, x1, y0 + 46);

	char l[100];
	int i = 0;

	char *lang = v3_cb->GetString("VIEWER_LANG");
	if(lang != NULL)
		sprintf(l, "%d. %s : %s", i + 1, lang, ZLLanguageList::languageName(myBookInfo->LanguageOption.value()).c_str());
	else
		sprintf(l, "%d. %s : %s", i + 1, "Language", ZLLanguageList::languageName(myBookInfo->LanguageOption.value()).c_str());
	pc->drawString(x0 + 10, y0 + 90 + 40 * i, l, strlen(l));
	i++;

	char *enc = v3_cb->GetString("VIEWER_ENC");
	if(enc != NULL)
		sprintf(l, "%d. %s: %s", i + 1, enc, myBookInfo->EncodingOption.value().c_str());
	else
		sprintf(l, "%d. %s: %s", i + 1, "Encoding", myBookInfo->EncodingOption.value().c_str());
	pc->drawString(x0 + 10, y0 + 90 + 40 * i, l, strlen(l));
	i++;

	pc->drawLine(x0, y1 - 45, x1, y1 - 45);
	pc->drawLine(x0, y1 - 46, x1, y1 - 46);

	sprintf(l, "%d / %d", 1, 1);
	pc->setFont("Arial", 14, false, false);
	pc->drawString(x0 + 5, y1 - 10, l, strlen(l));

	pc->drawLine(x0 + 95, y1 - 45, x0 + 95, y1);
	pc->drawLine(x0 + 96, y1 - 45, x0 + 96, y1);

	v3_cb->BeginDialog();
	if(!init) {
		v3_cb->BlitBitmap(x0, y0, x1 - x0, y1 - y0, x0, y0, 600, 800, buf);
		v3_cb->PartialPrint();
	}
	v3_cb->EndDialog();
}

int setEnc(int keyId);
int setLang(int keyId);

int menuEnc(int keyId)
{
	if(keyId == NULL) {
		drawMenuEnc(true);
		return 3;
	}

	int key;

	switch(keyId) {
		case KEY_OK:
		case KEY_CANCEL:
			cur_state = ST_NORMAL;
			mainApplication->refreshWindow();
			return 1;
			break;
//		case KEY_0: key = 0; break;
		case KEY_1: key = 1; 
					cur_state = ST_LANG;
					setLang(NULL);
					return 2;
					break;
		case KEY_2: key = 2; 
					cur_state = ST_ENC;
					setEnc(NULL);
					return 2;
					break;
/*		case KEY_3: key = 3; break;
		case KEY_4: key = 4; break;
		case KEY_5: key = 5; break;
		case KEY_6: key = 6; break;
		case KEY_7: key = 7; break;
		case KEY_8: key = 8; break;
//		case KEY_9: key = 9; break;
*/		
		default: return 2;
	}
	return 2;
}

void drawEnc(bool init = false)
{
	int x0 = 100 , y0 = 175, x1 = 500, y1 = 625;

	shared_ptr<ZLPaintContext> pc = mainApplication->context();

	if(init) {
		enc_pg = 0;
		encodings.clear();
		encodings.push_back("auto");
		encodings.push_back("UTF-8");
		encodings.push_back("KOI8-R");
		encodings.push_back("KOI8-U");
		encodings.push_back("windows-1251");
		encodings.push_back("windows-1252");
		encodings.push_back("ISO-8859-1");
		encodings.push_back("US-ASCII");
	}


	ZLColor c;
	c.Red = 0;
	c.Green = 0;
	c.Blue = 0;
	pc->setFillColor(c);
	pc->fillRectangle(x0, y0, x1 + 3, y1 + 3);
	c.Red = 0xff;
	c.Green = 0xff;
	c.Blue = 0xff;
	pc->setFillColor(c);
	pc->fillRectangle(x0 + 2, y0 + 3, x1, y1);

	pc->setFont("Arial", 12, false, false);
	char *bf = v3_cb->GetString("VIEWER_ENC");
	if(bf != NULL)
		pc->drawString(x0 + 5, y0 + 35, bf, strlen(bf));
	else
		pc->drawString(x0 + 5, y0 + 35, "Encoding", 8);
	pc->drawLine(x0, y0 + 45, x1, y0 + 45);
	pc->drawLine(x0, y0 + 46, x1, y0 + 46);

	char l[100];
	int i = 0;
	while((i < 8) && ((enc_pg * 8) + i) < encodings.size()) {
		int idx = enc_pg * 8 + i;
		sprintf(l, "%d. %s", i + 1, encodings.at(idx).c_str());
		pc->drawString(x0 + 10, y0 + 90 + 40 * i, l, strlen(l));
		i++;
	}

	pc->drawLine(x0, y1 - 45, x1, y1 - 45);
	pc->drawLine(x0, y1 - 46, x1, y1 - 46);

	int pgs = encodings.size() / 8; 
	if((encodings.size() % 8) > 0)
		pgs++;
	sprintf(l, "%d / %d", enc_pg + 1, pgs);
	pc->setFont("Arial", 14, false, false);
	pc->drawString(x0 + 5, y1 - 10, l, strlen(l));

	pc->drawLine(x0 + 95, y1 - 45, x0 + 95, y1);
	pc->drawLine(x0 + 96, y1 - 45, x0 + 96, y1);

	sprintf(l, "%s", myBookInfo->EncodingOption.value().c_str());
	pc->drawString(x0 + 106, y1 - 10, l, strlen(l));

	v3_cb->BeginDialog();
//	if(!init) {
	v3_cb->BlitBitmap(x0, y0, x1 - x0, y1 - y0, x0, y0, 600, 800, buf);
	v3_cb->PartialPrint();
//	}
	v3_cb->EndDialog();
}

int setEnc(int keyId)
{
	if(keyId == NULL) {
		drawEnc(true);
		return 2;
	}

	int key;

	switch(keyId) {
		case KEY_OK:
		case KEY_CANCEL:
			mainApplication->refreshWindow();
			cur_state = ST_MENU_ENC;
			drawMenuEnc();
			return 2;
			break;
		case KEY_DOWN:
		case KEY_0: key = 0; break;
		case KEY_1: key = 1; break;
		case KEY_2: key = 2; break;
		case KEY_3: key = 3; break;
		case KEY_4: key = 4; break;
		case KEY_5: key = 5; break;
		case KEY_6: key = 6; break;
		case KEY_7: key = 7; break;
		case KEY_8: key = 8; break;
		case KEY_UP:
		case KEY_9: key = 9; break;
		default: return 2;
	}
	if(key == 0) {
		int pgs = encodings.size() / 8; 
		if((encodings.size() % 8) > 0)
			pgs++;
		if(enc_pg < (pgs - 1)) {
			enc_pg++;
			drawEnc();
		}
	} else if(key == 9) {
		if(enc_pg > 0) {
			enc_pg--;
			drawEnc();
		}
	} else {
		int idx = key - 1 + enc_pg * 8;
		if(idx < encodings.size()) {
			ZLFile file(fileName_InitDoc);
			BookDescriptionPtr description = new BookDescription(fileName_InitDoc);
			description->myEncoding = encodings.at(idx);
			BookDescriptionUtil::saveInfo(file);
			
			//WritableBookDescription((BookDescriptionPtr&)description).encoding() = encodings.at(idx);

			// set encoding
			myBookInfo->EncodingOption.setValue(encodings.at(idx));

			//((FBReader *)mainApplication)->openFile(fileName_InitDoc);
			FBReader *fbr = (FBReader*)mainApplication;
			fbr->openFile(fbr->myModel->fileName());

			fbr->clearTextCaches();
			mainApplication->refreshWindow();
			cur_state = ST_MENU_ENC;
			drawMenuEnc();
			return 1;
		}
	}

	return 2;
}

std::vector<std::string> languages;
void drawLang(bool init = false)
{
	int x0 = 100 , y0 = 175, x1 = 500, y1 = 625;

	shared_ptr<ZLPaintContext> pc = mainApplication->context();

	if(init) {
		lang_pg = 0;

		const std::vector<std::string> &l = ZLLanguageList::languageCodes();
		languages.empty();
		languages.resize(l.size());
		copy(l.begin(), l.end(), languages.begin());
		languages.erase(find(languages.begin(), languages.end(), "ru"));
		languages.insert(languages.begin(), std::string("ru"));
	}


	ZLColor c;
	c.Red = 0;
	c.Green = 0;
	c.Blue = 0;
	pc->setFillColor(c);
	pc->fillRectangle(x0, y0, x1 + 3, y1 + 3);
	c.Red = 0xff;
	c.Green = 0xff;
	c.Blue = 0xff;
	pc->setFillColor(c);
	pc->fillRectangle(x0 + 2, y0 + 3, x1, y1);

	pc->setFont("Arial", 12, false, false);
	char *bf = v3_cb->GetString("VIEWER_LANG");
	if(bf != NULL)
		pc->drawString(x0 + 5, y0 + 35, bf, strlen(bf));
	else
		pc->drawString(x0 + 5, y0 + 35, "Language", 8);
	pc->drawLine(x0, y0 + 45, x1, y0 + 45);
	pc->drawLine(x0, y0 + 46, x1, y0 + 46);

	char l[100];
	int i = 0;
	while((i < 8) && ((lang_pg * 8) + i) < languages.size()) {
		int idx = lang_pg * 8 + i;
		sprintf(l, "%d. %s", i + 1, ZLLanguageList::languageName(languages.at(idx)).c_str());
		pc->drawString(x0 + 10, y0 + 90 + 40 * i, l, strlen(l));
		i++;
	}

	pc->drawLine(x0, y1 - 45, x1, y1 - 45);
	pc->drawLine(x0, y1 - 46, x1, y1 - 46);

	int pgs = languages.size() / 8; 
	if((languages.size() % 8) > 0)
		pgs++;
	sprintf(l, "%d / %d", lang_pg + 1, pgs);
	pc->setFont("Arial", 14, false, false);
	pc->drawString(x0 + 5, y1 - 10, l, strlen(l));

	pc->drawLine(x0 + 95, y1 - 45, x0 + 95, y1);
	pc->drawLine(x0 + 96, y1 - 45, x0 + 96, y1);

	sprintf(l, "%s", ZLLanguageList::languageName(myBookInfo->LanguageOption.value()).c_str());
	pc->drawString(x0 + 106, y1 - 10, l, strlen(l));

	v3_cb->BeginDialog();
//	if(!init) {
	v3_cb->BlitBitmap(x0, y0, x1 - x0, y1 - y0, x0, y0, 600, 800, buf);
	v3_cb->PartialPrint();
//	}
	v3_cb->EndDialog();
}

int setLang(int keyId)
{
	if(keyId == NULL) {
		drawLang(true);
		return 2;
	}

	int key;

	switch(keyId) {
		case KEY_OK:
		case KEY_CANCEL:
			mainApplication->refreshWindow();
			cur_state = ST_MENU_ENC;
			drawMenuEnc();
			return 2;
			break;
		case KEY_DOWN:
		case KEY_0: key = 0; break;
		case KEY_1: key = 1; break;
		case KEY_2: key = 2; break;
		case KEY_3: key = 3; break;
		case KEY_4: key = 4; break;
		case KEY_5: key = 5; break;
		case KEY_6: key = 6; break;
		case KEY_7: key = 7; break;
		case KEY_8: key = 8; break;
		case KEY_UP:
		case KEY_9: key = 9; break;
		default: return 2;
	}
	if(key == 0) {
		int pgs = languages.size() / 8; 
		if((languages.size() % 8) > 0)
			pgs++;
		if(lang_pg < (pgs - 1)) {
			lang_pg++;
			drawLang();
		}
	} else if(key == 9) {
		if(lang_pg > 0) {
			lang_pg--;
			drawLang();
		}
	} else {
		int idx = key - 1 + lang_pg * 8;
		if(idx < languages.size()) {
			FBReader *fbr = (FBReader*)mainApplication;

			BookDescriptionPtr description = new BookDescription(fbr->myModel->fileName());
			description->myLanguage = languages.at(idx);
			BookDescriptionUtil::saveInfo(ZLFile(fbr->myModel->fileName()));
			
			// set encoding
			myBookInfo->LanguageOption.setValue(languages.at(idx));

			fbr->openFile(fbr->myModel->fileName());

			fbr->clearTextCaches();
			mainApplication->refreshWindow();
			cur_state = ST_MENU_ENC;
			drawMenuEnc();
			return 1;
		}
	}

	return 2;
}

void drawMenuFont(bool init)
{
	int x0 = 100 , y0 = 175, x1 = 500, y1 = 625;

	shared_ptr<ZLPaintContext> pc = mainApplication->context();

	ZLColor c;
	c.Red = 0;
	c.Green = 0;
	c.Blue = 0;
	pc->setFillColor(c);
	pc->fillRectangle(x0, y0, x1 + 3, y1 + 3);
	c.Red = 0xff;
	c.Green = 0xff;
	c.Blue = 0xff;
	pc->setFillColor(c);
	pc->fillRectangle(x0 + 2, y0 + 3, x1, y1);

	if(myBookInfo == NULL)
		myBookInfo = new BookInfo(fileName_InitDoc);

	pc->setFont("Arial", 12, false, false);
	char *bf = v3_cb->GetString("VIEWER_MENU_FONT");
	if(bf != NULL)
		pc->drawString(x0 + 5, y0 + 35, bf, strlen(bf));
	else
		pc->drawString(x0 + 5, y0 + 35, "ûÒÉÆÔ", 8);
	pc->drawLine(x0, y0 + 45, x1, y0 + 45);
	pc->drawLine(x0, y0 + 46, x1, y0 + 46);

	char l[100];
	int i = 0;

	char *x = v3_cb->GetString("VIEWER_FONT");
	if(x != NULL)
		sprintf(l, "%d. %s", i + 1, x);//, ZLTextStyleCollection::instance().baseStyle().FontFamilyOption.value().c_str());
	else
		sprintf(l, "%d. %s", i + 1, "Base font family");//, ZLTextStyleCollection::instance().baseStyle().FontFamilyOption.value().c_str());
	pc->drawString(x0 + 10, y0 + 90 + 40 * i, l, strlen(l));
	i++;

	x = NULL;
	x = v3_cb->GetString("VIEWER_FONT_SIZE");
	if(x != NULL)
		sprintf(l, "%d. %s", i + 1, x); //, ZLTextStyleCollection::instance().baseStyle().FontSizeOption.value());
	else
		sprintf(l, "%d. %s", i + 1, "Font Size"); //, ZLTextStyleCollection::instance().baseStyle().FontSizeOption.value());
	pc->drawString(x0 + 10, y0 + 90 + 40 * i, l, strlen(l));
	i++;

	x = NULL;
	x = v3_cb->GetString("VIEWER_INTERLINE");
	if(x != NULL)
		sprintf(l, "%d. %s", i + 1, x);//, ZLTextStyleCollection::instance().baseStyle().LineSpacePercentOption.value());
	else
		sprintf(l, "%d. %s", i + 1, "Interline space");//, ZLTextStyleCollection::instance().baseStyle().LineSpacePercentOption.value());

	pc->drawString(x0 + 10, y0 + 90 + 40 * i, l, strlen(l));
	i++;

	pc->drawLine(x0, y1 - 45, x1, y1 - 45);
	pc->drawLine(x0, y1 - 46, x1, y1 - 46);

	sprintf(l, "%d / %d", 1, 1);
	pc->setFont("Arial", 14, false, false);
	pc->drawString(x0 + 5, y1 - 10, l, strlen(l));

	pc->drawLine(x0 + 95, y1 - 45, x0 + 95, y1);
	pc->drawLine(x0 + 96, y1 - 45, x0 + 96, y1);

	v3_cb->BeginDialog();
	if(!init) {
		v3_cb->BlitBitmap(x0, y0, x1 - x0, y1 - y0, x0, y0, 600, 800, buf);
		v3_cb->PartialPrint();
	}
	v3_cb->EndDialog();
}
int menuFont(int keyId)
{
	if(keyId == NULL) {
		drawMenuFont(true);
		return 3;
	}

	int key;

	switch(keyId) {
		case KEY_OK:
		case KEY_CANCEL:
			cur_state = ST_NORMAL;
			mainApplication->refreshWindow();
			return 1;
			break;
//		case KEY_0: key = 0; break;
		case KEY_1: key = 1; 
					cur_state = ST_FONT_SEL;
					setFont(NULL);
					return 2;
					break;
		case KEY_2: key = 2; 
					cur_state = ST_FONT_SIZE;
					setFontSize(NULL);
					return 2;
					break;
		case KEY_3: key = 3; 
					cur_state = ST_INTERLINE;
					setInterLine(NULL);
					return 2;
					break;
		case KEY_4: key = 4; break;
		case KEY_5: key = 5; break;
		case KEY_6: key = 6; break;
		case KEY_7: key = 7; break;
		case KEY_8: key = 8; break;
//		case KEY_9: key = 9; break;
		default: return 2;
	}
	return 2;
}

int OnKeyPressed(int keyId, int state)
{
	if(cur_state == ST_CLOCK) {
		return setClock(keyId);
	}
	
	if(cur_state == ST_MENU_FONT)
		return menuFont(keyId);


	if(cur_state == ST_FONT_SEL) {
		return setFont(keyId);
	}

	if(cur_state == ST_FONT_SIZE) {
		return setFontSize(keyId);
	}

	if(cur_state == ST_INTERLINE) {
		return setInterLine(keyId);
	}

	if(cur_state == ST_MENU_ENC) {
		return menuEnc(keyId);
	}

	if(cur_state == ST_ENC) {
		return setEnc(keyId);
	}

	if(cur_state == ST_LANG) {
		return setLang(keyId);
	}


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

	if(state != NORMALSTATE)
		return 0;

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


#define MENU_SETTINGS 1000
#define MENU_FONT 1001
#define MENU_ENC 1002
#define MENU_CLOCK 1003
#define MENU_DEL_AND_QUIT 1010
#define MENU_TRASH_AND_QUIT 1011

/**
* Call this function on final (non submenu) menu item selection.
*
* actionId - id of menu action. Set of standard actions should be defined in SDK header file. 
*            Some range should be reserved for plugin items.
*            E.g. 1..999 for standard, Viewer-defined actions
*                 1000-1999 reserved for plugins
*
* If return value is 1, this means that action has been processed in plugin and viewer should flush the screen.
* If return value is 2, this means that action has been processed in plugin and no more processing is required.
* If return value is 0, or no such function defined in plugin, default processing should be done by Viewer.
*/
int OnMenuAction( int actionId )
{
    switch ( actionId ) {
		case MENU_SETTINGS:
			return 2;
			break;

		case MENU_TRASH_AND_QUIT:
			// TODO ask user
			unlink(fileName_InitDoc);
			exit(0);
			break;

		case MENU_DEL_AND_QUIT:
			// TODO ask user
			//unlink(fileName_InitDoc);
			exit(0);
			break;

		case MENU_CLOCK:
			hour = 0;
			min = 0;
			pos = 0;
			cur_state = ST_CLOCK;
			setClock(NULL);
			return 1;
			break;

		case MENU_FONT:
			cur_state = ST_MENU_FONT;
			menuFont(NULL);
			return 1;
			break;

/*		case MENU_FONT:
			cur_state = ST_FONT_SEL;
			setFont(NULL);
			return 1;
			break;

		case MENU_FONT_SIZE:
			cur_state = ST_FONT_SIZE;
			setFontSize(NULL);
			return 1;
			break;

		case MENU_INTERLINE:
			cur_state = ST_INTERLINE;
			setInterLine(NULL);
			return 1;
			break;
*/			

		case MENU_ENC:
			cur_state = ST_MENU_ENC;
			menuEnc(NULL);
			return 1;
			break;
	}

    return 0;
}


static viewer_menu_item_t custom_menu_items[] =
{
//    {MENU_SETTINGS, "VIEWER_MENU_SETTINGS", NULL},
	{MENU_FONT, "VIEWER_MENU_FONT", NULL},
	{MENU_ENC, "VIEWER_MENU_ENC", NULL},
    {MENU_CLOCK, "VIEWER_MENU_CLOCK", NULL},
//	{MENU_FONT_SIZE, "VIEWER_FONT_SIZE", NULL},
//	{MENU_INTERLINE, "VIEWER_INTERLINE", NULL},
	
//    {MENU_DEL_AND_QUIT, "VIEWER_MENU_DEL_AND_QUIT", NULL},
//    {MENU_TRASH_AND_QUIT, "VIEWER_MENU_TRASH_AND_QUIT", NULL},
	{ 0, NULL, NULL }
};

/**
* Returns custom menu items array, NULL to use standard menu items only.
*/
const viewer_menu_item_t * GetCustomViewerMenu()
{
    return custom_menu_items;
}

//const char * GetAboutInfoText()
