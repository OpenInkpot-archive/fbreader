/*
 * Copyright (C) 2004-2010 Geometer Plus <contact@geometerplus.com>
 * Copyright (C) 2009-2010 Alexander Kerner <lunohod@openinkpot.org>
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
#include <ZLTimeManager.h>
#include <ZLLogger.h>

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
#include "../network/NetworkLinkCollection.h"
#include "../networkActions/NetworkOperationRunnable.h"
#include "../networkTree/NetworkView.h"

#include "../migration/migrate.h"

#include "../options/FBCategoryKey.h"
#include "../bookmodel/BookModel.h"
#include "../formats/FormatPlugin.h"
#include "../../../zlibrary/ui/src/ewl/util/ZLEwlUtil.h"

#include "../database/booksdb/BooksDB.h"
#include "../database/booksdb/BooksDBUtil.h"
#include "../library/Book.h"

#include <ZLTextSelectionModel.h>

static const std::string OPTIONS = "Options";

const std::string FBReader::PageIndexParameter = "pageIndex";

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
	KeyScrollingDelayOption(ZLCategoryKey::CONFIG, "Scrollings", "Delay", 0, 2000, 100),
	LinesToScrollOption(ZLCategoryKey::CONFIG, "SmallScrolling", "LinesToScroll", 1, 20, 1),
	LinesToKeepOption(ZLCategoryKey::CONFIG, "LargeScrolling", "LinesToKeepOption", 0, 20, 0),
	EnableTapScrollingOption(ZLCategoryKey::CONFIG, "TapScrolling", "Enabled", true),
	TapScrollingOnFingerOnlyOption(ZLCategoryKey::CONFIG, "TapScrolling", "FingerOnly", true),
	UseSeparateBindingsOption(ZLCategoryKey::CONFIG, "KeysOptions", "UseSeparateBindings", false),
	EnableSingleClickDictionaryOption(ZLCategoryKey::CONFIG, "Dictionary", "SingleClick", false),
	myBindings0(new ZLKeyBindings("Keys")),
	myBindings90(new ZLKeyBindings("Keys90")),
	myBindings180(new ZLKeyBindings("Keys180")),
	myBindings270(new ZLKeyBindings("Keys270")),
	myBookToOpen(bookToOpen),
	myBookAlreadyOpen(false),
	myActionOnCancel(UNFULLSCREEN) {

	myBookTextView = new BookTextView(*context());
	myFootnoteView = new FootnoteView(*context());
	myContentsView = new ContentsView(*context());
	//myNetworkLibraryView = new NetworkView(*context());
	//myLibraryByAuthorView = new LibraryByAuthorView(*context());
	//myLibraryByTagView = new LibraryByTagView(*context());
	myRecentBooksPopupData = new RecentBooksPopupData();
	myMode = UNDEFINED_MODE;
	myPreviousMode = BOOK_TEXT_MODE;
	setMode(BOOK_TEXT_MODE);

	addAction(ActionCode::SHOW_READING, new UndoAction(FBReader::ALL_MODES & ~FBReader::BOOK_TEXT_MODE));
    /*
	addAction(ActionCode::SHOW_LIBRARY, new SetModeAction(FBReader::LIBRARY_MODE, FBReader::BOOK_TEXT_MODE | FBReader::CONTENTS_MODE));
	addAction(ActionCode::SHOW_NETWORK_LIBRARY, new ShowNetworkLibraryAction());
	addAction(ActionCode::SEARCH_ON_NETWORK, new SimpleSearchOnNetworkAction());
	addAction(ActionCode::ADVANCED_SEARCH_ON_NETWORK, new AdvancedSearchOnNetworkAction());
	registerPopupData(ActionCode::SHOW_LIBRARY, myRecentBooksPopupData);
    */
	addAction(ActionCode::SHOW_OPTIONS_DIALOG, new ShowOptionsDialogAction());
	addAction(ActionCode::SHOW_MENU, new ShowMenuDialogAction());
	addAction(ActionCode::SHOW_TOC, new ShowContentsAction());
	addAction(ActionCode::SHOW_BOOK_INFO_DIALOG, new ShowBookInfoAction());
	addAction(ActionCode::ADD_BOOK, new AddBookAction(FBReader::BOOK_TEXT_MODE | FBReader::LIBRARY_MODE | FBReader::CONTENTS_MODE));
	addAction(ActionCode::UNDO, new UndoAction(FBReader::BOOK_TEXT_MODE));
	addAction(ActionCode::REDO, new RedoAction());
	addAction(ActionCode::SEARCH, new SearchPatternAction());
	addAction(ActionCode::FIND_NEXT, new FindNextAction());
	addAction(ActionCode::FIND_PREVIOUS, new FindPreviousAction());
	addAction(ActionCode::SCROLL_TO_HOME, new ScrollToHomeAction());
	addAction(ActionCode::SCROLL_TO_START_OF_TEXT, new ScrollToStartOfTextAction());
	addAction(ActionCode::SCROLL_TO_END_OF_TEXT, new ScrollToEndOfTextAction());
	addAction(ActionCode::PAGE_SCROLL_FORWARD, new PageScrollingAction(true));
	addAction(ActionCode::PAGE_SCROLL_BACKWARD, new PageScrollingAction(false));
	addAction(ActionCode::LINE_SCROLL_FORWARD, new LineScrollingAction(true));
	addAction(ActionCode::LINE_SCROLL_BACKWARD, new LineScrollingAction(false));
	addAction(ActionCode::MOUSE_SCROLL_FORWARD, new MouseWheelScrollingAction(true));
	addAction(ActionCode::MOUSE_SCROLL_BACKWARD, new MouseWheelScrollingAction(false));
	addAction(ActionCode::TAP_SCROLL_FORWARD, new TapScrollingAction(true));
	addAction(ActionCode::TAP_SCROLL_BACKWARD, new TapScrollingAction(false));
	addAction(ActionCode::INCREASE_FONT, new ChangeFontSizeAction(1));
	addAction(ActionCode::DECREASE_FONT, new ChangeFontSizeAction(-1));
	addAction(ActionCode::ROTATE_SCREEN, new RotationAction());
	addAction(ActionCode::TOGGLE_FULLSCREEN, new FBFullscreenAction());
	addAction(ActionCode::FULLSCREEN_ON, new FBFullscreenAction());
	addAction(ActionCode::CANCEL, new CancelAction());
	addAction(ActionCode::SHOW_HIDE_POSITION_INDICATOR, new ToggleIndicatorAction());
	addAction(ActionCode::QUIT, new QuitAction());
	addAction(ActionCode::FORCE_QUIT, new ForceQuitAction());
	addAction(ActionCode::OPEN_PREVIOUS_BOOK, new OpenPreviousBookAction());
	addAction(ActionCode::SHOW_HELP, new ShowHelpAction());
	addAction(ActionCode::GOTO_NEXT_TOC_SECTION, new GotoNextTOCSectionAction());
	addAction(ActionCode::GOTO_PREVIOUS_TOC_SECTION, new GotoPreviousTOCSectionAction());
	addAction(ActionCode::COPY_SELECTED_TEXT_TO_CLIPBOARD, new CopySelectedTextAction());
	addAction(ActionCode::OPEN_SELECTED_TEXT_IN_DICTIONARY, new OpenSelectedTextInDictionaryAction());
	addAction(ActionCode::CLEAR_SELECTION, new ClearSelectionAction());
	addAction(ActionCode::GOTO_PAGE_NUMBER, new GotoPageNumberAction(std::string()));
	addAction(ActionCode::GOTO_PAGE_NUMBER_WITH_PARAMETER, new GotoPageNumberAction(PageIndexParameter));
	shared_ptr<Action> booksOrderAction = new BooksOrderAction();
	addAction(ActionCode::ORGANIZE_BOOKS_BY_AUTHOR, booksOrderAction);
	addAction(ActionCode::ORGANIZE_BOOKS_BY_TAG, booksOrderAction);
	addAction(ActionCode::FILTER_LIBRARY, new FilterLibraryAction());

	addAction(ActionCode::SHOW_FOOTNOTES, new ShowFootnotes());
	addAction(ActionCode::HYPERLINK_NAV_START, new HyperlinkNavStart());
	addAction(ActionCode::OK_ACTION, new OkAction());
	addAction(ActionCode::UP_ACTION, new UpAction());
	addAction(ActionCode::DOWN_ACTION, new DownAction());
	addAction(ActionCode::LEFT_ACTION, new LeftAction());
	addAction(ActionCode::RIGHT_ACTION, new RightAction());
	addAction(ActionCode::DICT, new Dict());
	addAction(ActionCode::BMK_ADD, new BookmarkAdd());
	addAction(ActionCode::BMK_SHOW, new BookmarksShow());

	addAction(ActionCode::HELP, new Help());
	addAction(ActionCode::BOLD_TOGGLE, new BoldToggle());
	addAction(ActionCode::TURBO_TOGGLE, new TurboToggle());
	addAction(ActionCode::FONT_SIZE_DIALOG, new FontSizeDialog());

	myOpenFileHandler = new OpenFileHandler();
	//ZLCommunicationManager::Instance().registerHandler("openFile", myOpenFileHandler);
}

