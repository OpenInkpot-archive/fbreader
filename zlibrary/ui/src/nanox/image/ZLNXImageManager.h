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

#ifndef __ZLNXIMAGEMANAGER_H__
#define __ZLNXIMAGEMANAGER_H__

#include <ZLImageManager.h>
#include <ZLImage.h>

class ZLNXImageData : public ZLImageData {

	public:
		ZLNXImageData() : myImageData(0), myX(0), myY(0) {}
		~ZLNXImageData() { 
			if(myImageData != 0) free(myImageData); 
		}

		unsigned int width() const;
		unsigned int height() const;

		void init(unsigned int width, unsigned int height);
		void setPosition(unsigned int x, unsigned int y);
		void moveX(int delta);
		void moveY(int delta);
		void setPixel(unsigned char r, unsigned char g, unsigned char b);

		void copyFrom(const ZLImageData &source, unsigned int targetX, unsigned int targetY);

		char *getImageData() { return myImageData; }

	private:
		char *myImageData;
		int myRowStride;
		int myWidth, myHeight;

		unsigned int myX, myY;
		char *myPosition;
		int myShift;

		friend class ZLNXImageManager;
};

class ZLNXImageManager : public ZLImageManager {

	public:
		static void createInstance() { ourInstance = new ZLNXImageManager(); }

	private:
		ZLNXImageManager() {}

	protected:
		shared_ptr<ZLImageData> createData() const;
		void convertImageDirect(const std::string &stringData, ZLImageData &imageData) const;
		void convertImageDirectJpeg(const std::string &stringData, ZLImageData &imageData) const;
		void convertImageDirectPng(const std::string &stringData, ZLImageData &imageData) const;
};

#endif /* __ZLNXIMAGEMANAGER_H__ */
