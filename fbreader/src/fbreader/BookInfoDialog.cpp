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

#include <algorithm>
//#include <iostream>

#include <ZLDialogManager.h>
#include <ZLOptionsDialog.h>
#include <ZLOptionEntry.h>
#include <ZLFile.h>
#include <ZLLanguageList.h>
#include <ZLStringUtil.h>

#include <optionEntries/ZLStringInfoEntry.h>
#include <optionEntries/ZLSimpleOptionEntry.h>
#include <optionEntries/ZLLanguageOptionEntry.h>

#include "BookInfoDialog.h"

#include "../encodingOption/EncodingOptionEntry.h"

#include "../database/booksdb/BooksDBUtil.h"


static const unsigned AUTHOR_ENTRIES_POOL_SIZE = 64;
static const unsigned TAG_ENTRIES_POOL_SIZE = 64;


class AuthorDisplayNameEntry : public ZLComboOptionEntry {

public:
	AuthorDisplayNameEntry(BookInfoDialog &dialog, shared_ptr<DBAuthor> initialAuthor, bool &visible);

	const std::string &initialValue() const;
	const std::vector<std::string> &values() const;
	void onAccept(const std::string &value);

	bool useOnValueEdited() const;
	void onValueEdited(const std::string &value);
	void onValueSelected(int index);

private:
	void onValueChanged(const std::string &value);

private:
	BookInfoDialog &myInfoDialog;
	mutable std::vector<std::string> myValues;
	shared_ptr<DBAuthor> myCurrentAuthor;

	std::string myInitialValue;
	bool myEmpty;

friend class SeriesTitleEntry;
friend class BookInfoApplyAction;
};


class SeriesTitleEntry : public ZLComboOptionEntry {

public:
	SeriesTitleEntry(BookInfoDialog &dialog);

	const std::string &initialValue() const;
	const std::vector<std::string> &values() const;
	void onAccept(const std::string &value);

	bool useOnValueEdited() const;
	void onValueEdited(const std::string &value);
	void onValueSelected(int index);

private:
	BookInfoDialog &myInfoDialog;
	std::set<std::string> myOriginalValuesSet;
	mutable std::vector<std::string> myValues;
};


class BookNumberEntry : public ZLSpinOptionEntry {

public:
	static const int MIN_NUMBER;
	static const int MAX_NUMBER;
	
public:
	BookNumberEntry(BookInfoDialog &dialog);

	int initialValue() const;
	int minValue() const;
	int maxValue() const;
	int step() const;
	void onAccept(int value);

private:
	BookInfoDialog &myInfoDialog;
};




AuthorDisplayNameEntry::AuthorDisplayNameEntry(BookInfoDialog &dialog, shared_ptr<DBAuthor> initialAuthor, bool &visible) : 
	ZLComboOptionEntry(true), myInfoDialog(dialog), myCurrentAuthor(initialAuthor) {

	if (myCurrentAuthor.isNull()) {
		myInitialValue = "";
		myEmpty = true;
	} else {
		myInitialValue = myCurrentAuthor->name();
		myEmpty = myInitialValue.empty();
	}
	setVisible(visible || !myEmpty);
	if (visible && myEmpty) {
		visible = false;
	}
}

const std::string &AuthorDisplayNameEntry::initialValue() const {
	return myInitialValue;
}

const std::vector<std::string> &AuthorDisplayNameEntry::values() const {
	if (myValues.empty()) {
		const std::string &initial = initialValue();
		bool addInitial = true;
		const std::vector<shared_ptr<DBAuthor> > &authors = myInfoDialog.myCollection.authors();
		for (std::vector<shared_ptr<DBAuthor> >::const_iterator it = authors.begin(); it != authors.end(); ++it) {
			const std::string name = (*it)->name();
			if (addInitial && (name == initial)) {
				addInitial = false;
			}
			myValues.push_back(name);
		}
		if (addInitial) {
			myValues.push_back(initial);
		}
	}
	return myValues;
}

