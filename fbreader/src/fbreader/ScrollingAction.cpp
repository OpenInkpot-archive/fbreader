/*
 * Copyright (C) 2004-2009 Geometer Plus <contact@geometerplus.com>
 * Copyright (C) 2008,2009 Alexander Kerner <lunohod@openinkpot.org>
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

#include <ZLTextView.h>
#include <ZLBlockTreeView.h>

#include "FBReader.h"
#include "ScrollingAction.h"

#include "FBReaderActions.h"
#include "ContentsView.h"
#include "BookTextView.h"
#include "../bookmodel/BookModel.h"

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

extern bool turbo;

#ifndef FBIO_WAITFORVSYNC
#define FBIO_WAITFORVSYNC       _IOW('F', 0x20, uint32_t)
#endif
#define EINK_APOLLOFB_IOCTL_SET_AUTOREDRAW _IOW('F', 0x21, unsigned int)
#define EINK_APOLLOFB_IOCTL_FORCE_REDRAW _IO('F', 0x22)
#define EINK_APOLLOFB_IOCTL_SHOW_PREVIOUS _IO('F', 0x23)

ScrollingAction::ScrollingAction(
	ZLTextView::ScrollingMode textScrollingMode,
	ZLBlockTreeView::ScrollingMode blockScrollingMode,
	bool forward
) : myTextScrollingMode(textScrollingMode), myBlockScrollingMode(blockScrollingMode), myForward(forward) {
}

int ScrollingAction::scrollingDelay() const {
	return 0;
}

bool ScrollingAction::isEnabled() const {
	return true;
}

bool ScrollingAction::useKeyDelay() const {
	return false;
}

void ScrollingAction::run() {
	static int buffer = 0;
	FBReader &fbreader = FBReader::Instance();
	shared_ptr<ZLView> view = fbreader.currentView();
	int delay = fbreader.myLastScrollingTime.millisecondsTo(ZLTime());
	if (view.isNull() ||
			(delay >= 0 && delay < scrollingDelay())) {
		return;
	}

	if (view->typeId() == ZLTextView::TYPE_ID) {
		if(fbreader.mode() == FBReader::HYPERLINK_NAV_MODE
				|| fbreader.mode() == FBReader::DICT_MODE) {
			if(buffer != 0) {
				int x;
				x = open("/dev/fb0", O_NONBLOCK);
				ioctl(x, FBIO_WAITFORVSYNC);
				ioctl(x, EINK_APOLLOFB_IOCTL_SHOW_PREVIOUS);
				close(x);
				buffer = 0;
			}
		}

		if(fbreader.mode() == FBReader::HYPERLINK_NAV_MODE) {
			if(myForward)
				fbreader.highlightNextLink();
			else
				fbreader.highlightPrevLink();
			return;
		}

		if(fbreader.mode() == FBReader::DICT_MODE)
			return;

		if(turbo && myForward && (buffer != 0)) {
			int x;
			x = open("/dev/fb0", O_NONBLOCK);
			ioctl(x, FBIO_WAITFORVSYNC);
			if(buffer == 2)
				ioctl(x, EINK_APOLLOFB_IOCTL_FORCE_REDRAW);
			close(x);
		}

		if(fbreader.mode() == FBReader::BOOK_TEXT_MODE) {
			// jump to next section
			ZLTextWordCursor endC = fbreader.bookTextView().endCursor();
			if(endC.isNull())
				return;
			if(myForward
					&& endC.paragraphCursor().isLast()
					&& endC.isEndOfParagraph()) {
				fbreader.doAction(ActionCode::GOTO_NEXT_TOC_SECTION);
				buffer = 0;
				return;
			}

			// jump to previous section
			ZLTextWordCursor startC = fbreader.bookTextView().startCursor();
			ContentsView &contentsView = (ContentsView&)*fbreader.myContentsView;
			const ContentsModel &contentsModel = (const ContentsModel&)*contentsView.model();
			size_t current = contentsView.currentTextViewParagraph(false);
			if(!myForward
					&& !startC.isNull()
					&& startC.isStartOfParagraph()
					&& startC.paragraphCursor().isFirst()
					&& (current > 0)) {

				if(current == -1 || contentsModel.reference((const ZLTextTreeParagraph*)contentsModel[current]) == -1) {
					buffer = 0;
					return;
				}

				fbreader.doAction(ActionCode::GOTO_PREVIOUS_TOC_SECTION);
				fbreader.bookTextView().scrollToEndOfText();
				buffer = 0;
				return;
			}
		}


		if(turbo && myForward && (buffer != 0)) {
			int x;
			x = open("/dev/fb0", O_NONBLOCK);
			ioctl(x, FBIO_WAITFORVSYNC);
			ioctl(x, EINK_APOLLOFB_IOCTL_SET_AUTOREDRAW, 0);
			close(x);
		}

		if(turbo && !myForward && (buffer != 0)) {
			((ZLTextView&)*view).scrollPage(myForward, myTextScrollingMode, textOptionValue());
			FBReader::Instance().refreshWindow();
			buffer = 0;
		}

		((ZLTextView&)*view).scrollPage(myForward, myTextScrollingMode, textOptionValue());
		FBReader::Instance().refreshWindow();

		if(turbo && myForward) {
			if(buffer == 0) {
				buffer = 1;
				fbreader.doAction(ActionCode::PAGE_SCROLL_FORWARD);
			} else {
				buffer = 2;
				int x;
				x = open("/dev/fb0", O_NONBLOCK);
				ioctl(x, FBIO_WAITFORVSYNC);
				ioctl(x, EINK_APOLLOFB_IOCTL_SET_AUTOREDRAW, 1);
				close(x);
			}
		}
	} else if (view->typeId() == ZLBlockTreeView::TYPE_ID) {
		((ZLBlockTreeView&)*view).scroll(myBlockScrollingMode, !myForward);
	}
	fbreader.myLastScrollingTime = ZLTime();
}

LineScrollingAction::LineScrollingAction(bool forward) : ScrollingAction(ZLTextView::SCROLL_LINES, ZLBlockTreeView::ITEM, forward) {
}

int LineScrollingAction::scrollingDelay() const {
	return FBReader::Instance().KeyScrollingDelayOption.value();
}

size_t LineScrollingAction::textOptionValue() const {
	return FBReader::Instance().LinesToScrollOption.value();
}

PageScrollingAction::PageScrollingAction(bool forward) : ScrollingAction(ZLTextView::KEEP_LINES, ZLBlockTreeView::PAGE, forward) {
}

int PageScrollingAction::scrollingDelay() const {
	return FBReader::Instance().KeyScrollingDelayOption.value();
}

size_t PageScrollingAction::textOptionValue() const {
	return FBReader::Instance().LinesToKeepOption.value();
}

MouseWheelScrollingAction::MouseWheelScrollingAction(bool forward) : ScrollingAction(ZLTextView::SCROLL_LINES, ZLBlockTreeView::ITEM, forward) {
}

size_t MouseWheelScrollingAction::textOptionValue() const {
	return 1;
}

TapScrollingAction::TapScrollingAction(bool forward) : ScrollingAction(ZLTextView::KEEP_LINES, ZLBlockTreeView::NONE, forward) {
}

size_t TapScrollingAction::textOptionValue() const {
	return FBReader::Instance().LinesToKeepOption.value();
}

bool TapScrollingAction::isEnabled() const {
	return FBReader::Instance().EnableTapScrollingOption.value();
}
