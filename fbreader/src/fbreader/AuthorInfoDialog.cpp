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

#include <ZLDialogManager.h>
#include <ZLOptionsDialog.h>
#include <ZLOptionEntry.h>
//#include <ZLFile.h>
//#include <ZLLanguageList.h>
//#include <ZLStringUtil.h>

//#include <optionEntries/ZLSimpleOptionEntry.h>

#include "AuthorInfoDialog.h"

#include "../database/booksdb/BooksDBUtil.h"


class AuthorNameEntry : public ZLComboOptionEntry {
public:
	AuthorNameEntry(AuthorInfoDialog &dialog);

	const std::string &initialValue() const;
	const std::vector<std::string> &values() const;
	void onAccept(const std::string &value);
	void onValueSelected(int index);
//	bool useOnValueEdited() const;
//	void onValueEdited(const std::string &value);

private:
	AuthorInfoDialog &myInfoDialog;
	mutable std::vector<std::string> myValues;
	std::string myValue;
	int myIndex;

friend class AuthorSortKeyEntry;
};



class AuthorSortKeyEntry : public ZLStringOptionEntry {
public:
	AuthorSortKeyEntry(AuthorInfoDialog &dialog);

	const std::string &initialValue() const;
	void onAccept(const std::string &value);

private:
	AuthorInfoDialog &myInfoDialog;
};





inline AuthorNameEntry::AuthorNameEntry(AuthorInfoDialog &dialog) : ZLComboOptionEntry(true), myInfoDialog(dialog), myIndex(-1) {}

const std::string &AuthorNameEntry::initialValue() const {
	return myInfoDialog.myAuthor->name();
}

void AuthorNameEntry::onAccept(const std::string &value) {
	myInfoDialog.myNewName = value;
}

const std::vector<std::string> &AuthorNameEntry::values() const {
	if (myValues.empty()) {
		const DBAuthor &initial = *myInfoDialog.myAuthor;
		bool addInitial = true;
		const std::vector<shared_ptr<DBAuthor> > &authors = myInfoDialog.myCollection.authors();
		for (std::vector<shared_ptr<DBAuthor> >::const_iterator it = authors.begin(); it != authors.end(); ++it) {
			const DBAuthor &author = **it;
			if (addInitial && initial == author) {
				addInitial = false;
			}
			myValues.push_back(author.name());
		}
		if (addInitial) {
			myValues.push_back(initial.name());
		}
	}
	return myValues;
}

void AuthorNameEntry::onValueSelected(int index) {
	myIndex = index;
	myInfoDialog.mySortKey->resetView();
}

/*bool AuthorNameEntry::useOnValueEdited() const { 
	return true; 
}*/

/*void AuthorNameEntry::onValueEdited(const std::string &) {
	myIndex = -1;
}*/





AuthorSortKeyEntry::AuthorSortKeyEntry(AuthorInfoDialog &dialog) : myInfoDialog(dialog) {}

const std::string &AuthorSortKeyEntry::initialValue() const {
	const int index = myInfoDialog.myName->myIndex;
	const std::vector<shared_ptr<DBAuthor> > &authors = myInfoDialog.myCollection.authors();
	if (index < 0 || index >= (int) authors.size()) {
		return myInfoDialog.myAuthor->sortKey();
	} else {
		return authors[index]->sortKey();
	}
}

void AuthorSortKeyEntry::onAccept(const std::string &value) {
	myInfoDialog.myNewSortKey = value;
}





class AuthorInfoApplyAction : public ZLRunnable {

public:
	AuthorInfoApplyAction(AuthorInfoDialog &dialog);
	void run();

private:
	AuthorInfoDialog &myInfoDialog;
};

AuthorInfoApplyAction::AuthorInfoApplyAction(AuthorInfoDialog &dialog) : myInfoDialog(dialog) {}

void AuthorInfoApplyAction::run() {
	shared_ptr<DBAuthor> newAuthor = DBAuthor::create(myInfoDialog.myNewName, myInfoDialog.myNewSortKey);
	if (newAuthor.isNull()) {
		newAuthor = new DBAuthor();
	}
	myInfoDialog.myCollection.updateAuthor(myInfoDialog.myAuthor, newAuthor);
}



AuthorInfoDialog::AuthorInfoDialog(BookCollection &collection, shared_ptr<DBAuthor> author) : 
	myAuthor(author),
	myCollection(collection) {
	myDialog = ZLDialogManager::instance().createOptionsDialog(ZLResourceKey("AuthorInfoDialog"), new AuthorInfoApplyAction(*this));

	ZLDialogContent &commonTab = myDialog->createTab(ZLResourceKey("Common"));

	myName = new AuthorNameEntry(*this);
	mySortKey = new AuthorSortKeyEntry(*this);

	commonTab.addOption(ZLResourceKey("name"), myName);
	commonTab.addOption(ZLResourceKey("sortKey"), mySortKey);
}

