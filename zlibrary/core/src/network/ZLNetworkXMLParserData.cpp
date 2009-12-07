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

#include <iostream>
#include <ZLUnicodeUtil.h>
#include <ZLStringUtil.h>

#include <ZLXMLAbstractReader.h>
#include <ZLGzipXMLReaderDecorator.h>

#include "ZLNetworkXMLParserData.h"


static const std::string CONTENT_ENCODING = "content-encoding:";


static size_t handleHeader(void *ptr, size_t size, size_t nmemb, void *stream) {
	ZLNetworkXMLParserData *parserData = (ZLNetworkXMLParserData *) stream;
	return parserData->parseHeader(ptr, size, nmemb);
}

static size_t handleData(void *ptr, size_t size, size_t nmemb, void *data) {
	ZLNetworkXMLParserData *parserData = (ZLNetworkXMLParserData *) data;
	return parserData->parseData(ptr, size, nmemb);
}


ZLNetworkXMLParserData::ZLNetworkXMLParserData(const std::string &url, shared_ptr<ZLXMLAbstractReader> reader) : 
		ZLNetworkData(url), myReader(reader) {
	CURL *h = handle();
	if (h != 0) {
		curl_easy_setopt(h, CURLOPT_HEADERFUNCTION, handleHeader);
		curl_easy_setopt(h, CURLOPT_WRITEHEADER, this);
		curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, handleData);
		curl_easy_setopt(h, CURLOPT_WRITEDATA, this);
		myReader->initialize();
	}
}

ZLNetworkXMLParserData::~ZLNetworkXMLParserData() {
	if (handle() != 0) {
		myReader->shutdown();
	}
}


size_t ZLNetworkXMLParserData::parseHeader(void *ptr, size_t size, size_t nmemb) {
	std::string header = ZLUnicodeUtil::toLower(std::string((const char *) ptr, size * nmemb));

	if (ZLStringUtil::stringStartsWith(header, CONTENT_ENCODING)) {
		std::string encoding = header.substr(CONTENT_ENCODING.size());
		ZLStringUtil::stripWhiteSpaces(encoding);
		myEncoding = encoding;
	}

	return size * nmemb;
}


size_t ZLNetworkXMLParserData::parseData(void *ptr, size_t size, size_t nmemb) {
	if (myEncoding == "gzip") {
		myReader = new ZLGzipXMLReaderDecorator(myReader);
		myEncoding = "";
	}
	if (!myReader->readFromBuffer((const char*)ptr, size * nmemb)) {
		return 0;
	}
	return size * nmemb;
}

