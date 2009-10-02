/*
 * Copyright (C) 2004-2009 Geometer Plus <contact@geometerplus.com>
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

#include <algorithm>

#include <ZLStringUtil.h>
#include <ZLOptions.h>

#include "BookCollection.h"

#include "../database/booksdb/BooksDBUtil.h"


const size_t RecentBooks::MaxListSize = 10;

RecentBooks::RecentBooks() {
	BooksDBUtil::getRecentBooks(myBooks);
}

RecentBooks::~RecentBooks() {
//	BooksDB::instance().saveRecentBooks(myBooks);
}

void RecentBooks::reload() {
	myBooks.clear();
	BooksDBUtil::getRecentBooks(myBooks);
}

void RecentBooks::addBook(shared_ptr<DBBook> book) {
	if (book.isNull()) {
		return;
	}
	for (Books::iterator it = myBooks.begin(); it != myBooks.end(); ++it) {
		if ((*it)->fileName() == book->fileName()) {
			if (it == myBooks.begin()) {
				return;
			}
			myBooks.erase(it);
			break;
		}
	}
	myBooks.insert(myBooks.begin(), book);
	if (myBooks.size() > MaxListSize) {
		myBooks.erase(myBooks.begin() + MaxListSize, myBooks.end());
	}
	BooksDB::instance().saveRecentBooks(myBooks);
}

