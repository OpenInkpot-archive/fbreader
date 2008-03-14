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

extern char *buf;
#define ROUND_26_6_TO_INT(valuetoround) (((valuetoround) + 63) >> 6)

ZLNXPaintContext::ZLNXPaintContext() {
	myWidth = 600;
	myHeight = 800;

	myStringHeight = -1;
	mySpaceWidth = -1;
	myDescent = 0;

	fCurFamily = "";
	fCurSize = 0;
	fCurItalic = false;
	fCurBold = false;

	FT_Error error;

	error = FT_Init_FreeType( &library );

	fPath.push_back("/mnt/FBREADER/FONTS/");
	fPath.push_back("/mnt/CRENGINE/FONTS/");
	fPath.push_back("/root/fbreader/fonts/");
	fPath.push_back("/root/crengine/fonts/");
	fPath.push_back("/root/fonts/truetype/");

	cacheFonts();
}

ZLNXPaintContext::~ZLNXPaintContext() {

	for(std::map<std::string, std::map<int, Font> >::iterator x = fontCache.begin();
			x != fontCache.end();
			x++) {

		for(std::map<int, Font>::iterator y = x->second.begin();
				y != x->second.end();
				y++) {

			std::map<int, std::map<unsigned long, FT_BitmapGlyph> >::iterator piter = y->second.glyphCacheAll.begin();
			while(piter != y->second.glyphCacheAll.end()) {
				std::map<unsigned long, FT_BitmapGlyph>::iterator piter2 = piter->second.begin();
				while(piter2 != piter->second.end()) {
					FT_Bitmap_Done(library, (FT_Bitmap*)&(piter2->second->bitmap));			
					piter2++;
				}
				piter++;
			}

			if(y->second.myFace != NULL)
				FT_Done_Face(y->second.myFace);
		}
	}

	if(library)
		FT_Done_FreeType(library);
}


void ZLNXPaintContext::fillFamiliesList(std::vector<std::string> &families) const {
}

void ZLNXPaintContext::cacheFonts() const {
	DIR *dir_p;
	struct dirent *dp;
	int idx;
	bool bold, italic;
	int bi_hash;

	FT_Error error;
	FT_Face lFace;

	for(std::vector<std::string>::iterator it = fPath.begin();
			it != fPath.end();
			it++) {

		dir_p = NULL;
		dir_p = opendir(it->c_str());	
		if(dir_p == NULL)
			continue;

		while((dp = readdir(dir_p)) != NULL) {
			idx = strlen(dp->d_name);

			if(idx <= 4)
				continue;

			idx -= 4;
			if(strncasecmp(dp->d_name + idx, ".TTF", 4))
				continue;


			std::string fFullName = it->c_str();
			fFullName += "/";
			fFullName += dp->d_name;					

			for(int i = 0 ;; i++) {
				error = FT_New_Face(library, fFullName.c_str(), i, &lFace);
				if(error)
					break;

				if(FT_IS_SCALABLE(lFace)) {
					bold = lFace->style_flags & FT_STYLE_FLAG_BOLD;				
					italic = lFace->style_flags & FT_STYLE_FLAG_ITALIC;				
					bi_hash = (bold?2:0) + (italic?1:0);

					Font *fc = &(fontCache[lFace->family_name])[bi_hash];

					if(fc->fileName.length() == 0) {
						fc->familyName = lFace->family_name;
						fc->fileName = fFullName;
						fc->index = i;
						fc->isBold = bold;
						fc->isItalic = italic;
					}
				}

				FT_Done_Face(lFace);
			}
		}

/*		cout << "---------------------" << endl;

		for(std::map<std::string, std::map<int, Font> >::iterator x = fontCache.begin();
				x != fontCache.end();
				x++) {
			cout << "family: " << x->first << endl;

			for(std::map<int, Font>::iterator y = x->second.begin();
					y != x->second.end();
					y++) {
				cout << "	hash: " << y->first << endl;
				cout << "	file: " << y->second.fileName << endl;
				cout << "	index: " << y->second.index << endl;
				cout << "	b:	"	<< y->second.isBold << endl;
				cout << "	i:	"	<< y->second.isItalic << endl;
				cout << endl;
			}
		}
*/		

		closedir(dir_p);
	}
}

const std::string ZLNXPaintContext::realFontFamilyName(std::string &fontFamily) const {
}

void ZLNXPaintContext::setFont(const std::string &family, int size, bool bold, bool italic) {
	//printf("setFont: %s, %d, %d, %d\n", family.c_str(), size, bold?1:0, italic?1:0);
	FT_Error error;

	if((family == fCurFamily ) && (bold == fCurBold) && (italic == fCurItalic) && (size == fCurSize)) {
		return;
	}

	fCurFamily = family;
	fCurBold = bold;
	fCurItalic = italic;

	int bi_hash = (bold?2:0) + (italic?1:0);
	std::map<std::string, std::map<int, Font> >::iterator it = fontCache.find(family);
	if(it == fontCache.end()) {
		std::string defFont("Arial");
		it = fontCache.find(defFont);
	}

	Font *fc = &((it->second)[bi_hash]);

	/*
	   cout << "	hash: " << bi_hash << endl;
	   cout << "	family: " << fc->familyName << endl;
	   cout << "	file: " << fc->fileName << endl;
	   cout << "	index: " << fc->index << endl;
	   cout << "	b:	"	<< fc->isBold << endl;
	   cout << "	i:	"	<< fc->isItalic << endl;
	   cout << endl;
	   */

	if(fc->fileName.size() == 0)
		fc = &((it->second)[0]);

	if(fc->myFace == NULL) {
		error = FT_New_Face(library, fc->fileName.c_str(), fc->index, &fc->myFace);
		if(error)
			return;
	}

	face = &(fc->myFace);

	if(size >= 6)
		fCurSize = size;
	else
		fCurSize = 6; 

	FT_Set_Char_Size( *face, fCurSize * 64, 0, 160, 0 );

	charWidthCache = &(fc->charWidthCacheAll[fCurSize]);
	glyphCache = &(fc->glyphCacheAll[fCurSize]);

	myStringHeight = fCurSize * 160 / 72;
	myDescent = (abs((*face)->size->metrics.descender) + 63 ) >> 6;
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
	//	printf("drawImage: %d %d %d %d\n", image.width(), image.height(), x, y);
	char *c;
	char *c_src;
	int s, s_src;
	int iW = image.width();
	int iH = image.height();

	if((iW + x) > myWidth || y > myHeight)
		return;

	ZLNXImageData *source_image = (ZLNXImageData *)&image;

	char *src = source_image->getImageData();

	for(int j = 0; j < iH; j++)
		for(int i = 0; i < iW; i++) {
			c_src = src + i / 4 + iW * j / 4;
			s_src = (i & 3) << 1;

			c = buf + (i + x) / 4 + myWidth * (j + (y - iH)) /4;
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
