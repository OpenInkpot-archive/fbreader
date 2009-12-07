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
#include <ZLOptionsDialog.h>
#include <ZLStringUtil.h>
#include <ZLFile.h>

#include "CollectionView.h"
#include "CollectionModel.h"
#include "FBReader.h"
#include "FBReaderActions.h"
#include "BookInfoDialog.h"
#include "AuthorInfoDialog.h"

#include "../database/booksdb/BooksDB.h"
#include "../database/booksdb/BooksDBUtil.h"

class RebuildCollectionRunnable : public ZLRunnable {

public:
	RebuildCollectionRunnable(CollectionView &view) : myView(view) {}
	void run() {
		if (myView.myDoSynchronizeCollection) {
			if (!myView.collection().synchronize()) {
				return;
			}
		}
		myView.collection().rebuild(true);
		myView.myDoUpdateModel = true;
		myView.collection().authors();
	}

private:
	CollectionView &myView;
};

CollectionView::CollectionView(FBReader &reader, shared_ptr<ZLPaintContext> context) : FBView(reader, context),
	ShowAllBooksTagOption(ZLCategoryKey::LOOK_AND_FEEL, "Library", "ShowAllBooksTag", false),
	myDoSynchronizeCollection(false),
	myDoUpdateModel(true) {
	setModel(new CollectionModel(*this, myCollection), "");
	myOrganizeByTags = organizeByTags();
	myShowAllBooksList = ShowAllBooksTagOption.value();
}

CollectionView::~CollectionView() {
	setModel(0, "");
}

void CollectionView::synchronizeModel() {
	myDoSynchronizeCollection = true;
}

const std::string &CollectionView::caption() const {
	return ZLResource::resource("library")["caption"].value();
}

void CollectionView::selectBook(shared_ptr<DBBook> book) {
	mySelectedBook = book;

	if (myDoUpdateModel) {
		shared_ptr<ZLTextModel> oldModel = model();
		setModel(0, "");
		((CollectionModel&)*oldModel).update();
		setModel(oldModel, "");
		myDoUpdateModel = false;
	}

	if (!book.isNull()) {
		collectionModel().removeAllMarks();
		const std::vector<int> &toSelect = collectionModel().paragraphIndicesByBook(book);
		for (std::vector<int>::const_iterator it = toSelect.begin(); it != toSelect.end(); ++it) {
			highlightParagraph(*it);
		}
		if (!toSelect.empty()) {
			preparePaintInfo();
			gotoParagraph(toSelect[toSelect.size() - 1]);
			scrollPage(false, ZLTextView::SCROLL_PERCENTAGE, 40);
		}
	}
}

void CollectionView::paint() {
	if (myDoSynchronizeCollection) {
		myDoSynchronizeCollection = false;
		RebuildCollectionRunnable runnable(*this);
		ZLDialogManager::instance().wait(ZLResourceKey("loadingBookList"), runnable);
	}
	if ((myOrganizeByTags != organizeByTags()) ||
			(myShowAllBooksList != ShowAllBooksTagOption.value())) {
		myOrganizeByTags = organizeByTags();
		myShowAllBooksList = ShowAllBooksTagOption.value();
		myDoUpdateModel = true;
	}
	if (myDoUpdateModel) {
		shared_ptr<ZLTextModel> oldModel = model();
		setModel(0, "");
		((CollectionModel&)*oldModel).update();
		setModel(oldModel, "");
		myDoUpdateModel = false;
		selectBook(mySelectedBook);
	}
	FBView::paint();
}

bool CollectionView::_onStylusMove(int x, int y) {
	const ZLTextElementArea *imageArea = elementByCoordinates(x, y);
	if ((imageArea != 0) && (imageArea->Kind == ZLTextElement::IMAGE_ELEMENT)) {
		fbreader().setHyperlinkCursor(true);
		return true;
	}

	int index = paragraphIndexByCoordinates(x, y);
	if (index != -1) {
		shared_ptr<DBBook> book = collectionModel().bookByParagraphIndex(index);
		if (!book.isNull()) {
			fbreader().setHyperlinkCursor(true);
			return true;
		}
	}

	fbreader().setHyperlinkCursor(false);
	return false;
}

