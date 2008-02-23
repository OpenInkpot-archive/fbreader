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

extern char *buf;
#define ROUND_26_6_TO_INT(valuetoround) (((valuetoround) + 63) >> 6)

ZLNXPaintContext::ZLNXPaintContext() {
	myWidth = 600;
	myHeight = 800;

	myStringHeight = -1;
	mySpaceWidth = -1;
	myDescent = 0;

	char *fontname ="/root/fonts/truetype/arial.ttf";
	FT_Error error;

	error = FT_Init_FreeType( &library );
	error = FT_New_Face( library, fontname, 0, &face );
	error = FT_Set_Char_Size( face, 12 * 64, 0, 160, 0 ); 
}

ZLNXPaintContext::~ZLNXPaintContext() {
	if(face)
		FT_Done_Face(face);
	
	if(library)
		FT_Done_FreeType(library);
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
	int w = 0;
	char *p = (char *)str;
	unsigned long         codepoint;
	unsigned char         in_code;
	int                   expect;
	FT_UInt glyph_idx = 0;

	while ( *p && len-- > 0)
	{
		in_code = *p++ ;

		if ( in_code >= 0xC0 )
		{
			if ( in_code < 0xE0 )           /*  U+0080 - U+07FF   */
			{
				expect = 1;
				codepoint = in_code & 0x1F;
			}
			else if ( in_code < 0xF0 )      /*  U+0800 - U+FFFF   */
			{
				expect = 2;
				codepoint = in_code & 0x0F;
			}
			else if ( in_code < 0xF8 )      /* U+10000 - U+10FFFF */
			{
				expect = 3;
				codepoint = in_code & 0x07;
			}
			continue;
		}
		else if ( in_code >= 0x80 )
		{
			--expect;

			if ( expect >= 0 )
			{
				codepoint <<= 6;
				codepoint  += in_code & 0x3F;
			}
			if ( expect >  0 )
				continue;

			expect = 0;
		}
		else                              /* ASCII, U+0000 - U+007F */
			codepoint = in_code;

//		len--;

		glyph_idx = FT_Get_Char_Index(face, codepoint);

		if(!FT_Load_Glyph(face, glyph_idx,  FT_LOAD_DEFAULT)) {
			w += ROUND_26_6_TO_INT(face->glyph->advance.x);
		}
	}


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
		//FIXME
		myStringHeight = 20;
	}
	return myStringHeight;
}

int ZLNXPaintContext::descent() const {
	return myDescent;
}

void ZLNXPaintContext::drawString(int x, int y, const char *str, int len) {
	FT_GlyphSlot  slot;
	FT_BitmapGlyph glyph;
	FT_Matrix     matrix;                 /* transformation matrix */
	FT_Vector     pen;                    /* untransformed origin  */

	FT_UInt glyph_idx = 0;

    char *p = (char *)str;
    unsigned long         codepoint;
    unsigned char         in_code;
    int                   expect;


	pen.x = x;
	pen.y = y;


    while ( *p && len--)
    {
      in_code = *p++ ;

      if ( in_code >= 0xC0 )
      {
        if ( in_code < 0xE0 )           /*  U+0080 - U+07FF   */
        {
          expect = 1;
          codepoint = in_code & 0x1F;
        }
        else if ( in_code < 0xF0 )      /*  U+0800 - U+FFFF   */
        {
          expect = 2;
          codepoint = in_code & 0x0F;
        }
        else if ( in_code < 0xF8 )      /* U+10000 - U+10FFFF */
        {
          expect = 3;
          codepoint = in_code & 0x07;
        }
        continue;
      }
      else if ( in_code >= 0x80 )
      {
        --expect;

        if ( expect >= 0 )
        {
          codepoint <<= 6;
          codepoint  += in_code & 0x3F;
        }
        if ( expect >  0 )
          continue;

        expect = 0;
      }
      else                              /* ASCII, U+0000 - U+007F */
        codepoint = in_code;

	  glyph_idx = FT_Get_Char_Index(face, codepoint);

	  if(FT_Load_Glyph(face, glyph_idx,  FT_LOAD_RENDER |  FT_LOAD_TARGET_NORMAL)){
		  continue;
	  }

	  drawGlyph( &face->glyph->bitmap,
			  pen.x + face->glyph->bitmap_left,
			  pen.y - face->glyph->bitmap_top);

	  /* increment pen position */
	  pen.x += face->glyph->advance.x >> 6;
	  pen.y += face->glyph->advance.y >> 6;

	}
	
}

void ZLNXPaintContext::drawImage(int x, int y, const ZLImageData &image) {
}

void ZLNXPaintContext::drawLine(int x0, int y0, int x1, int y1) {
}

void ZLNXPaintContext::fillRectangle(int x0, int y0, int x1, int y1) {
}

void ZLNXPaintContext::drawFilledCircle(int x, int y, int r) {
}

void ZLNXPaintContext::clear(ZLColor color) {
	memset(buf, 0xff, 800*600/4);
}

int ZLNXPaintContext::width() const {
	return myWidth;
}

int ZLNXPaintContext::height() const {
	return myHeight;
}

void ZLNXPaintContext::drawGlyph( FT_Bitmap*  bitmap, FT_Int x, FT_Int y)
{
  FT_Int  i, j, p, q;
  FT_Int  x_max = x + bitmap->width;
  FT_Int  y_max = y + bitmap->rows;
  char *c = buf;
  unsigned char val;
  int s;

  for ( i = x, p = 0; i < x_max; i++, p++ ) {
    for ( j = y, q = 0; j < y_max; j++, q++ ) {

      if ( i < 0      || j < 0       ||
           i >= myWidth || j >= myHeight )
        continue;

		c = buf + i / 4 + myWidth * j / 4;

		s =  (i & 3) << 1;
		val = bitmap->buffer[q * bitmap->width + p];
		*c &= ~(0xc0 >> s);
		if(val < 0x2a)
			*c |= (0xc0 >> s);
		else if(val < 0x7f) 
			*c |= (0x80 >> s);
		else if(val < 0xd4)
			*c |= (0x40 >> s);
		else if(val <= 0xff)
			;
	}
  }
}
