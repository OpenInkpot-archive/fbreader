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

#ifndef __ZLNXPAINTCONTEXT_H__
#define __ZLNXPAINTCONTEXT_H__

#include <nano-X.h>
#include <nxcolors.h>
extern GR_WINDOW_ID win;
extern GR_GC_ID gc;
extern GR_FONT_ID fontid;

#include <ZLPaintContext.h>

class ZLNXPaintContext : public ZLPaintContext {

public:
	ZLNXPaintContext();
	~ZLNXPaintContext();

	int width() const;
	int height() const;

	void clear(ZLColor color);

	void fillFamiliesList(std::vector<std::string> &families) const;
	const std::string realFontFamilyName(std::string &fontFamily) const;

	void setFont(const std::string &family, int size, bool bold, bool italic);
	void setColor(ZLColor color, LineStyle style = SOLID_LINE);
	void setFillColor(ZLColor color, FillStyle style = SOLID_FILL);

	int stringWidth(const char *str, int len) const;
	int spaceWidth() const;
	int stringHeight() const;
	int descent() const;
	void drawString(int x, int y, const char *str, int len);

	void drawImage(int x, int y, const ZLImageData &image);

	void drawLine(int x0, int y0, int x1, int y1);
	void fillRectangle(int x0, int y0, int x1, int y1);
	void drawFilledCircle(int x, int y, int r);

private:
	int myWidth, myHeight;

//	GR_FONT_INFO myFontDescription;

	std::vector<std::string> myFontFamilies;

	mutable int myStringHeight;
	mutable int mySpaceWidth;
	int myDescent;
};

#endif /* __ZLNXPAINTCONTEXT_H__ */
