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
#include <ZLibrary.h>
#include <ZLFileImage.h>

#include "CollectionView.h"
#include "CollectionModel.h"

#include "../database/booksdb/BooksDBUtil.h"


class TagComparator {
public:
	bool operator() (shared_ptr<DBTag> tag1, shared_ptr<DBTag> tag2);
};

inline bool TagComparator::operator() (shared_ptr<DBTag> tag1, shared_ptr<DBTag> tag2) {
	return tag1->fullName() < tag2->fullName();
}


const std::string CollectionModel::RemoveBookImageId = "removeBook";
const std::string CollectionModel::BookInfoImageId = "bookInfo";
const std::string CollectionModel::AuthorInfoImageId = "authorInfo";
const std::string CollectionModel::SeriesOrderImageId = "seriesOrder";
const std::string CollectionModel::TagInfoImageId = "tagInfo";
const std::string CollectionModel::RemoveTagImageId = "removeTag";
const std::string CollectionModel::StrutImageId = "strut";

CollectionModel::CollectionModel(CollectionView &view, BookCollection &collection) : ZLTextTreeModel(), myView(view), myCollection(collection) {
	const std::string prefix = ZLibrary::ApplicationImageDirectory() + ZLibrary::FileNameDelimiter;
	myImageMap[RemoveBookImageId] = new ZLFileImage("image/png", prefix + "tree-removebook.png", 0);
	myImageMap[BookInfoImageId] = new ZLFileImage("image/png", prefix + "tree-bookinfo.png", 0);
	myImageMap[AuthorInfoImageId] = new ZLFileImage("image/png", prefix + "tree-authorinfo.png", 0);
	myImageMap[SeriesOrderImageId] = new ZLFileImage("image/png", prefix + "tree-order-series.png", 0);
	myImageMap[TagInfoImageId] = new ZLFileImage("image/png", prefix + "tree-taginfo.png", 0);
	myImageMap[RemoveTagImageId] = new ZLFileImage("image/png", prefix + "tree-removetag.png", 0);
	myImageMap[StrutImageId] = new ZLFileImage("image/png", prefix + "tree-strut.png", 0);
	myAllBooksParagraph = 0;
	myBooksWithoutTagsParagraph = 0;
}

CollectionModel::~CollectionModel() {
}

shared_ptr<DBBook> CollectionModel::bookByParagraphIndex(int num) {
	if ((num < 0) || ((int)paragraphsNumber() <= num)) {
		return 0;
	}
	std::map<ZLTextParagraph*, shared_ptr<DBBook> >::iterator it = myParagraphToBook.find((*this)[num]);
	return (it != myParagraphToBook.end()) ? it->second : 0;
}

shared_ptr<DBTag> CollectionModel::tagByParagraphIndex(int num, std::string &special) {
	ZLTextParagraph *paragraph = (*this)[num];

	if (paragraph == myAllBooksParagraph) {
		special = CollectionView::SpecialTagAllBooks;
		return 0;
	} else if (paragraph == myBooksWithoutTagsParagraph) {
		special = CollectionView::SpecialTagNoTagsBooks;
		return 0;
	}

	std::map<ZLTextParagraph*,shared_ptr<DBTag> >::iterator it = myParagraphToTag.find((*this)[num]);
	return (it != myParagraphToTag.end()) ? it->second : 0;
}

const std::vector<int> &CollectionModel::paragraphIndicesByBook(shared_ptr<DBBook> book) {
	return myBookToParagraph[book->fileName()];
}

void CollectionModel::build() {
	if (myCollection.books().empty()) {
		createParagraph();
		insertText(LIBRARY_ENTRY, ZLResource::resource("library")["noBooks"].value());
	} else {
		if (myView.organizeByTags()) {
			buildOrganizedByTags(false);
		} else {
			buildOrganizedByAuthors();
		}
	}
}

