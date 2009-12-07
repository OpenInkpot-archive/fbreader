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

#include "LitResAuthorizeDataParser.h"


const std::string LitResAuthorizeDataParser::ERROR_AUTHORIZATION_FAILED = "Authorization failed";
const std::string LitResAuthorizeDataParser::ERROR_INTERNAL_ERROR = "Inrernal litres.ru error";
const std::string LitResAuthorizeDataParser::ERROR_PURCHASE_NOT_ENOUGH_MONEY = "Not enough money";
const std::string LitResAuthorizeDataParser::ERROR_PURCHASE_MISSING_BOOK = "Missing book";
const std::string LitResAuthorizeDataParser::ERROR_PURCHASE_ALREADY_PURCHASED = "Already purchased";
const std::string LitResAuthorizeDataParser::ERROR_DOWNLOAD_NOT_PURCHASED = "Book was not purchased";
const std::string LitResAuthorizeDataParser::ERROR_DOWNLOAD_LIMIT_EXCEEDED = "Download limit exceeded";


static const std::string TAG_AUTHORIZATION_OK = "catalit-authorization-ok";
static const std::string TAG_AUTHORIZATION_FAILED = "catalit-authorization-failed";

static const std::string TAG_PURCHASE_OK = "catalit-purchase-ok";
static const std::string TAG_PURCHASE_FAILED = "catalit-purchase-failed";

static const std::string TAG_DOWNLOAD_FAILED = "catalit-download-failed";


LitResAuthorizeDataParser::LitResAuthorizeDataParser() {
}

void LitResAuthorizeDataParser::reset() {
	myErrorMsg.clear();
}

void LitResAuthorizeDataParser::startElementHandler(const char *tag, const char **attributes) {
	myAttributes.clear();
	while (*attributes != 0) {
		std::string name(*attributes++);
		if (*attributes == 0) {
			break;
		}
		std::string value(*attributes++);
		myAttributes.insert(std::make_pair(name, value));
	}
	processTag(tag);	
}





LitResLoginDataParser::LitResLoginDataParser(std::string &firstName, std::string &lastName, std::string &sid) : 
	myFirstName(firstName), myLastName(lastName), mySid(sid) {
}

void LitResLoginDataParser::processTag(const std::string &tag) {
	if (TAG_AUTHORIZATION_FAILED == tag) {
		setErrorMessage(ERROR_AUTHORIZATION_FAILED);
	} else if (TAG_AUTHORIZATION_OK == tag) {
		myFirstName = attributes()["first-name"];
		myLastName = attributes()["first-name"];
		mySid = attributes()["sid"];
	}
}


LitResPurchaseDataParser::LitResPurchaseDataParser(std::string &account, std::string &bookId) : 
	myAccount(account), myBookId(bookId) {
}

void LitResPurchaseDataParser::processTag(const std::string &tag) {
	if (TAG_AUTHORIZATION_FAILED == tag) {
		setErrorMessage(ERROR_AUTHORIZATION_FAILED);
	} else {
		myAccount = attributes()["account"];
		myBookId = attributes()["art"];
		if (TAG_PURCHASE_OK == tag) {
			// nop
		} else if (TAG_PURCHASE_FAILED == tag) {
			const std::string &error = attributes()["error"];
			if ("1" == error) {
				setErrorMessage(ERROR_PURCHASE_NOT_ENOUGH_MONEY);
			} else if ("2" == error) {
				setErrorMessage(ERROR_PURCHASE_MISSING_BOOK);
			} else if ("3" == error) {
				setErrorMessage(ERROR_PURCHASE_ALREADY_PURCHASED);
			} else {
				setErrorMessage(ERROR_INTERNAL_ERROR);
			}
		}
	}
}


LitResDownloadErrorDataParser::LitResDownloadErrorDataParser() {
}

void LitResDownloadErrorDataParser::processTag(const std::string &tag) {
	if (TAG_AUTHORIZATION_FAILED == tag) {
		setErrorMessage(ERROR_AUTHORIZATION_FAILED);
	} else {
		if (TAG_DOWNLOAD_FAILED == tag) {
			const std::string &error = attributes()["error"];
			if ("1" == error) {
				setErrorMessage(ERROR_DOWNLOAD_NOT_PURCHASED);
			} else if ("2" == error) {
				setErrorMessage(ERROR_DOWNLOAD_LIMIT_EXCEEDED);
			} else {
				setErrorMessage(ERROR_INTERNAL_ERROR);
			}
		}
	}
}

