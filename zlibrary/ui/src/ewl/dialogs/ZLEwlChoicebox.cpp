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

#include <ZLibrary.h>

#include "ZLEwlDialogs.h"
#include "ZLEwlChoicebox.h"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <sstream>

#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Evas.h>
#include <Edje.h>

#include "../../../../../fbreader/src/options/FBTextStyle.h"

#include <libintl.h>

extern "C" {
#include <xcb/xcb.h>
#define class class_
#include <xcb/xcb_aux.h>
#undef class

#include <libchoicebox.h>
#include <libeoi.h>
#include <libeoi_themes.h>
}

#define __UNUSED__ __attribute__((__unused__))

#define _(x) x

#define SETTINGS_LEFT_NAME "settings_left"
#define SETTINGS_RIGHT_NAME "settings_right"

/* FIXME */
#define BUFSIZE 512

/* FIXME */
const int header_h = 49;
const int footer_h = 49;

bool emergency_exit = false;

static Ecore_Evas *lcb_win, *fcb_win;

static void cb_rcb_destroy();

static bool reuse_fcb_win = false;

static bool bookmark_delete_mode = false;

void exit_all(void* param __UNUSED__) {
	emergency_exit = true;
	ecore_main_loop_quit();
}

static int exit_handler(void* param __UNUSED__, int ev_type __UNUSED__, void* event __UNUSED__)
{
	ecore_main_loop_quit();

	return 1;
}

static void lcb_win_close_handler(Ecore_Evas* main_win __UNUSED__)
{
	ecore_main_loop_quit();
}

static void lcb_page_updated_handler(Evas_Object* choicebox,
		int cur_page,
		int total_pages,
		void* param __UNUSED__)
{
    Evas* canvas = evas_object_evas_get(choicebox);
	Evas_Object *settings_window = evas_object_name_find(canvas, "settings-left-window");
	choicebox_aux_edje_footer_handler(settings_window, "footer", cur_page, total_pages);
}

static void textblock_style_add_font(Evas_Object *item, string font)
{
	const Evas_Object *tb = edje_object_part_object_get(item, "title");
	const Evas_Textblock_Style *st = evas_object_textblock_style_get(tb);

	if(!strstr(evas_textblock_style_get(st), font.c_str())) {
		char *style;
		asprintf(&style, "%s%s='+font=%s'/%s='-'", evas_textblock_style_get(st), font.c_str(), font.c_str(), font.c_str());
		evas_textblock_style_set((Evas_Textblock_Style*)st, style);
		evas_object_textblock_style_set((Evas_Object*)tb, (Evas_Textblock_Style*)st);
		free(style);
	}
}

static void textblock_style_add_fsize(Evas_Object *item, int fsize)
{
	const Evas_Object *tb = edje_object_part_object_get(item, "title");
	const Evas_Textblock_Style *st = evas_object_textblock_style_get(tb);

	char *c;
	asprintf(&c, "fsize%d", fsize);
	if(!strstr(evas_textblock_style_get(st), c)) {
		static int dpi = 0;

		if(!dpi) {
			xcb_connection_t     *connection;
			xcb_screen_t         *screen;
			int                   screen_number;

			connection = xcb_connect (NULL, &screen_number);
			if (xcb_connection_has_error(connection)) {
				fprintf (stderr, "ERROR: can't connect to an X server\n");
				exit(-1);
			}

			screen = xcb_aux_get_screen (connection, screen_number);

			dpi = (((double)screen->width_in_pixels) * 25.4) / ((double) screen->width_in_millimeters);

			xcb_disconnect(connection);
		}

		char *style;
		asprintf(&style, "%sfsize%d='+font_size=%d'/fsize%d='-'", evas_textblock_style_get(st), fsize, fsize * dpi / 72, fsize);
		evas_textblock_style_set((Evas_Textblock_Style*)st, style);
		evas_object_textblock_style_set((Evas_Object*)tb, (Evas_Textblock_Style*)st);
		free(style);
	}
	free(c);
}

