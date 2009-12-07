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

#ifndef __NETWORKNODES_H__
#define __NETWORKNODES_H__

#include "../blockTree/FBReaderNode.h"

#include "../network/NetworkLibraryItems.h"


class NetworkBookCollection;
class NetworkLink;
class NetworkAuthenticationManager;


class NetworkCatalogNode : public FBReaderNode {

public:
	static const std::string TYPE_ID;

private:
	class ExpandCatalogAction;
	class OpenInBrowserAction;
	class ReloadAction;

protected:
	NetworkCatalogNode(ZLBlockTreeView::RootNode *parent, shared_ptr<NetworkLibraryItem> item, size_t atPosition = -1);
	NetworkCatalogNode(NetworkCatalogNode *parent, shared_ptr<NetworkLibraryItem> item, size_t atPosition = -1);

friend class NetworkNodesFactory;

public:
	NetworkLibraryCatalogItem &item();
	const NetworkLibraryItemList &childrenItems();

	void updateChildren();

	shared_ptr<ZLRunnable> expandCatalogAction();
	shared_ptr<ZLRunnable> openInBrowserAction();
	shared_ptr<ZLRunnable> reloadAction();

protected:
	const std::string &typeId() const;
	shared_ptr<ZLImage> extractCoverImage() const;
	virtual shared_ptr<ZLImage> lastResortCoverImage() const;
	void paint(ZLPaintContext &context, int vOffset);

	virtual void paintHyperlinks(ZLPaintContext &context, int vOffset);

private:
	shared_ptr<NetworkLibraryItem> myItem;
	NetworkLibraryItemList myChildrenItems;
	shared_ptr<ZLRunnable> myExpandCatalogAction;
	shared_ptr<ZLRunnable> myOpenInBrowserAction;
	shared_ptr<ZLRunnable> myReloadAction;
};



class NetworkCatalogRootNode : public NetworkCatalogNode {

private:
	class LoginAction : public ZLRunnable {

	public:
		LoginAction(NetworkAuthenticationManager &mgr);
		void run();

	private:
		NetworkAuthenticationManager &myManager;
	};

	class LogoutAction : public ZLRunnable {

	public:
		LogoutAction(NetworkAuthenticationManager &mgr);
		void run();

	private:
		NetworkAuthenticationManager &myManager;
	};

	class RefillAccountAction : public ZLRunnable {

	public:
		RefillAccountAction(NetworkAuthenticationManager &mgr);
		void run();

	private:
		NetworkAuthenticationManager &myManager;
	};

	class DontShowAction : public ZLRunnable {

	public:
		DontShowAction(NetworkLink &link);
		void run();

	private:
		NetworkLink &myLink;
	};

public:
	NetworkCatalogRootNode(ZLBlockTreeView::RootNode *parent, NetworkLink &link, size_t atPosition = -1);

	const NetworkLink &link() const;

private:
	void paintHyperlinks(ZLPaintContext &context, int vOffset);
	shared_ptr<ZLImage> lastResortCoverImage() const;

private:
	NetworkLink &myLink;
	shared_ptr<ZLRunnable> myLoginAction;
	shared_ptr<ZLRunnable> myLogoutAction;
	shared_ptr<ZLRunnable> myDontShowAction;
	shared_ptr<ZLRunnable> myRefillAccountAction;
};

class SearchResultNode : public FBReaderNode {

public:
	static const std::string TYPE_ID;

public:
	SearchResultNode(ZLBlockTreeView::RootNode *parent, shared_ptr<NetworkBookCollection> searchResult, const std::string &summary, size_t atPosition = -1);

	shared_ptr<NetworkBookCollection> searchResult();

private:
	const std::string &typeId() const;
	shared_ptr<ZLImage> extractCoverImage() const;
	void paint(ZLPaintContext &context, int vOffset);

private:
	shared_ptr<NetworkBookCollection> mySearchResult;
	std::string mySummary;
};


