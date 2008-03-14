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

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_BITMAP_H

class ZLNXPaintContext : public ZLPaintContext {

public:
	ZLNXPaintContext();
	~ZLNXPaintContext();

	int width() const;
	int height() const;

	void clear(ZLColor color);

	void fillFamiliesList(std::vector<std::string> &families) const;
	void cacheFonts() const;
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

	class Font {
		public:
			Font() : 
				familyName(""),
				fileName(""),
				index(0),
				myFace(NULL),
				isBold(false),
				isItalic(false) {}

			~Font() { if(myFace) FT_Done_Face(myFace); }
	
			std::string familyName;
			std::string fileName;
			int index;

			FT_Face myFace;
			bool isBold;
			bool isItalic;

			std::map<int, std::map<unsigned long, int> > charWidthCacheAll;
			std::map<int, std::map<unsigned long, FT_BitmapGlyph> > glyphCacheAll;
	};
	

	mutable std::vector<std::string> fPath;
	mutable std::map<std::string, std::map<int, Font> > fontCache;
	mutable std::map<unsigned long, int> *charWidthCache;
	mutable std::map<unsigned long, FT_BitmapGlyph> *glyphCache;

	FT_Face	*face;
	std::string fCurFamily;
	int fCurSize;
	bool fCurItalic;
	bool fCurBold;
	int fColor;

	FT_Library library;

	std::vector<std::string> myFontFamilies;

	mutable int myStringHeight;
	mutable int mySpaceWidth;
	int myDescent;


	void drawGlyph( FT_Bitmap*  bitmap, FT_Int x, FT_Int y);
};

#endif /* __ZLNXPAINTCONTEXT_H__ */