FBReader::~FBReader() {
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
	}

	if (!myBookAlreadyOpen) {
		shared_ptr<Book> book;
		if (!myBookToOpen.empty()) {
			createBook(myBookToOpen, book);
		}
		if (book.isNull()) {
			const BookList &books = Library::Instance().recentBooks();
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

	//ZLTimeManager::Instance().addTask(new TimeUpdater(), 1000);
}

void FBReader::refreshWindow() {
	ZLApplication::refreshWindow();
	((RecentBooksPopupData&)*myRecentBooksPopupData).updateId();
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
		bookTextView.setModel(0, 0);
		bookTextView.setContentsModel(0);
		contentsView.setModel(0);
		myModel.reset();
		myModel = new BookModel(book);
		ZLTextHyphenator::Instance().load(book->language());
		bookTextView.setModel(myModel->bookTextModel(), book);
		bookTextView.setCaption(book->title());
		bookTextView.setContentsModel(myModel->contentsModel());
		footnoteView.setModel(0);
		footnoteView.setCaption(book->title());
		contentsView.setModel(myModel->contentsModel());
		contentsView.setCaption(book->title());

		Library::Instance().addBook(book);
		Library::Instance().addBookToRecentList(book);
		((RecentBooksPopupData&)*myRecentBooksPopupData).updateId();
		showBookTextView();
	}
}

