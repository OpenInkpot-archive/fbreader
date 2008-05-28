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

#define XCB_ALL_PLANES ~0

xcb_connection_t *connection;
xcb_window_t window;
xcb_screen_t *screen;
xcb_drawable_t rect;
unsigned int *pal;

void ZLNXViewWidget::updateCoordinates(int &x, int &y) {
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
	xcb_rectangle_t     rect_coord = { 0, 0, 600, 800};

	/* getting the connection */
	connection = xcb_connect (NULL, &screen_number);
	if (xcb_connection_has_error(connection)) {
		fprintf (stderr, "ERROR: can't connect to an X server\n");
		exit(-1);
	}

	/* getting the current screen */
/*	setup = xcb_get_setup (connection);

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
*/
	screen = xcb_aux_get_screen (connection, screen_number);

	gc = xcb_generate_id (connection);
	mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	values[0] = screen->black_pixel;
	values[1] = 0; /* no graphics exposures */
	xcb_create_gc (connection, gc, screen->root, mask, values);

	bgcolor = xcb_generate_id (connection);
	mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES;
	values[0] = screen->white_pixel;
	values[1] = 0; /* no graphics exposures */
	xcb_create_gc (connection, bgcolor, screen->root, mask, values);

	/* creating the window */
	window = xcb_generate_id(connection);
	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	values[0] = screen->white_pixel;
	values[1] =
		XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_POINTER_MOTION;

	uint8_t depth = xcb_aux_get_depth (connection, screen);
	xcb_create_window(connection,
			depth,
			window, screen->root,
			0, 0, 600, 800,
			0, XCB_WINDOW_CLASS_INPUT_OUTPUT,
			screen->root_visual,
			mask, values);

	rect = xcb_generate_id (connection);
	xcb_create_pixmap (connection, depth,
			rect, window,
			600, 800);


	xcb_map_window(connection, window);
	//	printf("depth: %d\n", screen->root_depth);

	xcb_colormap_t    colormap;
	colormap = screen->default_colormap;

	xcb_alloc_color_reply_t *rep;
	rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0, 0, 0), NULL);
	pal_[0] = rep->pixel;
	free(rep);
	rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0x55<<8, 0x55<<8, 0x55<<8), NULL);
	pal_[1] = rep->pixel;
	free(rep);
	rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0xaa<<8, 0xaa<<8, 0xaa<<8), NULL);
	pal_[2] = rep->pixel;
	free(rep);

	pal = pal_;

	xcb_flush(connection);
}

ZLNXViewWidget::~ZLNXViewWidget() {
}

void ZLNXViewWidget::repaint()	{
	doPaint();
}

void ZLNXViewWidget::trackStylus(bool track) {
}

void ZLNXViewWidget::doPaint()
{
	ZLNXPaintContext &pContext = (ZLNXPaintContext&)view()->context();

	pContext.image = xcb_image_get (connection, window,
			0, 0, 600, 800,
			XCB_ALL_PLANES, XCB_IMAGE_FORMAT_Z_PIXMAP);

	view()->paint();

	/*
	   for(int j = 50; j < 60; j++)
	   for(int i = 0; i < 100; i++)
	   xcb_image_put_pixel (pContext.image,
	   i, j,
	   pal[0]);

	   for(int j = 65; j < 75; j++)
	   for(int i = 0; i < 100; i++)
	   xcb_image_put_pixel (pContext.image,
	   i, j,
	   pal[1]);

	   for(int j = 80; j < 90; j++)
	   for(int i = 0; i < 100; i++)
	   xcb_image_put_pixel (pContext.image,
	   i, j,
	   pal[2]);

*/

	xcb_image_put (connection, window, gc, pContext.image,
			0, 0, 0);
	xcb_image_destroy(pContext.image);

	xcb_flush(connection);
}
