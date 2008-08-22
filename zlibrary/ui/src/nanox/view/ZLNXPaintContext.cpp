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

#include <iostream>
#include <vector>
#include <map>
#include <dirent.h>

#include <ZLUnicodeUtil.h>
#include <ZLImage.h>
#include "../image/ZLNXImageManager.h"

#include "ZLNXPaintContext.h"


using namespace std;

struct xxx_link {
	int x1, y1, x2, y2;
	std::string id;
	bool next;
};
extern std::vector<xxx_link> xxx_page_links;

static void
substitute_func (FcPattern *pattern,
                 gpointer   data)
{
      FcPatternDel (pattern, FC_HINTING);
      FcPatternAddBool (pattern, FC_HINTING, true);
  
      FcPatternDel (pattern, FC_AUTOHINT);
      FcPatternAddBool (pattern, FC_AUTOHINT, true);
}



ZLNXPaintContext::ZLNXPaintContext() {

	image = NULL;

	myWidth = 600;
	myHeight = 800;

	myContext = 0;

	myFontDescription = 0;
	myAnalysis.lang_engine = 0;
	myAnalysis.level = 0;
	myAnalysis.language = 0;
	myAnalysis.extra_attrs = 0;
	myString = pango_glyph_string_new();

	myStringHeight = -1;
	mySpaceWidth = -1;
	myDescent = 0;

	if (myContext == 0) {
		PangoFontMap *font_map;
		font_map = pango_ft2_font_map_new();
		pango_ft2_font_map_set_resolution (PANGO_FT2_FONT_MAP (font_map), 170, 170);
		myContext = pango_ft2_font_map_create_context (PANGO_FT2_FONT_MAP(font_map));
//		pango_ft2_font_map_set_default_substitute (PANGO_FT2_FONT_MAP (font_map), substitute_func, NULL, NULL);

		if (myFontDescription != 0) {
			myAnalysis.font = pango_context_load_font(myContext, myFontDescription);
			myAnalysis.shape_engine = pango_font_find_shaper(myAnalysis.font, 0, 0);
			PangoFontMetrics *metrics = pango_font_get_metrics(myAnalysis.font, myAnalysis.language);
			myDescent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;
		}
	}
}

ZLNXPaintContext::~ZLNXPaintContext() {
	pango_glyph_string_free(myString);
	
	if (myFontDescription != 0) {
		pango_font_description_free(myFontDescription);
	}

	if (myContext != 0) {
		g_object_unref(myContext);
	}
}


void ZLNXPaintContext::fillFamiliesList(std::vector<std::string> &families) const {
	if (myContext != 0) {
		PangoFontFamily **pangoFamilies;
		int nFamilies;
		pango_context_list_families(myContext, &pangoFamilies, &nFamilies);
		for (int i = 0; i < nFamilies; ++i) {
			cout << "font: " << pango_font_family_get_name(pangoFamilies[i]) << endl;
			families.push_back(pango_font_family_get_name(pangoFamilies[i]));
		}
		std::sort(families.begin(), families.end());
		g_free(pangoFamilies);
	}
}

const std::string ZLNXPaintContext::realFontFamilyName(std::string &fontFamily) const {
	if (myContext == 0) {
		return fontFamily;
	}
	PangoFontDescription *description = pango_font_description_new();
	pango_font_description_set_family(description, fontFamily.c_str());
	pango_font_description_set_size(description, 12);
	PangoFont *font = pango_context_load_font(myContext, description);
	pango_font_description_free(description);
	description = pango_font_describe(font);
	std::string realFamily = pango_font_description_get_family(description);
	pango_font_description_free(description);
	return realFamily;
}

void ZLNXPaintContext::setFont(const std::string &family, int size, bool bold, bool italic) {
	cout << "setFont: " << family << endl;
	bool fontChanged = false;

	if (myFontDescription == 0) {
		myFontDescription = pango_font_description_new();
		fontChanged = true;
	}

	const char *oldFamily = pango_font_description_get_family(myFontDescription);
	if ((oldFamily == 0) || (family != oldFamily)) {
		pango_font_description_set_family(myFontDescription, family.c_str());
		fontChanged = true;
	}

	int newSize = size * PANGO_SCALE;
	if (pango_font_description_get_size(myFontDescription) != newSize) {
		pango_font_description_set_size(myFontDescription, newSize);
		fontChanged = true;
	}

	PangoWeight newWeight = bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL;
	if (pango_font_description_get_weight(myFontDescription) != newWeight) {
		pango_font_description_set_weight(myFontDescription, newWeight);
		fontChanged = true;
	}

	PangoStyle newStyle = italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL;
	if (pango_font_description_get_style(myFontDescription) != newStyle) {
		pango_font_description_set_style(myFontDescription, newStyle);
		fontChanged = true;
	}

	if (fontChanged) {
		if (myContext != 0) {
			myAnalysis.font = pango_context_load_font(myContext, myFontDescription);
			myAnalysis.shape_engine = pango_font_find_shaper(myAnalysis.font, 0, 0);
			PangoFontMetrics *metrics = pango_font_get_metrics(myAnalysis.font, myAnalysis.language);
			myDescent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;
		}
		myStringHeight = -1;
		mySpaceWidth = -1;
	}
}