bool CollectionView::_onStylusPress(int x, int y) {
	fbreader().setHyperlinkCursor(false);

	const ZLTextElementArea *imageArea = elementByCoordinates(x, y);
	if ((imageArea != 0) && (imageArea->Kind == ZLTextElement::IMAGE_ELEMENT)) {
		ZLTextWordCursor cursor = startCursor();
		cursor.moveToParagraph(imageArea->ParagraphIndex);
		cursor.moveTo(imageArea->ElementIndex, 0);
		const ZLTextElement &element = cursor.element();
		if (element.kind() != ZLTextElement::IMAGE_ELEMENT) {
			return false;
		}
		const ZLTextImageElement &imageElement = (ZLTextImageElement&)element;

		const std::string &id = imageElement.id();

		if (id == CollectionModel::BookInfoImageId) {
			editBookInfo(collectionModel().bookByParagraphIndex(imageArea->ParagraphIndex));
			return true;
		} else if (id == CollectionModel::RemoveBookImageId) {
			removeBook(collectionModel().bookByParagraphIndex(imageArea->ParagraphIndex));
			return true;
		} else if (id == CollectionModel::RemoveTagImageId) {
			std::string special;
			shared_ptr<DBTag> tag = collectionModel().tagByParagraphIndex(imageArea->ParagraphIndex, special);
			removeTag(tag, special);
			return true;
		} else if (id == CollectionModel::TagInfoImageId) {
			std::string special;
			shared_ptr<DBTag> tag = collectionModel().tagByParagraphIndex(imageArea->ParagraphIndex, special);
			editTagInfo(tag, special);
			return true;
		} else if (id == CollectionModel::AuthorInfoImageId) {
			editAuthorInfo(collectionModel().authorByParagraphIndex(imageArea->ParagraphIndex));
			return true;
		} else {
			return false;
		}
	}

	int index = paragraphIndexByCoordinates(x, y);
	if (index == -1) {
		return false;
	}

	shared_ptr<DBBook> book = collectionModel().bookByParagraphIndex(index);
	if (!book.isNull()) {
		fbreader().openBook(book);
		fbreader().showBookTextView();
		return true;
	}

	return false;
}

void CollectionView::editBookInfo(shared_ptr<DBBook> book) {
	if (!book.isNull() && BookInfoDialog(myCollection, book).dialog().run()) {
		myCollection.rebuild(false);
		myDoUpdateModel = true;
		selectBook(book);
		application().refreshWindow();
	}
}

bool CollectionView::removeBookDialog(shared_ptr<DBBook> book, bool &removeFile) const {
	bool canRemoveFile = BooksDBUtil::canRemoveFile(book->fileName());
	bool canRemoveLink = myCollection.isBookExternal(book);

	if (!canRemoveFile && !canRemoveLink) {
		return false;
	}

	ZLResourceKey boxKey("removeBookBox");
	const ZLResource &msgResource = ZLResource::resource("dialog")[boxKey];

	if (!canRemoveLink) {
		const std::string message = ZLStringUtil::printf(msgResource["deleteFile"].value(), book->title());
		if (ZLDialogManager::instance().questionBox(boxKey, message, ZLDialogManager::YES_BUTTON, ZLDialogManager::NO_BUTTON) == 0) {
			removeFile = true;
			return true;
		}
		return false;
	} else if (!canRemoveFile) {
		const std::string message = ZLStringUtil::printf(ZLDialogManager::dialogMessage(boxKey), book->title());
		if (ZLDialogManager::instance().questionBox(boxKey, message, ZLDialogManager::YES_BUTTON, ZLDialogManager::NO_BUTTON) == 0) {
			removeFile = false;
			return true;
		}
		return false;
	}

	ZLResourceKey removeFileKey("removeFile");
	ZLResourceKey removeLinkKey("removeLink");

	const std::string message = ZLStringUtil::printf(msgResource["deleteMode"].value(), book->title());
	int res = ZLDialogManager::instance().questionBox(boxKey, message, removeLinkKey, removeFileKey, ZLDialogManager::CANCEL_BUTTON);
	if (res != 2) {
		removeFile = (res == 1);
		return true;
	}
	return false;
}

