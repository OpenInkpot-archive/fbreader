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

#include <ZLTextView.h>
#include <ZLBlockTreeView.h>

#include "FBReader.h"
#include "ScrollingAction.h"

ScrollingAction::ScrollingAction(const FBReader::ScrollingOptions &options, ZLBlockTreeView::ScrollingMode mode, bool forward) : myOptions(options), myMode(mode), myForward(forward) {
}

bool ScrollingAction::isEnabled() const {
	const FBReader &fbreader = FBReader::Instance();
	return
		(&myOptions != &fbreader.TapScrollingOptions) ||
		fbreader.EnableTapScrollingOption.value();
}

bool ScrollingAction::useKeyDelay() const {
	return false;
}

void ScrollingAction::run() {
	FBReader &fbreader = FBReader::Instance();
	int delay = fbreader.myLastScrollingTime.millisecondsTo(ZLTime());
	shared_ptr<ZLView> view = fbreader.currentView();
	if (view.isNull() ||
			(delay >= 0 && delay < myOptions.DelayOption.value())) {
		return;
	}

	if (view->typeId() == ZLTextView::TYPE_ID) {
		ZLTextView::ScrollingMode oType = (ZLTextView::ScrollingMode)myOptions.ModeOption.value();
		unsigned int oValue = 0;
		switch (oType) {
			case ZLTextView::KEEP_LINES:
				oValue = myOptions.LinesToKeepOption.value();
				break;
			case ZLTextView::SCROLL_LINES:
				oValue = myOptions.LinesToScrollOption.value();
				break;
			case ZLTextView::SCROLL_PERCENTAGE:
				oValue = myOptions.PercentToScrollOption.value();
				break;
			default:
				break;
		}
		((ZLTextView&)*view).scrollPage(myForward, oType, oValue);
		FBReader::Instance().refreshWindow();
	} else if (view->typeId() == ZLBlockTreeView::TYPE_ID) {
		((ZLBlockTreeView&)*view).scroll(myMode, !myForward);
	}
	fbreader.myLastScrollingTime = ZLTime();
}
