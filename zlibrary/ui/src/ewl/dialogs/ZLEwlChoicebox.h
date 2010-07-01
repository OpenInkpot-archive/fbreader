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

#ifndef ZLEWLCHOICEBOX_NEW_H
#define ZLEWLCHOICEBOX_NEW_H

using namespace std;

enum cb_olist_item_type {
	ITEM_TEXT,
	ITEM_SUBMENU,
	ITEM_OPTION
};

enum cb_value_type {
	VAL_INT,
	VAL_BOOL,
	VAL_STRING,
	VAL_NONE
};

typedef struct _cb_olist_item cb_olist_item;
typedef struct _cb_olist cb_olist;
typedef struct _cb_item_value cb_item_value;

typedef struct _cb_list cb_list;
typedef struct _cb_list_item cb_list_item;

struct _cb_list_item {
	string text;
	string title;
	string value;
	string icon;
};

struct _cb_list {
	string name;
	string alt_text;
	vector<cb_list_item> items;

	int(*item_handler)(int, bool);
};

struct _cb_item_value {
	cb_value_type type;
	int ival;
	bool ibool;
	string sval;
	string text;
};

struct _cb_olist_item {
	string name;
	cb_olist_item_type type;

	cb_item_value current_value;
	vector<cb_item_value> values;
	int curval_idx;

	void (*item_handler)(int, bool);
	void *data;
};

struct _cb_olist {
	string name;
	vector<cb_olist_item> items;

	cb_olist *parent;
	int parent_item_idx;

	bool fsize_list;

	void(*item_handler)(int, bool);
	void(*destroy_handler)();
};

typedef struct _cb_vlist cb_vlist;

struct _cb_vlist {
	string name;
	bool font_list;
	bool fsize_list;
	vector<cb_item_value> values;
	cb_olist *parent;
	int parent_item_idx;
	void(*item_handler)(int, bool);
	void(*destroy_handler)();
};


void cb_fcb_new(cb_list *list, int select_item = -1);
void cb_fcb_redraw(int newsize = -1);
void cb_fcb_invalidate(int idx);
void cb_fcb_invalidate_interval(int start, int end);

void cb_rcb_new(int select_item = -1);

void cb_lcb_new(int select_item = -1);
void cb_lcb_redraw();
void cb_lcb_invalidate(int idx);

extern vector<cb_olist *> olists;
extern cb_vlist *vlist;

#define ADD_OPTION_STRING(__name, __value) \
		ADD_OPTION_STRING_DATA(__name, __value, NULL)

#define ADD_OPTION_STRING_DATA(__name, __value, __data) \
		i.name = (__name);	\
		i.type = ITEM_OPTION;	\
		i.values.clear();	\
		i.curval_idx = -1;	\
		i.current_value.text = i.current_value.sval = (__value);	\
		i.current_value.type = VAL_STRING;	\
		i.item_handler = NULL; \
		i.data = (__data); \
		options->items.push_back(i);

#define ADD_OPTION_INT_T(__name, __value, __valuetext) \
		i.name = (__name);	\
		i.type = ITEM_OPTION;	\
		i.values.clear();	\
		i.curval_idx = -1;	\
		i.current_value.text = (__valuetext); \
		i.current_value.ival = (__value);	\
		i.current_value.type = VAL_INT;	\
		options->items.push_back(i);

#define ADD_OPTION_INT(__name, __value) \
		i.name = (__name);	\
		i.type = ITEM_OPTION;	\
		i.values.clear();	\
		i.curval_idx = -1;	\
		{	\
			char *t;	\
			asprintf(&t, "%ld", __value);	\
			i.current_value.text = t; \
			free(t);	\
		}	\
		i.current_value.ival = (__value);	\
		i.current_value.type = VAL_INT;	\
		i.item_handler = NULL; \
		i.data = NULL; \
		options->items.push_back(i);