static void lcb_draw_handler(Evas_Object* choicebox __UNUSED__,
		Evas_Object* item,
		int item_num,
		int page_position __UNUSED__,
		void* param __UNUSED__)
{
	if(olists.empty())
		return;

	cb_olist *l = olists.back();
	if(!l)
		return;

	if(item_num >= (int)l->items.size())
		return;

	cb_olist_item *i = &l->items.at(item_num);
	if(i->type == ITEM_SUBMENU || i->type == ITEM_TEXT) {
		edje_object_part_text_set(item, "title", i->name.c_str());
		edje_object_part_text_set(item, "value", "");
	} else if(i->type == ITEM_OPTION) {
		if(l->fsize_list) {
			std::string fn = FBTextStyle::Instance().FontFamilyOption.value();
			fn.erase(remove_if(fn.begin(), fn.end(), static_cast<int(*)(int)>( isspace )), fn.end());
			textblock_style_add_font(item, fn);
			textblock_style_add_fsize(item, i->current_value.ival);

			char *s2;
			asprintf(&s2, "<%s><fsize%d>%s</fsize%d></%s>", fn.c_str(), i->current_value.ival, _("ABCabc"), i->current_value.ival, fn.c_str());
			edje_object_part_text_set(item, "value", i->current_value.text.c_str());
			edje_object_part_text_set(item, "title", s2);
			free(s2);
		} else {
			edje_object_part_text_set(item, "title", i->name.c_str());

			if(i->values.empty())
				edje_object_part_text_set(item, "value",  i->current_value.text.c_str());
			else if(i->values.size() <= 3) {
				string s;
				for(unsigned int z = 0; z < i->values.size(); z++)
					if((int)z != i->curval_idx) {
						s += "  <inactive>";
						s += i->values.at(z).text;
						s += "</inactive>";
					} else
						s += "  " + i->values.at(z).text;

					edje_object_part_text_set(item, "value", s.c_str());
			}
		}
	}

//	fprintf(stderr, "handle: choicebox: %p, item: %p, item_num: %d, page_position: %d, param: %p\n",
//			choicebox, item, item_num, page_position, param);
}

static void lcb_handler(Evas_Object* choicebox,
		int item_num,
		bool is_alt,
		void* param __UNUSED__)
{
//	printf("handle: choicebox: %p, item_num: %d, is_alt: %d, param: %p\n",
//			choicebox, item_num, is_alt, param);

	if(olists.empty())
		return;

	cb_olist *l = olists.back();
	if(!l || !l->item_handler)
		return;

	choicebox_set_selection(choicebox, item_num);

	if(l->items.at(item_num).item_handler != NULL)
		l->items.at(item_num).item_handler(item_num, is_alt);
	else
		l->item_handler(item_num, is_alt);
}

static void lcb_win_resized(Ecore_Evas *ee __UNUSED__, Evas_Object *object, int w, int h,
             void *param __UNUSED__)
{
	evas_object_resize(object, w, h);
}

static void lcb_win_resized_2(Ecore_Evas *ee, Evas_Object *object, int w, int h,
             void *param __UNUSED__)
{
	if(edje_object_part_swallow_get(evas_object_name_find(ecore_evas_get(ee), "main-window"), "right-overlay"))
		evas_object_resize(object, w/2, h);
	else
		evas_object_resize(object, w, h);
}

static void choicebox_resized(Ecore_Evas *ee __UNUSED__, Evas_Object *object, int w __UNUSED__, int h __UNUSED__,
             void *param)
{
	if(param) {
		int *select_item = (int*)param;
		if(*select_item >= 0)
			choicebox_set_selection(object, *select_item);
		*select_item = -1;
	}
}

void cb_lcb_invalidate(int idx)
{
	Evas* canvas = ecore_evas_get(lcb_win);
	Evas_Object* choicebox = evas_object_name_find(canvas, SETTINGS_LEFT_NAME);
	choicebox_invalidate_item(choicebox, idx);
}

