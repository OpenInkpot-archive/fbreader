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
#include <ZLApplication.h>
#include "../../../../../fbreader/src/fbreader/FBReader.h"
#include "../../../../../fbreader/src/fbreader/BookTextView.h"
#include "../../../../../fbreader/src/fbreader/FBReaderActions.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include "Viewer.h"
#include <sys/types.h>

#include <map>

#include <sys/time.h>

//#define _DEBUG 1

int glb_page;
bool init;

//std::map<int, std::string> xxx_myTOC;

struct xxx_toc_entry {
	int paragraph;
	std::string text;
};

std::vector<xxx_toc_entry> xxx_myTOC;

bool toc_jump;

enum state_t {
	ST_NORMAL = 0,
	ST_LINK_NAV = 1
};

state_t cur_state;

std::vector<std::string> xxx_notes;

struct xxx_link {
	int x1, y1, x2, y2;
	std::string id;
	bool next;
};

std::vector<struct xxx_link> xxx_page_links;

int cur_link_idx;

#define ALLOW_RUN_EXE 1
