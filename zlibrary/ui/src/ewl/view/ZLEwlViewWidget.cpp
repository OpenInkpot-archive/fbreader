/*
 * Copyright (C) 2008 Alexander Kerner <lunohod@openinkpot.org>
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

#include "ZLEwlViewWidget.h"
#include "ZLEwlPaintContext.h"

#include <assert.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define XCB_ALL_PLANES ~0

xcb_connection_t *connection;
xcb_window_t window;
xcb_screen_t *screen;
xcb_drawable_t rect;
xcb_shm_segment_info_t shminfo;
xcb_image_t *im;
xcb_gcontext_t		gc;
unsigned int *pal;

static void updatePoint(ZLEwlViewWidget *viewWidget, int &x, int &y) {
	switch (viewWidget->rotation()) {
		default:
			break;
		case ZLView::DEGREES90:
		{
			int tmp = x;
			x = viewWidget->height() - y;
			y = tmp;
			break;
		}
		case ZLView::DEGREES180:
			x = viewWidget->width() - x;
			y = viewWidget->height() - y;
			break;
		case ZLView::DEGREES270:
		{
			int tmp = x;
			x = y;
			y = viewWidget->width() - tmp;
			break;
		}
	}
}

int ZLEwlViewWidget::width() const {
	return w;
}

int ZLEwlViewWidget::height() const {
	return h;
}

ZLEwlViewWidget::ZLEwlViewWidget(ZLApplication *application, ZLView::Angle initialAngle) : ZLViewWidget(initialAngle) {
	myApplication = application;
	w = 600;
	h = 800;

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

	screen = xcb_aux_get_screen (connection, screen_number);

	xcb_randr_query_version_reply_t *rr_version;
	rr_version = xcb_randr_query_version_reply(connection, xcb_randr_query_version(connection, 1, 1), 0);

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
		XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_POINTER_MOTION |
		XCB_EVENT_MASK_VISIBILITY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_FOCUS_CHANGE      | XCB_EVENT_MASK_PROPERTY_CHANGE;

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

	xcb_colormap_t    colormap;
	colormap = screen->default_colormap;

	xcb_alloc_color_reply_t *rep;
/*	rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0, 0, 0), NULL);
	pal_[0] = rep->pixel;
	free(rep);
	rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0x55<<8, 0x55<<8, 0x55<<8), NULL);
	pal_[1] = rep->pixel;
	free(rep);
	rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0xaa<<8, 0xaa<<8, 0xaa<<8), NULL);
	pal_[2] = rep->pixel;
	free(rep);
	rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, 0xff<<8, 0xff<<8, 0xff<<8), NULL);
	pal_[3] = rep->pixel;
	free(rep);
*/

	for(int i = 0; i < 256; i++) {
		// FIXME: workaround for broken palitre on n516
		if(i < 8) {
			pal_[i] = 0;
			continue;
		}

		rep = xcb_alloc_color_reply (connection, xcb_alloc_color (connection, colormap, i<<8, i<<8, i<<8), NULL);
		pal_[i] = rep->pixel;
		free(rep);
	}

	pal = pal_;

	xcb_shm_query_version_reply_t *rep_shm;

	rep_shm = xcb_shm_query_version_reply (connection,
			xcb_shm_query_version (connection),
			NULL);
	if(rep_shm) {
		xcb_image_format_t format;
		int shmctl_status;

		if (rep_shm->shared_pixmaps &&
				(rep_shm->major_version > 1 || rep_shm->minor_version > 0))
			format = (xcb_image_format_t)rep_shm->pixmap_format;
		else
			format = (xcb_image_format_t)0;

		im = xcb_image_create_native (connection, 600, 800,
				format, depth, NULL, ~0, NULL);
		assert(im);

		shminfo.shmid = shmget (IPC_PRIVATE,
				im->stride*im->height,
				IPC_CREAT | 0777);
		assert(shminfo.shmid != -1);
		shminfo.shmaddr = (uint8_t*)shmat (shminfo.shmid, 0, 0);
		assert(shminfo.shmaddr);
		im->data = shminfo.shmaddr;

		shminfo.shmseg = xcb_generate_id (connection);
		xcb_shm_attach (connection, shminfo.shmseg,
				shminfo.shmid, 0);
		shmctl_status = shmctl(shminfo.shmid, IPC_RMID, 0);
		assert(shmctl_status != -1);
		free (rep_shm);
	}

	xcb_flush(connection);
}