class NetworkAuthorNode : public FBReaderNode {

public:
	static const std::string TYPE_ID;

protected:
	NetworkAuthorNode(NetworkCatalogNode *parent, const NetworkLibraryBookItem::AuthorData &author);
	NetworkAuthorNode(SearchResultNode *parent, const NetworkLibraryBookItem::AuthorData &author);

friend class NetworkNodesFactory;

public:
	const NetworkLibraryBookItem::AuthorData &author();

private:
	const std::string &typeId() const;
	shared_ptr<ZLImage> extractCoverImage() const;
	void paint(ZLPaintContext &context, int vOffset);

private:
	NetworkLibraryBookItem::AuthorData myAuthor;
};


class NetworkSeriesNode : public FBReaderNode {

public:
	static const std::string TYPE_ID;

protected:
	NetworkSeriesNode(NetworkAuthorNode *parent, const std::string &series);

friend class NetworkNodesFactory;

public:
	const std::string &series();

private:
	const std::string &typeId() const;
	shared_ptr<ZLImage> extractCoverImage() const;
	void paint(ZLPaintContext &context, int vOffset);

private:
	std::string mySeries;
};


class NetworkBookInfoNode : public FBReaderNode {

public:
	static const std::string TYPE_ID;

private:
	class ReadAction : public ZLRunnable {

	public:
		ReadAction(shared_ptr<NetworkLibraryItem> book);
		void run();

	private:
		shared_ptr<NetworkLibraryItem> myBook;
	};

	class DownloadAction;

	class ReadDemoAction : public ZLRunnable {

	public:
		ReadDemoAction(shared_ptr<NetworkLibraryItem> book);
		void run();

	private:
		shared_ptr<NetworkLibraryItem> myBook;
	};

	class BuyAction : public ZLRunnable {

	public:
		BuyAction(shared_ptr<NetworkLibraryItem> book);
		void run();

	private:
		shared_ptr<NetworkLibraryItem> myBook;
	};

	class DeleteAction : public ZLRunnable {

	public:
		DeleteAction(shared_ptr<NetworkLibraryItem> book);
		void run();

	private:
		void removeFormat(NetworkLibraryBookItem &book, NetworkLibraryBookItem::URLType format);

	private:
		shared_ptr<NetworkLibraryItem> myBook;
	};

private:
	NetworkBookInfoNode(NetworkAuthorNode *parent, shared_ptr<NetworkLibraryItem> book);
	NetworkBookInfoNode(NetworkSeriesNode *parent, shared_ptr<NetworkLibraryItem> book);
	NetworkBookInfoNode(NetworkCatalogNode *parent, shared_ptr<NetworkLibraryItem> book);

private:
	void init();

friend class NetworkNodesFactory;

public:
	shared_ptr<NetworkLibraryItem> book();

private:
	const std::string &typeId() const;
	shared_ptr<ZLImage> extractCoverImage() const;
	void paint(ZLPaintContext &context, int vOffset);

	NetworkLibraryBookItem &bookItem();

	bool hasLocalCopy();
	bool hasDirectLink();
	bool canBePurchased();
	static bool hasLocalCopy(NetworkLibraryBookItem &book, NetworkLibraryBookItem::URLType format);

private:
	shared_ptr<NetworkLibraryItem> myBook;
	shared_ptr<ZLRunnable> myReadAction;
	shared_ptr<ZLRunnable> myDownloadAction;
	shared_ptr<ZLRunnable> myReadDemoAction;
	shared_ptr<ZLRunnable> myDownloadDemoAction;
	shared_ptr<ZLRunnable> myBuyAction;
	shared_ptr<ZLRunnable> myDeleteAction;
};

inline shared_ptr<NetworkLibraryItem> NetworkBookInfoNode::book() { return myBook; }
inline NetworkLibraryBookItem &NetworkBookInfoNode::bookItem() { return (NetworkLibraryBookItem &) *myBook; }

#endif /* __NETWORKNODES_H__ */
