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

#include "FeedBooksLink.h"
#include "FeedBooksDataParser.h"
#include "../fbreaderOrg/FBReaderOrgDataParser.h"
#include "../NetworkBookInfo.h"

static const std::string URL_PREFIX = "http://www.fbreader.org/library/";

static void addSubPattern(std::string &url, const std::string &name, const std::string &value) {
	if (!value.empty()) {
		url += "&" + name + "=" + ZLNetworkUtil::htmlEncode(value);
	}
}

FeedBooksLink::FeedBooksLink() : NetworkLink("FeedBooks.Com - old xml", "feedbooks.com") {
}

shared_ptr<ZLNetworkData> FeedBooksLink::simpleSearchData(SearchResult &result, const std::string &pattern) {
	return new ZLNetworkXMLParserData(
		"http://feedbooks.com/books/search.xml?query=" + ZLNetworkUtil::htmlEncode(pattern),
		new FeedBooksDataParser(result.Books)
	);
}

shared_ptr<ZLNetworkData> FeedBooksLink::advancedSearchData(SearchResult &result, const std::string &title, const std::string &author, const std::string &series, const std::string &tag, const std::string &annotation) {
	std::string request = URL_PREFIX + "advanced_query.php?type=xml&library=2";
	addSubPattern(request, "title", title);
	addSubPattern(request, "author", author);
	addSubPattern(request, "series", series);
	addSubPattern(request, "tag", tag);
	addSubPattern(request, "annotation", annotation);

	return new ZLNetworkXMLParserData(request, new FBReaderOrgDataParser(result.Books));
}
