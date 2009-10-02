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

#ifndef __COLLECTIONMODEL_H__
#define __COLLECTIONMODEL_H__

#include <ZLTextModel.h>
#include <ZLTextParagraph.h>

#include "../bookmodel/FBTextKind.h"
#include "../collection/BookCollection.h"

#include "../database/booksdb/DBBook.h"

class CollectionView;

class CollectionModel : public ZLTextTreeModel {

public:
	static const std::string RemoveBookImageId;
	static const std::string BookInfoImageId;
	static const std::string AuthorInfoImageId;
	static const std::string SeriesOrderImageId;
	static const std::string TagInfoImageId;
	static const std::string RemoveTagImageId;
	static const std::string StrutImageId;

public:
	CollectionModel(CollectionView &view, BookCollection &collection);
	~CollectionModel();

	shared_ptr<DBBook> bookByParagraphIndex(int num);
	const std::vector<int> &paragraphIndicesByBook(shared_ptr<DBBook> book);
	shared_ptr<DBTag> tagByParagraphIndex(int num, std::string &special);

	void update();

	void removeBook(shared_ptr<DBBook> book);

	bool empty() const;

	shared_ptr<DBAuthor> authorByParagraphIndex(int num);

private:
	void build();
	void buildOrganizedByTags(bool buildAuthorTree);
	void buildOrganizedByAuthors();

	void addBooks(bool asTree, const Books &books, ZLTextTreeParagraph *root);
	void addBooksTree(const Books &books, ZLTextTreeParagraph *root);
	void addBooksPlain(const Books &books, ZLTextTreeParagraph *root);

	void insertText(FBTextKind kind, const std::string &text);
	void insertImage(const std::string &id);

private:
	CollectionView &myView;
	BookCollection &myCollection;

	ZLImageMap myImageMap;
	std::map<ZLTextParagraph*, shared_ptr<DBBook> > myParagraphToBook;
	std::map<ZLTextParagraph*, shared_ptr<DBTag> > myParagraphToTag;
	std::map<std::string, std::vector<int> > myBookToParagraph;

	ZLTextTreeParagraph *myAllBooksParagraph;
	ZLTextTreeParagraph *myBooksWithoutTagsParagraph;

	std::map<ZLTextParagraph*, shared_ptr<DBAuthor> > myParagraphToAuthor;
};

#endif /* __COLLECTIONMODEL_H__ */