static void cb_lcb_destroy()
{
	if(olists.empty())
		return;

	cb_olist *l = olists.back();
	if(!l)
		return;

	if(l->destroy_handler != NULL)
		l->destroy_handler();

	delete l;
	olists.pop_back();

	if(olists.size() > 0)
		cb_lcb_redraw();
	else
		ecore_main_loop_quit();
}

static void lcb_win_key_up_handler(void* param __UNUSED__, Evas* e, Evas_Object* o, void* event_info)
{
	int i;
	Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
    bool is_alt = evas_key_modifier_is_set(ev->modifiers, "Alt");

	Evas_Object* r = evas_object_name_find(e, SETTINGS_LEFT_NAME);

    /* FIXME: this might become configurable in future */
	if(!strcmp(ev->keyname, "Left")) {
		cb_olist_item *item;
		if((i = choicebox_get_selection(r)) != -1 &&
				(item = &olists.back()->items.at(i)) &&
				(item->values.size() == 3 || item->values.size() == 2)) {
			if(item->values.size() == 3)
				choicebox_activate_current(r, is_alt);
			choicebox_activate_current(r, is_alt);
            return;
		}
	}
	if(!strcmp(ev->keyname, "Right")) {
		cb_olist_item *item;
		if((i = choicebox_get_selection(r)) != -1 &&
				(item = &olists.back()->items.at(i)) &&
				(item->values.size() == 3 || item->values.size() == 2)) {
			choicebox_activate_current(r, is_alt);
            return;
		}
	}
	
	// fixme
	if(!strcmp(ev->keyname, "XF86Search") && !olists.back()->name.compare(gettext("Font Size"))) {
		choicebox_request_close(r);
		return;
	}

	choicebox_aux_key_up_handler(o, ev);
}

static void lcb_close_handler(Evas_Object* choicebox __UNUSED__, void* param __UNUSED__)
{
    cb_lcb_destroy();
}

void cb_lcb_redraw()
{
	if(olists.size() < 1)
		return;

	cb_olist *l = olists.back();
	if(!l)
		return;

	Evas* canvas = ecore_evas_get(lcb_win);
	Evas_Object* choicebox = evas_object_name_find(canvas, SETTINGS_LEFT_NAME);
	Evas_Object* header = evas_object_name_find(canvas, "lcb_header");
	edje_object_part_text_set(header, "text", olists.empty() ? "" : olists.back()->name.c_str());
	choicebox_set_size(choicebox, l->items.size());
	choicebox_invalidate_interval(choicebox, 0, l->items.size());
	if(l->items.size() > 0) {
		choicebox_scroll_to(choicebox, 0);
		choicebox_set_selection(choicebox, -1);
	}

	bookmark_delete_mode = false;
}

static Ecore_Idle_Enterer *idle_enterer = NULL;

static int
idle_render(void *data __UNUSED__)
{
	void refresh_view();
	refresh_view();

	return 0;
}

void ee_init()
{
	static int _init = 0;
	if(!_init) {
		if(!evas_init())
			return;
		if(!ecore_init())
			return;
		if(!ecore_x_init(NULL))
			return;
		if(!ecore_evas_init())
			return;
		if(!edje_init())
			return;

		ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_handler, NULL);
		ecore_x_io_error_handler_set(exit_all, NULL);
		idle_enterer = ecore_idle_enterer_add(idle_render, NULL);

		_init = 1;
	}
}

static int lcb_screen_change_handler(void *data, int type, void *event)
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

	ecore_evas_move(win, 0, 0);
	if(edje_object_part_swallow_get(evas_object_name_find(ecore_evas_get(win), "main-window"), "right-overlay")) {
		ecore_evas_resize(win, w, h);
	} else {
		ecore_evas_resize(win, w/2, h);
	}

	return 0;
}