void CollectionModel::buildOrganizedByTags(bool buildAuthorTree) {
	const ZLResource &resource = ZLResource::resource("library");

	if (myView.ShowAllBooksTagOption.value()) {
		myAllBooksParagraph = createParagraph();
		insertText(LIBRARY_ENTRY, resource["allBooks"].value());
		addBidiReset();
		insertImage(TagInfoImageId);
		//myParagraphToTag[allBooksParagraph] = CollectionView::SpecialTagAllBooks;
		addBooks(buildAuthorTree, myCollection.books(), myAllBooksParagraph);
	}

	std::map<shared_ptr<DBTag>, Books, TagComparator> tagMap;
	Books booksWithoutTags;

	const Books &books = myCollection.books();
	for (Books::const_iterator it = books.begin(); it != books.end(); ++it) {
		const std::vector<shared_ptr<DBTag> > &bookTags = ((const DBBook &) **it).tags();
		if (bookTags.empty()) {
			booksWithoutTags.push_back(*it);
		} else {
			for (std::vector<shared_ptr<DBTag> >::const_iterator jt = bookTags.begin(); jt != bookTags.end(); ++jt) {
				tagMap[*jt].push_back(*it);
			}
		}
	}

	if (!booksWithoutTags.empty()) {
		myBooksWithoutTagsParagraph = createParagraph();
		insertText(LIBRARY_ENTRY, resource["booksWithoutTags"].value());
		addBidiReset();
		insertImage(TagInfoImageId);
		//myParagraphToTag[booksWithoutTagsParagraph] = CollectionView::SpecialTagNoTagsBooks;
		addBooks(buildAuthorTree, booksWithoutTags, myBooksWithoutTagsParagraph);
	}

	std::vector<shared_ptr<DBTag> > tagStack;
	ZLTextTreeParagraph *tagParagraph = 0;
	std::map<ZLTextTreeParagraph*, shared_ptr<DBTag> > paragraphToTagMap;
	for (std::map<shared_ptr<DBTag>, Books>::const_iterator it = tagMap.begin(); it != tagMap.end(); ++it) {
		shared_ptr<DBTag> fullTagPtr = it->first;
		std::vector<shared_ptr<DBTag> > subtags;
		DBTag::fillParents(*fullTagPtr, subtags);
		bool useExistingTagStack = true;
		for (int depth = 0; (size_t)depth < subtags.size(); ++depth) {
			shared_ptr<DBTag> subTag = subtags[depth];
			if (useExistingTagStack) {
				if (tagStack.size() == (size_t)depth) {
					useExistingTagStack = false;
				} else if (tagStack[depth] != subTag) {
					for (int i = tagStack.size() - depth; i > 0; --i) {
						std::map<ZLTextTreeParagraph*, shared_ptr<DBTag> >::iterator jt =
							paragraphToTagMap.find(tagParagraph);
						if (jt != paragraphToTagMap.end()) {
							addBooks(buildAuthorTree, tagMap[jt->second], tagParagraph);
						}
						tagParagraph = tagParagraph->parent(); // опускаемся к предку
					}
					tagStack.resize(depth);
					useExistingTagStack = false;
				}
			}
			if (!useExistingTagStack) {
				tagStack.push_back(subTag);
				tagParagraph = createParagraph(tagParagraph);
				myParagraphToTag[tagParagraph] = subTag;
				insertText(LIBRARY_ENTRY, subTag->fullName());
				addBidiReset();
				insertImage(TagInfoImageId);
				insertImage(RemoveTagImageId);
			}
		}
		paragraphToTagMap[tagParagraph] = fullTagPtr;
	}
	while (tagParagraph != 0) {
		std::map<ZLTextTreeParagraph*, shared_ptr<DBTag> >::iterator jt = paragraphToTagMap.find(tagParagraph);
		if (jt != paragraphToTagMap.end()) {
			addBooks(buildAuthorTree, tagMap[jt->second], tagParagraph);
		}
		tagParagraph = tagParagraph->parent();
	}
}

void CollectionModel::buildOrganizedByAuthors() {
	if (myView.ShowAllBooksTagOption.value()) {
		const ZLResource &resource = ZLResource::resource("library");
		myAllBooksParagraph = createParagraph();
		insertText(LIBRARY_ENTRY, resource["allBooks"].value());
		addBidiReset();
		insertImage(StrutImageId);
		//myParagraphToTag[allBooksParagraph] = CollectionView::SpecialTagAllBooks;
		addBooks(false, myCollection.books(), myAllBooksParagraph);
	}

	addBooksTree(myCollection.books(), 0);
}

void CollectionModel::addBooks(bool asTree, const Books &books, ZLTextTreeParagraph *root) {
	if (asTree) {
		addBooksTree(books, root);
	} else {
		addBooksPlain(books, root);
	}
}

void CollectionModel::addBooksPlain(const Books &books, ZLTextTreeParagraph *root) {
	for (Books::const_iterator jt = books.begin(); jt != books.end(); ++jt) {
		shared_ptr<DBBook> book = *jt;
		
		std::string authorName = book->authorDisplayName();

		ZLTextTreeParagraph *bookParagraph = createParagraph(root);
		insertText(LIBRARY_ENTRY, authorName + ". ");
		const std::string &seriesName = book->seriesName();
		if (!seriesName.empty()) {
			addText(seriesName + ". ");
		}
		addText(book->title());
		addBidiReset();
		insertImage(BookInfoImageId);
		if (myCollection.isBookExternal(book)) {
			insertImage(RemoveBookImageId);
		}
		myParagraphToBook[bookParagraph] = book;
		myBookToParagraph[book->fileName()].push_back(paragraphsNumber() - 1);
	}
}