void FBReader::openLinkInBrowser(const std::string &url) const {
	if (url.empty()) {
		return;
	}
	shared_ptr<ProgramCollection> collection = webBrowserCollection();
	if (collection.isNull()) {
		return;
	}
	shared_ptr<Program> program = collection->currentProgram();
	if (program.isNull()) {
		return;
	}
	std::string copy = url;
	//NetworkLinkCollection::Instance().rewriteUrl(copy, true);
	ZLLogger::Instance().println("URL", copy);
	program->run("openLink", copy);
}

void FBReader::tryShowFootnoteView(const std::string &id, const std::string &type) {
	if (type == "external") {
		openLinkInBrowser(id);
	} else if (type == "internal") {
		if (((myMode == BOOK_TEXT_MODE) || (myMode == FOOTNOTE_MODE) || (myMode == HYPERLINK_NAV_MODE)) && !myModel.isNull()) {
			BookModel::Label label = myModel->label(id);
			if (!label.Model.isNull()) {
				if ((myMode != FOOTNOTE_MODE) && (label.Model == myModel->bookTextModel())) {
					bookTextView().gotoParagraph(label.ParagraphNumber);
				} else {
					FootnoteView &view = ((FootnoteView&)*myFootnoteView);
					view.setModel(label.Model);
					setMode(FOOTNOTE_MODE);
					view.gotoParagraph(label.ParagraphNumber);
				}
				setHyperlinkCursor(false);
				refreshWindow();
			}
		}
	} else if (type == "book") {
        /*
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
        */
	}
}

