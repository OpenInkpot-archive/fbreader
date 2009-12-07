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

#ifndef __FEEDBOOKSATOMDATAPARSER_H__
#define __FEEDBOOKSATOMDATAPARSER_H__

#include <ZLXMLReader.h>

#include "../NetworkBookInfo.h"

class FeedBooksAtomDataParser : public ZLXMLReader {

public:
	FeedBooksAtomDataParser(NetworkBookList &books);

private:
	void startElementHandler(const char *tag, const char **attributes);
	void endElementHandler(const char *tag);
	void characterDataHandler(const char *text, size_t len);
	bool processNamespaces() const;
	void namespaceListChangedHandler();

private:
	NetworkBookList &myBooks;
	std::string myBuffer;
	shared_ptr<NetworkBookInfo> myCurrentBook;
	std::string myDCPrefix;
	
	int myDeep;
	std::string myLastXmlTag;
};

#endif /* __FEEDBOOKSATOMDATAPARSER_H__ */
