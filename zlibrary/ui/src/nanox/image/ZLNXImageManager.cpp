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

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

#include <ZLImage.h>

#include "ZLNXImageManager.h"






typedef struct {
    struct jpeg_source_mgr pub;   /* public fields */
	int len;
    JOCTET *buffer;      /* start of buffer */
    bool start_of_file;    /* have we gotten any data yet? */
} my_jpeg_source_mgr;

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

void my_jpeg_error (j_common_ptr cinfo) {
	printf("jpeg error huj\n");
  	my_error_ptr myerr = (my_error_ptr) cinfo->err;

    char buffer[JMSG_LENGTH_MAX];

    /* Create the message */
    (*cinfo->err->format_message) (cinfo, buffer);
  
    fprintf( stderr, "message: %s\n", buffer );
    
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    //my_error_ptr myerr = (my_error_ptr) cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
  	longjmp(myerr->setjmp_buffer, 1);
}

void my_init_source (j_decompress_ptr cinfo)
{
	printf("init_source\n");
    my_jpeg_source_mgr * src = (my_jpeg_source_mgr*) cinfo->src;
    src->start_of_file = true;
}

boolean my_fill_input_buffer (j_decompress_ptr cinfo)
{
	printf("my_fill_input_buffer\n");

    my_jpeg_source_mgr * src = (my_jpeg_source_mgr *) cinfo->src;


    src->pub.next_input_byte = src->buffer;
    src->pub.bytes_in_buffer = src->len; 

    src->start_of_file = false;

    return true;
}

void my_skip_input_data (j_decompress_ptr cinfo, long num_bytes) {
}

boolean my_resync_to_restart (j_decompress_ptr cinfo, int desired) {
	return false;
}

void my_term_source (j_decompress_ptr cinfo) {
}














unsigned int ZLNXImageData::width() const {
	return myWidth;
}

unsigned int ZLNXImageData::height() const {
	return myHeight;
}

void ZLNXImageData::init(unsigned int width, unsigned int height) {
	printf("new image: %d %d\n", width, height);
	myWidth = width;
	myHeight = height;

	myImageData = (char*)malloc((myHeight * myWidth + 3) / 4);
}

void ZLNXImageData::setPosition(unsigned int x, unsigned int y) {
	myPosition = myImageData + x / 4 + myWidth * y / 4;
	myShift = (x & 3) << 1;
}

void ZLNXImageData::moveX(int delta) {
	myPosition += delta / 4;
	myShift = (((myShift >> 1) + delta) & 3) << 1;
}

void ZLNXImageData::moveY(int delta) {
	myPosition += myWidth * delta / 4;
}

void ZLNXImageData::setPixel(unsigned char r, unsigned char g, unsigned char b) {
	int pixel = (0.299 * r + 0.587 * g + 0.114 * b ) / 64;
	pixel = (pixel & 3) << 6;

	*myPosition &= ~(0xc0 >> myShift);
	*myPosition |= (pixel >> myShift);
}

void ZLNXImageData::copyFrom(const ZLImageData &source, unsigned int targetX, unsigned int targetY) {
	printf("copyFrom\n");

	char *c;
	char *c_src;
	int s, s_src;

	ZLNXImageData *source_image = (ZLNXImageData *)&source;

	char *src = source_image->getImageData();

	for(int i = 0; i < source.width(); i++)
		for(int j = 0; j < source.height(); j++) {
			c_src = src + i / 4 + source.width() * j / 4;
			s_src = (i & 3) << 1;


			c = myImageData + (i + targetX) / 4 + myWidth * (j + targetY) / 4;
			s = ((i + targetX) & 3) << 1;

			*c &= ~(0xc0 >> s);
			*c |= (((*c_src << s_src) & 0xc0) >> s);
		}
}

shared_ptr<ZLImageData> ZLNXImageManager::createData() const {
	printf("imagemanager::createData()\n");
	return new ZLNXImageData();
}

void ZLNXImageManager::convertImageDirect(const std::string &stringData, ZLImageData &data) const {
	printf("convertImageDirect\n");
	return;
	
	struct jpeg_decompress_struct cinfo;
	int row_stride;		/* physical row width in output buffer */

	printf("=> %d\n", sizeof(struct jpeg_decompress_struct));

  	struct my_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr.pub);
  	jerr.pub.error_exit = my_jpeg_error;

	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		return;
	}

    my_jpeg_source_mgr *src;

	src = (my_jpeg_source_mgr *) new my_jpeg_source_mgr;
	cinfo.src = (struct jpeg_source_mgr *) src;
	src->len = stringData.size() + 2;
	src->buffer = new JOCTET[src->len];

	memcpy(src->buffer, stringData.c_str(), stringData.size());
    src->buffer[stringData.size()] = (JOCTET) 0xFF;
    src->buffer[stringData.size() + 1] = (JOCTET) JPEG_EOI;

    src->pub.init_source = my_init_source;
    src->pub.fill_input_buffer = my_fill_input_buffer;
    src->pub.skip_input_data = my_skip_input_data;
    src->pub.resync_to_restart = my_resync_to_restart;
    src->pub.term_source = my_term_source;
    src->pub.bytes_in_buffer = src->len;
    src->pub.next_input_byte = src->buffer;

	printf("convertImageDirect2\n");
	jpeg_create_decompress(&cinfo);
	printf("convertImageDirect3");
	(void) jpeg_read_header(&cinfo, true);
	printf("%d %d........\n", cinfo.image_width, cinfo.image_height);

	if(src->buffer) 
		delete src->buffer;
	if(src)
		delete src;
	jpeg_destroy_decompress(&cinfo);
}