void FBReader::invertRegion(HyperlinkCoord link, bool flush)
{
	((ZLApplication*)this)->invertRegion(link.x0, link.y0, link.x1, link.y1, flush);
}

void FBReader::invertRegion(ZLTextElementRectangle e)
{
	const ZLTextArea &area = ((ZLTextView&)*myBookTextView).textArea();

	e.XStart += area.hOffset();
	e.XEnd += area.hOffset();
	e.YStart += area.vOffset();
	e.YEnd += area.vOffset();

	((ZLApplication*)this)->invertRegion(
		e.XStart,
		e.YStart,
		e.XEnd,
		e.YEnd,
		true);
}

inline bool FBReader::isword(ZLTextElementIterator e)
{
	bool ret = false;
	ZLTextSelectionModel &sm = ((BookTextView*)&*myBookTextView)->selectionModel();

	const ZLTextArea &area = ((ZLTextView&)*myBookTextView).textArea();
	ZLTextElementRectangle r = *e;

	r.XStart += area.hOffset();
	r.XEnd += area.hOffset();
	r.YStart += area.vOffset();
	r.YEnd += area.vOffset();

	if (r.Kind == ZLTextElement::WORD_ELEMENT) {
		sm.selectWord(r.XStart, r.YStart);
		if (!sm.text().empty())
			ret = true;
		sm.clear();
	}

	return ret;
}

void FBReader::highlightFirstWord()
{
	//ZLTextSelectionModel &sm = ((BookTextView*)&*myBookTextView)->selectionModel();
	const ZLTextElementMap &elementMap = ((ZLTextView&)*myBookTextView).textArea().myTextElementMap;

	for (word_it = elementMap.begin(); word_it != elementMap.end(); ++word_it) {
		if(isword(word_it)) {
			setMode(DICT_MODE);
			invertRegion(*word_it);
			return;
		}
	}
}

void FBReader::highlightNextWord()
{
	//ZLTextSelectionModel &sm = ((BookTextView*)&*myBookTextView)->selectionModel();
	const ZLTextElementMap &elementMap = ((ZLTextView&)*myBookTextView).textArea().myTextElementMap;

	ZLTextElementIterator w = word_it;

	do {
		if(w == elementMap.end() || w+1 == elementMap.end())
			w = elementMap.begin();
		else
			w++;

		if (isword(w)) {
			invertRegion(*word_it);
			invertRegion(*w);
			word_it = w;
			return;
		}
	} while (w != word_it);
}

void FBReader::highlightNextLineWord()
{
	//ZLTextSelectionModel &sm = ((BookTextView*)&*myBookTextView)->selectionModel();
	const ZLTextElementMap &elementMap = ((ZLTextView&)*myBookTextView).textArea().myTextElementMap;
	//int y = word_it->YStart;
	bool cycle = false;

	ZLTextElementIterator w = word_it;

	do {
		if(w == elementMap.end() || w+1 == elementMap.end()) {
			w = elementMap.begin();
			cycle = true;
		} else
			w++;

		if ((cycle || w->YStart > word_it->YStart) && isword(w)) {

			invertRegion(*word_it);
			invertRegion(*w);
			word_it = w;

			return;
		}
	} while (w != word_it);
}

void FBReader::highlightPrevWord()
{
	//ZLTextSelectionModel &sm = ((BookTextView*)&*myBookTextView)->selectionModel();
	const ZLTextElementMap &elementMap = ((ZLTextView&)*myBookTextView).textArea().myTextElementMap;

	ZLTextElementIterator w = word_it;

	do {
		if(w == elementMap.begin())
			w = elementMap.end();

		w--;

		if (isword(w)) {
			invertRegion(*word_it);
			invertRegion(*w);
			word_it = w;
			return;
		}
	} while (w != word_it);
}