void cb_lcb_new(int select_item)
{
	ee_init();

	/*
	Ecore_X_Randr_Rotation r;
	Ecore_X_Screen_Size s;
	Ecore_X_Window root;

	root = ecore_x_window_root_first_get();

	ecore_x_randr_get_screen_info_prefetch(root);
	ecore_x_randr_get_screen_info_fetch();
	r = ecore_x_randr_screen_rotation_get(root);
	s = ecore_x_randr_current_screen_size_get(root);

	if(r == ECORE_X_RANDR_ROT_90 || r == ECORE_X_RANDR_ROT_270) {
		w = s.width;
		h = s.height;
	} else {
		h = s.width;
		w = s.height;
	}
	*/

	int w, h;
	extern xcb_window_t window;
	ecore_x_drawable_geometry_get_prefetch(window);
	ecore_x_drawable_geometry_get_fetch();
	ecore_x_window_size_get(window, &w, &h);


	lcb_win = ecore_evas_software_x11_new(0, 0, 0, 0, w, h);

	ecore_evas_title_set(lcb_win, "LCB");
	ecore_evas_name_class_set(lcb_win, "LCB", "LCB");

	extern xcb_window_t window;
	ecore_x_icccm_transient_for_set(
			ecore_evas_software_x11_window_get(lcb_win),
			window);

	ecore_x_randr_events_select(ecore_evas_software_x11_window_get(lcb_win), 1);
	Ecore_Event_Handler *sc_handler = ecore_event_handler_add(ECORE_X_EVENT_SCREEN_CHANGE, lcb_screen_change_handler, lcb_win);

	ecore_evas_resize(lcb_win, w/2, h);
	ecore_evas_move(lcb_win, 0, 0);

	Evas* main_canvas = ecore_evas_get(lcb_win);

	ecore_evas_callback_delete_request_set(lcb_win, lcb_win_close_handler);

	Evas_Object *bg = evas_object_rectangle_add(main_canvas);
	evas_object_move(bg, 0, 0);
	evas_object_resize(bg, w/2, h);
	evas_object_color_set(bg, 255, 255, 255, 255);
	evas_object_show(bg);
	eoi_resize_object_register(lcb_win, bg, lcb_win_resized_2, NULL);

	Evas_Object *settings_window = eoi_settings_left_create(main_canvas);
	evas_object_name_set(settings_window, "settings-left-window");
	evas_object_show(settings_window);

	edje_object_part_text_set(settings_window, "title", olists.empty() ? "" : olists.back()->name.c_str()); 

    choicebox_info_t info = {
        NULL,
        "choicebox",
        "settings-left",
		"choicebox",
        "item-settings",
        lcb_handler,
        lcb_draw_handler,
        lcb_page_updated_handler,
        lcb_close_handler,
    };

	Evas_Object* choicebox = choicebox_new(main_canvas, &info, NULL);
	choicebox_set_size(choicebox, olists.empty() ? 0 : olists.back()->items.size());
	evas_object_name_set(choicebox, SETTINGS_LEFT_NAME);
	evas_object_show(choicebox);
    eoi_register_fullscreen_choicebox(choicebox);

	edje_object_part_swallow(settings_window, "contents", choicebox);
	eoi_resize_object_register(lcb_win, settings_window, lcb_win_resized_2, NULL);

	if(select_item >= 0) {
		choicebox_scroll_to(choicebox, select_item);

		eoi_resize_object_register(lcb_win, choicebox, choicebox_resized, &select_item);
//		choicebox_set_selection(choicebox, select_item);
	}

    eoi_register_fullscreen_choicebox(choicebox);

	evas_object_focus_set(choicebox, true);
	evas_object_event_callback_add(choicebox,
			EVAS_CALLBACK_KEY_UP,
			&lcb_win_key_up_handler,
			NULL);

	ecore_evas_show(lcb_win);

	bookmark_delete_mode = false;

	ecore_main_loop_begin();
	if(emergency_exit)
		return;

	if(lcb_win) {
		ecore_event_handler_del(sc_handler);
		ecore_x_randr_events_select(ecore_evas_software_x11_window_get(lcb_win), 0);
		ecore_evas_hide(lcb_win);
		ecore_evas_free(lcb_win);
		lcb_win = NULL;
	}
}

