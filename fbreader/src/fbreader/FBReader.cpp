/*
 * Copyright (C) 2004-2009 Geometer Plus <contact@geometerplus.com>
 * Copyright (C) 2009 Alexander Kerner <lunohod@openinkpot.org>
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

#include <queue>

#include <ZLibrary.h>
#include <ZLFile.h>
#include <ZLDialogManager.h>
#include <ZLOptionsDialog.h>
#include <ZLDir.h>
#include <ZLStringUtil.h>
#include <ZLResource.h>
#include <ZLMessage.h>

#include <ZLTextStyleCollection.h>
#include <ZLTextHyphenator.h>

#include "FBReader.h"
#include "FBReaderActions.h"
#include "ScrollingAction.h"
#include "BookTextView.h"
#include "FootnoteView.h"
#include "ContentsView.h"
#include "RecentBooksPopupData.h"
#include "BookInfoDialog.h"
#include "TimeUpdater.h"

#include "../libraryTree/LibraryView.h"
#include "../networkTree/NetworkView.h"
#include "../networkTree/NetworkOperationRunnable.h"

#include "../migration/migrate.h"

#include "../options/FBCategoryKey.h"
#include "../bookmodel/BookModel.h"
#include "../formats/FormatPlugin.h"
#include "../../../zlibrary/ui/src/ewl/util/ZLEwlUtil.h"

#include "../database/booksdb/BooksDB.h"
#include "../database/booksdb/BooksDBUtil.h"
#include "../library/Book.h"

#include "../network/litres/LitResUtil.h"

static const std::string OPTIONS = "Options";

const std::string PAGE_SCROLLING = "LargeScrolling";
const std::string LINE_SCROLLING = "SmallScrolling";
const std::string MOUSE_SCROLLING = "MouseScrolling";
const std::string TAP_SCROLLING = "TapScrolling";

const std::string DELAY = "ScrollingDelay";
const std::string MODE = "Mode";
const std::string LINES_TO_KEEP = "LinesToKeep";
const std::string LINES_TO_SCROLL = "LinesToScroll";
const std::string PERCENT_TO_SCROLL = "PercentToScroll";

const std::string FBReader::PageIndexParameter = "pageIndex";

FBReader::ScrollingOptions::ScrollingOptions(const std::string &groupName, long delayValue, long modeValue, long linesToKeepValue, long linesToScrollValue, long percentToScrollValue) :
	DelayOption(ZLCategoryKey::CONFIG, groupName, DELAY, 0, 5000, delayValue),
	ModeOption(ZLCategoryKey::CONFIG, groupName, MODE, modeValue),
	LinesToKeepOption(ZLCategoryKey::CONFIG, groupName, LINES_TO_KEEP, 1, 100, linesToKeepValue),
	LinesToScrollOption(ZLCategoryKey::CONFIG, groupName, LINES_TO_SCROLL, 1, 100, linesToScrollValue),
	PercentToScrollOption(ZLCategoryKey::CONFIG, groupName, PERCENT_TO_SCROLL, 1, 100, percentToScrollValue) {
}

class OpenFileHandler : public ZLMessageHandler {

public:
	void onMessageReceived(const std::vector<std::string> &arguments) {
		if (arguments.size() == 1) {
			FBReader &fbreader = FBReader::Instance();
			fbreader.myBookAlreadyOpen = true;
			fbreader.presentWindow();
			fbreader.openFile(arguments[0]);
		}
	}
};

FBReader &FBReader::Instance() {
	return (FBReader&)ZLApplication::Instance();
}

FBReader::FBReader(const std::string &bookToOpen) :
	ZLApplication("FBReader"),
	QuitOnCancelOption(ZLCategoryKey::CONFIG, OPTIONS, "QuitOnCancel", true),
	PageScrollingOptions(PAGE_SCROLLING, 250, ZLTextView::NO_OVERLAPPING, 1, 1, 50),
	LineScrollingOptions(LINE_SCROLLING, 50, ZLTextView::SCROLL_LINES, 1, 1, 50),
	MouseScrollingOptions(MOUSE_SCROLLING, 0, ZLTextView::SCROLL_LINES, 1, 1, 50),
	TapScrollingOptions(TAP_SCROLLING, 0, ZLTextView::NO_OVERLAPPING, 1, 1, 50),
	EnableTapScrollingOption(ZLCategoryKey::CONFIG, TAP_SCROLLING, "Enabled", true),
	TapScrollingOnFingerOnlyOption(ZLCategoryKey::CONFIG, TAP_SCROLLING, "FingerOnly", true),
	UseSeparateBindingsOption(ZLCategoryKey::CONFIG, "KeysOptions", "UseSeparateBindings", false),
	EnableSingleClickDictionaryOption(ZLCategoryKey::CONFIG, "Dictionary", "SingleClick", false),
	myBindings0(new ZLKeyBindings("Keys")),
	myBindings90(new ZLKeyBindings("Keys90")),
	myBindings180(new ZLKeyBindings("Keys180")),
	myBindings270(new ZLKeyBindings("Keys270")),
	myBookToOpen(bookToOpen),
	myBookAlreadyOpen(false),
	myActionOnCancel(UNFULLSCREEN) {

	myModel = 0;
	myBookTextView = new BookTextView(*context());
	myFootnoteView = new FootnoteView(*context());
	myContentsView = new ContentsView(*context());
	myNetworkLibraryView = new NetworkView(*context());
	myLibraryByAuthorView = new LibraryByAuthorView(*context());
	myLibraryByTagView = new LibraryByTagView(*context());
	myRecentBooksPopupData = new RecentBooksPopupData();
	myMode = UNDEFINED_MODE;
	myPreviousMode = BOOK_TEXT_MODE;
	setMode(BOOK_TEXT_MODE);

	addAction(ActionCode::SHOW_READING, new UndoAction(FBReader::ALL_MODES & ~FBReader::BOOK_TEXT_MODE));
	addAction(ActionCode::SHOW_COLLECTION, new SetModeAction(FBReader::LIBRARY_MODE, FBReader::BOOK_TEXT_MODE | FBReader::CONTENTS_MODE));
	addAction(ActionCode::SHOW_NET_LIBRARY, new ShowNetworkLibraryAction());
	addAction(ActionCode::SEARCH_ON_NETWORK, new SimpleSearchOnNetworkAction());
	addAction(ActionCode::ADVANCED_SEARCH_ON_NETWORK, new AdvancedSearchOnNetworkAction());
	registerPopupData(ActionCode::SHOW_COLLECTION, myRecentBooksPopupData);
	addAction(ActionCode::SHOW_OPTIONS, new ShowOptionsDialogAction());
	addAction(ActionCode::SHOW_MENU, new ShowMenuDialogAction());
	addAction(ActionCode::SHOW_CONTENTS, new ShowContentsAction());
	addAction(ActionCode::SHOW_BOOK_INFO, new ShowBookInfoAction());
	addAction(ActionCode::ADD_BOOK, new AddBookAction(FBReader::BOOK_TEXT_MODE | FBReader::LIBRARY_MODE | FBReader::CONTENTS_MODE));
	addAction(ActionCode::UNDO, new UndoAction(FBReader::BOOK_TEXT_MODE));
	addAction(ActionCode::REDO, new RedoAction());
	addAction(ActionCode::SEARCH, new SearchPatternAction());
	addAction(ActionCode::FIND_NEXT, new FindNextAction());
	addAction(ActionCode::FIND_PREVIOUS, new FindPreviousAction());
	addAction(ActionCode::SCROLL_TO_HOME, new ScrollToHomeAction());
	addAction(ActionCode::SCROLL_TO_START_OF_TEXT, new ScrollToStartOfTextAction());
	addAction(ActionCode::SCROLL_TO_END_OF_TEXT, new ScrollToEndOfTextAction());
	addAction(ActionCode::PAGE_SCROLL_FORWARD, new ScrollingAction(PageScrollingOptions, ZLBlockTreeView::PAGE, true));
	addAction(ActionCode::PAGE_SCROLL_BACKWARD, new ScrollingAction(PageScrollingOptions, ZLBlockTreeView::PAGE, false));
	addAction(ActionCode::LINE_SCROLL_FORWARD, new ScrollingAction(LineScrollingOptions, ZLBlockTreeView::ITEM, true));
	addAction(ActionCode::LINE_SCROLL_BACKWARD, new ScrollingAction(LineScrollingOptions, ZLBlockTreeView::ITEM, false));
	addAction(ActionCode::MOUSE_SCROLL_FORWARD, new ScrollingAction(MouseScrollingOptions, ZLBlockTreeView::ITEM, true));
	addAction(ActionCode::MOUSE_SCROLL_BACKWARD, new ScrollingAction(MouseScrollingOptions, ZLBlockTreeView::ITEM, false));
	addAction(ActionCode::TAP_SCROLL_FORWARD, new ScrollingAction(TapScrollingOptions, ZLBlockTreeView::NONE, true));
	addAction(ActionCode::TAP_SCROLL_BACKWARD, new ScrollingAction(TapScrollingOptions, ZLBlockTreeView::NONE, false));
	addAction(ActionCode::INCREASE_FONT, new ChangeFontSizeAction(2));
	addAction(ActionCode::DECREASE_FONT, new ChangeFontSizeAction(-2));
	addAction(ActionCode::ROTATE_SCREEN, new RotationAction());
	addAction(ActionCode::TOGGLE_FULLSCREEN, new FBFullscreenAction());
	addAction(ActionCode::FULLSCREEN_ON, new FBFullscreenAction());
	addAction(ActionCode::CANCEL, new CancelAction());
	addAction(ActionCode::SHOW_HIDE_POSITION_INDICATOR, new ToggleIndicatorAction());
	addAction(ActionCode::QUIT, new QuitAction());
	addAction(ActionCode::OPEN_PREVIOUS_BOOK, new OpenPreviousBookAction());
	addAction(ActionCode::SHOW_HELP, new ShowHelpAction());
	addAction(ActionCode::GOTO_NEXT_TOC_SECTION, new GotoNextTOCSectionAction());
	addAction(ActionCode::GOTO_PREVIOUS_TOC_SECTION, new GotoPreviousTOCSectionAction());
	addAction(ActionCode::COPY_SELECTED_TEXT_TO_CLIPBOARD, new CopySelectedTextAction());
	addAction(ActionCode::OPEN_SELECTED_TEXT_IN_DICTIONARY, new OpenSelectedTextInDictionaryAction());
	addAction(ActionCode::CLEAR_SELECTION, new ClearSelectionAction());
	addAction(ActionCode::GOTO_PAGE_NUMBER, new GotoPageNumber(std::string()));
	addAction(ActionCode::GOTO_PAGE_NUMBER_WITH_PARAMETER, new GotoPageNumber(PageIndexParameter));
	shared_ptr<Action> booksOrderAction = new BooksOrderAction();
	addAction(ActionCode::ORGANIZE_BOOKS_BY_AUTHOR, booksOrderAction);
	addAction(ActionCode::ORGANIZE_BOOKS_BY_TAG, booksOrderAction);

	addAction(ActionCode::SHOW_FOOTNOTES, new ShowFootnotes());
	addAction(ActionCode::HYPERLINK_NAV_START, new HyperlinkNavStart());
	addAction(ActionCode::BMK_ADD, new BookmarkAdd());
	addAction(ActionCode::BMK_SHOW, new BookmarksShow());

	addAction(ActionCode::HELP, new Help());
	addAction(ActionCode::BOLD_TOGGLE, new BoldToggle());
	addAction(ActionCode::TURBO_TOGGLE, new TurboToggle());
	addAction(ActionCode::FONT_SIZE_DIALOG, new FontSizeDialog());

	myOpenFileHandler = new OpenFileHandler();
	ZLCommunicationManager::instance().registerHandler("openFile", myOpenFileHandler);
}

FBReader::~FBReader() {
	if (myModel != 0) {
		delete myModel;
	}
	ZLTextStyleCollection::deleteInstance();
	PluginCollection::deleteInstance();
	ZLTextHyphenator::deleteInstance();
}
void FBReader::initWindow() {
	ZLApplication::initWindow();
	trackStylus(true);

	MigrationRunnable migration;
	if (migration.shouldMigrate()) {
		ZLDialogManager::Instance().wait(ZLResourceKey("migrate"), migration);
		myRecentBooks.reload();
	}

	if (!myBookAlreadyOpen) {
		shared_ptr<Book> book;
		if (!myBookToOpen.empty()) {
			createBook(myBookToOpen, book);
		}
		if (book.isNull()) {
			const BookList &books = myRecentBooks.books();
			if (!books.empty()) {
				book = books[0];
			}
		}
		if (book.isNull()) {
			book = BooksDBUtil::getBook(helpFileName(ZLibrary::Language()));
		}
		if (book.isNull()) {
			book = BooksDBUtil::getBook(helpFileName("en"));
		}
		openBook(book);
	}
	// this refresh is not needed
	// the page will be rendered when the window becomes unobscured
	//refreshWindow();

//	ZLTimeManager::Instance().addTask(new TimeUpdater(), 1000);
}

bool FBReader::createBook(const std::string& fileName, shared_ptr<Book> &book) {
	ZLFile bookFile = ZLFile(fileName);

	shared_ptr<FormatPlugin> plugin =
		PluginCollection::Instance().plugin(ZLFile(fileName), false);
	if (!plugin.isNull()) {
		std::string error = plugin->tryOpen(fileName);
		if (!error.empty()) {
			ZLResourceKey boxKey("openBookErrorBox");
			ZLDialogManager::Instance().errorBox(
				boxKey,
				ZLStringUtil::printf(ZLDialogManager::dialogMessage(boxKey), error)
			);
		} else {
			book = BooksDBUtil::getBook(bookFile.path());
			if (!book.isNull()) {
				BooksDB::Instance().insertIntoBookList(*book);
			}
		}
		return true;
	}

	if (!bookFile.isArchive()) {
		return false;
	}

	std::queue<std::string> archiveNames;
	archiveNames.push(bookFile.path());

	std::vector<std::string> items;

	while (!archiveNames.empty()) {
		shared_ptr<ZLDir> archiveDir = ZLFile(archiveNames.front()).directory();
		archiveNames.pop();
		if (archiveDir.isNull()) {
			continue;
		}
		archiveDir->collectFiles(items, true);
		for (std::vector<std::string>::const_iterator it = items.begin(); it != items.end(); ++it) {
			const std::string itemName = archiveDir->itemPath(*it);
			ZLFile subFile(itemName);
			if (subFile.isArchive()) {
				archiveNames.push(itemName);
			} else if (createBook(itemName, book)) {
				return true;
			}
		}
		items.clear();
	}

	return false;
}

class OpenBookRunnable : public ZLRunnable {

public:
	OpenBookRunnable(shared_ptr<Book> book) : myBook(book) {}
	void run() { FBReader::Instance().openBookInternal(myBook); }

private:
	shared_ptr<Book> myBook;
};

void FBReader::openBook(shared_ptr<Book> book) {
	OpenBookRunnable runnable(book);
	ZLDialogManager::Instance().wait(ZLResourceKey("loadingBook"), runnable);
	resetWindowCaption();
}

void FBReader::openBookInternal(shared_ptr<Book> book) {
	if (!book.isNull()) {
		BookTextView &bookTextView = (BookTextView&)*myBookTextView;
		ContentsView &contentsView = (ContentsView&)*myContentsView;
		FootnoteView &footnoteView = (FootnoteView&)*myFootnoteView;

		bookTextView.saveState();
		bookTextView.setModel(0, "", 0);
		bookTextView.setContentsModel(0);
		contentsView.setModel(0, "");
		if (myModel != 0) {
			delete myModel;
		}
		myModel = new BookModel(book);
		const std::string &lang = book->language();
		ZLTextHyphenator::Instance().load(lang);
		bookTextView.setModel(myModel->bookTextModel(), lang, book);
		bookTextView.setCaption(book->title());
		bookTextView.setContentsModel(myModel->contentsModel());
		footnoteView.setModel(0, lang);
		footnoteView.setCaption(book->title());
		contentsView.setModel(myModel->contentsModel(), lang);
		contentsView.setCaption(book->title());

		myRecentBooks.addBook(book);
		((RecentBooksPopupData&)*myRecentBooksPopupData).updateId();
	}
}

void FBReader::openLinkInBrowser(const std::string &url) const {
	shared_ptr<ProgramCollection> collection = webBrowserCollection();
	if (collection.isNull()) {
		return;
	}
	shared_ptr<Program> program = collection->currentProgram();
	if (program.isNull()) {
		return;
	}
	std::string copy = url;
	transformUrl(copy);
	program->run("openLink", copy);
}

void FBReader::tryShowFootnoteView(const std::string &id, const std::string &type) {
	if (type == "external") {
		openLinkInBrowser(id);
	} else if (type == "internal") {
		if (((myMode == BOOK_TEXT_MODE) || (myMode == FOOTNOTE_MODE) || (myMode == HYPERLINK_NAV_MODE)) && (myModel != 0)) {
			BookModel::Label label = myModel->label(id);
			if (!label.Model.isNull()) {
				if ((myMode != FOOTNOTE_MODE) && (label.Model == myModel->bookTextModel())) {
					bookTextView().gotoParagraph(label.ParagraphNumber);
				} else {
					FootnoteView &view = ((FootnoteView&)*myFootnoteView);
					view.setModel(label.Model, myModel->book()->language());
					setMode(FOOTNOTE_MODE);
					view.gotoParagraph(label.ParagraphNumber);
				}
				setHyperlinkCursor(false);
				refreshWindow();
			}
		}
	} else if (type == "book") {
		DownloadBookRunnable downloader(id);
		downloader.executeWithUI();
		if (downloader.hasErrors()) {
			downloader.showErrorMessage();
		} else {
			shared_ptr<Book> book;
			createBook(downloader.fileName(), book);
			if (!book.isNull()) {
				Library::Instance().addBook(book);
				openBook(book);
				refreshWindow();
			}
		}
	}
}

void FBReader::invertRegion(HyperlinkCoord link, bool flush)
{
	((ZLApplication*)this)->invertRegion(link.x0, link.y0, link.x1, link.y1, flush);
}

void FBReader::highlightCurrentLink()
{
	for(int i = currentLinkIdx; (i >= 0) && (i < pageLinks.size()); i++) {
		if(!pageLinks.at(i).next || (i == (pageLinks.size() - 1))) {
			invertRegion(pageLinks.at(i), true);
			break;
		} else {
			invertRegion(pageLinks.at(i), false);
		}
	}
}

void FBReader::startNavigationMode()
{
	if(!pageLinks.empty() && ((myMode == BOOK_TEXT_MODE) || (myMode == FOOTNOTE_MODE))) {
		setMode(HYPERLINK_NAV_MODE);
		currentLinkIdx = 0;
		highlightCurrentLink();
	}
}

void FBReader::openHyperlink()
{
	if(!pageLinks.empty()) {
		restorePreviousMode();
		tryShowFootnoteView(pageLinks.at(currentLinkIdx).id, "internal");
	}
}

void FBReader::highlightNextLink()
{
	int end = pageLinks.size() - 1;

	if(myMode == HYPERLINK_NAV_MODE) {
		for(int i = currentLinkIdx; (i >= 0) && (i <= end); i++) {
			invertRegion(pageLinks.at(i), false);
			if(!pageLinks.at(i).next || (i == end)) {
				currentLinkIdx = i + 1;
				break;
			}
		}

		if(currentLinkIdx > end)
			currentLinkIdx = 0;

		highlightCurrentLink();
	}
}

void FBReader::highlightPrevLink()
{
	int end = pageLinks.size() - 1;

	if(myMode == HYPERLINK_NAV_MODE) {
		for(int i = currentLinkIdx; (i >= 0) && (i <= end); i++) {
			invertRegion(pageLinks.at(i), false);
			if(!pageLinks.at(i).next || (i == end)) {
				break;
			}
		}

		currentLinkIdx--;
		if(currentLinkIdx < 0)
			currentLinkIdx = end;

		for(int i = currentLinkIdx; (i >= 0) && (i <= end); i--) {
			if(i == 0) {
				currentLinkIdx = 0;
			} else if(!pageLinks.at(i).id.empty()) {
				currentLinkIdx = i;
				break;
			}
		}

		highlightCurrentLink();
	}
}

FBReader::ViewMode FBReader::mode() const {
	return myMode;
}

bool FBReader::isViewFinal() const {
	return myMode == BOOK_TEXT_MODE;
}

void FBReader::showLibraryView() {
	if (ZLStringOption(ZLCategoryKey::LOOK_AND_FEEL, "ToggleButtonGroup", "booksOrder", "").value() == ActionCode::ORGANIZE_BOOKS_BY_TAG) {
		setView(myLibraryByTagView);
	} else {
		setView(myLibraryByAuthorView);
	}
}

void FBReader::setMode(ViewMode mode) {
	if (mode == myMode) {
		return;
	}

	if (mode != BOOK_TEXT_MODE) {
		myActionOnCancel = RETURN_TO_TEXT_MODE;
	}

	myPreviousMode = myMode;
	myMode = mode;

	switch (myMode) {
		case HYPERLINK_NAV_MODE:
		case BOOK_TEXT_MODE:
			setHyperlinkCursor(false);
			((ZLTextView&)*myBookTextView).forceScrollbarUpdate();
			setView(myBookTextView);
			break;
		case CONTENTS_MODE:
			((ContentsView&)*myContentsView).gotoReference();
			setView(myContentsView);
			break;
		case FOOTNOTE_MODE:
			setView(myFootnoteView);
			break;
		case LIBRARY_MODE:
		{
			shared_ptr<Book> currentBook = myModel->book();
			((LibraryView&)*myLibraryByAuthorView).showBook(currentBook);
			((LibraryView&)*myLibraryByTagView).showBook(currentBook);
			showLibraryView();
			break;
		}
		case BOOKMARKS_MODE:
			break;
		case NETWORK_LIBRARY_MODE:
			setView(myNetworkLibraryView);
			break;
		case UNDEFINED_MODE:
		case ALL_MODES:
			break;
	}
	refreshWindow();
}

BookTextView &FBReader::bookTextView() const {
	return (BookTextView&)*myBookTextView;
}

void FBReader::showBookTextView() {
	setMode(BOOK_TEXT_MODE);
}

void FBReader::restorePreviousMode() {
	if(myMode == HYPERLINK_NAV_MODE) {
		invertRegion(pageLinks.at(currentLinkIdx), true);
	}

	setMode(myPreviousMode);
	myPreviousMode = BOOK_TEXT_MODE;
}

bool FBReader::closeView() {
	if (myMode == BOOK_TEXT_MODE) {
		quit();
		return true;
	} else {
		restorePreviousMode();
		return false;
	}
}

std::string FBReader::helpFileName(const std::string &language) const {
	return ZLibrary::ApplicationDirectory() + ZLibrary::FileNameDelimiter + "help" + ZLibrary::FileNameDelimiter + "MiniHelp." + language + ".fb2";
}

void FBReader::openFile(const std::string &fileName) {
	shared_ptr<Book> book;
	createBook(fileName, book);
	if (!book.isNull()) {
		openBook(book);
		refreshWindow();
	}
}

void FBReader::clearTextCaches() {
	((ZLTextView&)*myBookTextView).clearCaches();
	((ZLTextView&)*myFootnoteView).clearCaches();
	((ZLTextView&)*myContentsView).clearCaches();
}

shared_ptr<ZLKeyBindings> FBReader::keyBindings() {
	return UseSeparateBindingsOption.value() ?
		keyBindings(rotation()) : myBindings0;
}

shared_ptr<ZLKeyBindings> FBReader::keyBindings(ZLView::Angle angle) {
	switch (angle) {
		case ZLView::DEGREES0:
			return myBindings0;
		case ZLView::DEGREES90:
			return myBindings90;
		case ZLView::DEGREES180:
			return myBindings180;
		case ZLView::DEGREES270:
			return myBindings270;
	}
	return 0;
}

shared_ptr<ProgramCollection> FBReader::dictionaryCollection() const {
	return myProgramCollectionMap.collection("Dictionary");
}

bool FBReader::isDictionarySupported() const {
	shared_ptr<ProgramCollection> collection = dictionaryCollection();
	return !collection.isNull() && !collection->currentProgram().isNull();
}

void FBReader::openInDictionary(const std::string &word) {
	shared_ptr<Program> dictionary = dictionaryCollection()->currentProgram();
	dictionary->run("present", ZLibrary::ApplicationName());
	dictionary->run("showWord", word);
}

shared_ptr<ProgramCollection> FBReader::webBrowserCollection() const {
	return myProgramCollectionMap.collection("Web Browser");
}

RecentBooks &FBReader::recentBooks() {
	return myRecentBooks;
}

const RecentBooks &FBReader::recentBooks() const {
	return myRecentBooks;
}

shared_ptr<Book> FBReader::currentBook() const {
	return myModel->book();
}

void FBReader::transformUrl(std::string &url) const {
	static const std::string LFROM_PREFIX = "lfrom=";
	if (url.find("litres.ru") != std::string::npos) {
		size_t index = url.find(LFROM_PREFIX);
		if (index == std::string::npos) {
			url = LitResUtil::appendLFrom(url);
		} else {
			size_t index2 = index + LFROM_PREFIX.size();
			while (index2 < url.size() && isdigit(url[index2])) {
				++index2;
			}
			url.replace(index, index2 - index, LitResUtil::LFROM);
		}
	}
}

void FBReader::invalidateNetworkView() {
	((NetworkView &) *myNetworkLibraryView).invalidate();
}

void FBReader::invalidateAccountDependents() {
	((NetworkView &) *myNetworkLibraryView).invalidateAccountDependents();
}
