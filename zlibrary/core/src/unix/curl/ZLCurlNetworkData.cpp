/*
 * Copyright (C) 2008-2010 Geometer Plus <contact@geometerplus.com>
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

#include <ZLLogger.h>

#include "ZLCurlNetworkData.h"

ZLCurlNetworkData::ZLCurlNetworkData(const std::string &url, const std::string &sslCertificate) : ZLNetworkData(url, sslCertificate) {
	myHandle = curl_easy_init();
	if (myHandle != 0) {
		curl_easy_setopt(myHandle, CURLOPT_URL, url.c_str());
		if (!sslCertificate.empty()) {
			curl_easy_setopt(myHandle, CURLOPT_CAINFO, sslCertificate.c_str());
		}
		ZLLogger::Instance().println("URL", url);
	}
}

ZLCurlNetworkData::~ZLCurlNetworkData() {
	if (myHandle != 0) {
		curl_easy_cleanup(myHandle);
	}
}

CURL *ZLCurlNetworkData::handle() {
	return myHandle;
}
