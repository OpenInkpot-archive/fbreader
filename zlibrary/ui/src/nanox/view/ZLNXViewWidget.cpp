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

#include <ewl/Ewl.h>
#include <Evas.h>

#include "../../../../../fbreader/src/fbreader/FBReaderActions.h"

static void
cb_window_destroy(Ewl_Widget *w, void *ev, void *data)
{
	    ewl_main_quit();
}

static void
cb_window_close(Ewl_Widget *w, void *ev, void *data)
{
	    ewl_widget_destroy(w);
}

static void
cb_image_reveal(Ewl_Widget *w, void *ev, void *data)
{
	ZLNXViewWidget *vw = (ZLNXViewWidget*)data;
	vw->doPaint();
}

static void
cb_key_down(Ewl_Widget *w, void *ev, void *data)
{
    Ewl_Event_Key_Down *e;
	Evas_Object *img;
	Ewl_Embed *emb;
	Evas_Coord img_w, img_h;
	int *_data;
	int i;


    e = (Ewl_Event_Key_Down*)ev;
	ZLApplication *application = (ZLApplication*)data;

    if(!strcmp(e->base.keyname, "q")
            || !strcmp(e->base.keyname, "Escape")) {
        Ewl_Widget *win;

		application->doAction(ActionCode::CANCEL);
        win = ewl_widget_name_find("main_win");
        ewl_widget_destroy(win);
    } else if(!strcmp(e->base.keyname, "0")) {
		application->doAction(ActionCode::LARGE_SCROLL_FORWARD);

    } else if(!strcmp(e->base.keyname, "9")) {
		application->doAction(ActionCode::LARGE_SCROLL_BACKWARD);
	}
}



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
	Ewl_Widget *win, *o;

	myApplication = application;

    win = ewl_window_new();
    ewl_window_title_set(EWL_WINDOW(win), "FBReader");
    ewl_window_class_set(EWL_WINDOW(win), "fbreader");
    ewl_window_name_set(EWL_WINDOW(win), "fbreader");
    ewl_object_fill_policy_set(EWL_OBJECT(win), EWL_FLAG_FILL_ALL);
    ewl_object_size_request(EWL_OBJECT(win), 600, 800);
    ewl_callback_append(win, EWL_CALLBACK_DELETE_WINDOW, cb_window_close, application);
    ewl_callback_append(win, EWL_CALLBACK_KEY_DOWN, cb_key_down, application);
    ewl_callback_append(win, EWL_CALLBACK_DESTROY, cb_window_destroy, application);
    ewl_widget_name_set(win, "main_win");
    ewl_widget_show(win);

    o = ewl_image_new();
	EWL_IMAGE(o)->image = NULL;
    ewl_object_fill_policy_set(EWL_OBJECT(o), EWL_FLAG_FILL_FILL);
    ewl_container_child_append(EWL_CONTAINER(win), o);
    ewl_callback_append(o, EWL_CALLBACK_REVEAL, cb_image_reveal, this);
    ewl_object_position_request(EWL_OBJECT(o), CURRENT_X(win), CURRENT_Y(win));
    ewl_object_size_request(EWL_OBJECT(o), CURRENT_W(win), CURRENT_H(win));
    ewl_widget_name_set(o, "image");
    ewl_widget_show(o);
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

	Ewl_Widget *win, *o;
	Evas_Object *img;
	Ewl_Embed *emb;
	Evas_Coord img_w, img_h;
	int *data;
	int i;

	win = ewl_widget_name_find("main_win");
	o = ewl_widget_name_find("image");

	img = (Evas_Object*)EWL_IMAGE(o)->image;
	if(!img)
		return;

	evas_object_image_size_set(img, CURRENT_W(win), CURRENT_H(win));
	evas_object_image_size_get(img, &img_w, &img_h);

	printf("WxH: %dx%d\n", img_w, img_h);
	
	data = (int*)evas_object_image_data_get(img, 1);		
	if(!data)
		return;

	pContext.image = data;

	view()->paint();

	//evas_object_image_data_set(img, data);
	evas_object_image_data_update_add(img, 0, 0, img_w, img_h);
	//ewl_widget_configure(o);
}