void AuthorDisplayNameEntry::onAccept(const std::string &value) {
	if (!isVisible() || value.empty()) {
		myCurrentAuthor = 0;
		return;
	}
	if (!myCurrentAuthor.isNull() && value == myCurrentAuthor->name()) {
		//myCurrentAuthor = myCurrentAuthor;
		return;
	}
	std::string authorName = value;
	ZLStringUtil::stripWhiteSpaces(authorName);
	myCurrentAuthor = DBAuthor::create(authorName);
}


bool AuthorDisplayNameEntry::useOnValueEdited() const {
	return true;
}

void AuthorDisplayNameEntry::onValueEdited(const std::string &value) {
	onValueChanged(value);
}

void AuthorDisplayNameEntry::onValueSelected(int index) {
	const std::vector<shared_ptr<DBAuthor> > &authors = myInfoDialog.myCollection.authors();
	myCurrentAuthor = (((size_t)index) < authors.size()) ? authors[index] : 0;
	myInfoDialog.mySeriesTitleEntry->resetView();
	onValueChanged(myValues[index]);
}

void AuthorDisplayNameEntry::onValueChanged(const std::string &value) {
	if (!myInfoDialog.myAuthorsDone || !isVisible()) {
		return;
	}

	myEmpty = value.empty();
	if (myEmpty) {
		for (unsigned i = 0; i < myInfoDialog.myAuthorEntries.size(); ++i) {
			AuthorDisplayNameEntry &entry = *myInfoDialog.myAuthorEntries[i];
			if (entry.myEmpty && entry.isVisible() && this != &entry) {
				entry.setVisible(false);
			}
		}
	} else {
		unsigned i, lastvisible = (unsigned) -1;
		for (i = 0; i < myInfoDialog.myAuthorEntries.size(); ++i) {
			AuthorDisplayNameEntry &entry = *myInfoDialog.myAuthorEntries[i];
			if (entry.isVisible()) {
				lastvisible = i;
				if (entry.myEmpty) {
					break;
				}
			}
		}
		if (i == myInfoDialog.myAuthorEntries.size()) {
			if (lastvisible + 1 < i) {
				AuthorDisplayNameEntry &entry = *myInfoDialog.myAuthorEntries[lastvisible + 1];
				entry.setVisible(true);
			}
			// else pool is over
		}
	}
}




SeriesTitleEntry::SeriesTitleEntry(BookInfoDialog &dialog) : ZLComboOptionEntry(true), myInfoDialog(dialog) {
	const std::vector<shared_ptr<DBAuthor> > &authors = myInfoDialog.myBook->authors();
	myOriginalValuesSet.insert(initialValue());
	myOriginalValuesSet.insert("");
	for (std::vector<shared_ptr<DBAuthor> >::const_iterator it = authors.begin(); it != authors.end(); ++it) {
		BooksDB::instance().collectSeriesNames(**it, myOriginalValuesSet);
	}
}

const std::string &SeriesTitleEntry::initialValue() const {
	return myInfoDialog.myBook->seriesName();
}

const std::vector<std::string> &SeriesTitleEntry::values() const {
	std::set<std::string> valuesSet(myOriginalValuesSet);

	const std::vector<shared_ptr<DBAuthor> > &authors = myInfoDialog.myBook->authors();
	for (std::vector<AuthorDisplayNameEntry *>::const_iterator it = myInfoDialog.myAuthorEntries.begin(); it != myInfoDialog.myAuthorEntries.end(); ++it) {
		shared_ptr<DBAuthor> currentAuthor = (*it)->myCurrentAuthor;
		if (!currentAuthor.isNull() && std::find(authors.begin(), authors.end(), currentAuthor) == authors.end()) {
			BooksDB::instance().collectSeriesNames(*currentAuthor, valuesSet);
		}
	}

	/*myValues.clear();
	for (std::set<std::string>::const_iterator it = valuesSet.begin(); it != valuesSet.end(); ++it) {
		myValues.push_back(*it);
	}*/
	myValues.assign(valuesSet.begin(), valuesSet.end());
	return myValues;
}

