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

#include <ZLPaintContext.h>

#include <algorithm>
#include <iostream>
#include <vector>
#include <map>

#undef PANGO_DISABLE_DEPRECATED
#include <pango/pango.h>
#include <pango/pangoft2.h>

class ZLNXPaintContext : public ZLPaintContext {

public:
	ZLNXPaintContext();
	~ZLNXPaintContext();

	int width() const;
	int height() const;

	int *image;

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
	void drawLine(int x0, int y0, int x1, int y1, bool fill);
	void fillRectangle(int x0, int y0, int x1, int y1);
	void drawFilledCircle(int x, int y, int r);

private:
	int myWidth, myHeight;

	PangoContext *myContext;

	PangoFontDescription *myFontDescription;

	mutable PangoAnalysis myAnalysis;
	PangoGlyphString *myString;

	std::vector<std::string> myFontFamilies;

	mutable int myStringHeight;
	mutable int mySpaceWidth;
	int myDescent;
	FT_Bitmap *ft2bmp;

	ZLColor fColor;

	//FT_Bitmap *ft2bmp;
	FT_Bitmap *createFTBitmap(int width, int height);
	void freeFTBitmap(FT_Bitmap *bitmap);
	void modifyFTBitmap(FT_Bitmap *bitmap, int width, int height);
	void setFTBitmap(FT_Bitmap *bitmap, int width, int height);
	void clearFTBitmap(FT_Bitmap *bitmap);

	//void drawGlyph( FT_Bitmap*  bitmap, FT_Int x, FT_Int y);
};

#endif /* __ZLNXPAINTCONTEXT_H__ */
