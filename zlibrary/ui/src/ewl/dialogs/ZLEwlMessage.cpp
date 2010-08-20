/*
 * Copyright (C) 2009 Alexander Kerner <lunohod@openinkpot.org>
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

#include <iostream>
#include <cstdlib>
#include <cstring>
extern "C" {
#include <xcb/xcb.h>
#include <libeoi.h>
#include <libeoi_themes.h>
#include <libeoi_entry.h>
}

#include "ZLEwlMessage.h"
#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Evas.h>
#include <Edje.h>

#define __UNUSED__ __attribute__((__unused__))

using namespace std;

extern void ee_init();
extern bool emergency_exit;

static void __attribute__((__used__)) die(const char* fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   vfprintf(stderr, fmt, ap);
   va_end(ap);
   exit(EXIT_FAILURE);
}

static int __attribute__((__used__)) exit_handler(void* param __UNUSED__, int ev_type __UNUSED__, void* event __UNUSED__)
{
   ecore_main_loop_quit();
   return 1;
}

static void main_win_close_handler(Ecore_Evas* main_win __UNUSED__)
{
   ecore_main_loop_quit();
}


static void main_win_resize_handler(Ecore_Evas* main_win)
{
   Evas* canvas = ecore_evas_get(main_win);
   int w, h;
   evas_output_size_get(canvas, &w, &h);

   Evas_Object* main_canvas_edje = evas_object_name_find(canvas, "main_canvas_edje");
   evas_object_resize(main_canvas_edje, w, h);
}

static void main_win_key_handler(void* param, Evas* e __UNUSED__, Evas_Object* o, void* event_info)
{
    Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;

	if(param) {
		void (*handler)(Evas_Object*, char *) = (void(*)(Evas_Object*, char*))param;
		handler(o, ev->keyname);
	}
	if(!strcmp(ev->keyname, "Escape") || !strcmp(ev->keyname, "Return"))
		ecore_main_loop_quit();
}

void show_message(char *text, void *handler)
{
	ee_init();

	Ecore_Evas* main_win = ecore_evas_software_x11_8_new(0, 0, 0, 0, 600, 800);
	ecore_evas_title_set(main_win, "MB");
	ecore_evas_name_class_set(main_win, "MB", "MB");

	extern xcb_window_t window;
	ecore_x_icccm_transient_for_set(
			ecore_evas_software_x11_8_window_get(main_win),
			window);

	Evas* main_canvas = ecore_evas_get(main_win);

	ecore_evas_callback_delete_request_set(main_win, main_win_close_handler);

	Evas_Object* main_canvas_edje = eoi_create_themed_edje(main_canvas, "fbreader_messagebox", "message");
	evas_object_name_set(main_canvas_edje, "main_canvas_edje");
//	edje_object_signal_callback_add(main_canvas_edje, "*", "*", main_win_signal_handler, NULL);
	edje_object_part_text_set(main_canvas_edje, "text", text);
	evas_object_move(main_canvas_edje, 0, 0);
	evas_object_resize(main_canvas_edje, 600, 800);

	Evas_Coord x, y, w, h;
	const Evas_Object *t = edje_object_part_object_get(main_canvas_edje, "text");
	evas_object_geometry_get(t, &x, &y, &w, &h);

	w += 20;
	h += 20;
	ecore_evas_resize(main_win, w, h);
	ecore_evas_move(main_win, (600 - w)/2, (800 - h)/2);
	evas_object_resize(main_canvas_edje, w, h);
	evas_object_move(main_canvas_edje, 0, 0);
	evas_object_show(main_canvas_edje);

	evas_object_focus_set(main_canvas_edje, 1);
	evas_object_event_callback_add(main_canvas_edje,
			EVAS_CALLBACK_KEY_UP,
			&main_win_key_handler,
			handler);

	ecore_evas_callback_resize_set(main_win, main_win_resize_handler);

	ecore_evas_show(main_win);

	ecore_main_loop_iterate();

	ecore_main_loop_begin();
	if(emergency_exit)
		return;

	ecore_evas_hide(main_win);
	ecore_evas_free(main_win);
}

// entry

#define MAXDIGITS	6

struct entry_number {
	char *text;
	long number;
	int cnt;
};

static void entry_win_key_handler(void* param, Evas* e __UNUSED__, Evas_Object* o, void* event_info)
{
    Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;

	struct entry_number *number = (struct entry_number*)param;

	if(isdigit(ev->keyname[3]) && !ev->keyname[4] && number->cnt < MAXDIGITS) {
		if(number->number > 0)
			number->number *= 10;
		else
			number->number = 0;

		number->number += ev->keyname[3] - '0';
		if(number->number > 0)
			number->cnt++;

		char *t;
		if(number->number > 0)
			asprintf(&t, "%s: %-*ld", number->text, MAXDIGITS, number->number);
		else
			asprintf(&t, "%s: %-*c", number->text, MAXDIGITS, 0);
		edje_object_part_text_set(o, "entrytext", t);
		free(t);
	}
	if(!strcmp(ev->keyname, "Escape")) {
		if(number->cnt > 0) {
			number->cnt--;
			number->number /= 10;

			char *t;
			if(number->number > 0)
				asprintf(&t, "%s: %-*ld", number->text, MAXDIGITS, number->number);
			else
				asprintf(&t, "%s: %-*c", number->text, MAXDIGITS, 0);
			edje_object_part_text_set(o, "entrytext", t);
			free(t);
		} else {
			number->number = -1;
			ecore_main_loop_quit();
		}
	}
	if(!strcmp(ev->keyname, "Return"))
		ecore_main_loop_quit();
}

long read_number(char *text)
{
	ee_init();

	struct entry_number number;
	number.text = text;
	number.number = -1;
	number.cnt = 0;

	Ecore_Evas* main_win = ecore_evas_software_x11_8_new(0, 0, 0, 0, 600, 800);
	ecore_evas_title_set(main_win, "EB");
	ecore_evas_name_class_set(main_win, "EB", "EB");

	extern xcb_window_t window;
	ecore_x_icccm_transient_for_set(
			ecore_evas_software_x11_8_window_get(main_win),
			window);

	Evas* main_canvas = ecore_evas_get(main_win);

	ecore_evas_callback_delete_request_set(main_win, main_win_close_handler);

	Evas_Object* main_canvas_edje = eoi_create_themed_edje(main_canvas, "fbreader_entrybox", "entrybox");
	evas_object_name_set(main_canvas_edje, "main_canvas_edje");
//	edje_object_signal_callback_add(main_canvas_edje, "*", "*", main_win_signal_handler, NULL);
	evas_object_move(main_canvas_edje, 0, 0);
	evas_object_resize(main_canvas_edje, 600, 800);

	char *t;
	asprintf(&t, "%s: %-*d", text, MAXDIGITS, 999999);
	edje_object_part_text_set(main_canvas_edje, "entrytext", t);
	free(t);

	Evas_Coord x, y, w, h;
	evas_object_geometry_get(
			edje_object_part_object_get(main_canvas_edje, "entrytext"),
			&x, &y, &w, &h);

	asprintf(&t, "%s: %-*c", text, MAXDIGITS, 0);
	edje_object_part_text_set(main_canvas_edje, "entrytext", t);
	free(t);

	w += 40;
	h += 20;
	ecore_evas_resize(main_win, w, h);
	ecore_evas_move(main_win, (600 - w)/2, (800 - h)/2);
	evas_object_resize(main_canvas_edje, w, h);
	evas_object_move(main_canvas_edje, 0, 0);
	evas_object_show(main_canvas_edje);

	evas_object_focus_set(main_canvas_edje, 1);
	evas_object_event_callback_add(main_canvas_edje,
			EVAS_CALLBACK_KEY_UP,
			&entry_win_key_handler,
			(void*)&number);

	ecore_evas_callback_resize_set(main_win, main_win_resize_handler);

	ecore_evas_show(main_win);

	ecore_main_loop_iterate();

	ecore_main_loop_begin();
	if(emergency_exit)
		return -1;

	ecore_evas_hide(main_win);
	ecore_evas_free(main_win);

	return number.number;
}

static void
entry_handler(Evas_Object *entry __UNUSED__,
        const char *text,
        void* param)
{
	if(param) {
		void (*handler)(const char *) = (void(*)(const char*))param;
		handler(text);
	}

	ecore_main_loop_quit();
}

static void text_entry_resized(Ecore_Evas *ee __UNUSED__, Evas_Object *object, int w, int h,
             void *param __UNUSED__)
{
	evas_object_resize(object, w, h);
}

static void evas_object_callback_del(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
	ecore_main_loop_quit();
}

Ecore_Evas *text_entry(char *text, void (*handler)(const char*))
{
	ee_init();

	Ecore_Evas* main_win = ecore_evas_software_x11_8_new(0, 0, 0, 0, 600, 800);
	ecore_evas_title_set(main_win, "TE");
	ecore_evas_name_class_set(main_win, "TE", "TE");

	extern xcb_window_t window;
	ecore_x_icccm_transient_for_set(
			ecore_evas_software_x11_8_window_get(main_win),
			window);

	Evas* main_canvas = ecore_evas_get(main_win);

	ecore_evas_callback_delete_request_set(main_win, main_win_close_handler);

	/*
	Evas_Object *wm = eoi_main_window_create(main_canvas);
	evas_object_name_set(wm, "main-window");
	eoi_resize_object_register(lcb_win, wm, lcb_win_resized, NULL);
	evas_object_move(wm, 0, 0);
	evas_object_resize(wm, w, h);
	evas_object_show(wm);
	*/

	Evas_Object *bg = evas_object_rectangle_add(main_canvas);
	evas_object_move(bg, 0, 0);
	evas_object_resize(bg, 600, 800);
	evas_object_color_set(bg, 255, 255, 255, 255);
	evas_object_show(bg);
	eoi_resize_object_register(main_win, bg, text_entry_resized, NULL);

	Evas_Object *e = entry_new(main_canvas, entry_handler, "entry",
			text, (void*)handler);

	evas_object_event_callback_add(e, EVAS_CALLBACK_DEL, evas_object_callback_del, NULL);

	//eoi_resize_object_register(main_win, wm, text_entry_resized, NULL);

	ecore_evas_show(main_win);

	ecore_main_loop_iterate();

	return main_win;
}
