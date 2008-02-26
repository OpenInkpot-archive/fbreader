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
	printf("setFont: %s, %d, %d, %d\n", family.c_str(), size, bold?1:0, italic?1:0);
}

void ZLNXPaintContext::setColor(ZLColor color, LineStyle style) {
}

void ZLNXPaintContext::setFillColor(ZLColor color, FillStyle style) {
}

int ZLNXPaintContext::stringWidth(const char *str, int len) const {
	int w = 0;
	int ch_w;
	char *p = (char *)str;
	unsigned long         codepoint;
	unsigned char         in_code;
	int                   expect;
	FT_UInt glyph_idx = 0;


	while ( *p && len-- > 0)
	{
		in_code = *p++ ;
		if(in_code == 'ü')
			printf("len %d, in_code %c, in_code_x  %x, expect %d, codepoint %u\n", len, in_code, in_code, expect, codepoint);

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

		if(charWidthCache.find(codepoint) != charWidthCache.end()) {
			w += charWidthCache[codepoint];
			continue;
		}

		glyph_idx = FT_Get_Char_Index(face, codepoint);	
//		printf("symbol found: codepoint %u, glyph_idx %d\n", codepoint, glyph_idx);

		if(!FT_Load_Glyph(face, glyph_idx,  FT_LOAD_DEFAULT)) {
			ch_w = ROUND_26_6_TO_INT(face->glyph->advance.x); // or face->glyph->metrics->horiAdvance << 6
			w += ch_w;
			charWidthCache.insert(std::make_pair(codepoint, ch_w));
		} else
			printf("huj glyph %d\n", glyph_idx);
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
		myStringHeight = 24;
	}
	return myStringHeight;
}

int ZLNXPaintContext::descent() const {
	return myDescent;
}

void ZLNXPaintContext::drawString(int x, int y, const char *str, int len) {
	FT_GlyphSlot  slot = face->glyph;
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

	  drawGlyph( &slot->bitmap,
			  pen.x + slot->bitmap_left,
			  pen.y - slot->bitmap_top);

	  /* increment pen position */
	  pen.x += slot->advance.x >> 6;

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

/* rotation - 90	  
		c = buf + i * myHeight / 4 + (myHeight - j - 1) / 4;
		s = (3  - j & 3) * 2;
*/		
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
