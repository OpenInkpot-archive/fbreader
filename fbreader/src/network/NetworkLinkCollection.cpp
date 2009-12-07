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

#include <ZLFile.h>
#include <ZLDir.h>
#include <ZLStringUtil.h>
#include <ZLResource.h>
#include <ZLNetworkManager.h>

#include "NetworkLink.h"

#include "feedbooks/FeedBooksLink.h"
#include "fbreaderOrg/FBReaderOrgLink.h"

#include "../options/FBOptions.h"

#include "../database/booksdb/BooksDB.h"

#include "feedbooks_atom/FeedBooksAtomLink.h"
#include "litres/LitResLink.h"

NetworkLinkCollection *NetworkLinkCollection::ourInstance = 0;

NetworkLinkCollection &NetworkLinkCollection::instance() {
	if (ourInstance == 0) {
		ourInstance = new NetworkLinkCollection();
	}
	return *ourInstance;
}

NetworkLinkCollection::NetworkLinkCollection() :
	DirectoryOption(ZLCategoryKey::NETWORK, "Options", "DownloadDirectory", "") {
	//myLinks.push_back(new FeedBooksLink());
	//myLinks.push_back(new FBReaderOrgLink());
	myLinks.push_back(new FeedBooksAtomLink());
	myLinks.push_back(new LitResLink());
}

NetworkLinkCollection::~NetworkLinkCollection() {
}

static std::string normalize(const std::string &url) {
	static const std::string PREFIX0 = "http://feedbooks.com/";
	static const std::string PREFIX1 = "http://www.feedbooks.com/";
	static const std::string STANZA_PREFIX = "http://feedbooks.com/book/stanza/";

	std::string nURL = url;
	if (ZLStringUtil::stringStartsWith(nURL, PREFIX1)) {
		nURL = PREFIX0 + nURL.substr(PREFIX1.length());
	}
	if (ZLStringUtil::stringStartsWith(nURL, STANZA_PREFIX)) {
		nURL = PREFIX0 + "book/" + nURL.substr(STANZA_PREFIX.length()) + ".epub";
	}
	return nURL;
}

std::string NetworkLinkCollection::bookFileName(const std::string &url) const {
	//return ZLStringOption(ZLCategoryKey::NETWORK, "Files", ::normalize(url), "").value();
	return BooksDB::instance().getNetFile(::normalize(url));
}

std::string NetworkLinkCollection::downloadBook(const std::string &url, std::string &fileName) const {
	const std::string nURL = ::normalize(url);
	//ZLStringOption fileNameOption(ZLCategoryKey::NETWORK, "Files", nURL, "");
	std::string storedFileName = BooksDB::instance().getNetFile(nURL);
	if (!storedFileName.empty() && ZLFile(storedFileName).exists()) {
		fileName = storedFileName;
		return "";
	}

	const ZLResource &errorResource = ZLResource::resource("dialog")["networkError"];
	shared_ptr<ZLDir> dir = ZLFile(DirectoryOption.value()).directory(true);
	if (dir.isNull()) {
		return ZLStringUtil::printf(errorResource["couldntCreateDirectoryMessage"].value(), DirectoryOption.value());
	}

	if (fileName.empty()) {
		size_t index = nURL.rfind('/');
		if (index >= nURL.length() - 1) {
			return errorResource["unknownErrorMessage"].value();
		}
		fileName = nURL.substr(index + 1);
	}
	fileName = dir->itemPath(fileName);
	if (ZLFile(fileName).exists()) {
		ZLFile(fileName).remove();
	}
	//fileNameOption.setValue(fileName);
	BooksDB::instance().setNetFile(nURL, fileName);
	return ZLNetworkManager::instance().downloadFile(nURL, fileName);
}

std::string NetworkLinkCollection::simpleSearch(NetworkBookList &books, const std::string &pattern) {
	std::vector<shared_ptr<ZLNetworkData> > dataList;

	std::vector<SearchResult> searchResults(myLinks.size());

	int count = 0;

	for (LinkVector::const_iterator it = myLinks.begin(); it != myLinks.end(); ++it) {
		NetworkLink &link = **it;
		SearchResult &searchResult = searchResults[count++];
		if (link.OnOption.value()) {
			shared_ptr<ZLNetworkData> data = link.simpleSearchData(searchResult, pattern);
			if (!data.isNull()) {
				dataList.push_back(data);
			}
		}
	}

	std::string result;
	while (result.empty() && !dataList.empty()) {
		result = ZLNetworkManager::instance().perform(dataList);
		
		for (std::vector<SearchResult>::const_iterator jt = searchResults.begin(); jt != searchResults.end(); ++jt) {
			books.insert(books.end(), jt->Books.begin(), jt->Books.end());
		}

		count = 0;
		dataList.clear();

		for (LinkVector::const_iterator it = myLinks.begin(); it != myLinks.end(); ++it) {
			NetworkLink &link = **it;
			SearchResult &searchResult = searchResults[count++];
			if (link.OnOption.value()) {
				shared_ptr<ZLNetworkData> data = link.resume(searchResult);
				if (!data.isNull()) {
					dataList.push_back(data);
				}
			}
		}
	}

	return result;
}

std::string NetworkLinkCollection::advancedSearch(NetworkBookList &books, const std::string &title, const std::string &author, const std::string &series, const std::string &tag, const std::string &annotation) {
	std::vector<shared_ptr<ZLNetworkData> > dataList;

	std::vector<SearchResult> searchResults(myLinks.size());
	int count = 0;

	for (LinkVector::const_iterator it = myLinks.begin(); it != myLinks.end(); ++it) {
		NetworkLink &link = **it;
		SearchResult &searchResult = searchResults[count++];
		if (link.OnOption.value()) {
			shared_ptr<ZLNetworkData> data = link.advancedSearchData(searchResult, title, author, series, tag, annotation);
			if (!data.isNull()) {
				dataList.push_back(data);
			}
		}
	}

	std::string result;
	while (result.empty() && !dataList.empty()) {
		result = ZLNetworkManager::instance().perform(dataList);

		for (std::vector<SearchResult>::const_iterator jt = searchResults.begin(); jt != searchResults.end(); ++jt) {
			books.insert(books.end(), jt->Books.begin(), jt->Books.end());
		}

		count = 0;
		dataList.clear();

		for (LinkVector::const_iterator it = myLinks.begin(); it != myLinks.end(); ++it) {
			NetworkLink &link = **it;
			SearchResult &searchResult = searchResults[count++];
			if (link.OnOption.value()) {
				shared_ptr<ZLNetworkData> data = link.resume(searchResult);
				if (!data.isNull()) {
					dataList.push_back(data);
				}
			}
		}
	}

	return result;
}

size_t NetworkLinkCollection::size() const {
	return myLinks.size();
}

NetworkLink &NetworkLinkCollection::link(size_t index) const {
	return *myLinks[index];
}

bool NetworkLinkCollection::containsEnabledLinks() const {
	for (LinkVector::const_iterator it = myLinks.begin(); it != myLinks.end(); ++it) {
		if ((*it)->OnOption.value()) {
			return true;
		}
	}
	return false;
}