void SeriesTitleEntry::onAccept(const std::string &value) {
	myInfoDialog.myBook->setSeriesName(value);
}

void SeriesTitleEntry::onValueSelected(int index) {
	myInfoDialog.myBookNumberEntry->setVisible(index != 0);
}

bool SeriesTitleEntry::useOnValueEdited() const {
	return true;
}

void SeriesTitleEntry::onValueEdited(const std::string &value) {
	myInfoDialog.myBookNumberEntry->setVisible(!value.empty());
}


const int BookNumberEntry::MIN_NUMBER = 0;
const int BookNumberEntry::MAX_NUMBER = 100;

BookNumberEntry::BookNumberEntry(BookInfoDialog &dialog) : myInfoDialog(dialog) {
}

int BookNumberEntry::initialValue() const {
	return myInfoDialog.myBook->numberInSeries();
}

int BookNumberEntry::minValue() const {
	return MIN_NUMBER;
}

int BookNumberEntry::maxValue() const {
	return MAX_NUMBER;
}

int BookNumberEntry::step() const {
	return 1;
}

void BookNumberEntry::onAccept(int value) {
	myInfoDialog.myBook->setNumberInSeries(value);
}







class BookTitleEntry : public ZLStringOptionEntry {

public:
	BookTitleEntry(BookInfoDialog &dialog);

	const std::string &initialValue() const;
	void onAccept(const std::string &value);

private:
	BookInfoDialog &myInfoDialog;
};

BookTitleEntry::BookTitleEntry(BookInfoDialog &dialog) : myInfoDialog(dialog) {
}

const std::string &BookTitleEntry::initialValue() const {
	return myInfoDialog.myBook->title();
}

void BookTitleEntry::onAccept(const std::string &value) {
	myInfoDialog.myBook->setTitle(value);
}






class BookEncodingEntry : public AbstractEncodingEntry {

public:
	BookEncodingEntry(BookInfoDialog &dialog);

	void onAcceptValue(const std::string &value);

private:
	BookInfoDialog &myInfoDialog;
};

BookEncodingEntry::BookEncodingEntry(BookInfoDialog &dialog) : 
	AbstractEncodingEntry(dialog.myBook->encoding()), 
	myInfoDialog(dialog) {
}

void BookEncodingEntry::onAcceptValue(const std::string &value) {
	myInfoDialog.myBook->setEncoding(value);
}



class BookLanguageEntry : public ZLAbstractLanguageOptionEntry {

public:
	BookLanguageEntry(BookInfoDialog &dialog, const std::vector<std::string> &languageCodes);

	void onAcceptCode(const std::string &code);

private:
	BookInfoDialog &myInfoDialog;
};

BookLanguageEntry::BookLanguageEntry(BookInfoDialog &dialog, const std::vector<std::string> &languageCodes) : 
	ZLAbstractLanguageOptionEntry(dialog.myBook->language(), languageCodes),
	myInfoDialog(dialog) {
}

void BookLanguageEntry::onAcceptCode(const std::string &code) {
	myInfoDialog.myBook->setLanguage(code);
}





class BookTagEntry : public ZLComboOptionEntry {

public:
	BookTagEntry(BookInfoDialog &dialog, std::string initialTag, bool &visible);

	const std::string &initialValue() const;
	const std::vector<std::string> &values() const;
	void onAccept(const std::string &value);

	bool useOnValueEdited() const;
	void onValueEdited(const std::string &value);
	void onValueSelected(int index);

private:
	void onValueChanged(const std::string &value);

private:
	BookInfoDialog &myInfoDialog;
	std::string myInitialValue;
	bool myEmpty;

	mutable std::vector<std::string> myValues;
};

