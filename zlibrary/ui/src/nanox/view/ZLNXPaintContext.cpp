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
#include "../image/ZLNXImageManager.h"

#include "ZLNXPaintContext.h"

extern char *buf;
#define ROUND_26_6_TO_INT(valuetoround) (((valuetoround) + 63) >> 6)

ZLNXPaintContext::ZLNXPaintContext() {
	myWidth = 600;
	myHeight = 800;

	myStringHeight = -1;
	mySpaceWidth = -1;
	myDescent = 0;

	fCurSize = 0;
	fItalic = false;
	fBold = false;

	FT_Error error;

	error = FT_Init_FreeType( &library );
}

ZLNXPaintContext::~ZLNXPaintContext() {
	if(faceNormal)
		FT_Done_Face(faceNormal);
	if(faceItalic)
		FT_Done_Face(faceItalic);
	if(faceBold)
		FT_Done_Face(faceBold);
	if(faceItalicBold)
		FT_Done_Face(faceItalicBold);

	std::map<int, std::map<unsigned long, FT_BitmapGlyph> >::iterator piter = glyphCacheAll.begin();
	while(piter != glyphCacheAll.end()) {
		std::map<unsigned long, FT_BitmapGlyph>::iterator piter2 = piter->second.begin();
		while(piter2 != piter->second.end()) {
			FT_Bitmap_Done(library, (FT_Bitmap*)&(piter2->second->bitmap));			
			piter2++;
		}
		piter++;
	}

	if(library)
		FT_Done_FreeType(library);
}


void ZLNXPaintContext::fillFamiliesList(std::vector<std::string> &families) const {
}

const std::string ZLNXPaintContext::realFontFamilyName(std::string &fontFamily) const {
}

void ZLNXPaintContext::setFont(const std::string &family, int size, bool bold, bool italic) {
	//printf("setFont: %s, %d, %d, %d\n", family.c_str(), size, bold?1:0, italic?1:0);

	static bool fontInit = false;

	if(!fontInit) {		
		fontInit = true;
		FT_Error error;

		std::string fname;
		fname = "/root/fonts/truetype/" + family + ".ttf";
		error = FT_New_Face( library, fname.c_str(), 0, &faceNormal );
		if(error) {
			error = FT_New_Face( library, "/root/fonts/truetype/arial.ttf", 0, &faceNormal );
		}
		fname = "/root/fonts/truetype/" + family + "i.ttf";
		error = FT_New_Face( library, fname.c_str(), 0, &faceItalic );
		if(error) {
			error = FT_New_Face( library, "/root/fonts/truetype/ariali.ttf", 0, &faceItalic );
		}
		fname = "/root/fonts/truetype/" + family + "bd.ttf";
		error = FT_New_Face( library, fname.c_str(), 0, &faceBold );
		if(error) {
			error = FT_New_Face( library, "/root/fonts/truetype/arialbd.ttf", 0, &faceBold);
		}
		fname = "/root/fonts/truetype/" + family + "bi.ttf";
		error = FT_New_Face( library, fname.c_str(), 0, &faceItalicBold );
		if(error) {
			error = FT_New_Face( library, "/root/fonts/truetype/arialbi.ttf", 0, &faceItalicBold );
		}
	}

	if(fItalic != italic)
		fItalic = italic;

	if(fBold != bold)
		fBold = bold;

	if(fItalic && fBold)
		face = &faceItalicBold;
	else if(fItalic)
		face = &faceItalic;
	else if(fBold)
		face = &faceBold;
	else
		face = &faceNormal;


	if(size >= 6)
		fCurSize = size;
	else
		fCurSize = 6; 

	FT_Set_Char_Size( *face, fCurSize * 64, 0, 160, 0 );


	int key = fCurSize * 10 + (fItalic ? 1 : 0) + (fBold ? 2 : 0);

	charWidthCache = &charWidthCacheAll[key];
	glyphCache = &glyphCacheAll[key];

	myStringHeight = fCurSize * 2;
	mySpaceWidth = -1;
}

void ZLNXPaintContext::setColor(ZLColor color, LineStyle style) {
	//printf("setColor\n");
}

void ZLNXPaintContext::setFillColor(ZLColor color, FillStyle style) {
	//printf("setFillColor\n");
	fColor = (0.299 * color.Red + 0.587 * color.Green + 0.114 * color.Blue ) / 64;
	fColor &= 3;
	fColor <<= 6;
}

