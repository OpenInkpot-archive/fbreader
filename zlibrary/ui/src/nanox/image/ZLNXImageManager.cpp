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

#include <ZLImage.h>

#include "ZLNXImageManager.h"

unsigned int ZLNXImageData::width() const {
	return 0;
}

unsigned int ZLNXImageData::height() const {
	return 0;
}

void ZLNXImageData::init(unsigned int width, unsigned int height) {
}

void ZLNXImageData::setPosition(unsigned int x, unsigned int y) {
	myPosition = myImageData + 3 * x + myRowStride * y;
}

void ZLNXImageData::moveX(int delta) {
	myPosition += 3 * delta;
}

void ZLNXImageData::moveY(int delta) {
	myPosition += myRowStride * delta;
}

void ZLNXImageData::setPixel(unsigned char r, unsigned char g, unsigned char b) {
	myPosition[0] = r;
	myPosition[1] = g;
	myPosition[2] = b;
}

void ZLNXImageData::copyFrom(const ZLImageData &source, unsigned int targetX, unsigned int targetY) {
}

shared_ptr<ZLImageData> ZLNXImageManager::createData() const {
	return new ZLNXImageData();
}

void ZLNXImageManager::convertImageDirect(const std::string &stringData, ZLImageData &data) const {
}
