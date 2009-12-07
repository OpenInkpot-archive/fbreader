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

#include <ZLStringUtil.h>

#include "OPDSXMLParser.h"
#include "../../constants/XMLNamespace.h"



static const std::string TAG_FEED = "feed";
static const std::string TAG_ENTRY = "entry";
static const std::string TAG_AUTHOR = "author";
static const std::string TAG_NAME = "name";
static const std::string TAG_URI = "uri";
static const std::string TAG_EMAIL = "email";
static const std::string TAG_ID = "id";
static const std::string TAG_CATEGORY = "category";
static const std::string TAG_LINK = "link";
static const std::string TAG_PUBLISHED = "published";
static const std::string TAG_SUMMARY = "summary";
static const std::string TAG_TITLE = "title";
static const std::string TAG_UPDATED = "updated";

static const std::string DC_TAG_LANGUAGE = "language";
static const std::string DC_TAG_ISSUED = "issued";
static const std::string DC_TAG_PUBLISHER = "publisher";




OPDSXMLParser::OPDSXMLParser(shared_ptr<OPDSFeedReader> feedReader) : myFeedReader(feedReader) {
	myState = START;
}

bool OPDSXMLParser::processNamespaces() const {
	return true;
}

void OPDSXMLParser::namespaceListChangedHandler() {
	myDCPrefix.erase();
	myAtomPrefix.erase();
	const std::map<std::string,std::string> &nsMap = namespaces();
	int prefixes = 0;
	for (std::map<std::string,std::string>::const_iterator it = nsMap.begin(); it != nsMap.end() && prefixes != 0x03; ++it) {
		if (ZLStringUtil::stringStartsWith(it->second, XMLNamespace::DublinCoreTermsPrefix)) {
			if (!it->first.empty()) {
				myDCPrefix = it->first + ':';
			}
			prefixes |= 0x01;
		} else if (it->second == XMLNamespace::Atom) {
			if (!it->first.empty()) {
				myAtomPrefix = it->first + ':';
			}
			prefixes |= 0x02;
		}
	}
}


bool OPDSXMLParser::checkAtomTag(const std::string &tag, const std::string &pattern) const {
	return checkNSTag(tag, myAtomPrefix, pattern);
}

bool OPDSXMLParser::checkDCTag(const std::string &tag, const std::string &pattern) const {
	return checkNSTag(tag, myDCPrefix, pattern);
}

bool OPDSXMLParser::checkNSTag(const std::string &tag, const std::string &nsprefix, const std::string &pattern) {
	if (nsprefix.empty()) {
		return tag == pattern;
	}
	if (ZLStringUtil::stringStartsWith(tag, nsprefix)) {
		std::string tagname = tag.substr(nsprefix.length());
		return tagname == pattern;
	}
	return false;
}


void OPDSXMLParser::startElementHandler(const char *tag, const char **attributes) {
	myAttributes.clear();
	while (*attributes != 0) {
		std::string name(*attributes++);
		if (*attributes == 0) {
			break;
		}
		std::string value(*attributes++);
		myAttributes.insert(std::make_pair(name, value));
	}
	processState(tag, false);
	myState = getNextState(tag, false);
	myBuffer.clear();
}

void OPDSXMLParser::endElementHandler(const char *tag) {
	myAttributes.clear();
	processState(tag, true);
	myState = getNextState(tag, true);
	myBuffer.clear();
}

void OPDSXMLParser::characterDataHandler(const char *data, size_t len) {
	myBuffer.append(data, len);
}