ZLEwlViewWidget::~ZLEwlViewWidget() {
}

void ZLEwlViewWidget::doPaint()	{
	ZLEwlPaintContext &pContext = (ZLEwlPaintContext&)view()->context();

	pContext.image = im;
	pContext.myWidth = w;
	pContext.myHeight = h;

	view()->paint();

	void update_position_property();
	update_position_property();

	/*for(int i = 0; i < 256; i++)  {
		fprintf(stderr, "%d: %x\n", i, pal[i]);
		for(int j = 0; j < 600; j++) {
			xcb_image_put_pixel(im, j, i, pal[i]);
		}
	}
	*/

	xcb_image_shm_put (connection, window, gc,
			pContext.image, shminfo,
			0, 0, 0, 0, w, h, 0);
	xcb_flush(connection);

	//sleep(10);
}

void ZLEwlViewWidget::trackStylus(bool track) {
}

void ZLEwlViewWidget::resize(int w, int h) {
	if(w == this->w && h == this->h)
		return;

	this->w = w;
	this->h = h;

	xcb_shm_detach(connection, shminfo.shmseg);
	shmdt(im->data);
	xcb_image_destroy(im);

	xcb_shm_query_version_reply_t *rep_shm;

	rep_shm = xcb_shm_query_version_reply (connection,
			xcb_shm_query_version (connection),
			NULL);
	if(rep_shm) {
		xcb_image_format_t format;
		int shmctl_status;

		if (rep_shm->shared_pixmaps &&
				(rep_shm->major_version > 1 || rep_shm->minor_version > 0))
			format = (xcb_image_format_t)rep_shm->pixmap_format;
		else
			format = (xcb_image_format_t)0;

		uint8_t depth = xcb_aux_get_depth (connection, screen);
		im = xcb_image_create_native (connection, w, h,
				format, depth, NULL, ~0, NULL);
		assert(im);

		shminfo.shmid = shmget (IPC_PRIVATE,
				im->stride*im->height,
				IPC_CREAT | 0777);
		assert(shminfo.shmid != -1);
		shminfo.shmaddr = (uint8_t*)shmat (shminfo.shmid, 0, 0);
		assert(shminfo.shmaddr);
		im->data = shminfo.shmaddr;

		shminfo.shmseg = xcb_generate_id (connection);
		xcb_shm_attach (connection, shminfo.shmseg,
				shminfo.shmid, 0);
		shmctl_status = shmctl(shminfo.shmid, IPC_RMID, 0);
		assert(shmctl_status != -1);
		free (rep_shm);
	}
}

void ZLEwlViewWidget::repaint()	{
	doPaint();
}

void ZLEwlViewWidget::invertRegion(int x0, int y0, int x1, int y1, bool flush)
{
	unsigned int pixel;

	for(int i = x0; i <= x1; i++) {
		for(int j = y0; j <= y1; j++) {
			pixel = 0xffffff & xcb_image_get_pixel(im, i, j);
			for(int idx = 0; idx < 256; idx++) {
				if(pixel == (0xffffff & pal[idx])) {
					xcb_image_put_pixel(im, i, j, pal[~idx & 0xff]);
					break;
				}
			}
		}
	}

	uint8_t send_event;
	if(flush)
		send_event = 1;
	else
		send_event = 0;

	xcb_image_shm_put (connection, window, gc,
			im, shminfo,
			x0, y0, x0, y0, x1 - x0, y1 - y0, send_event);

	if(flush)
		xcb_flush(connection);
}
