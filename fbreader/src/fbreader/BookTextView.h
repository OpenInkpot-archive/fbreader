/*
 * Copyright (C) 2004-2010 Geometer Plus <contact@geometerplus.com>
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

#ifndef __BOOKTEXTVIEW_H__
#define __BOOKTEXTVIEW_H__

#include <deque>

#include "ReadingState.h"

#include <ZLOptions.h>

#include "FBView.h"

class Book;

class BookTextView : public FBView {

public:
	ZLBooleanOption ShowTOCMarksOption;

public:
	BookTextView(ZLPaintContext &context);
	~BookTextView();

	void setModel(shared_ptr<ZLTextModel> model, shared_ptr<Book> book);
	void setContentsModel(shared_ptr<ZLTextModel> contentsModel);
	void saveState();
	void saveBookmarks();

	void gotoParagraph(int num, bool end = false);
	bool canUndoPageMove();
	void undoPageMove();
	bool canRedoPageMove();
	void redoPageMove();

	void scrollToHome();

	bool _onStylusPress(int x, int y);
	bool _onStylusMove(int x, int y);
	bool _onStylusRelease(int x, int y);
	bool onStylusClick(int x, int y, int count);

	shared_ptr<ZLTextModel> myContentsModel;

private:
	typedef ReadingState Position;
	Position cursorPosition(const ZLTextWordCursor &cursor) const;
	bool pushCurrentPositionIntoStack(bool doPushSamePosition = true);
	void replaceCurrentPositionInStack();

	void preparePaintInfo();

	bool getHyperlinkInfo(const ZLTextElementRectangle &rectangle, std::string &id, std::string &type) const;

	shared_ptr<PositionIndicator> createPositionIndicator(const ZLTextPositionIndicatorInfo &info);

	void paint();

private:
	class PositionIndicatorWithLabels;

private:
	void readBookState(const Book &book);
	int readStackPos(const Book &book);
	void saveBookState(const Book &book);

private:
	shared_ptr<Book> myBook;

	typedef std::deque<Position> PositionStack;
	PositionStack myPositionStack;
	unsigned int myCurrentPointInStack;
	unsigned int myMaxStackSize;

	bool myLockUndoStackChanges;

	int myPressedX;
	int myPressedY;

	bool myStackChanged;

	typedef std::vector<std::pair<Position, int> > BookmarkVector;
	typedef std::vector<std::pair<Position, std::pair<int, std::string> > > BookmarkTextVector;
	BookmarkVector myBookmarks;
	unsigned int myCurrentBookmarkSize;
public:
	unsigned int getBookmarksSize();
	std::vector<std::pair<BookTextView::Position, std::pair<int, std::string> > > getBookmarks();
	void addBookmark();
	void removeBookmark(unsigned int idx);
	void gotoBookmark(unsigned int idx);
};

inline void BookTextView::preparePaintInfo() {
	ZLTextView::preparePaintInfo();
	saveState();
}

#endif /* __BOOKTEXTVIEW_H__ */
