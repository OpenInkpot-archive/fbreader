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


xcb_connection_t     *connection;
xcb_window_t          window;
xcb_screen_t         *screen;
ZLNXApplicationWindow::ZLNXApplicationWindow(ZLApplication *application) :
	ZLDesktopApplicationWindow(application) {

	xcb_screen_iterator_t screen_iter;
	const xcb_setup_t    *setup;
	xcb_generic_event_t  *e;
	xcb_generic_error_t  *error;
	xcb_void_cookie_t     cookie_window;
	xcb_void_cookie_t     cookie_map;
	uint32_t              mask;
	uint32_t              values[2];
	int                   screen_number;
	uint8_t               is_hand = 0;

	/* getting the connection */
	connection = xcb_connect (NULL, &screen_number);
	if (!connection) {
		fprintf (stderr, "ERROR: can't connect to an X server\n");
		exit(-1);
	}

	/* getting the current screen */
	setup = xcb_get_setup (connection);

	screen = NULL;
	screen_iter = xcb_setup_roots_iterator (setup);
	for (; screen_iter.rem != 0; --screen_number, xcb_screen_next (&screen_iter))
		if (screen_number == 0)
		{
			screen = screen_iter.data;
			break;
		}
	if (!screen) {
		fprintf (stderr, "ERROR: can't get the current screen\n");
		xcb_disconnect(connection);
		exit(-1);
	}
	
	/* creating the window */
	window = xcb_generate_id(connection);
	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	values[0] = screen->white_pixel;
	values[1] =
		XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_POINTER_MOTION;
	xcb_create_window(connection,
			screen->root_depth,
			window, screen->root,
			20, 200, 600, 800,
			0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			screen->root_visual,
			mask, values);
	xcb_map_window(connection, window);

	xcb_flush(connection);
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