// rcb
static void rcb_page_updated_handler(Evas_Object* choicebox,
		int cur_page,
		int total_pages,
		void* param __UNUSED__)
{
    Evas* canvas = evas_object_evas_get(choicebox);
	Evas_Object *settings_window = evas_object_name_find(canvas, "settings-right-window");
	choicebox_aux_edje_footer_handler(settings_window, "footer", cur_page, total_pages);
}

static void rcb_draw_handler(Evas_Object* choicebox __UNUSED__,
		Evas_Object* item,
		int item_num,
		int page_position __UNUSED__,
		void* param __UNUSED__)
{
	if(!vlist)
		return;

	if(item_num >= (int)vlist->values.size())
		return;

	cb_item_value *iv = &vlist->values.at(item_num);

	if(vlist->font_list) {
		std::string fn = iv->text;
		fn.erase(remove_if(fn.begin(), fn.end(), static_cast<int(*)(int)>( isspace )), fn.end());
		textblock_style_add_font(item, fn);

		std::stringstream s;
		s << "<" << fn << ">" << iv->text << "</" << fn << ">";
		edje_object_part_text_set(item, "title", s.str().c_str());
		edje_object_part_text_set(item, "value", "");
	} else if(vlist->fsize_list) {
		std::string fn = FBTextStyle::Instance().FontFamilyOption.value();
		fn.erase(remove_if(fn.begin(), fn.end(), static_cast<int(*)(int)>( isspace )), fn.end());
		textblock_style_add_font(item, fn);
		textblock_style_add_fsize(item, iv->ival);

		char *s2;
		asprintf(&s2, "<%s><fsize%d>%s</fsize%d></%s>", fn.c_str(), iv->ival, _("ABCabc"), iv->ival, fn.c_str());
		edje_object_part_text_set(item, "value", iv->text.c_str());
		edje_object_part_text_set(item, "title", s2);
		free(s2);
	} else {
		edje_object_part_text_set(item, "title", iv->text.c_str());
		edje_object_part_text_set(item, "value", "");
	}

//	fprintf(stderr, "rcd_draw_handle: choicebox: %p, item: %p, item_num: %d, page_position: %d, param: %p\n",
//			choicebox, item, item_num, page_position, param);
}

static void rcb_handler(Evas_Object* choicebox __UNUSED__,
		int item_num,
		bool is_alt,
		void* param __UNUSED__)
{
//	printf("rcb_handle: choicebox: %p, item_num: %d, is_alt: %d, param: %p\n",
//			choicebox, item_num, is_alt, param);

	if(!vlist || !vlist->item_handler)
		return;

	vlist->item_handler(item_num, is_alt);

	cb_rcb_destroy();
}

static void __attribute__((__used__)) cb_rcb_destroy()
{
	Evas* e = ecore_evas_get(lcb_win);

	Evas_Object *wm = evas_object_name_find(e, "main-window");
	if(wm) {
		Evas_Object *o;
		o = edje_object_part_swallow_get(wm, "contents");
		if(o) {
			edje_object_part_unswallow(wm, o);
			evas_object_hide(o);
			evas_object_del(o);
		}

		o = edje_object_part_swallow_get(wm, "right-overlay");
		if(o) {
			edje_object_part_unswallow(wm, o);

			Evas_Object *o2 = edje_object_part_swallow_get(o, "contents");
			if(o2) {
				free(evas_object_data_get(o, "sel"));
				evas_object_hide(o2);
				evas_object_del(o2);
			}

			evas_object_hide(o);
			evas_object_del(o);
		}

		o = edje_object_part_swallow_get(wm, "left-overlay");
		if(o)
			edje_object_part_unswallow(wm, o);

		evas_object_hide(wm);
		evas_object_del(wm);
	}

	int w, h;
	evas_output_size_get(ecore_evas_get(lcb_win), &w, &h);
	ecore_evas_resize(lcb_win, w / 2, h);

	evas_object_focus_set(evas_object_name_find(e, SETTINGS_LEFT_NAME), true);

	if(vlist) {
		if(vlist->destroy_handler) {
			vlist->destroy_handler();
		}

		delete vlist;
		vlist = NULL;
	}

	void set_refresh_flag();
	set_refresh_flag();
}

