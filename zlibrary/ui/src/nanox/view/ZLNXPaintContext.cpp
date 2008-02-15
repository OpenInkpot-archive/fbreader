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

#include <algorithm>

#include <ZLUnicodeUtil.h>
#include <ZLImage.h>

#include "ZLNXPaintContext.h"

ZLNXPaintContext::ZLNXPaintContext() {
	myWidth = 600;
	myHeight = 800;

	myStringHeight = -1;
	mySpaceWidth = -1;
	myDescent = 0;
}

ZLNXPaintContext::~ZLNXPaintContext() {
}


void ZLNXPaintContext::fillFamiliesList(std::vector<std::string> &families) const {
}

const std::string ZLNXPaintContext::realFontFamilyName(std::string &fontFamily) const {
}

void ZLNXPaintContext::setFont(const std::string &family, int size, bool bold, bool italic) {
}

void ZLNXPaintContext::setColor(ZLColor color, LineStyle style) {
}

void ZLNXPaintContext::setFillColor(ZLColor color, FillStyle style) {
}

int ZLNXPaintContext::stringWidth(const char *str, int len) const {
	GR_SIZE w, h, b;

	GrGetGCTextSize(gc, (void *)str, len, GR_TFUTF8, &w, &h, &b); 

	printf("w: %d\n", w);

	return w;
}

int ZLNXPaintContext::spaceWidth() const {
	if (mySpaceWidth == -1) {
		mySpaceWidth = stringWidth(" ", 1);
	}
	return mySpaceWidth;
}

int ZLNXPaintContext::stringHeight() const {
	if (myStringHeight == -1) {
		GR_FONT_INFO myFontDescription;
		GrGetFontInfo(fontid, &myFontDescription);
		myStringHeight = myFontDescription.height;
	}
	return myStringHeight;
}

int ZLNXPaintContext::descent() const {
	return myDescent;
}

void ZLNXPaintContext::drawString(int x, int y, const char *str, int len) {
	printf("x %d, y %d: %s\n", str);
 	GrText_Apollo(win, gc, x, y, (void *)str, len, GR_TFUTF8);
}

void ZLNXPaintContext::drawImage(int x, int y, const ZLImageData &image) {
}

void ZLNXPaintContext::drawLine(int x0, int y0, int x1, int y1) {
	GrLine_Apollo(win, gc, x0, y0, x1, y1);
}

void ZLNXPaintContext::fillRectangle(int x0, int y0, int x1, int y1) {
	GrRect_Apollo(win, gc, x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}

void ZLNXPaintContext::drawFilledCircle(int x, int y, int r) {
}

void ZLNXPaintContext::clear(ZLColor color) {
	GrClearShareMem_Apollo(win, gc, 0xff);
}

int ZLNXPaintContext::width() const {
	return myWidth;
}

int ZLNXPaintContext::height() const {
	return myHeight;
}

