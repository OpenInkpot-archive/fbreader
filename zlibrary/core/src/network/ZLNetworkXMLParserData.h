/*
 * Copyright (C) 2008-2009 Geometer Plus <contact@geometerplus.com>
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

#ifndef __ZLNETWORKXMLPARSERDATA_H__
#define __ZLNETWORKXMLPARSERDATA_H__

#include <shared_ptr.h>

#include "ZLNetworkData.h"

class ZLXMLAbstractReader;

class ZLNetworkXMLParserData : public ZLNetworkData {

public:
	ZLNetworkXMLParserData(const std::string &url, shared_ptr<ZLXMLAbstractReader> reader);
	~ZLNetworkXMLParserData();

	size_t parseHeader(void *ptr, size_t size, size_t nmemb);
	size_t parseData(void *ptr, size_t size, size_t nmemb);

private:
	shared_ptr<ZLXMLAbstractReader> myReader;
	std::string myEncoding;
};

#endif /* __ZLNETWORKXMLPARSERDATA_H__ */