void ZLNXPaintContext::setColor(ZLColor color, LineStyle style) {
	//printf("setColor\n");
}

void ZLNXPaintContext::setFillColor(ZLColor color, FillStyle style) {
	//printf("setFillColor\n");
	fColor = color;
//	fColor = (0.299 * color.Red + 0.587 * color.Green + 0.114 * color.Blue ) / 64;
//	fColor = pal[fColor & 3];
}

int ZLNXPaintContext::stringWidth(const char *str, int len) const {
	if (myContext == 0) {
		return 0;
	}

	if (!g_utf8_validate(str, len, 0)) {
		return 0;
	}

	pango_shape(str, len, &myAnalysis, myString);
	PangoRectangle logicalRectangle;
	pango_glyph_string_extents(myString, myAnalysis.font, 0, &logicalRectangle);
//	cout << "stringWidth: " << (logicalRectangle.width + PANGO_SCALE / 2) / PANGO_SCALE << endl;
	return (logicalRectangle.width + PANGO_SCALE / 2) / PANGO_SCALE;
}

int ZLNXPaintContext::spaceWidth() const {
	if (mySpaceWidth == -1) {
		mySpaceWidth = stringWidth(" ", 1);
	}
	return mySpaceWidth;
}

int ZLNXPaintContext::stringHeight() const {
	if (myFontDescription == 0) {
		return 0;
	}
	if (myStringHeight == -1) {
		if (pango_font_description_get_size_is_absolute(myFontDescription)) {
			myStringHeight = pango_font_description_get_size(myFontDescription) / PANGO_SCALE + 2;
		} else {
			myStringHeight = pango_font_description_get_size(myFontDescription) * 170 / 72 / PANGO_SCALE + 2;
		}
	}
	return myStringHeight;
}

int ZLNXPaintContext::descent() const {
	return myDescent;
}

void ZLNXPaintContext::drawString(int x, int y, const char *str, int len) {
	if (!g_utf8_validate(str, len, 0)) {
		return;
	}

	pango_shape(str, len, &myAnalysis, myString);

	PangoRectangle logicalRectangle;
	pango_glyph_string_extents(myString, myAnalysis.font, 0, &logicalRectangle);

/*	cout << "metrics:" <<
	(logicalRectangle.width + PANGO_SCALE / 2) / PANGO_SCALE << "x" <<
	(logicalRectangle.height + PANGO_SCALE / 2) / PANGO_SCALE << endl;
*/	

	FT_Bitmap *ft2bmp = createFTBitmap(
	(logicalRectangle.width + PANGO_SCALE / 2) / PANGO_SCALE,
	(logicalRectangle.height + PANGO_SCALE / 2) / PANGO_SCALE);

	pango_ft2_render(ft2bmp, myAnalysis.font, myString, 0, ft2bmp->rows - myDescent);

	unsigned char val;
	unsigned char *p_ft = (unsigned char *)ft2bmp->buffer;;
	for(int i = ft2bmp->rows - 1; i >= 0; i--) {
			for (int k = 0; 
					k < ft2bmp->width;
					k++) {

				int level;
				if (p_ft[k]==0) {
					continue;
				}
				val = 0x55 * ((unsigned char)~p_ft[k] / 64);
				image[x+k + (y-i) * myWidth] = (255 << 24) | (val << 16) | (val << 8) | val;		
			}
		p_ft += ft2bmp->pitch;
	}
	freeFTBitmap(ft2bmp);
//	clearFTBitmap(ft2bmp);
//TODO
}

void ZLNXPaintContext::clearFTBitmap(FT_Bitmap *bitmap)
{
	unsigned char *p = (unsigned char *)bitmap->buffer;
	int length = bitmap->pitch * bitmap->rows;
	memset(p, 0, length);
}


void ZLNXPaintContext::drawImage(int x, int y, const ZLImageData &image) {
	//	printf("drawImage: %d %d %d %d\n", image.width(), image.height(), x, y);
	char *c;
	char *c_src;
	int s, s_src;
	int iW = image.width();
	int iH = image.height();
	unsigned char val;

	if((iW + x) > myWidth || y > myHeight)
		return;

	ZLNXImageData *source_image = (ZLNXImageData *)&image;

	char *src = source_image->getImageData();

	for(int j = 0; j < iH; j++)
		for(int i = 0; i < iW; i++) {
			c_src = src + i / 4 + iW * j / 4;
			s_src = (i & 3) << 1;

			val = (*c_src << s_src) & 0xc0;

			if(val == 0xc0)
				continue;

			if(val == 0x00)
				this->image[i + x + (j + y - iH) * myWidth] = 0xff << 24;
			else if(val == 0x40)
				this->image[i + x + (j + y - iH) * myWidth] = 0xff555555;
			else
				this->image[i + x + (j + y - iH) * myWidth] = 0xffaaaaaa;
		}
}