void OPDSXMLParser::processState(const std::string &tag, bool closed) {
	switch(myState) {
	case START: 
		if (!closed && checkAtomTag(tag, TAG_FEED)) {
			myFeedReader->processFeedStart();
			myFeed = new OPDSFeedMetadata();
			myFeed->readAttributes(myAttributes);
		}
		break;
	case FEED: 
		if (!closed) {
			if (checkAtomTag(tag, TAG_AUTHOR)) {
				myAuthor = new ATOMAuthor();
				myAuthor->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_ID)) {
				myId = new ATOMId();
				myId->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_LINK)) {
				myLink = new ATOMLink();
				myLink->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_CATEGORY)) {
				myCategory = new ATOMCategory();
				myCategory->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_TITLE)) {
				//myTitle = new ATOMTitle(); // TODO: implement ATOMTextConstruct & ATOMTitle
				//myTitle->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_UPDATED)) {
				myUpdated = new ATOMUpdated();
				myUpdated->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_ENTRY)) {
				myFeedReader->processFeedMetadata(myFeed);
				//myFeed = 0; // TODO: check, that this can be done; that after this moment there will be no calls to myFeed
				myEntry = new OPDSEntry();
				myEntry->readAttributes(myAttributes);
			} 
		} else {
			if (checkAtomTag(tag, TAG_FEED)) {
				//myReady = true;
				myFeedReader->processFeedEnd();
			} 
		}
		break;
	case F_ENTRY: 
		if (!closed) {
			if (checkAtomTag(tag, TAG_AUTHOR)) {
				myAuthor = new ATOMAuthor();
				myAuthor->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_ID)) {
				myId = new ATOMId();
				myId->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_CATEGORY)) {
				myCategory = new ATOMCategory();
				myCategory->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_LINK)) {
				myLink = new ATOMLink();
				myLink->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_PUBLISHED)) {
				myPublished = new ATOMPublished();
				myPublished->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_SUMMARY)) {
				//mySummary = new ATOMSummary(); // TODO: implement ATOMTextConstruct & ATOMSummary
				//mySummary->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_TITLE)) {
				//myTitle = new ATOMTitle(); // TODO: implement ATOMTextConstruct & ATOMTitle
				//myTitle->readAttributes(myAttributes);
			} else if (checkAtomTag(tag, TAG_UPDATED)) {
				myUpdated = new ATOMUpdated();
				myUpdated->readAttributes(myAttributes);
			} else if (checkDCTag(tag, DC_TAG_LANGUAGE)) {
				// nothing to do
			} else if (checkDCTag(tag, DC_TAG_ISSUED)) {
				// nothing to do
			} 
		} else {
			if (checkAtomTag(tag, TAG_ENTRY)) {
				//myFeed->entries().push_back(myEntry);
				myFeedReader->processFeedEntry(myEntry);
				myEntry = 0;
			} 
		}
		break;
	case F_ID: 
		if (closed && checkAtomTag(tag, TAG_ID)) {
			// FIXME: uri can be lost: buffer will be empty, if there are extention tags inside the <id> tag
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			myId->setUri(myBuffer);
			myFeed->setId(myId);
			myId = 0;
		} 
		break;
	case F_LINK: 
		if (closed && checkAtomTag(tag, TAG_LINK)) {
			myFeed->links().push_back(myLink);
			myLink = 0;
		} 
		break;
	case F_CATEGORY: 
		if (closed && checkAtomTag(tag, TAG_CATEGORY)) {
			myFeed->categories().push_back(myCategory);
			myCategory = 0;
		} 
		break;
	case F_TITLE: 
		if (closed && checkAtomTag(tag, TAG_TITLE)) {
			// FIXME: title can be lost: buffer will be empty, if there are extention tags inside the <title> tag
			// TODO: implement ATOMTextConstruct & ATOMTitle
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			myFeed->setTitle(myBuffer);
		} 
		break;
	case F_UPDATED: 
		if (closed && checkAtomTag(tag, TAG_UPDATED)) {
			// FIXME: uri can be lost: buffer will be empty, if there are extention tags inside the <id> tag
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			ATOMDateConstruct::parse(myBuffer, *myUpdated);
			myFeed->setUpdated(myUpdated);
			myUpdated = 0;
		} 
		break;
	case F_AUTHOR: 
		if (!closed) {
			if (checkAtomTag(tag, TAG_NAME)) {
				// nothing to do
			} else if (checkAtomTag(tag, TAG_URI)) {
				// nothing to do
			} else if (checkAtomTag(tag, TAG_EMAIL)) {
				// nothing to do
			} 
		} else {
			if (checkAtomTag(tag, TAG_AUTHOR)) {
				myFeed->authors().push_back(myAuthor);
				myAuthor = 0;
			} 
		}
		break;
	case FA_NAME: 
	case FEA_NAME: 
		if (closed && checkAtomTag(tag, TAG_NAME)) {
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			myAuthor->setName(myBuffer);
		}
		break;
	case FA_URI: 
	case FEA_URI: 
		if (closed && checkAtomTag(tag, TAG_URI)) {
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			myAuthor->setUri(myBuffer);
		}
		break;
	case FA_EMAIL: 
	case FEA_EMAIL:
		if (closed && checkAtomTag(tag, TAG_EMAIL)) {
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			myAuthor->setEmail(myBuffer);
		}
		break;
	case FE_AUTHOR: 
		if (!closed) {
			if (checkAtomTag(tag, TAG_NAME)) {
				// nothing to do
			} else if (checkAtomTag(tag, TAG_URI)) {
				// nothing to do
			} else if (checkAtomTag(tag, TAG_EMAIL)) {
				// nothing to do
			} 
		} else {
			if (checkAtomTag(tag, TAG_AUTHOR)) {
				myEntry->authors().push_back(myAuthor);
				myAuthor = 0;
			} 
		}
		break;
	case FE_ID: 
		if (closed && checkAtomTag(tag, TAG_ID)) {
			// FIXME: uri can be lost: buffer will be empty, if there are extention tags inside the <id> tag
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			myId->setUri(myBuffer);
			myEntry->setId(myId);
			myId = 0;
		}
		break;
	case FE_CATEGORY: 
		if (closed && checkAtomTag(tag, TAG_CATEGORY)) {
			myEntry->categories().push_back(myCategory);
			myCategory = 0;
		}
		break;
	case FE_LINK: 
		if (closed && checkAtomTag(tag, TAG_LINK)) {
			myEntry->links().push_back(myLink);
			myLink = 0;
		}
		break;
	case FE_PUBLISHED: 
		if (closed && checkAtomTag(tag, TAG_PUBLISHED)) {
			// FIXME: uri can be lost: buffer will be empty, if there are extention tags inside the <id> tag
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			ATOMDateConstruct::parse(myBuffer, *myPublished);
			myEntry->setPublished(myPublished);
			myPublished = 0;
		}
		break;
	case FE_SUMMARY: 
		if (closed && checkAtomTag(tag, TAG_SUMMARY)) {
			// FIXME: summary can be lost: buffer will be empty, if there are extention tags inside the <summary> tag
			// TODO: implement ATOMTextConstruct & ATOMSummary
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			myEntry->setSummary(myBuffer);
		}
		break;
	case FE_TITLE: 
		if (closed && checkAtomTag(tag, TAG_TITLE)) {
			// FIXME: title can be lost: buffer will be empty, if there are extention tags inside the <title> tag
			// TODO: implement ATOMTextConstruct & ATOMTitle
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			myEntry->setTitle(myBuffer);
		}
		break;
	case FE_UPDATED: 
		if (closed && checkAtomTag(tag, TAG_UPDATED)) {
			// FIXME: uri can be lost: buffer will be empty, if there are extention tags inside the <id> tag
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			ATOMDateConstruct::parse(myBuffer, *myUpdated);
			myEntry->setUpdated(myUpdated);
			myUpdated = 0;
		}
		break;
	case FE_DC_LANGUAGE: 
		if (closed && checkDCTag(tag, DC_TAG_LANGUAGE)) {
			// FIXME: language can be lost: buffer will be empty, if there are extention tags inside the <dc:language> tag
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			myEntry->setDCLanguage(myBuffer);
		}
		break;
	case FE_DC_ISSUED: 
		if (closed && checkDCTag(tag, DC_TAG_ISSUED)) {
			// FIXME: issued can be lost: buffer will be empty, if there are extention tags inside the <dc:issued> tag
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			DCDate *issued = new DCDate();
			ATOMDateConstruct::parse(myBuffer, *issued);
			myEntry->setDCIssued(issued);
		}
		break;
	case FE_DC_PUBLISHER:
		if (closed && checkDCTag(tag, DC_TAG_PUBLISHER)) {
			// FIXME: publisher can be lost: buffer will be empty, if there are extention tags inside the <dc:publisher> tag
			ZLStringUtil::stripWhiteSpaces(myBuffer);
			myEntry->setDCPublisher(myBuffer);
		}
		break;
	case FEED_CONTENT:
		if (!closed) {
			if (checkAtomTag(tag, TAG_ENTRY)) {
				myEntry = new OPDSEntry();
				myEntry->readAttributes(myAttributes);
			} 
		} else {
			if (checkAtomTag(tag, TAG_FEED)) {
				myFeedReader->processFeedEnd();
			} 
		}
		break;
	}
}


