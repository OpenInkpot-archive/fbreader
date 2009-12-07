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

#ifndef __SCROLLINGACTION_H__
#define __SCROLLINGACTION_H__

#include <ZLApplication.h>
#include <ZLBlockTreeView.h>

#include "FBReader.h"

class ScrollingAction : public ZLApplication::Action {

public:
	ScrollingAction(const FBReader::ScrollingOptions &options, ZLBlockTreeView::ScrollingMode mode, bool forward);
	bool isEnabled() const;
	bool useKeyDelay() const;
	void run();

private:
	const FBReader::ScrollingOptions &myOptions;
	const ZLBlockTreeView::ScrollingMode myMode;
	const bool myForward;
};

#endif /* __SCROLLINGACTION_H__ */
