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

#include <ZLStringUtil.h>

#include "NetworkOPDSFeedReader.h"


NetworkOPDSFeedReader::NetworkOPDSFeedReader(SearchResult &result) : myResult(result) {
}

NetworkOPDSFeedReader::~NetworkOPDSFeedReader() {
}


void NetworkOPDSFeedReader::processFeedStart() {}

void NetworkOPDSFeedReader::processFeedMetadata(shared_ptr<OPDSFeedMetadata> feed) {
	for (unsigned i = 0; i < feed->links().size(); ++i) {
		ATOMLink &link = *(feed->links()[i]);
		const std::string &href = link.href();
		const std::string &rel = link.rel();
		const std::string &type = link.type();
		if (type == OPDSConstants::MIME_APP_ATOM) {
			if (rel == "self") {
				std::cerr << "<link type=\"application/atom+xml\" rel=\"self\" href=\"" << href << "\"/>" << std::endl;
			} else if (rel == "next") {
				myResult.ResumeURI = href;
				std::cerr << "<link type=\"application/atom+xml\" rel=\"next\" href=\"" << href << "\" title=\"Next Page\"/>" << std::endl;
			}
		}
	}
}

void NetworkOPDSFeedReader::processFeedEnd() {}


void NetworkOPDSFeedReader::processFeedEntry(shared_ptr<OPDSEntry> entry) {
	if (entry.isNull()) {
		return;
	}
	OPDSEntry &e = *entry;
	shared_ptr<NetworkBookInfo> bookPtr = new NetworkBookInfo(e.id()->uri());
	NetworkBookInfo &book = *bookPtr;

	book.Title = e.title();
	book.Language = e.dcLanguage();
	book.Date = e.dcIssued()->getDateTime(true);
	book.Annotation = e.summary();

	for (unsigned i = 0; i < e.categories().size(); ++i) {
		ATOMCategory &category = *(e.categories()[i]);
		book.Tags.push_back(category.term());
	}

	for (unsigned i = 0; i < e.links().size(); ++i) {
		ATOMLink &link = *(e.links()[i]);
		const std::string &href = link.href();
		const std::string &rel = link.rel();
		const std::string &type = link.type();
		if (rel == OPDSConstants::REL_COVER) {
			if (type == OPDSConstants::MIME_IMG_PNG) {
				book.Cover = href;
			}
		} else if (rel == OPDSConstants::REL_ACQUISITION) {
			if (type == OPDSConstants::MIME_APP_EPUB) {
				book.URLByType[NetworkBookInfo::BOOK_EPUB] = href;
			} else if (type == OPDSConstants::MIME_APP_MOBI) {
				book.URLByType[NetworkBookInfo::BOOK_MOBIPOCKET] = href;
			//} else if (type == OPDSConstants::MIME_APP_PDF) {
				//book.URLByType[NetworkBookInfo::BOOK_PDF] = href;
			}
		}
	}

	for (unsigned i = 0; i < e.authors().size(); ++i) {
		ATOMAuthor &author = *(e.authors()[i]);
		NetworkBookInfo::AuthorData authorData;
		std::string name = author.name();
		int index = name.find(',');
		if (index >= 0) {
			std::string before = name.substr(0, index);
			std::string after = name.substr(index + 1);
			ZLStringUtil::stripWhiteSpaces(before);
			ZLStringUtil::stripWhiteSpaces(after);
			authorData.SortKey = before;
			authorData.DisplayName = after + ' ' + before;
		} else {
			ZLStringUtil::stripWhiteSpaces(name);
			index = name.rfind(' ');
			authorData.SortKey = name.substr(index + 1);
			authorData.DisplayName = name;
		}
		book.Authors.push_back(authorData);
	}

	//e.dcPublisher();
	//e.updated();
	//e.published();
	/*for (unsigned i = 0; i < e.contributors().size(); ++i) {
		ATOMContributor &contributor = *(e.contributors()[i]);
		std::cerr << "\t\t<contributor>" << std::endl;
		std::cerr << "\t\t\t<name>"  << contributor.name()  << "</name>"  << std::endl;
		if (!contributor.uri().empty())   std::cerr << "\t\t\t<uri>"   << contributor.uri()   << "</uri>"   << std::endl;
		if (!contributor.email().empty()) std::cerr << "\t\t\t<email>" << contributor.email() << "</email>" << std::endl;
		std::cerr << "\t\t</contributor>" << std::endl;
	}*/
	//e.rights();

	myResult.Books.push_back(bookPtr);
}

