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

#include <iostream>
#include <queue>
#include <algorithm>

#include <ZLibrary.h>
#include <ZLStringUtil.h>
#include <ZLFile.h>
#include <ZLDir.h>

#include "BookCollection.h"
#include "../formats/FormatPlugin.h"

#include "../database/booksdb/BooksDBUtil.h"


class DescriptionComparator {

public:
	bool operator() (const shared_ptr<DBBook> b1, const shared_ptr<DBBook> b2);
};

bool DescriptionComparator::operator() (const shared_ptr<DBBook> b1, const shared_ptr<DBBook> b2) {
	const DBBook &book1 = (const DBBook &) *b1;
	const DBBook &book2 = (const DBBook &) *b2;

	int comp = book1.authorSortKey().compare(book2.authorSortKey());
	if (comp != 0) {
		return comp < 0;
	}
	comp = book1.authorDisplayName().compare(book2.authorDisplayName());
	if (comp != 0) {
		return comp < 0;
	}

	const std::string &seriesName1 = book1.seriesName();
	const std::string &seriesName2 = book2.seriesName();
	if (seriesName1.empty() && seriesName2.empty()) {
		return b1->title() < b2->title();
	}
	if (seriesName1.empty()) {
		return b1->title() < seriesName2;
	}
	if (seriesName2.empty()) {
		return seriesName1 <= b2->title();
	}
	if (seriesName1 != seriesName2) {
		return seriesName1 < seriesName2;
	}
	if (book1.numberInSeries() == 0 && book2.numberInSeries() == 0) {
		return book1.title() < book2.title();
	}
	return book1.numberInSeries() < book2.numberInSeries();
}

static const std::string OPTIONS = "Options";

BookCollection::BookCollection() :
	PathOption(ZLCategoryKey::CONFIG, OPTIONS, "BookPath", ""),
	ScanSubdirsOption(ZLCategoryKey::CONFIG, OPTIONS, "ScanSubdirs", false),
	CollectAllBooksOption(ZLCategoryKey::CONFIG, OPTIONS, "CollectAllBooks", false),
	myDoStrongRebuild(true),
	myDoWeakRebuild(false) {
}

void BookCollection::rebuild(bool strong) {
	if (strong) {
		myDoStrongRebuild = true;
	} else {
		myDoWeakRebuild = true;
	}
}

void BookCollection::collectBookFileNames(std::set<std::string> &bookFileNames) const {
	std::set<std::string> dirs;
	collectDirNames(dirs);

	//for (std::set<std::string>::iterator it = dirs.begin(); it != dirs.end(); ++it) {
	while (!dirs.empty()) {
		std::string dirname = *dirs.begin();
		dirs.erase(dirs.begin());
		
		ZLFile dirfile(dirname);
		std::vector<std::string> files;
		bool inZip = false;

		shared_ptr<ZLDir> dir = dirfile.directory();
		if (dir.isNull()) {
			continue;
		}

		if (dirfile.extension() == "zip") {
			ZLFile phys(dirfile.physicalFilePath());
			if (!BooksDBUtil::checkInfo(phys)) {
				BooksDBUtil::resetZipInfo(phys);
				BooksDBUtil::saveInfo(phys);
			}
			BooksDBUtil::listZipEntries(dirfile, files);
			inZip = true;
		} else {
			dir->collectFiles(files, true);
		}
		if (!files.empty()) {
			const bool collectBookWithoutMetaInfo = CollectAllBooksOption.value();
			for (std::vector<std::string>::const_iterator jt = files.begin(); jt != files.end(); ++jt) {
				const std::string fileName = (inZip) ? (*jt) : (dir->itemPath(*jt));
				ZLFile file(fileName);
				if (PluginCollection::instance().plugin(file, !collectBookWithoutMetaInfo) != 0) {
					bookFileNames.insert(fileName);
				// TODO: zip -> any archive
				} else if (file.extension() == "zip") {
					if (myScanSubdirs || !inZip) {
						dirs.insert(fileName);
					}
				}
			}
		}
	}
}