#define ADD_OPTION_INT_F(__name, __value, __format) \
		i.name = (__name);	\
		i.type = ITEM_OPTION;	\
		i.values.clear();	\
		i.curval_idx = -1;	\
		{	\
			char *t;	\
			asprintf(&t, (__format), __value);	\
			i.current_value.text = t; \
			free(t);	\
		}	\
		i.current_value.ival = (__value);	\
		i.current_value.type = VAL_INT;	\
		i.item_handler = NULL; \
		i.data = NULL; \
		options->items.push_back(i);

#define ADD_OPTION_BOOL(__name, __value) \
		i.name = (__name); 	\
		i.type = ITEM_OPTION;	\
		i.item_handler = NULL; \
		i.data = NULL; \
		{	\
			i.values.clear();	\
			cb_item_value iv;	\
			iv.type = VAL_BOOL;	\
			iv.text = gettext("On");	\
			iv.ibool = true;	\
			i.values.push_back(iv);	\
			iv.text = gettext("Off");	\
			iv.ibool = false;	\
			i.values.push_back(iv);	\
			if((__value))	\
				i.curval_idx = 0;	\
			else	\
				i.curval_idx = 1;	\
		}	\
		options->items.push_back(i);

#define ADD_OPTION_BOOL_H(__name, __value, __handler, __data) \
		i.name = (__name); 	\
		i.type = ITEM_OPTION;	\
		i.item_handler = (__handler); \
		i.data = (__data); \
		{	\
			i.values.clear();	\
			cb_item_value iv;	\
			iv.type = VAL_BOOL;	\
			iv.text = gettext("On");	\
			iv.ibool = true;	\
			i.values.push_back(iv);	\
			iv.text = gettext("Off");	\
			iv.ibool = false;	\
			i.values.push_back(iv);	\
			if((__value))	\
				i.curval_idx = 0;	\
			else	\
				i.curval_idx = 1;	\
		}	\
		options->items.push_back(i);

#define ADD_SUBMENU_ITEM(__name) \
		i.name = (__name);	\
		i.type = ITEM_SUBMENU;	\
		i.values.clear();	\
		i.curval_idx = -1;	\
		i.current_value.type = VAL_NONE;	\
		i.item_handler = NULL; \
		i.data = NULL; \
		options->items.push_back(i);

#define ADD_VALUE_STRING(__value)	\
	{	\
	cb_item_value iv;	\
	iv.type = VAL_STRING;	\
	iv.text = iv.sval = (__value);	\
	vlist->values.push_back(iv);	\
	}
		
#define ADD_VALUE_INT(__value)	\
	{	\
	cb_item_value iv;	\
	iv.type = VAL_INT;	\
	char *t;	\
	asprintf(&t, "%d", (__value));	\
	iv.text = t;	\
	free(t);	\
	iv.ival = (__value);	\
	vlist->values.push_back(iv);	\
	}

#define ADD_VALUE_INT_T(__value, __text)	\
	{	\
	cb_item_value iv;	\
	iv.type = VAL_INT;	\
	iv.text = (__text);	\
	iv.ival = (__value);	\
	vlist->values.push_back(iv);	\
	}

#define ADD_VALUE_INT_F(__value, __format)	\
	{	\
	cb_item_value iv;	\
	iv.type = VAL_INT;	\
	char *t;	\
	asprintf(&t, (__format), (__value));	\
	iv.text = t;	\
	free(t);	\
	iv.ival = (__value);	\
	vlist->values.push_back(iv);	\
	}
	

#define INIT_VLIST(__name, __handler) \
		if(vlist) {	\
			delete vlist;	\
			vlist = NULL;	\
		}	\
		vlist = new cb_vlist;	\
		vlist->name = (__name);	\
		vlist->font_list = false; \
		vlist->fsize_list = false; \
		vlist->item_handler = (__handler);	\
		vlist->destroy_handler = refresh_view; \
		vlist->parent = olists.back();	\
		vlist->parent_item_idx = idx;

#endif
