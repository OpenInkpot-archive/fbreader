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
#include <libeoi_help.h>

#include <libchoicebox.h>
}

#include <libintl.h>

#include "ZLEwlMessage.h"
#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Evas.h>
#include <Edje.h>

#define __UNUSED__ __attribute__((__unused__))

using namespace std;

extern void ee_init();
extern bool emergency_exit;

static void __attribute__ ((__used__)) die(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

static int __attribute__ ((__used__)) exit_handler(void* param __UNUSED__, int ev_type __UNUSED__, void* event __UNUSED__)
{
    ecore_main_loop_quit();
    return 1;
}

static void __attribute__ ((__used__)) main_win_close_handler(Ecore_Evas* main_win __UNUSED__)
{
    ecore_main_loop_quit();
}

static void main_win_resize_handler(Ecore_Evas* main_win)
{
    Evas* canvas = ecore_evas_get(main_win);
    int w, h;
    evas_output_size_get(canvas, &w, &h);

    Evas_Object* rr = evas_object_name_find(canvas, "help-window");
    evas_object_hide(rr);
    evas_object_resize(rr, w, h);
    evas_object_show(rr);
}

static void help_closed(Evas_Object* help)
{
    Evas* evas = evas_object_evas_get(help);
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas);
    ecore_evas_hide(ee);
    ecore_main_loop_quit();
}

static void page_updated_handler(Evas_Object* tb,
        int cur_page,
        int total_pages,
        const char* header __UNUSED__,
        void* param __UNUSED__)
{
    Evas* canvas = evas_object_evas_get(tb);
    Evas_Object* rr = evas_object_name_find(canvas, "help-window");
    choicebox_aux_edje_footer_handler(rr, "footer", cur_page, total_pages);
}

static Eina_Bool screen_change_handler(void *data, int type, void *event)
{
    if(type != ECORE_X_EVENT_SCREEN_CHANGE)
        return 0;

    Ecore_Evas *win = (Ecore_Evas*)data;
    Ecore_X_Event_Screen_Change *e = (Ecore_X_Event_Screen_Change*)event;

    int w, h;

    if(e->rotation == ECORE_X_RANDR_ROT_90 || e->rotation == ECORE_X_RANDR_ROT_270) {
        h = e->width;
        w = e->height;
    } else {
        w = e->width;
        h = e->height;
    }

    ecore_evas_resize(win, w, h);

    return 0;
}

void show_help()
{
    ee_init();

    Ecore_Evas *main_win = ecore_evas_software_x11_new(0, 0, 0, 0, 600, 800);
    ecore_evas_borderless_set(main_win, 0);
    ecore_evas_title_set(main_win, gettext("FBReader: Help"));
    ecore_evas_name_class_set(main_win, "fbreader_help", "fbreader_help");

    extern xcb_window_t window;
    ecore_x_icccm_transient_for_set(
            ecore_evas_software_x11_window_get(main_win),
            window);

    ecore_x_randr_events_select(ecore_evas_software_x11_window_get(main_win), 1);
    //Ecore_Event_Handler *sc_handler = 
	ecore_event_handler_add(ECORE_X_EVENT_SCREEN_CHANGE, screen_change_handler, main_win);

    ecore_evas_callback_resize_set(main_win, main_win_resize_handler);

    Evas *main_canvas = ecore_evas_get(main_win);

    Evas_Object* rr = eoi_main_window_create(main_canvas);

    edje_object_part_text_set(rr, "title", gettext("FBReader: Help"));
    edje_object_part_text_set(rr, "footer", "0/0");

    evas_object_name_set(rr, "help-window");
    evas_object_move(rr, 0, 0);
    evas_object_resize(rr, 600, 800);
    evas_object_show(rr);

    Evas_Object *help = eoi_help_new(main_canvas, "fbreader", page_updated_handler, help_closed);
    evas_object_show(help);

    edje_object_part_swallow(rr, "contents", help);
    evas_object_focus_set(help, 1);

    ecore_evas_show(main_win);

    ecore_main_loop_iterate();

    ecore_main_loop_begin();
    if(emergency_exit)
        return;

    ecore_evas_hide(main_win);
    ecore_evas_free(main_win);
}