bool BookCollection::synchronize() const {
	bool doStrongRebuild =
		myDoStrongRebuild ||
		(myScanSubdirs != ScanSubdirsOption.value()) ||
		(myPath != PathOption.value());

	if (!doStrongRebuild && !myDoWeakRebuild) {
		return false;
	}

	myPath = PathOption.value();
	myScanSubdirs = ScanSubdirsOption.value();
	myDoWeakRebuild = false;
	myDoStrongRebuild = false;

	if (doStrongRebuild) {
		myBooks.clear();
		myAuthors.clear();
		myExternalBookFileNames.clear();
		
		std::map<std::string, shared_ptr<DBBook> > booksmap;
		BooksDBUtil::getBooks(booksmap);

		std::set<std::string> fileNamesSet;
		collectBookFileNames(fileNamesSet);
		for (std::set<std::string>::iterator it = fileNamesSet.begin(); it != fileNamesSet.end(); ++it) {
			std::map<std::string, shared_ptr<DBBook> >::iterator jt = booksmap.find(*it);
			if (jt == booksmap.end()) {
				addBook(BooksDBUtil::getBook(*it));
			} else {
				addBook(jt->second);
				booksmap.erase(jt);
			}
		}

		for (std::map<std::string, shared_ptr<DBBook> >::iterator jt = booksmap.begin(); jt != booksmap.end(); ++jt) {
			shared_ptr<DBBook> book = jt->second;
			if (!book.isNull()) {
				if (BooksDB::instance().checkBookList(*book)) {
					addBook(book);
					myExternalBookFileNames.insert(book->fileName());
				}
			}
		}
	} else {
		std::vector<std::string> fileNames;
		for (Books::const_iterator it = myBooks.begin(); it != myBooks.end(); ++it) {
			const std::string itname = (*it)->fileName();
			if (myExternalBookFileNames.find(itname) == myExternalBookFileNames.end() || 
					BooksDB::instance().checkBookList(**it)) {
				fileNames.push_back(itname);
			}
		}
		myBooks.clear();
		myAuthors.clear();

		std::map<std::string, shared_ptr<DBBook> > booksmap;
		BooksDBUtil::getBooks(booksmap, false);

		for (std::vector<std::string>::iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
			std::map<std::string, shared_ptr<DBBook> >::iterator jt = booksmap.find(*it);
			if (jt != booksmap.end()) {
				addBook(jt->second);
			} else { 
				// TODO: remove debug code
				std::cerr << "BookCollection::synchronize(): Weak Rebuild Error (book isn't in db)" << std::endl;
			}
		}
	}

	std::sort(myBooks.begin(), myBooks.end(), DescriptionComparator());
	return true;
}

void BookCollection::collectDirNames(std::set<std::string> &nameSet) const {
	std::queue<std::string> nameQueue;

	std::string path = myPath;
	int pos = path.find(ZLibrary::PathDelimiter);
	while (pos != -1) {
		nameQueue.push(path.substr(0, pos));
		path.erase(0, pos + 1);
		pos = path.find(ZLibrary::PathDelimiter);
	}
	if (!path.empty()) {
		nameQueue.push(path);
	}

	while (!nameQueue.empty()) {
		std::string name = nameQueue.front();
		nameQueue.pop();
		ZLFile f(name);
		const std::string resolvedName = f.resolvedPath();
		if (nameSet.find(resolvedName) == nameSet.end()) {
			if (myScanSubdirs) {
				shared_ptr<ZLDir> dir = f.directory();
				if (!dir.isNull()) {
					std::vector<std::string> subdirs;
					dir->collectSubDirs(subdirs, true);
					for (std::vector<std::string>::const_iterator it = subdirs.begin(); it != subdirs.end(); ++it) {
						nameQueue.push(dir->itemPath(*it));
					}
				}
			}
			nameSet.insert(resolvedName);
		}
	}
}

BookCollection::~BookCollection() {
}

void BookCollection::addBook(shared_ptr<DBBook> book) const {
	if (!book.isNull()) {
		myBooks.push_back(book);
	}
}

