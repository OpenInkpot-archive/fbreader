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

#include "ZLNXViewWidget.h"
#include "ZLNXPaintContext.h"

extern xcb_connection_t     *connection;
extern xcb_window_t          window;
extern xcb_screen_t         *screen;

void ZLNXViewWidget::updateCoordinates(int &x, int &y) {
	xcb_screen_iterator_t screen_iter;
	const xcb_setup_t    *setup;
	xcb_screen_t         *screen;
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
	
	gc = xcb_generate_id (connection);

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

	switch (rotation()) {
		default:
			break;
		case ZLViewWidget::DEGREES90:
		{
			int tmp = x;
			x = height() - y;
			y = tmp;
			break;
		}
		case ZLViewWidget::DEGREES180:
			x = width() - x;
			y = height() - y;
			break;
		case ZLViewWidget::DEGREES270:
		{
			int tmp = x;
			x = y;
			y = width() - tmp;
			break;
		}
	}
}

int ZLNXViewWidget::width() const {
	return 600;
}

int ZLNXViewWidget::height() const {
	return 800;
}

ZLNXViewWidget::ZLNXViewWidget(ZLApplication *application, Angle initialAngle) : ZLViewWidget(initialAngle) {
	myApplication = application;


}

ZLNXViewWidget::~ZLNXViewWidget() {
}

void ZLNXViewWidget::repaint()	{
	ZLNXPaintContext &pContext = (ZLNXPaintContext&)view()->context();
	
	view()->paint();

	printf("huj1\n");
    xcb_copy_area (connection,
                   *(pContext.pixmap()),  /* drawable we want to paste */
                   window,  /* drawable on which we copy the previous Drawable */
                   gc,            
                   0,         /* top left x coordinate of the region we want to copy */
                   0,         /* top left y coordinate of the region we want to copy */
                   0,         /* top left x coordinate of the region where we want to copy */
                   0,         /* top left y coordinate of the region where we want to copy */
                   600,         /* pixel width of the region we want to copy */
                   800);      /* pixel height of the region we want to copy */
}

void ZLNXViewWidget::trackStylus(bool track) {
}

void ZLNXViewWidget::doPaint()
{
	ZLNXPaintContext &pContext = (ZLNXPaintContext&)view()->context();
	view()->paint();
	
	printf("huj2\n");
    xcb_copy_area (connection,
                   *(pContext.pixmap()),  /* drawable we want to paste */
                   window,  /* drawable on which we copy the previous Drawable */
                   gc,            
                   0,         /* top left x coordinate of the region we want to copy */
                   0,         /* top left y coordinate of the region we want to copy */
                   0,         /* top left x coordinate of the region where we want to copy */
                   0,         /* top left y coordinate of the region where we want to copy */
                   600,         /* pixel width of the region we want to copy */
                   800);      /* pixel height of the region we want to copy */
}