void FBReader::highlightPrevLineWord()
{
	//ZLTextSelectionModel &sm = ((BookTextView*)&*myBookTextView)->selectionModel();
	const ZLTextElementMap &elementMap = ((ZLTextView&)*myBookTextView).textArea().myTextElementMap;
	//int y = word_it->YStart;
	bool cycle = false;

	ZLTextElementIterator w = word_it;

	// find a word on previous line
	do {
		if(w == elementMap.begin()) {
			w = elementMap.end();
			cycle = true;
		}

		w--;

		if ((cycle || w->YStart < word_it->YStart) && isword(w))
			break;
	} while (w != word_it);

	if(w == word_it)
		return;

	// find first word on current line
	ZLTextElementIterator w2 = w;
	while(w2->YStart == w->YStart) {
		w2--;
		if(w2->YStart != w->YStart)
			break;

		if (isword(w2))
			w = w2;
	}

	invertRegion(*word_it);
	invertRegion(*w);
	word_it = w;
}

void FBReader::highlightWordOnLineAtY(int ycoord)
{
	const ZLTextElementMap &elementMap = ((ZLTextView&)*myBookTextView).textArea().myTextElementMap;

	for (ZLTextElementIterator w = elementMap.begin(); w != elementMap.end(); ++w) {
		if(!isword(w))
			continue;

		if(w->YStart > ycoord) {
			invertRegion(*word_it);
			word_it = w;
			invertRegion(*word_it);
			return;
		}
	}
}

void FBReader::openDict()
{
	const ZLTextArea &area = ((ZLTextView&)*myBookTextView).textArea();
	ZLTextSelectionModel &sm = ((BookTextView*)&*myBookTextView)->selectionModel();
	sm.selectWord(word_it->XStart + area.hOffset(), word_it->YStart + area.vOffset());
	if(isDictionarySupported())
		openInDictionary(ZLUnicodeUtil::toLower(sm.text()));
	sm.clear();
	restorePreviousMode();
}

void FBReader::highlightCurrentLink()
{
	for(int i = currentLinkIdx; (i >= 0) && (i < (int)pageLinks.size()); i++) {
		if(!pageLinks.at(i).next || (i == ((int)pageLinks.size() - 1))) {
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

void FBReader::_refreshWindow() {
	ZLApplication::refreshWindow();
	if(myMode == DICT_MODE)
		invertRegion(*word_it);
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
		case DICT_MODE:
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
			//((LibraryView&)*myLibraryByAuthorView).showBook(currentBook);
			//((LibraryView&)*myLibraryByTagView).showBook(currentBook);
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
	if(myMode == DICT_MODE)
		invertRegion(*word_it);

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

void FBReader::openFile(const std::string &filePath) {
	shared_ptr<Book> book;
	createBook(filePath, book);
	if (!book.isNull()) {
		openBook(book);
		refreshWindow();
	}
}

bool FBReader::canDragFiles(const std::vector<std::string> &filePaths) const {
	switch (myMode) {
		case BOOK_TEXT_MODE:
		case FOOTNOTE_MODE:
		case CONTENTS_MODE:
		case LIBRARY_MODE:
			return filePaths.size() > 0;
		default:
			return false;
	}
}

void FBReader::dragFiles(const std::vector<std::string> &filePaths) {
	switch (myMode) {
		case BOOK_TEXT_MODE:
		case FOOTNOTE_MODE:
		case CONTENTS_MODE:
			if (filePaths.size() > 0) {
				openFile(filePaths[0]);
			}
			break;
		case LIBRARY_MODE:
			if (filePaths.size() > 0) {
				openFile(filePaths[0]);
			}
			break;
		default:
			break;
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

shared_ptr<Book> FBReader::currentBook() const {
	return myModel->book();
}

void FBReader::invalidateNetworkView() {
	((NetworkView &) *myNetworkLibraryView).invalidate();
}

void FBReader::invalidateAccountDependents() {
	((NetworkView &) *myNetworkLibraryView).invalidateAccountDependents();
}
