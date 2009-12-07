/*
 * Copyright (C) 2009 Geometer Plus <contact@geometerplus.com>
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

//#include <cctype>

#include <ZLStringUtil.h>
#include <ZLNetworkUtil.h>
#include <ZLNetworkXMLParserData.h>

#include "FeedBooksAtomLink.h"

#include "../opds/OPDSXMLParser.h"
#include "../NetworkOPDSFeedReader.h"


static void addSubPattern(std::string &url, const std::string &name, const std::string &value) {
	if (!value.empty()) {
		int start = 0, end;
		do {
			end = value.find(' ', start);
			std::string ss = value.substr(start, end - start);
			ZLStringUtil::stripWhiteSpaces(ss);
			if (!ss.empty()) {
				if (!url.empty()) {
					url.append("+");
				}
				url.append(name);
				url.append(ZLNetworkUtil::htmlEncode(ss));
			}
			start = end + 1;
		} while (end != -1);
	}
}


FeedBooksAtomLink::FeedBooksAtomLink() : NetworkLink("feedbooks.com", "feedbooks.com/atom") {
}

shared_ptr<ZLNetworkData> FeedBooksAtomLink::simpleSearchData(SearchResult &result, const std::string &pattern) {
	return new ZLNetworkXMLParserData(
		"http://feedbooks.com/books/search.atom?query=" + ZLNetworkUtil::htmlEncode(pattern),
		new OPDSXMLParser( new NetworkOPDSFeedReader( result ) )
	);
}

shared_ptr<ZLNetworkData> FeedBooksAtomLink::advancedSearchData(SearchResult &result, 
		const std::string &title, const std::string &author, const std::string &/*series*/, 
		const std::string &tag, const std::string &annotation) {
	std::string request;
	addSubPattern(request, "title:", title);
	addSubPattern(request, "author:", author);
	//addSubPattern(request, "???:", series);
	addSubPattern(request, "type:", tag);
	addSubPattern(request, "description:", annotation);
	if (request.empty()) {
		return 0;
	}
	return new ZLNetworkXMLParserData(
		"http://feedbooks.com/books/search.atom?query=" + request,
		new OPDSXMLParser( new NetworkOPDSFeedReader( result ) )
	);
}

shared_ptr<ZLNetworkData> FeedBooksAtomLink::resume(SearchResult &result) {
	std::string uri = result.ResumeURI;
	result.clear();
	if (uri.empty()) {
		return 0;
	}
	return new ZLNetworkXMLParserData(uri,
		new OPDSXMLParser( new NetworkOPDSFeedReader( result ) )
	);
}

