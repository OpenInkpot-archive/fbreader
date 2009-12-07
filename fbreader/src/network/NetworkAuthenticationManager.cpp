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

#include "NetworkAuthenticationManager.h"

#include "NetworkLibraryItems.h"
#include "NetworkErrors.h"


NetworkAuthenticationManager::NetworkAuthenticationManager(const std::string &siteName) :
	SiteName(siteName), 
	UserNameOption(ZLCategoryKey::NETWORK, siteName, "userName", ""),
	SkipIPOption(ZLCategoryKey::NETWORK, siteName, "skipIP", false) {
}

NetworkAuthenticationManager::~NetworkAuthenticationManager() {
}

std::string NetworkAuthenticationManager::networkBookId(const NetworkLibraryBookItem &) {
	return "";
}

NetworkLibraryBookItem::URLType NetworkAuthenticationManager::downloadLinkType(const NetworkLibraryBookItem &) {
	return NetworkLibraryBookItem::LINK_HTTP;
}

bool NetworkAuthenticationManager::needsInitialization() {
	return false;
}

std::string NetworkAuthenticationManager::initialize() {
	return NetworkErrors::errorMessage(NetworkErrors::ERROR_UNSUPPORTED_OPERATION);
}

bool NetworkAuthenticationManager::needPurchase(const NetworkLibraryBookItem &) {
	return false;
}

std::string NetworkAuthenticationManager::purchaseBook(NetworkLibraryBookItem &) {
	return NetworkErrors::errorMessage(NetworkErrors::ERROR_UNSUPPORTED_OPERATION);
}

std::string NetworkAuthenticationManager::downloadLink(const NetworkLibraryBookItem &) {
	return "";
}

std::string NetworkAuthenticationManager::refillAccountLink() {
	return "";
}

std::string NetworkAuthenticationManager::currentAccount() {
	return "";
}

bool NetworkAuthenticationManager::skipIPSupported() {
	return false;
}
