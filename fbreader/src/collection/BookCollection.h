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

#ifndef __BOOKCOLLECTION_H__
#define __BOOKCOLLECTION_H__

#include <string>
#include <vector>
#include <set>
#include <map>

#include <ZLOptions.h>

#include "../database/booksdb/DBBook.h"

typedef std::vector<shared_ptr<DBBook> > Books;


class BookCollection {

public:
	ZLStringOption PathOption;
	ZLBooleanOption ScanSubdirsOption;
	ZLBooleanOption CollectAllBooksOption;

public:
	BookCollection();
	~BookCollection();

	const std::vector<shared_ptr<DBAuthor> > &authors() const;
	const Books &books() const;
	bool isBookExternal(shared_ptr<DBBook> book) const;

	void rebuild(bool strong);
	bool synchronize() const;

	void removeTag(shared_ptr<DBTag> tag, bool includeSubTags);
	void renameTag(shared_ptr<DBTag> from, shared_ptr<DBTag> to, bool includeSubTags);
	void cloneTag(shared_ptr<DBTag> from, shared_ptr<DBTag> to, bool includeSubTags);
	void addTagToAllBooks(shared_ptr<DBTag> tag);
	void addTagToBooksWithNoTags(shared_ptr<DBTag> tag);
	bool hasBooks(shared_ptr<DBTag> tag) const;
	bool hasSubtags(shared_ptr<DBTag> tag) const;

	void updateAuthor(shared_ptr<DBAuthor> from, shared_ptr<DBAuthor> to);

private:
	void collectDirNames(std::set<std::string> &names) const;
	void collectBookFileNames(std::set<std::string> &bookFileNames) const;

	void addBook(shared_ptr<DBBook> book) const;

private:
	mutable Books myBooks;
	mutable std::vector<shared_ptr<DBAuthor> > myAuthors;
	mutable std::set<std::string> myExternalBookFileNames;

	mutable std::string myPath;
	mutable bool myScanSubdirs;
	mutable bool myDoStrongRebuild;
	mutable bool myDoWeakRebuild;
};

class RecentBooks {

public:
	static const size_t MaxListSize;

public:
	RecentBooks();
	~RecentBooks();
	void addBook(shared_ptr<DBBook> book);
	const Books &books() const;

	void reload();

private:
	Books myBooks;
};

inline const Books &BookCollection::books() const {
	synchronize();
	return myBooks;
}

inline bool BookCollection::isBookExternal(shared_ptr<DBBook> book) const {
	synchronize();
	return myExternalBookFileNames.find(book->fileName()) != myExternalBookFileNames.end();
}

inline const Books &RecentBooks::books() const { return myBooks; }

#endif /* __BOOKCOLLECTION_H__ */