void CollectionModel::addBooksTree(const Books &books, ZLTextTreeParagraph *root) {
	std::map<shared_ptr<DBAuthor>, Books, DBAuthorComparator> authorMap;
	for (Books::const_iterator it = books.begin(); it != books.end(); ++it) {
		const std::vector<shared_ptr<DBAuthor> > &authors = ((const DBBook &) **it).authors();
		for (std::vector<shared_ptr<DBAuthor> >::const_iterator jt = authors.begin(); jt != authors.end(); ++jt) {
			authorMap[*jt].push_back(*it);
		}
	}
	for (std::map<shared_ptr<DBAuthor>, Books, DBAuthorComparator>::const_iterator it = authorMap.begin(); it != authorMap.end(); ++it) {
		const shared_ptr<DBAuthor> author = it->first;
		const Books &books = it->second;
		ZLTextTreeParagraph *authorParagraph = createParagraph(root);
		insertText(LIBRARY_ENTRY, author->name());
		addBidiReset();
		insertImage(AuthorInfoImageId);
		myParagraphToAuthor[authorParagraph] = author;
		std::string currentSeriesName;
		ZLTextTreeParagraph *seriesParagraph = 0;
		for (Books::const_iterator jt = books.begin(); jt != books.end(); ++jt) {
			const shared_ptr<DBBook> book = *jt;
			const std::string &seriesName = book->seriesName();
			if (seriesName.empty()) {
				currentSeriesName.erase();
				seriesParagraph = 0;
			} else if (seriesName != currentSeriesName) {
				currentSeriesName = seriesName;
				seriesParagraph = createParagraph(authorParagraph);
				insertText(LIBRARY_ENTRY, seriesName);
				addBidiReset();
				insertImage(StrutImageId);
			}
			ZLTextTreeParagraph *bookParagraph = createParagraph(
				(seriesParagraph == 0) ? authorParagraph : seriesParagraph
			);
			insertText(LIBRARY_ENTRY, book->title());
			addBidiReset();
			insertImage(BookInfoImageId);
			if (myCollection.isBookExternal(book)) {
				insertImage(RemoveBookImageId);
			} else if (BooksDBUtil::canRemoveFile(book->fileName())) {
				insertImage(RemoveBookImageId); // TODO: replace with other color???				
			}
			myParagraphToBook[bookParagraph] = book;
			myBookToParagraph[book->fileName()].push_back(paragraphsNumber() - 1);
		}
	}
}

void CollectionModel::update() {
	myParagraphToBook.clear();
	myBookToParagraph.clear();
	myParagraphToTag.clear();
	for (int i = paragraphsNumber() - 1; i >= 0; --i) {
		removeParagraph(i);
	}
	build();
}

void CollectionModel::insertText(FBTextKind kind, const std::string &text) {
	addControl(kind, true);
	addText(text);
}

void CollectionModel::insertImage(const std::string &id) {
	addFixedHSpace(1);
	addImage(id, myImageMap, 0);
}

void CollectionModel::removeBook(shared_ptr<DBBook> book) {
	std::map<std::string, std::vector<int> >::iterator it = myBookToParagraph.find(book->fileName());
	if (it == myBookToParagraph.end()) {
		return;
	}
	const std::vector<int> paragraphIndices = it->second;
	myBookToParagraph.erase(it);
	for (int i = paragraphIndices.size() - 1; i >= 0; --i) {
		int index = paragraphIndices[i];
		ZLTextTreeParagraph *paragraph = (ZLTextTreeParagraph*)(*this)[index];
		int count = 1;
		for (ZLTextTreeParagraph *parent = paragraph->parent(); (parent != 0) && (parent->children().size() == 1); parent = parent->parent()) {
			++count;
		}

		if (count > index) {
			count = index + 1;
		}

		for (std::map<std::string, std::vector<int> >::iterator jt = myBookToParagraph.begin(); jt != myBookToParagraph.end(); ++jt) {
			std::vector<int> &indices = jt->second;
			for (std::vector<int>::iterator kt = indices.begin(); kt != indices.end(); ++kt) {
				if (*kt > index) {
					*kt -= count;
				}
			}
		}

		for (; count > 0; --count) {
			removeParagraph(index--);
		}
	}
	std::cerr << "paragraphsNumber() == " << paragraphsNumber() << std::endl;
	if (paragraphsNumber() == 0) {
		createParagraph();
		insertText(LIBRARY_ENTRY, ZLResource::resource("library")["noBooks"].value());
	}
}

bool CollectionModel::empty() const {
	return myCollection.books().empty();
}

shared_ptr<DBAuthor> CollectionModel::authorByParagraphIndex(int num) {
	if ((num < 0) || ((int)paragraphsNumber() <= num)) {
		return 0;
	}
	std::map<ZLTextParagraph*, shared_ptr<DBAuthor> >::iterator it = myParagraphToAuthor.find((*this)[num]);
	return (it != myParagraphToAuthor.end()) ? it->second : 0;
}
