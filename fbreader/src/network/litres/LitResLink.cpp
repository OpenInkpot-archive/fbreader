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

#include <iostream>
#include <cctype>

#include <ZLStringUtil.h>
#include <ZLUnicodeUtil.h>

#include <ZLNetworkUtil.h>
#include <ZLNetworkXMLParserData.h>
#include <ZLNetworkManager.h>

#include "LitResLink.h"
#include "LitResDataParser.h"
#include "LitResGenresParser.h"

#include "../NetworkBookInfo.h"


std::map<std::string, std::string> LitResLink::ourGenres;


static void addSubPattern(std::string &url, const std::string &name, const std::string &value) {
	std::string val(value);
	ZLStringUtil::stripWhiteSpaces(val);
	if (!val.empty()) {
		url.append("&");
		url.append(name);
		url.append("=");
		url.append(ZLNetworkUtil::htmlEncode(val));
	}
}


LitResLink::LitResLink() : NetworkLink("litres.ru", "litres.ru/api") {
}

shared_ptr<ZLNetworkData> LitResLink::simpleSearchData(SearchResult &result, const std::string &pattern) {
	return new ZLNetworkXMLParserData(
		"http://robot.litres.ru/pages/catalit_browser/?checkpoint=2000-01-01&search=" + ZLNetworkUtil::htmlEncode(pattern),
		new LitResDataParser(result.Books)
	);
}

shared_ptr<ZLNetworkData> LitResLink::advancedSearchData(SearchResult &result, const std::string &title, const std::string &author, const std::string &series, const std::string &tag, const std::string &annotation) {
	std::string request;
	addSubPattern(request, "search_title", title + " " + series);
	addSubPattern(request, "search_person", author);
	if (!tag.empty()) {
		if (ourGenres.empty()) {
			loadGenres();
		}
		std::vector<std::string> genres;
		fillGenres(tag, genres);
		if (genres.empty()) {
			return 0;
		}
		for (std::vector<std::string>::const_iterator it = genres.begin(); it != genres.end(); ++it) {
			addSubPattern(request, "genre", *it);
		}
	}
	//addSubPattern(request, "search", annotation); // if it is included, than annotation words also are searched in title, author, etc.

	if (request.empty()) {
		return 0;
	}
	
	std::cerr << "<LINK>" << std::endl;
	std::cerr << "http://robot.litres.ru/pages/catalit_browser/?checkpoint=2000-01-01" + request << std::endl;
	std::cerr << "</LINK>" << std::endl;
	
	return new ZLNetworkXMLParserData(
		"http://robot.litres.ru/pages/catalit_browser/?checkpoint=2000-01-01" + request,
		new LitResDataParser(result.Books)
	);
}


void LitResLink::loadGenres() {
	std::vector<shared_ptr<ZLNetworkData> > dataList;
	dataList.push_back(
		new ZLNetworkXMLParserData("http://robot.litres.ru/pages/catalit_genres/", new LitResGenresParser(ourGenres))
	);
	ZLNetworkManager::instance().perform(dataList);
}

void LitResLink::fillGenres(const std::string &tag, std::vector<std::string> &ids) {
	std::vector<std::string> words;
	int index = 0;

std::cerr << "words:" << std::endl;

	do {
		int index2 = tag.find(' ', index);
		std::string word = tag.substr(index, index2 - index);
		ZLStringUtil::stripWhiteSpaces(word);
		if (!word.empty()) {
			words.push_back(ZLUnicodeUtil::toLower(word));
			std::cerr << "\t" << words.back() << std::endl;
		}
		index = index2 + 1;
	} while (index != 0);

std::cerr << std::endl;

std::cerr << "tags:" << std::endl;

	for (std::map<std::string, std::string>::const_iterator it = ourGenres.begin(); it != ourGenres.end(); ++it) {
		const std::string &title = it->first;
		bool containsAll = true;
		for (std::vector<std::string>::const_iterator jt = words.begin(); jt != words.end(); ++jt) {
			if (title.find(*jt) == std::string::npos) {
				containsAll = false;
				break;
			}
		}
		if (containsAll) {
			ids.push_back(it->second);
			std::cerr << "\t" << it->second << ":" << it->first << std::endl;
		}
	}
std::cerr << std::endl;
}

