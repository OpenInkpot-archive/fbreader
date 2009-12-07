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

#ifndef __LITRESAUTHORIZEDATAPARSER_H__
#define __LITRESAUTHORIZEDATAPARSER_H__

#include <ZLXMLReader.h>


class LitResAuthorizeDataParser : public ZLXMLReader {

public:
	static const std::string ERROR_AUTHORIZATION_FAILED;
	static const std::string ERROR_INTERNAL_ERROR;
	static const std::string ERROR_PURCHASE_NOT_ENOUGH_MONEY;
	static const std::string ERROR_PURCHASE_MISSING_BOOK;
	static const std::string ERROR_PURCHASE_ALREADY_PURCHASED;
	static const std::string ERROR_DOWNLOAD_NOT_PURCHASED;
	static const std::string ERROR_DOWNLOAD_LIMIT_EXCEEDED;

public:
	LitResAuthorizeDataParser();

	const std::string &error() const;
	void reset();

private:
	void startElementHandler(const char *tag, const char **attributes);

protected:
	void setErrorMessage(const std::string &msg);
	std::map<std::string, std::string> &attributes();

	virtual void processTag(const std::string &tag) = 0;

private:
	std::map<std::string, std::string> myAttributes;
	std::string myErrorMsg;
};

inline const std::string &LitResAuthorizeDataParser::error() const { return myErrorMsg; }
inline void LitResAuthorizeDataParser::setErrorMessage(const std::string &msg) { myErrorMsg = msg; }
inline std::map<std::string, std::string> &LitResAuthorizeDataParser::attributes() { return myAttributes; }


class LitResLoginDataParser : public LitResAuthorizeDataParser {

public:
	LitResLoginDataParser(std::string &firstName, std::string &lastName, std::string &sid);

private:
	void processTag(const std::string &tag);

private:
	std::string &myFirstName;
	std::string &myLastName;
	std::string &mySid;
};


class LitResPurchaseDataParser : public LitResAuthorizeDataParser {

public:
	LitResPurchaseDataParser(std::string &account, std::string &bookId);

private:
	void processTag(const std::string &tag);

private:
	std::string &myAccount;
	std::string &myBookId;
};


class LitResDownloadErrorDataParser : public LitResAuthorizeDataParser {

public:
	LitResDownloadErrorDataParser();

private:
	void processTag(const std::string &tag);

};

#endif /* __LITRESAUTHORIZEDATAPARSER_H__ */