BookTagEntry::BookTagEntry(BookInfoDialog &dialog, std::string initialTag, bool &visible) : 
		ZLComboOptionEntry(true), myInfoDialog(dialog), myInitialValue(initialTag) {

	myEmpty = myInitialValue.empty();
	setVisible(visible || !myEmpty);
	if (visible && myEmpty) {
		visible = false;
	}
}

const std::string &BookTagEntry::initialValue() const {
	return myInitialValue;
}

const std::vector<std::string> &BookTagEntry::values() const {
	if (myValues.empty()) {
		myValues.push_back("");
		DBTag::collectTagNames(myValues);
	}
	return myValues;
}

void BookTagEntry::onAccept(const std::string &value) {
	if (isVisible() && !value.empty()) {
		myInfoDialog.myNewTags.push_back(value);
	}
}

bool BookTagEntry::useOnValueEdited() const {
	return true;
}

void BookTagEntry::onValueEdited(const std::string &value) {
	onValueChanged(value);
}

void BookTagEntry::onValueSelected(int index) {
	onValueChanged(myValues[index]);
}

void BookTagEntry::onValueChanged(const std::string &value) {
	if (!myInfoDialog.myTagsDone || !isVisible()) {
		return;
	}

	myEmpty = value.empty();
	if (myEmpty) {
		for (unsigned i = 0; i < myInfoDialog.myTagEntries.size(); ++i) {
			BookTagEntry &entry = *myInfoDialog.myTagEntries[i];
			if (entry.myEmpty && entry.isVisible() && this != &entry) {
				entry.setVisible(false);
			}
		}
	} else {
		unsigned i, lastvisible = (unsigned) -1;
		for (i = 0; i < myInfoDialog.myTagEntries.size(); ++i) {
			BookTagEntry &entry = *myInfoDialog.myTagEntries[i];
			if (entry.isVisible()) {
				lastvisible = i;
				if (entry.myEmpty) {
					break;
				}
			}
		}
		if (i == myInfoDialog.myTagEntries.size()) {
			if (lastvisible + 1 < i) {
				BookTagEntry &entry = *myInfoDialog.myTagEntries[lastvisible + 1];
				entry.setVisible(true);
			}
		}
	}
}






class BookInfoApplyAction : public ZLRunnable {

public:
	BookInfoApplyAction(BookInfoDialog &dialog);
	void run();

private:
	BookInfoDialog &myInfoDialog;
};

BookInfoApplyAction::BookInfoApplyAction(BookInfoDialog &dialog) : myInfoDialog(dialog) {}

void BookInfoApplyAction::run() {
	std::vector<shared_ptr<DBAuthor> > &authors = myInfoDialog.myBook->authors();
	authors.clear();
	for (unsigned i = 0; i < myInfoDialog.myAuthorEntries.size(); ++i) {
		shared_ptr<DBAuthor> author = myInfoDialog.myAuthorEntries[i]->myCurrentAuthor;
		if (!author.isNull() 
			&& std::find_if(authors.begin(), authors.end(), DBAuthorPredicate(author)) == authors.end()) {
			authors.push_back(author);
		}
	}
	if (authors.empty()) {
		authors.push_back( new DBAuthor() );
	}

	myInfoDialog.myBook->removeAllTags();
	for (unsigned i = 0; i < myInfoDialog.myNewTags.size(); ++i) {
		const std::string &tag = myInfoDialog.myNewTags[i];
		shared_ptr<DBTag> ptr = DBTag::getSubTag(tag);
		if (!ptr.isNull()) {
			myInfoDialog.myBook->addTag(ptr);
		}
	}

	BooksDB::instance().saveBook(myInfoDialog.myBook);
}






