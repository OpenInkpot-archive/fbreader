/*
 * Copyright (C) 2008-2009 Geometer Plus <contact@geometerplus.com>
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
#include <ZLDialog.h>
#include <ZLOptionEntry.h>
#include <ZLApplication.h>
#include <ZLStringUtil.h>

#include "CollectionView.h"
#include "CollectionModel.h"

#include "../options/FBOptions.h"
#include "../collection/BookCollection.h"

const std::string CollectionView::SpecialTagAllBooks = ",AllBooks,";
const std::string CollectionView::SpecialTagNoTagsBooks = ",NoTags,";

class EditOrCloneEntry : public ZLChoiceOptionEntry {

public:
	EditOrCloneEntry(const ZLResource &resource, bool &editNotClone);
	const std::string &text(int index) const;
	int choiceNumber() const;
	int initialCheckedIndex() const;
	void onAccept(int index);

private:
	const ZLResource &myResource;
	bool &myEditNotClone;
};

EditOrCloneEntry::EditOrCloneEntry(const ZLResource &resource, bool &editNotClone) : myResource(resource), myEditNotClone(editNotClone) {
}

const std::string &EditOrCloneEntry::text(int index) const {
	std::string keyName;
	switch (index) {
		case 0:
			keyName = "edit";
			break;
		case 1:
			keyName = "clone";
			break;
	}
	return myResource[keyName].value();
}

int EditOrCloneEntry::choiceNumber() const {
	return 2;
}

int EditOrCloneEntry::initialCheckedIndex() const {
	return myEditNotClone ? 0 : 1;
}

void EditOrCloneEntry::onAccept(int index) {
	myEditNotClone = (index == 0);
}

class TagNameEntry : public ZLComboOptionEntry {

public:
	TagNameEntry(const std::vector<std::string> &values, std::string &initialValue);

	const std::string &initialValue() const;
	const std::vector<std::string> &values() const;
	void onAccept(const std::string &value);

private:
	const std::vector<std::string> &myValues;
	std::string &myValue;
};

TagNameEntry::TagNameEntry(const std::vector<std::string> &values, std::string &initialValue) : ZLComboOptionEntry(true), myValues(values), myValue(initialValue) {
}

const std::string &TagNameEntry::initialValue() const {
	return myValue;
}

const std::vector<std::string> &TagNameEntry::values() const {
	return myValues;
}

void TagNameEntry::onAccept(const std::string &value) {
	myValue = value;
}

class IncludeSubtagsEntry : public ZLBooleanOptionEntry {

public:
	IncludeSubtagsEntry(bool &initialValue);

	bool initialState() const;
	void onAccept(bool state);

private:
	bool &myValue;
};

IncludeSubtagsEntry::IncludeSubtagsEntry(bool &initialValue) : myValue(initialValue) {
}

bool IncludeSubtagsEntry::initialState() const {
	return myValue;
}

void IncludeSubtagsEntry::onAccept(bool state) {
	myValue = state;
}

bool CollectionView::runEditTagInfoDialog(const bool tagIsSpecial, std::string &tagValue, 
		bool &editNotClone, bool &includeSubtags, const bool showIncludeSubtagsEntry, const bool hasBooks) {
	shared_ptr<ZLDialog> dialog = ZLDialogManager::instance().createDialog(ZLResourceKey("editTagInfoDialog"));

	ZLResourceKey editOrCloneKey("editOrClone");
	EditOrCloneEntry *editOrCloneEntry = new EditOrCloneEntry(dialog->resource(editOrCloneKey), editNotClone);
	editOrCloneEntry->setActive(!tagIsSpecial);
	dialog->addOption(editOrCloneKey, editOrCloneEntry);

	std::set<shared_ptr<DBTag> > tagSet;
	const Books &books = myCollection.books();
	for (Books::const_iterator it = books.begin(); it != books.end(); ++it) {
		const std::vector<shared_ptr<DBTag> > &bookTags = ((const DBBook &) **it).tags();
		tagSet.insert(bookTags.begin(), bookTags.end());
	}
	std::set<std::string> fullTagSet;
	for (std::set<shared_ptr<DBTag> >::const_iterator it = tagSet.begin(); it != tagSet.end(); ++it) {
		DBTag *tagPtr = &**it;
		while (true) {
			fullTagSet.insert(tagPtr->fullName());
			if (!tagPtr->hasParent()) {
				break;
			}
			tagPtr = &tagPtr->parent();
		}
	}
	std::vector<std::string> names;
	if (fullTagSet.find(tagValue) == fullTagSet.end()) {
		names.push_back(tagValue);
	}
	names.insert(names.end(), fullTagSet.begin(), fullTagSet.end());
	TagNameEntry *tagNameEntry = new TagNameEntry(names, tagValue);
	dialog->addOption(ZLResourceKey("name"), tagNameEntry);

	IncludeSubtagsEntry *includeSubtagsEntry = new IncludeSubtagsEntry(includeSubtags);
	if (showIncludeSubtagsEntry) {
		if (!hasBooks) {
			includeSubtagsEntry->setActive(false);
		}
		dialog->addOption(ZLResourceKey("includeSubtags"), includeSubtagsEntry);
	}

	dialog->addButton(ZLDialogManager::OK_BUTTON, true);
	dialog->addButton(ZLDialogManager::CANCEL_BUTTON, false);

	if (dialog->run()) {
		dialog->acceptValues();
		return true;
	} else {
		return false;
	}
}

void CollectionView::editTagInfo(shared_ptr<DBTag> tag, const std::string &special) {
	const bool tagIsSpecial = (special == SpecialTagAllBooks) || (special == SpecialTagNoTagsBooks);
	if (!tagIsSpecial && tag.isNull()) {
		return;
	}
	std::string tagValue = tagIsSpecial ? "" : tag->fullName();
	bool editNotClone = special != SpecialTagAllBooks;
	bool includeSubtags = !tagIsSpecial && myCollection.hasSubtags(tag);
	const bool showIncludeSubtagsEntry = includeSubtags;
	const bool hasBooks = myCollection.hasBooks(tag);

	while (runEditTagInfoDialog(tagIsSpecial, tagValue, editNotClone, includeSubtags, showIncludeSubtagsEntry, hasBooks)) {
		ZLStringUtil::stripWhiteSpaces(tagValue);
		if (tagValue.empty()) {
			ZLDialogManager::instance().errorBox(ZLResourceKey("tagMustBeNonEmpty"));
			continue;
		}
		/*if (tagValue.find(',') != (size_t)-1) {
			ZLDialogManager::instance().errorBox(ZLResourceKey("tagMustNotContainComma"));
			continue;
		}*/
		collectionModel().removeAllMarks();
		shared_ptr<DBTag> to = DBTag::getSubTag(tagValue);
		if (special == SpecialTagAllBooks) {
			myCollection.addTagToAllBooks(to);
		} else if (special == SpecialTagNoTagsBooks) {
			myCollection.addTagToBooksWithNoTags(to);
		} else if (editNotClone) {
			myCollection.renameTag(tag, to, includeSubtags);
		} else {
			myCollection.cloneTag(tag, to, includeSubtags);
		}
		myCollection.rebuild(true);
		myDoUpdateModel = true;
		selectBook(mySelectedBook);
		application().refreshWindow();
		break;
	}
}