void CollectionView::removeBook(shared_ptr<DBBook> book) {
	if (book.isNull()) {
		return;
	}

	if (book == mySelectedBook) {
		mySelectedBook = 0;
	}

	CollectionModel &cModel = collectionModel();
	
	bool removeFromDisk;
	if (removeBookDialog(book, removeFromDisk)) {
		cModel.removeAllMarks();
		BooksDB::instance().deleteFromBookList(*book);
		cModel.removeBook(book);
		if (removeFromDisk) {
			ZLFile physFile(ZLFile(book->fileName()).physicalFilePath());
			if (!physFile.remove()) {
				// TODO: tell user about failure
				std::cerr << "UNABLE TO DELETE " << physFile.path() << std::endl;
			}
		}
		if (cModel.paragraphsNumber() == 0) {
			setStartCursor(0);
		} else {
			size_t index = startCursor().paragraphCursor().index();
			if (index >= cModel.paragraphsNumber()) {
				index = cModel.paragraphsNumber() - 1;
			}
			while (!((ZLTextTreeParagraph*)cModel[index])->parent()->isOpen()) {
				--index;
			}
			gotoParagraph(index);
		}
		rebuildPaintInfo(true);
		selectBook(mySelectedBook);
		application().refreshWindow();
		myCollection.rebuild(removeFromDisk);
	}
}

void CollectionView::removeTag(shared_ptr<DBTag> tag, const std::string &special) {
	if (tag.isNull()) {
		return;
	}

	ZLResourceKey boxKey("removeTagBox");
	const std::string message =
		ZLStringUtil::printf(ZLDialogManager::dialogMessage(boxKey), tag->fullName());
	enum { REMOVE_TAG, REMOVE_SUBTREE, DONT_REMOVE } code = DONT_REMOVE;
	if (myCollection.hasSubtags(tag)) {
		if (myCollection.hasBooks(tag)) {
			switch (ZLDialogManager::instance().questionBox(boxKey, message,
								ZLResourceKey("thisOnly"),
								ZLResourceKey("withSubtags"),
								ZLDialogManager::CANCEL_BUTTON
							)) {
				case 0:
					code = REMOVE_TAG;
					break;
				case 1:
					code = REMOVE_SUBTREE;
					break;
			}
		} else {
			if (ZLDialogManager::instance().questionBox(boxKey, message,
				ZLResourceKey("withSubtags"), ZLDialogManager::CANCEL_BUTTON) == 0) {
				code = REMOVE_SUBTREE;
			}
		}
	} else {
		if (ZLDialogManager::instance().questionBox(boxKey, message,
			ZLDialogManager::YES_BUTTON, ZLDialogManager::CANCEL_BUTTON) == 0) {
			code = REMOVE_TAG;
		}
	}
	if (code != DONT_REMOVE) {
		collectionModel().removeAllMarks();
		myCollection.removeTag(tag, code == REMOVE_SUBTREE);
		myCollection.rebuild(true);
		myDoUpdateModel = true;
		selectBook(mySelectedBook);
		application().refreshWindow();
	}
}

CollectionModel &CollectionView::collectionModel() {
	return (CollectionModel&)*model();
}

void CollectionView::openWithBook(shared_ptr<DBBook> book) {
	RebuildCollectionRunnable runnable(*this);
	ZLDialogManager::instance().wait(ZLResourceKey("loadingBookList"), runnable);
	if (book != 0) {
		selectBook(book);
	}
}

bool CollectionView::organizeByTags() const {
	return ZLStringOption(ZLCategoryKey::LOOK_AND_FEEL, "ToggleButtonGroup", "booksOrder", "").value() == ActionCode::ORGANIZE_BOOKS_BY_TAG;
}

bool CollectionView::hasContents() const {
	return !((CollectionModel&)*model()).empty();
}

void CollectionView::editAuthorInfo(shared_ptr<DBAuthor> author) {
	if (!author.isNull() && AuthorInfoDialog(myCollection, author).dialog().run()) {
		myCollection.rebuild(false);
		myDoUpdateModel = true;
		selectBook(mySelectedBook);
		application().refreshWindow();
	}
}