void ZLNXPaintContext::drawLine(int x0, int y0, int x1, int y1) {
	drawLine(x0, y0, x1, y1, false);
}

void ZLNXPaintContext::drawLine(int x0, int y0, int x1, int y1, bool fill) {
	//printf("drawLine: %d %d %d %d\n", x0, y0, x1, y1);

	int i, j;
	int k, s;
	int p;
	bool done = false;

	if(x1 != x0) {
		k = (y1 - y0) / (x1 - x0);
		j = y0;
		i = x0;

		do {
			if(i == x1)
				done = true;

			if(fill)
				image[i + j * myWidth] = (255 << 24) | (fColor.Red << 16) | (fColor.Green << 8) | fColor.Blue;		
			else
				image[i + j * myWidth] = 0xff000000;

			j += k;

			if(x1 > x0)
				i++;
			else 
				i--;

		} while(!done);

	} else {
		i = x0;
		j = y0;

		do {
			if(j == y1)
				done = true;

			if(fill)
				image[i + j * myWidth] = (255 << 24) | (fColor.Red << 16) | (fColor.Green << 8) | fColor.Blue;		
			else
				image[i + j * myWidth] = 0xff000000;

			if(y1 > y0)
				j++;
			else if(y1 < y0)
				j--;

		} while(!done);
	}
}

void ZLNXPaintContext::fillRectangle(int x0, int y0, int x1, int y1) {
	//printf("fillRectangle\n");
	int j;

	j = y0;
	do {
		drawLine(x0, j, x1, j, true);

		if(y1 > y0)
			j++;
		else if(y1 < y0)
			j--;
	} while(( y1 > y0) && ( j <= y1 )  ||
			(j <= y0));
}

void ZLNXPaintContext::drawFilledCircle(int x, int y, int r) {
	//printf("drawFilledCircle\n");
}

void ZLNXPaintContext::clear(ZLColor color) {
	memset(image, 0xff, myWidth * myHeight * sizeof(int));


	xxx_page_links.clear();
}

int ZLNXPaintContext::width() const {
	return myWidth;
}

int ZLNXPaintContext::height() const {
	return myHeight;
}

/*void ZLNXPaintContext::drawGlyph( FT_Bitmap*  bitmap, FT_Int x, FT_Int y)
{
	FT_Int  i, j, p, q;
	FT_Int  x_max = x + bitmap->width;
	FT_Int  y_max = y + bitmap->rows;
	unsigned char val;
	int s;

	for ( i = x, p = 0; i < x_max; i++, p++ ) {
		for ( j = y, q = 0; j < y_max; j++, q++ ) {

			if ( i < 0      || j < 0       ||
					i >= myWidth || j >= myHeight )
				continue;

			val = bitmap->buffer[q * bitmap->width + p];

			val = ~val;

			image[i + (j * myWidth)] = (255 << 24) | (val << 16) | (val << 8) | val;

//			if(val < 64)
//				continue;
//
//			if(val >= 192)
//				xcb_image_put_pixel (image, i, j, pal[0]);
//			else if(val >= 128)
//				xcb_image_put_pixel (image, i, j, pal[1]);
//			else
//				xcb_image_put_pixel (image, i, j, pal[2]);
//		
		}
		
	}
}
*/

void ZLNXPaintContext::setFTBitmap(FT_Bitmap *bitmap, int width, int height)
{
	bitmap->width = width;
	bitmap->rows = height;
	bitmap->pitch = (width + 3) & ~3;
	bitmap->num_grays = 256;
	bitmap->pixel_mode = FT_PIXEL_MODE_GRAY;
}

FT_Bitmap * ZLNXPaintContext::createFTBitmap(int width, int height)
{
	FT_Bitmap *bitmap;
	bitmap = (FT_Bitmap*)g_malloc(sizeof(FT_Bitmap));
	setFTBitmap(bitmap, width, height);
	bitmap->buffer = g_new0(guchar, bitmap->pitch * bitmap->rows);
	return bitmap;
}

void ZLNXPaintContext::freeFTBitmap(FT_Bitmap *bitmap)
{
	if (bitmap) {
		g_free(bitmap->buffer);
		g_free(bitmap);
		bitmap = NULL;
	}
}

void ZLNXPaintContext::modifyFTBitmap(FT_Bitmap *bitmap, int width, int height)
{
	if (bitmap->width != width || bitmap->rows != height) {
		setFTBitmap(bitmap, width, height);
		bitmap->buffer = (unsigned char *)g_realloc(bitmap->buffer, bitmap->pitch * bitmap->rows);
	}
	clearFTBitmap(bitmap);
}