static void rcb_close_handler(Evas_Object* choicebox __UNUSED__, void* param __UNUSED__)
{
    cb_rcb_destroy();
}

void cb_rcb_new(int select_item)
{
	Evas* main_canvas = ecore_evas_get(lcb_win);
	int w, h;
	evas_output_size_get(main_canvas, &w, &h);
	ecore_evas_resize(lcb_win, w * 2, h);

	//new
	Evas_Object *settings_window = eoi_settings_right_create(main_canvas);
	evas_object_name_set(settings_window, "settings-right-window");
	evas_object_show(settings_window);

	edje_object_part_text_set(settings_window, "title", vlist->name.c_str());

    choicebox_info_t info = {
        NULL,
        "choicebox",
        "settings-right",
        "choicebox",
        "item-settings",
        rcb_handler,
        rcb_draw_handler,
        rcb_page_updated_handler,
        rcb_close_handler,
    };
	Evas_Object* choicebox = choicebox_new(main_canvas, &info, NULL);


	choicebox_set_size(choicebox, vlist->values.size());
	evas_object_name_set(choicebox, SETTINGS_RIGHT_NAME);
	evas_object_show(choicebox);

	edje_object_part_swallow(settings_window, "contents", choicebox);

	Evas_Object *bg = evas_object_rectangle_add(main_canvas);
	evas_object_color_set(bg, 255, 255, 255, 255);
	evas_object_show(bg);

	Evas_Object *wm = eoi_main_window_create(main_canvas);
	evas_object_name_set(wm, "main-window");
	eoi_resize_object_register(lcb_win, wm, lcb_win_resized, NULL);
	evas_object_move(wm, 0, 0);
	evas_object_resize(wm, w, h);
	evas_object_show(wm);

	if(select_item >= 0) {
		choicebox_scroll_to(choicebox, select_item);

		int *i = (int*)malloc(sizeof(int));
		*i = select_item;
		eoi_resize_object_register(lcb_win, choicebox, choicebox_resized, i);

		evas_object_data_set(choicebox, "sel", i);
//		choicebox_set_selection(choicebox, select_item);
	}

	edje_object_part_swallow(wm, "contents", bg);
	edje_object_part_swallow(wm, "right-overlay", settings_window);
	edje_object_part_swallow(wm, "left-overlay", evas_object_name_find(main_canvas, "settings-left-window"));

    eoi_register_fullscreen_choicebox(choicebox);

    choicebox_aux_subscribe_key_up(choicebox);
	evas_object_focus_set(choicebox, true);

	bookmark_delete_mode = false;
}

// fcb
static void fcb_win_close_handler(Ecore_Evas* main_win __UNUSED__)
{
	ecore_main_loop_quit();
}

static void fcb_page_updated_handler(Evas_Object* choicebox,
		int cur_page,
		int total_pages,
		void* param __UNUSED__)
{
    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* main_canvas_edje = evas_object_name_find(canvas, "fcb-window");
	choicebox_aux_edje_footer_handler(main_canvas_edje, "footer", cur_page, total_pages);
}

static void fcb_draw_handler(Evas_Object* choicebox __UNUSED__,
		Evas_Object* item,
		int item_num,
		int page_position __UNUSED__,
		void* param)
{
	cb_list *l = (cb_list*)param;
	
	if(l->items.empty())
		return;

	cb_list_item *i = &l->items.at(item_num);
	if(!i->text.empty()) {
		edje_object_part_text_set(item, "text", i->text.c_str());
		edje_object_part_text_set(item, "title", "");
		edje_object_part_text_set(item, "value", "");
		if(i->icon.empty())
			edje_object_signal_emit(item, "no-icon", "");
		else
			edje_object_signal_emit(item, i->icon.c_str(), "");
	} else {
		edje_object_part_text_set(item, "text", "");
		edje_object_part_text_set(item, "title", i->title.c_str());
		edje_object_part_text_set(item, "value", i->value.c_str());
		edje_object_signal_emit(item, "no-icon", "");
	}

//	fprintf(stderr, "handle: choicebox: %p, item: %p, item_num: %d, page_position: %d, param: %p\n",
//			choicebox, item, item_num, page_position, param);
}

