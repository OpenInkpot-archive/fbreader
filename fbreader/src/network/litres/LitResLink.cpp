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

#include <cctype>

#include <ZLNetworkUtil.h>
#include <ZLNetworkXMLParserData.h>

#include "LitResLink.h"
#include "LitResDataParser.h"

#include "../NetworkBookInfo.h"


LitResLink::LitResLink() : NetworkLink("litres.ru", "litres.ru/api") {
}

shared_ptr<ZLNetworkData> LitResLink::simpleSearchData(SearchResult &result, const std::string &pattern) {
	return new ZLNetworkXMLParserData(
		"http://robot.litres.ru/pages/catalit_browser/?checkpoint=2000-01-01&search=" + ZLNetworkUtil::htmlEncode(pattern),
		new LitResDataParser(result.Books)
	);
}

shared_ptr<ZLNetworkData> LitResLink::advancedSearchData(SearchResult &result, const std::string &title, const std::string &author, const std::string &series, const std::string &tag, const std::string &annotation) {
	return 0;
}

