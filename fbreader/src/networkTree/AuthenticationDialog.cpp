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

#include <ZLDialog.h>
#include <ZLDialogManager.h>
#include <ZLOptionsDialog.h>
#include <ZLOptionEntry.h>

#include <optionEntries/ZLSimpleOptionEntry.h>


#include "AuthenticationDialog.h"
#include "NetworkOperationRunnable.h"
#include "UserNamesEntry.h"

#include "../network/NetworkAuthenticationManager.h"

class PasswordOptionEntry : public ZLPasswordOptionEntry {

public:
	PasswordOptionEntry(std::string &password);

	virtual const std::string &initialValue() const;
	virtual void onAccept(const std::string &value);

private:
	std::string &myPassword;
};

PasswordOptionEntry::PasswordOptionEntry(std::string &password) : myPassword(password) {
}

const std::string &PasswordOptionEntry::initialValue() const {
	static const std::string _empty;
	return _empty;
}

void PasswordOptionEntry::onAccept(const std::string &value) {
	myPassword = value;
}

AuthenticationDialog::AuthenticationDialog(NetworkAuthenticationManager &mgr, const std::string &errorMessage, std::string &password) {
	myDialog = ZLDialogManager::Instance().createDialog(ZLResourceKey("AuthenticationDialog"));

	if (!errorMessage.empty()) {
		myDialog->addOption("", "", new ZLSimpleStaticTextOptionEntry(errorMessage));
	}

	myDialog->addOption(ZLResourceKey("login"), new UserNamesEntry(myUserList, mgr.UserNameOption));
	myDialog->addOption(ZLResourceKey("password"), new PasswordOptionEntry(password));
	if (mgr.skipIPSupported()) {
		myDialog->addOption(ZLResourceKey("skipIP"), mgr.SkipIPOption);
	}

	myDialog->addButton(ZLDialogManager::OK_BUTTON, true);
	myDialog->addButton(ZLDialogManager::CANCEL_BUTTON, false);
}

bool AuthenticationDialog::runDialog(NetworkAuthenticationManager &mgr, const std::string &errorMessage, std::string &password) {
	AuthenticationDialog dlg(mgr, errorMessage, password);
	if (dlg.dialog().run()) {
		dlg.dialog().acceptValues();
		return true;
	}
	return false;
}

bool AuthenticationDialog::run(NetworkAuthenticationManager &mgr) {
	std::string errorMessage;
	while (true) {
		std::string password;
		if (!runDialog(mgr, errorMessage, password)) {
			mgr.logOut();
			return false;
		}

		if (mgr.UserNameOption.value().empty()) {
			const ZLResource &resource = ZLResource::resource("dialog")["AuthenticationDialog"];
			errorMessage = resource["loginIsEmpty"].value();
			continue;
		}

		AuthoriseRunnable authoriser(mgr, password);
		authoriser.executeWithUI();
		if (authoriser.hasErrors()) {
			errorMessage = authoriser.errorMessage();
			mgr.logOut();
		} else {
			InitializeAuthenticationManagerRunnable initializer(mgr);
			initializer.executeWithUI();
			if (initializer.hasErrors()) {
				errorMessage = initializer.errorMessage();
				mgr.logOut();
			} else {
				return true;
			}
		}
	}
}

