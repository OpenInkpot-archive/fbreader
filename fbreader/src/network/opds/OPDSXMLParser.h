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

#ifndef __OPDSXMLPARSER_H__
#define __OPDSXMLPARSER_H__

#include <ZLXMLReader.h>

#include "OPDSMetadata.h"
#include "OPDSFeedReader.h"


class OPDSXMLParser : public ZLXMLReader {

public:
	OPDSXMLParser(shared_ptr<OPDSFeedReader> feedReader);

private:
	void startElementHandler(const char *tag, const char **attributes);
	void endElementHandler(const char *tag);
	void characterDataHandler(const char *text, size_t len);
	bool processNamespaces() const;
	void namespaceListChangedHandler();

private:
	enum State {
		START, 
		FEED, F_ENTRY, F_ID, F_LINK, F_CATEGORY, F_TITLE, F_UPDATED, F_AUTHOR,
		FA_NAME, FA_URI, FA_EMAIL,
		FE_AUTHOR, FE_ID, FE_CATEGORY, FE_LINK, FE_PUBLISHED, FE_SUMMARY, FE_TITLE, FE_UPDATED, FE_DC_LANGUAGE, FE_DC_ISSUED, FE_DC_PUBLISHER,
		FEA_NAME, FEA_URI, FEA_EMAIL,
		FEED_CONTENT,
	};

	void processState(const std::string &tag, bool closed);
	State getNextState(const std::string &tag, bool closed);

private:
	bool checkAtomTag(const std::string &tag, const std::string &pattern) const;
	bool checkDCTag(const std::string &tag, const std::string &pattern) const;

	static bool checkNSTag(const std::string &tag, const std::string &nsprefix, const std::string &pattern);
	
private:
	shared_ptr<OPDSFeedReader> myFeedReader;

	std::string myBuffer;
	std::string myDCPrefix;
	std::string myAtomPrefix;

	State myState;

	std::map<std::string, std::string> myAttributes;

	shared_ptr<OPDSFeedMetadata> myFeed;
	shared_ptr<OPDSEntry> myEntry;

	shared_ptr<ATOMAuthor> myAuthor;
	shared_ptr<ATOMId> myId;
	shared_ptr<ATOMLink> myLink;
	shared_ptr<ATOMCategory> myCategory;
	shared_ptr<ATOMUpdated> myUpdated;
	shared_ptr<ATOMPublished> myPublished;
	//shared_ptr<ATOMTitle> myTitle;      // TODO: implement ATOMTextConstruct & ATOMTitle
	//shared_ptr<ATOMSummary> mySummary;  // TODO: implement ATOMTextConstruct & ATOMSummary
};

#endif /* __OPDSXMLPARSER_H__ */