BookInfoDialog::BookInfoDialog(const BookCollection &collection, shared_ptr<DBBook> book) : 
		myCollection(collection), myBook(book) {

	myDialog = ZLDialogManager::instance().createOptionsDialog(ZLResourceKey("InfoDialog"), new BookInfoApplyAction(*this));

	ZLDialogContent &commonTab = myDialog->createTab(ZLResourceKey("Common"));
	commonTab.addOption(ZLResourceKey("file"), 
		new ZLStringInfoEntry(ZLFile::fileNameToUtf8(ZLFile( book->fileName() ).path()))
	);
	commonTab.addOption(ZLResourceKey("title"), new BookTitleEntry(*this));

	myEncodingEntry = new BookEncodingEntry(*this);
	myEncodingSetEntry =
		(myEncodingEntry->initialValue() != "auto") ?
		new EncodingSetEntry(*(EncodingEntry*)myEncodingEntry) : 0;
	std::vector<std::string> languageCodes = ZLLanguageList::languageCodes();
	languageCodes.push_back("de-traditional");
	myLanguageEntry = new BookLanguageEntry(*this, languageCodes);
	mySeriesTitleEntry = new SeriesTitleEntry(*this);
	myBookNumberEntry = new BookNumberEntry(*this);

	commonTab.addOption(ZLResourceKey("language"), myLanguageEntry);
	if (myEncodingSetEntry != 0) {
		commonTab.addOption(ZLResourceKey("encodingSet"), myEncodingSetEntry);
	}
	commonTab.addOption(ZLResourceKey("encoding"), myEncodingEntry);

	initAuthorEntries();

	ZLDialogContent &seriesTab = myDialog->createTab(ZLResourceKey("Series"));
	seriesTab.addOption(ZLResourceKey("seriesTitle"), mySeriesTitleEntry);
	seriesTab.addOption(ZLResourceKey("bookNumber"), myBookNumberEntry);

	mySeriesTitleEntry->onValueEdited(mySeriesTitleEntry->initialValue());
	/*
	ZLOrderOptionEntry *orderEntry = new ZLOrderOptionEntry();
	orderEntry->values().push_back("First");
	orderEntry->values().push_back("Second");
	orderEntry->values().push_back("Third");
	orderEntry->values().push_back("Fourth");
	orderEntry->values().push_back("Fifth");
	seriesTab.addOption(orderEntry);
	*/

	initTagEntries();

	FormatPlugin *plugin = PluginCollection::instance().plugin(ZLFile( book->fileName() ), false);
	if (plugin != 0) {
		myFormatInfoPage = plugin->createInfoPage(*myDialog, book->fileName());
	}
}

void BookInfoDialog::initTagEntries() {
	bool visible = true;
	const std::vector<shared_ptr<DBTag> > &tags = myBook->tags();
	myTagsDone = false;
	myTagsTab = &myDialog->createTab(ZLResourceKey("Tags"));
	for (unsigned i = 0; i < TAG_ENTRIES_POOL_SIZE; ++i) {
		std::string tag = (i < tags.size()) ? tags[i]->fullName() : "";
		BookTagEntry *entry = new BookTagEntry(*this, tag, visible);
		myTagEntries.push_back(entry);
		myTagsTab->addOption(ZLResourceKey("tags"), entry);
	}
	myTagsDone = true;
}

void BookInfoDialog::initAuthorEntries() {
	bool visible = true;
	const std::vector<shared_ptr<DBAuthor> > &authors = myBook->authors();
	myAuthorsDone = false;
	myAuthorsTab = &myDialog->createTab(ZLResourceKey("Authors"));
	for (unsigned i = 0; i < AUTHOR_ENTRIES_POOL_SIZE; ++i) {
		shared_ptr<DBAuthor> author = (i < authors.size()) ? authors[i] : 0;
		AuthorDisplayNameEntry *entry = new AuthorDisplayNameEntry(*this, author, visible);
		myAuthorEntries.push_back(entry);
		myAuthorsTab->addOption(ZLResourceKey("authorDisplayName"), entry);
	}
	myAuthorsDone = true;
}

