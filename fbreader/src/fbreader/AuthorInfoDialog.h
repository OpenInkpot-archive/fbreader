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

#ifndef __AUTHORINFODIALOG_H__
#define __AUTHORINFODIALOG_H__

#include <string>

#include <ZLOptionEntry.h>

#include "../collection/BookCollection.h"
#include "../database/booksdb/DBAuthor.h"

class ZLOptionsDialog;
class AuthorNameEntry;
class AuthorSortKeyEntry;


class AuthorInfoDialog {

public:
	AuthorInfoDialog(BookCollection &collection, shared_ptr<DBAuthor> author);

	ZLOptionsDialog &dialog();

private:
	shared_ptr<ZLOptionsDialog> myDialog;
	shared_ptr<DBAuthor> myAuthor;

	AuthorNameEntry *myName;
	AuthorSortKeyEntry *mySortKey;

	BookCollection &myCollection;

	std::string myNewName;
	std::string myNewSortKey;

friend class AuthorInfoApplyAction;
friend class AuthorNameEntry;
friend class AuthorSortKeyEntry;
};

inline ZLOptionsDialog &AuthorInfoDialog::dialog() { return *myDialog; }

#endif /* __AUTHORINFODIALOG_H__ */
