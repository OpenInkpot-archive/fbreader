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

#include "ZLNXApplicationWindow.h"

#include "../view/ZLNXViewWidget.h"
#include "../../../../core/src/dialogs/ZLOptionView.h"
#include "../dialogs/ZLNXDialogManager.h"

#include <nano-X.h>
#include <nxcolors.h>


ZLNXApplicationWindow::ZLNXApplicationWindow(ZLApplication *application) :
	ZLDesktopApplicationWindow(application) {

	GrOpen();
 	win = GrNewWindow_Apollo(2, 0, 0, 600, 800, 0, GR_COLOR_WHITE, 0);
 
 	GrSetFocus(win);
 
 	gc = GrNewGC_Apollo();

	GrSetGCBackground(gc, GR_COLOR_WHITE);
	GrSetGCForeground (gc, GR_COLOR_BLACK);
 
	GrClearShareMem_Apollo(win, gc, 0xff);

 	GrSelectEvents(win, GR_EVENT_MASK_KEY_DOWN |
 		GR_EVENT_MASK_CLOSE_REQ | GR_EVENT_MASK_EXPOSURE);
 
   	unsigned char fontname[] = "arial";
 
 	fontid = GrCreateFont(fontname, 20, NULL);
 
 	GrSetGCFont(gc, fontid);
 	GrSetFontAttr(fontid, GR_TFKERNING | GR_TFANTIALIAS, 0);

 	GrText_Apollo(win, gc, 5, 20, (void *)"init", 4, GR_TFUTF8);
	GrPrint_Apollo();
}

void ZLNXApplicationWindow::init() {
	ZLDesktopApplicationWindow::init();
	switch (myWindowStateOption.value()) {
		case NORMAL:
			break;
		case FULLSCREEN:
			setFullscreen(true);
			break;
		case MAXIMIZED:
			break;
	}
}

ZLNXApplicationWindow::~ZLNXApplicationWindow() {
	GrClose();
}

void ZLNXApplicationWindow::onButtonPress() {
//		application().doActionByKey(ZLNXKeyUtil::keyName(event));
}

void ZLNXApplicationWindow::addToolbarItem(ZLApplication::Toolbar::ItemPtr item) {
}

void ZLNXApplicationWindow::setFullscreen(bool fullscreen) {
}

bool ZLNXApplicationWindow::isFullscreen() const {
	return true;
}

ZLViewWidget *ZLNXApplicationWindow::createViewWidget() {
	ZLNXViewWidget *viewWidget = new ZLNXViewWidget(&application(), (ZLViewWidget::Angle)application().AngleStateOption.value());
	return viewWidget;
}

void ZLNXApplicationWindow::close() {
}

void ZLNXApplicationWindow::grabAllKeys(bool) {
}

void ZLNXDialogManager::createApplicationWindow(ZLApplication *application) const {
	ZLNXApplicationWindow *mw = new ZLNXApplicationWindow(application);
}


#define KEY_BASE 48 
#define KEY_0 (KEY_BASE)
#define KEY_9 (9+KEY_BASE)
#define KEY_PREV KEY_9
#define KEY_NEXT KEY_0
#define KEY_CANCEL 'n'
#define KEY_OK 'y'

void mainLoop(ZLApplication *application)
{
	GR_EVENT event;
	std::string x;

	while (1) {
    	GrGetNextEvent(&event);
		switch (event.type) {
			case GR_EVENT_TYPE_KEY_DOWN:
				switch(event.keystroke.ch) {		
					case KEY_NEXT:
						x = "<PageDown>";
						application->doActionByKey(x);
						break;

					case KEY_PREV:
						x = "<PageUp>";
						application->doActionByKey(x);
						break;

					case KEY_CANCEL:
						return;
				}
		}
	}
}
	
