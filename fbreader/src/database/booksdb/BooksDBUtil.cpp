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


#include <ZLFile.h>
#include <ZLDir.h>
#include <ZLStringUtil.h>

#include "BooksDBUtil.h"
#include "BooksDB.h"


shared_ptr<DBBook> BooksDBUtil::getBook(const std::string &fileName, bool checkFile) {
	const std::string physicalFileName = ZLFile(fileName).physicalFilePath();

	ZLFile file(physicalFileName);
	if (checkFile && !file.exists()) {
		return 0;
	}

	if (!checkFile || checkInfo(file)) {
		shared_ptr<DBBook> book = loadFromDB(fileName);
		if (!book.isNull() && isBookFull(*book)) {
			return book;
		}
	} else {
		if (physicalFileName != fileName) {
			resetZipInfo(file);
		}
		saveInfo(file);
	}

	shared_ptr<DBBook> book = DBBook::loadFromFile(fileName);
	if (book.isNull()) {
		return 0;
	}
	BooksDB::instance().saveBook(book);
	return book;
}

bool BooksDBUtil::getRecentBooks(std::vector<shared_ptr<DBBook> > &books) {
	std::vector<std::string> fileNames;
	if (!BooksDB::instance().loadRecentBooks(fileNames)) {
		return false;
	}
	for (std::vector<std::string>::const_iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
		shared_ptr<DBBook> book = getBook(*it /*, true OR false ? */); // TODO: check file ???
		if (!book.isNull()) {
			books.push_back(book);
		}
	}
	return true;
}

bool BooksDBUtil::getBooks(std::map<std::string, shared_ptr<DBBook> > &booksmap, bool checkFile) {
	std::vector<shared_ptr<DBBook> > books;
	if (!BooksDB::instance().loadBooks(books)) {
		return false;
	}
	for (std::vector<shared_ptr<DBBook> >::iterator it = books.begin(); it != books.end(); ++it) {
		DBBook &book = **it;
		const std::string &fileName = book.fileName();
		const std::string physicalFileName = ZLFile(fileName).physicalFilePath();
		ZLFile file(physicalFileName);
		if (!checkFile || file.exists()) {
			if (!checkFile || checkInfo(file)) {
				if (isBookFull(book)) {
					booksmap.insert(std::make_pair(fileName, *it));
					continue;
				}
			} else {
				if (physicalFileName != fileName) {
					resetZipInfo(file);
				}
				saveInfo(file);
			}
			shared_ptr<DBBook> bookptr = DBBook::loadFromFile(fileName);
			if (!bookptr.isNull()) {
				BooksDB::instance().saveBook(bookptr);
				booksmap.insert(std::make_pair(fileName, bookptr));
			}
		}
	}
	return true;
}

bool BooksDBUtil::isBookFull(const DBBook &book) {
	return
		!book.authors().empty() &&
		!book.title().empty() &&
		!book.encoding().empty();
}

shared_ptr<DBBook> BooksDBUtil::loadFromDB(const std::string &fileName) {
	if (fileName.empty()) {
		return 0;
	}
	return BooksDB::instance().loadBook(fileName);
}


void BooksDBUtil::fixFB2Encoding(const DBBook &book) {
	static const std::string _auto = "auto";
	// SEE: FB2Plugin.cpp:43
	BooksDB::instance().setEncoding(book, _auto);
}


bool BooksDBUtil::checkInfo(const ZLFile &file) {
	return BooksDB::instance().getFileSize(file.path()) == (int) file.size();
}

void BooksDBUtil::saveInfo(const ZLFile &file) {
	BooksDB::instance().setFileSize(file.path(), file.size());
}

void BooksDBUtil::listZipEntries(const ZLFile &zipFile, std::vector<std::string> &entries) {
	entries.clear();
	BooksDB::instance().loadFileEntries(zipFile.path(), entries);
	if (entries.empty()) {
		resetZipInfo(zipFile.path());
		BooksDB::instance().loadFileEntries(zipFile.path(), entries);
	}
}

void BooksDBUtil::resetZipInfo(const ZLFile &zipFile) {
	shared_ptr<ZLDir> zipDir = zipFile.directory();
	if (!zipDir.isNull()) {
		std::vector<std::string> entries;
		zipDir->collectFiles(entries, false);
		BooksDB::instance().saveFileEntries(zipFile.path(), entries);
	}
}