const std::vector<shared_ptr<DBAuthor> > &BookCollection::authors() const {
	synchronize();
	if (myAuthors.empty() && !myBooks.empty()) {
		DBAuthorComparator comparator;
		for (Books::const_iterator it = myBooks.begin(); it != myBooks.end(); ++it) {
			const std::vector<shared_ptr<DBAuthor> > &bookAuthors = ((const DBBook &) **it).authors();
			for(std::vector<shared_ptr<DBAuthor> >::const_iterator jt = bookAuthors.begin(); jt != bookAuthors.end(); ++jt) {
				shared_ptr<DBAuthor> author = *jt;
				std::vector<shared_ptr<DBAuthor> >::iterator middle = std::lower_bound(myAuthors.begin(), myAuthors.end(), author, comparator);
				if (middle == myAuthors.end() || comparator(author, *middle)) {
					myAuthors.insert(middle, author);
				}
			}
		} // myAuthors is sorted ascending
	}
	return myAuthors;
}

void BookCollection::removeTag(shared_ptr<DBTag> tag, bool includeSubTags) {
	synchronize();
	for (Books::const_iterator it = myBooks.begin(); it != myBooks.end(); ++it) {
		BooksDBUtil::removeTag(*it, tag, includeSubTags);
	}
}

void BookCollection::addTagToAllBooks(shared_ptr<DBTag> tag) {
	synchronize();
	for (Books::const_iterator it = myBooks.begin(); it != myBooks.end(); ++it) {
		BooksDBUtil::addTag(*it, tag);
	}
}

void BookCollection::addTagToBooksWithNoTags(shared_ptr<DBTag> tag) {
	synchronize();
	for (Books::const_iterator it = myBooks.begin(); it != myBooks.end(); ++it) {
		if (((const DBBook &) **it).tags().empty()) {
			BooksDBUtil::addTag(*it, tag);
		}
	}
}

void BookCollection::renameTag(shared_ptr<DBTag> from, shared_ptr<DBTag> to, bool includeSubTags) {
	if (to != from) {
		synchronize();
		for (Books::const_iterator it = myBooks.begin(); it != myBooks.end(); ++it) {
			BooksDBUtil::renameTag(*it, from, to, includeSubTags);
		}
	}
}

void BookCollection::cloneTag(shared_ptr<DBTag> from, shared_ptr<DBTag> to, bool includeSubTags) {
	if (to != from) {
		synchronize();
		for (Books::const_iterator it = myBooks.begin(); it != myBooks.end(); ++it) {
			BooksDBUtil::cloneTag(*it, from, to, includeSubTags);
		}
	}
}

bool BookCollection::hasBooks(shared_ptr<DBTag> tag) const {
	synchronize();
	for (Books::const_iterator it = myBooks.begin(); it != myBooks.end(); ++it) {
		const std::vector<shared_ptr<DBTag> > &tags = ((const DBBook &) **it).tags();
		for (std::vector<shared_ptr<DBTag> >::const_iterator jt = tags.begin(); jt != tags.end(); ++jt) {
			if (*jt == tag) {
				return true;
			}
		}
	}
	return false;
}

bool BookCollection::hasSubtags(shared_ptr<DBTag> tag) const {
	synchronize();
	for (Books::const_iterator it = myBooks.begin(); it != myBooks.end(); ++it) {
		const std::vector<shared_ptr<DBTag> > &tags = ((const DBBook &) **it).tags();
		for (std::vector<shared_ptr<DBTag> >::const_iterator jt = tags.begin(); jt != tags.end(); ++jt) {
			if ((*jt)->isChildFor(*tag)) {
				return true;
			}
		}
	}
	return false;
}

void BookCollection::updateAuthor(shared_ptr<DBAuthor> from, shared_ptr<DBAuthor> to) {
	if (*to != *from) {
		synchronize();
		for (Books::const_iterator it = myBooks.begin(); it != myBooks.end(); ++it) {
			BooksDBUtil::updateAuthor(*it, from, to);
		}
	}
}