int ZLNXPaintContext::stringWidth(const char *str, int len) const {
	int w = 0;
	int ch_w;
	char *p = (char *)str;
	unsigned long         codepoint;
	unsigned char         in_code;
	int                   expect;
	FT_UInt glyph_idx = 0;
	FT_UInt previous;
	FT_Bool use_kerning;
	FT_Vector delta; 
	int kerning = 0;

	use_kerning = (*face)->face_flags & FT_FACE_FLAG_KERNING;

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

		glyph_idx = FT_Get_Char_Index(*face, codepoint);	
		if ( use_kerning && previous && glyph_idx ) { 
			FT_Get_Kerning( *face, previous, glyph_idx, FT_KERNING_DEFAULT, &delta ); 
			kerning = delta.x >> 6;
		} else 
			kerning = 0;

		if(charWidthCache->find(codepoint) != charWidthCache->end()) {
			w += (*charWidthCache)[codepoint] + kerning;
		} else {
			if(!FT_Load_Glyph(*face, glyph_idx,  FT_LOAD_DEFAULT)) {
				ch_w = ROUND_26_6_TO_INT((*face)->glyph->advance.x); // or face->glyph->metrics->horiAdvance >> 6
				w += ch_w + kerning;
				charWidthCache->insert(std::make_pair(codepoint, ch_w));
			} 
		//	else
		//		printf("glyph %d not found\n", glyph_idx);
		}
		previous = glyph_idx;

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
//		myStringHeight = (*face)->size->metrics.height >> 6;
//		printf("myStringHeight: %d\n", myStringHeight);
	}
	return myStringHeight;
}

int ZLNXPaintContext::descent() const {
	return myDescent;
}

void ZLNXPaintContext::drawString(int x, int y, const char *str, int len) {
	FT_GlyphSlot  slot = (*face)->glyph;
	FT_BitmapGlyph glyph;
	FT_BitmapGlyph *pglyph;
	FT_Matrix     matrix;                 /* transformation matrix */
	FT_Vector     pen;                    /* untransformed origin  */

	FT_UInt glyph_idx = 0;
	FT_UInt previous;
	FT_Bool use_kerning;
	FT_Vector delta; 

	use_kerning = (*face)->face_flags & FT_FACE_FLAG_KERNING;

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

		glyph_idx = FT_Get_Char_Index(*face, codepoint);	
		if ( use_kerning && previous && glyph_idx ) { 
			FT_Get_Kerning( *face, previous, glyph_idx, FT_KERNING_DEFAULT, &delta ); 
			pen.x += delta.x >> 6;		
		}

		if(glyphCache->find(codepoint) != glyphCache->end()) { 
			pglyph = &(*glyphCache)[codepoint];
		} else {
			if(FT_Load_Glyph(*face, glyph_idx,  FT_LOAD_RENDER |  FT_LOAD_TARGET_NORMAL)){
				continue;
			}	

			FT_Get_Glyph(slot, (FT_Glyph*)&glyph);

			glyph->root.advance.x = slot->advance.x;	  			

			(*glyphCache)[codepoint] = glyph;		  
			pglyph = &glyph;
		}

		drawGlyph( &(*pglyph)->bitmap,
				pen.x + (*pglyph)->left,
				pen.y - (*pglyph)->top);


		/* increment pen position */
		pen.x += (*pglyph)->root.advance.x >> 6;
	}
	previous = glyph_idx;
	
}

void ZLNXPaintContext::drawImage(int x, int y, const ZLImageData &image) {
	//printf("drawImage: %d %d\n", image.width(), image.height());
	if((image.width() + x) > myWidth || (image.height() + y) > myHeight)
		return;

	char *c;
	char *c_src;
	int s, s_src;

	ZLNXImageData *source_image = (ZLNXImageData *)&image;

	char *src = source_image->getImageData();

	for(int i = 0; i < image.width(); i++)
		for(int j = 0; j < image.height(); j++) {
			c_src = src + i / 4 + image.width() * j / 4;
			s_src = (i & 3) << 1;

			c = buf + (i + x) / 4 + myWidth * (j + y) /4;
			s = ((i + x)  & 3) << 1;

			*c &= ~(0xc0 >> s);
			*c |= (((*c_src << s_src) & 0xc0) >> s);
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
	char *c = buf;
	bool done = false;

	if(x1 != x0) {
		k = (y1 - y0) / (x1 - x0);
		j = y0;
		i = x0;

		do {
			if(i == x1)
				done = true;

			c = buf + i / 4 + myWidth * j /4;
			s = (i & 3) << 1;
		
			*c &= ~(0xc0 >> s);

			if(fill)
				*c |= (fColor >> s);


			j += k;

			if(x1 > x0)
				i++;
			else 
				i--;

		} while(!done);

	} else {
		i = x0;
		j = y0;
		s = (i & 3) << 1;

		do {
			if(j == y1)
				done = true;

			c = buf + i / 4 + myWidth * j /4;
			*c &= ~(0xc0 >> s);

			if(fill)
				*c |= (fColor >> s);

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

		if(val >= 64) {
			*c &= ~(0xc0 >> s);
			if(val < 128)
				*c |= (0x80 >> s);
			else if(val < 192)
				*c |= (0x40 >> s);
		}

	}
  }
}