static void fcb_handler(Evas_Object* choicebox __UNUSED__,
		int item_num,
		bool is_alt,
		void* param)
{
//	printf("handle: choicebox: %p, item_num: %d, is_alt: %d, param: %p\n",
//			choicebox, item_num, is_alt, param);

	cb_list *l = (cb_list *)param;

	if(!l || !l->item_handler)
		return;

	switch(l->item_handler(item_num, is_alt || bookmark_delete_mode)) {
		case 1:
			reuse_fcb_win = false;
			ecore_main_loop_quit();
			break;
		case 2:
			reuse_fcb_win = true;
			ecore_main_loop_quit();
		case 0:
		default:
			break;
	}
}

static int fcb_screen_change_handler(void *data, int type, void *event)
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

	ecore_evas_move(win, 0, 0);
    ecore_evas_resize(win, w, h);

	return 0;
}

static void cb_fcb_destroy()
{
//	if(l->destroy_handler != NULL)
//		l->destroy_handler();

	ecore_main_loop_quit();
}

static void fcb_win_key_up_handler(void* param, Evas* e, Evas_Object* o, void* event_info)
{
	Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
//	fprintf(stderr, "kn: %s, k: %s, s: %s, c: %s\n", ev->keyname, ev->key, ev->string, ev->compose);

	const char *k = ev->key;

	//Evas_Object* r = evas_object_name_find(e, "cb_full");

    /* FIXME: in far future this should be made configurable */
	if(!strcmp(k, "space")) {
		bookmark_delete_mode = !bookmark_delete_mode;

		cb_list *list = (cb_list*)param;
		if(!list->alt_text.empty()) {
			Evas_Object *del_icon = evas_object_name_find(e, "del-icon");

			if(!del_icon && bookmark_delete_mode) {
				del_icon = eoi_create_themed_edje(e, "fbreader", "del_icon");
				evas_object_name_set(del_icon, "del-icon");
				edje_object_part_swallow(evas_object_name_find(e, "fcb-window"), "state-icons", del_icon);
			}

			if(!del_icon)
				return;

			if(bookmark_delete_mode)
				evas_object_show(del_icon);
			else
				evas_object_hide(del_icon);
		}

        return;
	}

    choicebox_aux_key_up_handler(o, ev);
}

static void fcb_close_handler(Evas_Object* choicebox __UNUSED__, void* param __UNUSED__)
{
    cb_fcb_destroy();
}

void cb_fcb_invalidate(int idx)
{
	Evas* canvas = ecore_evas_get(fcb_win);
	Evas_Object* choicebox = evas_object_name_find(canvas, "cb_full");
	choicebox_invalidate_item(choicebox, idx);
}

void cb_fcb_invalidate_interval(int start, int end)
{
	Evas* canvas = ecore_evas_get(fcb_win);
	Evas_Object* choicebox = evas_object_name_find(canvas, "cb_full");
	choicebox_invalidate_interval(choicebox, start, end);
}

void cb_fcb_redraw(int newsize)
{
	Evas* canvas = ecore_evas_get(fcb_win);
	Evas_Object* choicebox = evas_object_name_find(canvas, "cb_full");
	if(newsize >= 0) {
		choicebox_set_size(choicebox, newsize);
		choicebox_invalidate_interval(choicebox, 0, newsize);
	}
	if(newsize > 0) {
		choicebox_scroll_to(choicebox, 0);
		choicebox_set_selection(choicebox, -1);
	}

	//bookmark_delete_mode = false;
}