OPDSXMLParser::State OPDSXMLParser::getNextState(const std::string &tag, bool closed) {
	switch(myState) {
	case START: 
		if (!closed && checkAtomTag(tag, TAG_FEED)) {
			return FEED;
		}
		break;
	case FEED: 
		if (!closed) {
			if (checkAtomTag(tag, TAG_AUTHOR)) {
				return F_AUTHOR;
			} else if (checkAtomTag(tag, TAG_ID)) {
				return F_ID;
			} else if (checkAtomTag(tag, TAG_LINK)) {
				return F_LINK;
			} else if (checkAtomTag(tag, TAG_CATEGORY)) {
				return F_CATEGORY;
			} else if (checkAtomTag(tag, TAG_TITLE)) {
				return F_TITLE;
			} else if (checkAtomTag(tag, TAG_UPDATED)) {
				return F_UPDATED;
			} else if (checkAtomTag(tag, TAG_ENTRY)) {
				return F_ENTRY;
			} 
		} else {
			if (checkAtomTag(tag, TAG_FEED)) {
				return START;
			} 
		}
		break;
	case F_ENTRY: 
		if (!closed) {
			if (checkAtomTag(tag, TAG_AUTHOR)) {
				return FE_AUTHOR;
			} else if (checkAtomTag(tag, TAG_ID)) {
				return FE_ID;
			} else if (checkAtomTag(tag, TAG_CATEGORY)) {
				return FE_CATEGORY;
			} else if (checkAtomTag(tag, TAG_LINK)) {
				return FE_LINK;
			} else if (checkAtomTag(tag, TAG_PUBLISHED)) {
				return FE_PUBLISHED;
			} else if (checkAtomTag(tag, TAG_SUMMARY)) {
				return FE_SUMMARY;
			} else if (checkAtomTag(tag, TAG_TITLE)) {
				return FE_TITLE;
			} else if (checkAtomTag(tag, TAG_UPDATED)) {
				return FE_UPDATED;
			} else if (checkDCTag(tag, DC_TAG_LANGUAGE)) {
				return FE_DC_LANGUAGE;
			} else if (checkDCTag(tag, DC_TAG_ISSUED)) {
				return FE_DC_ISSUED;
			} else if (checkDCTag(tag, DC_TAG_PUBLISHER)) {
				return FE_DC_PUBLISHER;
			}
		} else {
			if (checkAtomTag(tag, TAG_ENTRY)) {
				return FEED_CONTENT;
			} 
		}
		break;
	case F_ID: 
		if (closed && checkAtomTag(tag, TAG_ID)) {
			return FEED;
		} 
		break;
	case F_LINK: 
		if (closed && checkAtomTag(tag, TAG_LINK)) {
			return FEED;
		} 
		break;
	case F_CATEGORY: 
		if (closed && checkAtomTag(tag, TAG_CATEGORY)) {
			return FEED;
		} 
		break;
	case F_TITLE: 
		if (closed && checkAtomTag(tag, TAG_TITLE)) {
			return FEED;
		} 
		break;
	case F_UPDATED: 
		if (closed && checkAtomTag(tag, TAG_UPDATED)) {
			return FEED;
		} 
		break;
	case F_AUTHOR: 
		if (!closed) {
			if (checkAtomTag(tag, TAG_NAME)) {
				return FA_NAME;
			} else if (checkAtomTag(tag, TAG_URI)) {
				return FA_URI;
			} else if (checkAtomTag(tag, TAG_EMAIL)) {
				return FA_EMAIL;
			} 
		} else {
			if (checkAtomTag(tag, TAG_AUTHOR)) {
				return FEED;
			} 
		}
		break;
	case FA_NAME: 
		if (closed && checkAtomTag(tag, TAG_NAME)) {
			return F_AUTHOR;
		}
		break;
	case FA_URI: 
		if (closed && checkAtomTag(tag, TAG_URI)) {
			return F_AUTHOR;
		}
		break;
	case FA_EMAIL: 
		if (closed && checkAtomTag(tag, TAG_EMAIL)) {
			return F_AUTHOR;
		}
		break;
	case FE_AUTHOR: 
		if (!closed) {
			if (checkAtomTag(tag, TAG_NAME)) {
				return FEA_NAME;
			} else if (checkAtomTag(tag, TAG_URI)) {
				return FEA_URI;
			} else if (checkAtomTag(tag, TAG_EMAIL)) {
				return FEA_EMAIL;
			} 
		} else {
			if (checkAtomTag(tag, TAG_AUTHOR)) {
				return F_ENTRY;
			} 
		}
		break;
	case FE_ID: 
		if (closed && checkAtomTag(tag, TAG_ID)) {
			return F_ENTRY;
		}
		break;
	case FE_CATEGORY: 
		if (closed && checkAtomTag(tag, TAG_CATEGORY)) {
			return F_ENTRY;
		}
		break;
	case FE_LINK: 
		if (closed && checkAtomTag(tag, TAG_LINK)) {
			return F_ENTRY;
		}
		break;
	case FE_PUBLISHED: 
		if (closed && checkAtomTag(tag, TAG_PUBLISHED)) {
			return F_ENTRY;
		}
		break;
	case FE_SUMMARY: 
		if (closed && checkAtomTag(tag, TAG_SUMMARY)) {
			return F_ENTRY;
		}
		break;
	case FE_TITLE: 
		if (closed && checkAtomTag(tag, TAG_TITLE)) {
			return F_ENTRY;
		}
		break;
	case FE_UPDATED: 
		if (closed && checkAtomTag(tag, TAG_UPDATED)) {
			return F_ENTRY;
		}
		break;
	case FE_DC_LANGUAGE: 
		if (closed && checkDCTag(tag, DC_TAG_LANGUAGE)) {
			return F_ENTRY;
		}
		break;
	case FE_DC_ISSUED: 
		if (closed && checkDCTag(tag, DC_TAG_ISSUED)) {
			return F_ENTRY;
		}
		break;
	case FE_DC_PUBLISHER:
		if (closed && checkDCTag(tag, DC_TAG_PUBLISHER)) {
			return F_ENTRY;
		}
		break;
	case FEA_NAME: 
		if (closed && checkAtomTag(tag, TAG_NAME)) {
			return FE_AUTHOR;
		}
		break;
	case FEA_URI: 
		if (closed && checkAtomTag(tag, TAG_URI)) {
			return FE_AUTHOR;
		}
		break;
	case FEA_EMAIL:
		if (closed && checkAtomTag(tag, TAG_EMAIL)) {
			return FE_AUTHOR;
		}
		break;
	case FEED_CONTENT:
		if (!closed) {
			if (checkAtomTag(tag, TAG_ENTRY)) {
				return F_ENTRY;
			} 
		} else {
			if (checkAtomTag(tag, TAG_FEED)) {
				return START;
			} 
		}
		break;
	}
	return myState;
}

