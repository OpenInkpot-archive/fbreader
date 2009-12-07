/*
 * Copyright (C) 2009 Geometer Plus <contact@geometerplus.com>
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

#include <ZLOptions.h>
#include <ZLStringUtil.h>

#include "UserList.h"


static const unsigned int MAX_USER_NAMES = 10;

static const std::string USER_LIST = "userList";
static const std::string USER = "user";

UserList::UserList() {
	for (unsigned int i = 0; i < MAX_USER_NAMES; ++i) {
		std::string userOptionName(USER);
		ZLStringUtil::appendNumber(userOptionName, i);
		std::string userName = ZLStringOption(ZLCategoryKey::NETWORK, USER_LIST, userOptionName, "").value();
		if (!userName.empty()) {
			myUserNames.push_back(userName);
		}
	}
	if (myUserNames.empty()) {
		myUserNames.push_back(std::string());
	}
}

UserList::~UserList() {
	for (unsigned int i = 0; i < MAX_USER_NAMES; ++i) {
		std::string userOptionName(USER);
		ZLStringUtil::appendNumber(userOptionName, i);
		ZLStringOption userOption(ZLCategoryKey::NETWORK, USER_LIST, userOptionName, "");
		if (i < myUserNames.size()) {
			userOption.setValue(myUserNames[i]);
		} else {
			userOption.setValue("");
		}		
	}
}

const std::vector<std::string> &UserList::users() const {
	return myUserNames;
}

void UserList::addUser(const std::string &user) {
	std::vector<std::string>::iterator it;
	while ((it = std::find(myUserNames.begin(), myUserNames.end(), user)) != myUserNames.end()) {
		myUserNames.erase(it);
	}
	myUserNames.insert(myUserNames.begin(), user);
	if (myUserNames.size() > MAX_USER_NAMES) {
		myUserNames.erase(myUserNames.begin() + MAX_USER_NAMES, myUserNames.end());
	}
}