void cb_fcb_new(cb_list *list, int select_item)
{
	Evas_Object *main_canvas_edje;
	Ecore_Event_Handler *sc_handler;

	if(reuse_fcb_win && fcb_win) {
		Evas* main_canvas = ecore_evas_get(fcb_win);
		Evas_Object *choicebox = evas_object_name_find(main_canvas, "cb_full");

		main_canvas_edje = evas_object_name_find(main_canvas, "fcb-window");
		edje_object_part_text_set(main_canvas_edje, "title", list->name.c_str());
		edje_object_part_text_set(main_canvas_edje, "footer", list->alt_text.c_str());
		evas_object_show(main_canvas_edje);

		choicebox_set_size(choicebox, list->items.size());

		choicebox_invalidate_interval(choicebox, 0, list->items.size());
		if(list->items.size() > 0) {
			choicebox_scroll_to(choicebox, 0);
			choicebox_set_selection(choicebox, -1);
		}
	} else {
		ee_init();

		int w, h;
		extern xcb_window_t window;
		ecore_x_drawable_geometry_get_prefetch(window);
		ecore_x_drawable_geometry_get_fetch();
		ecore_x_window_size_get(window, &w, &h);

		fcb_win = ecore_evas_software_x11_new(0, 0, 0, 0, w, h);

		ecore_evas_title_set(fcb_win, "FCB");
		ecore_evas_name_class_set(fcb_win, "FCB", "FCB");

		Evas* main_canvas = ecore_evas_get(fcb_win);

		ecore_evas_callback_delete_request_set(fcb_win, fcb_win_close_handler);

        extern xcb_window_t window;
        ecore_x_icccm_transient_for_set(
                ecore_evas_software_x11_window_get(fcb_win),
                window);

        ecore_x_randr_events_select(ecore_evas_software_x11_window_get(fcb_win), 1);
        sc_handler = ecore_event_handler_add(ECORE_X_EVENT_SCREEN_CHANGE, fcb_screen_change_handler, fcb_win);
		main_canvas_edje = eoi_main_window_create(main_canvas);
		fprintf(stderr, "main_canvas_edje: %p\n", main_canvas_edje);
		evas_object_name_set(main_canvas_edje, "fcb-window");
		eoi_fullwindow_object_register(ecore_evas_ecore_evas_get(main_canvas),
				main_canvas_edje);

		edje_object_part_text_set(main_canvas_edje, "title", list->name.c_str());

		evas_object_move(main_canvas_edje, 0, 0);
		evas_output_size_get(main_canvas, &w, &h);
		evas_object_resize(main_canvas_edje, w, h);
		evas_object_show(main_canvas_edje);

        choicebox_info_t info = {
            NULL,
			"choicebox",
            "full",
            "fbreader",
            "item",
            fcb_handler,
            fcb_draw_handler,
            fcb_page_updated_handler,
            fcb_close_handler,
        };

		Evas_Object* choicebox = choicebox_new(main_canvas, &info, list);
		choicebox_set_size(choicebox, list->items.size());
		evas_object_name_set(choicebox, "cb_full");

		edje_object_part_swallow(main_canvas_edje, "contents", choicebox);
		evas_object_show(choicebox);
		
        eoi_register_fullscreen_choicebox(choicebox);

		evas_object_focus_set(choicebox, true);
		evas_object_event_callback_add(choicebox,
				EVAS_CALLBACK_KEY_UP,
				&fcb_win_key_up_handler,
				(void*)list);

		if(select_item >= 0) {
			choicebox_scroll_to(choicebox, select_item);
			choicebox_set_selection(choicebox, select_item);
		}
	}

	ecore_evas_show(fcb_win);

	reuse_fcb_win = false;

	bookmark_delete_mode = false;

	ecore_main_loop_begin();
	if(emergency_exit)
		return;

	ecore_event_handler_del(sc_handler);
	if(fcb_win) {
		evas_object_hide(main_canvas_edje);
		if(!reuse_fcb_win) {
			ecore_evas_hide(fcb_win);
			ecore_evas_free(fcb_win);
			fcb_win = NULL;
		}
	}
}
