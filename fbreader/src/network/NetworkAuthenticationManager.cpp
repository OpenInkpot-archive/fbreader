/*
 * Copyright (C) 2009-2010 Geometer Plus <contact@geometerplus.com>
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

#include <ZLNetworkSSLCertificate.h>

#include "NetworkAuthenticationManager.h"

#include "NetworkItems.h"
#include "NetworkErrors.h"


NetworkAuthenticationManager::NetworkAuthenticationManager(const std::string &siteName) :
	SiteName(siteName), 
	UserNameOption(ZLCategoryKey::NETWORK, siteName, "userName", ""),
	SkipIPOption(ZLCategoryKey::NETWORK, siteName, "skipIP", false) {
}

NetworkAuthenticationManager::~NetworkAuthenticationManager() {
}

std::string NetworkAuthenticationManager::networkBookId(const NetworkBookItem &) {
	return "";
}

NetworkItem::URLType NetworkAuthenticationManager::downloadLinkType(const NetworkBookItem &) {
	return NetworkItem::URL_NONE;
}

bool NetworkAuthenticationManager::needsInitialization() {
	return false;
}

std::string NetworkAuthenticationManager::initialize() {
	return NetworkErrors::errorMessage(NetworkErrors::ERROR_UNSUPPORTED_OPERATION);
}

bool NetworkAuthenticationManager::needPurchase(const NetworkBookItem &) {
	return true;
}

std::string NetworkAuthenticationManager::purchaseBook(NetworkBookItem &) {
	return NetworkErrors::errorMessage(NetworkErrors::ERROR_UNSUPPORTED_OPERATION);
}

std::string NetworkAuthenticationManager::downloadLink(const NetworkBookItem &) {
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

bool NetworkAuthenticationManager::registrationSupported() {
	return false;
}

std::string NetworkAuthenticationManager::registerUser(const std::string &, const std::string &, const std::string &) {
	return NetworkErrors::errorMessage(NetworkErrors::ERROR_UNSUPPORTED_OPERATION);
}

bool NetworkAuthenticationManager::passwordRecoverySupported() {
	return false;
}

std::string NetworkAuthenticationManager::recoverPassword(const std::string &) {
	return NetworkErrors::errorMessage(NetworkErrors::ERROR_UNSUPPORTED_OPERATION);
}

const ZLNetworkSSLCertificate &NetworkAuthenticationManager::certificate() {
	return ZLNetworkSSLCertificate::NULL_CERTIFICATE;
}
