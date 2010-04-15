/*
 * Copyright (C) 2008 Alexander Kerner <lunohod@openinkpot.org>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

#include <Ecore.h>

#include <ZLApplication.h>
#include <ZLibrary.h>

#include "../../../../core/src/unix/library/ZLibraryImplementation.h"

#include "../filesystem/ZLEwlFSManager.h"
#include "../time/ZLEwlTime.h"
#include "../dialogs/ZLEwlDialogManager.h"
#include "../image/ZLEwlImageManager.h"
#include "../view/ZLEwlPaintContext.h"
#include "../view/ZLEwlViewWidget.h"
#include "../util/ZLEwlUtil.h"
#include "../../unix/message/ZLUnixMessage.h"
#include "../../../../core/src/util/ZLKeyUtil.h"
#include "../../../../core/src/unix/xmlconfig/XMLConfig.h"
#include "../../../../core/src/unix/iconv/IConvEncodingConverter.h"
#include "../../../../../fbreader/src/fbreader/FBReader.h"
#include "../../../../../fbreader/src/bookmodel/BookModel.h"
#include "../../../../../fbreader/src/fbreader/BookTextView.h"
#include "../../../../../zlibrary/text/include/ZLTextPositionIndicator.h"
#include "../../../../../fbreader/src/library/Book.h"
#include "../../../../../fbreader/src/library/Author.h"

extern "C" {
#include <xcb/xcb_atom.h>
#include <xcb/randr.h>
}

extern const xcb_atom_t INTEGER;

#define FBR_FIFO "/tmp/.FBReader-fifo"
#define PIDFILE "/tmp/fbreader.pid"

#ifndef NAME_MAX
#define NAME_MAX 4096
#endif

extern xcb_connection_t *connection;
extern xcb_window_t window;
extern xcb_screen_t *screen;
ZLApplication *myapplication;
static bool in_main_loop;

std::string cover_image_file = "";

static void init_properties();
static void set_properties();
static void delete_properties();

void set_busy_cursor(bool set)
{
	xcb_cursor_t cursor = 0;

	if(set) {
		int shape = 26;

		xcb_font_t font = xcb_generate_id(connection);
		xcb_open_font(connection, font, strlen("cursor"), "cursor");

		cursor = xcb_generate_id(connection);
		xcb_create_glyph_cursor (connection,
				cursor,
				font,
				font,
				shape,
				shape + 1,
				0, 0, 0,
				65535, 65535, 65535);

		xcb_close_font(connection, font);
	}

	uint32_t value_list = cursor;

	xcb_change_window_attributes(connection, window,
			XCB_CW_CURSOR, &value_list);

	if(cursor)
		xcb_free_cursor(connection, cursor);
}

char *get_rotated_key(char **keys)
{
	xcb_randr_get_screen_info_cookie_t cookie;
	xcb_randr_get_screen_info_reply_t *reply;
	xcb_randr_rotation_t rotation;

	cookie = xcb_randr_get_screen_info(connection, screen->root);
	reply = xcb_randr_get_screen_info_reply(connection, cookie, NULL);

	rotation = (xcb_randr_rotation_t)reply->rotation;
	free(reply);

	switch(rotation) {
		case XCB_RANDR_ROTATION_ROTATE_0:
			return keys[0];
		case XCB_RANDR_ROTATION_ROTATE_90:
			return keys[1];
		case XCB_RANDR_ROTATION_ROTATE_180:
			return keys[2];
		case XCB_RANDR_ROTATION_ROTATE_270:
			return keys[3];
		default:
			return keys[0];
	}
}

class ZLEwlLibraryImplementation : public ZLibraryImplementation {

	private:
		void init(int &argc, char **&argv);
		ZLPaintContext *createContext();
		void run(ZLApplication *application);
};

void initLibrary() {
	new ZLEwlLibraryImplementation();
}

void sigalrm_handler(int)
{
}

void ZLEwlLibraryImplementation::init(int &argc, char **&argv) {
	int pid;
	struct stat pid_stat;
	FILE *pidfile;

	int done;

	do {
		done = 1;

		if(stat(PIDFILE, &pid_stat) == -1)
			break;

		pidfile = fopen(PIDFILE, "r");
		if(!pidfile)
			break;

		fscanf(pidfile, "%d", &pid);
		fclose(pidfile);

		if(!pid)
			break;

		if(pid <= 0 || pid == getpid() || kill(pid, 0))
			break;

		struct sigaction act;
		act.sa_handler = sigalrm_handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0;
		sigaction(SIGALRM, &act, NULL);

		if(mkfifo(FBR_FIFO, 0666) && errno != EEXIST)
			break;

		kill(pid, SIGUSR1);

		alarm(1);
		int fifo = open(FBR_FIFO, O_WRONLY);
		if(fifo < 0) {
			if(errno == EINTR) {
				done = 0;
				continue;
			}

			unlink(FBR_FIFO);
			break;
		}

		char *p;
		if(argc > 1)
			p = argv[1];
		else
			p = "";
		int len = strlen(p);
		int ret;
		while(len) {
			ret = write(fifo, p, len);
			if(ret == -1 && errno != EINTR)
				break;

			len -= ret;
			p += ret;
		}

		close(fifo);
		unlink(FBR_FIFO);
		exit(0);
	} while(!done);

	pidfile = fopen(PIDFILE, "w");
	if(pidfile) {
		fprintf(pidfile, "%d", getpid());
		fclose(pidfile);
	}

	ZLibrary::parseArguments(argc, argv);

	XMLConfigManager::createInstance();
	ZLEwlFSManager::createInstance();
	ZLEwlTimeManager::createInstance();
	ZLEwlDialogManager::createInstance();
	ZLUnixCommunicationManager::createInstance();
	ZLEwlImageManager::createInstance();
	ZLEncodingCollection::Instance().registerProvider(new IConvEncodingConverterProvider());

	//ZLKeyUtil::setKeyNamesFileName("keynames-xcb.xml");
}

ZLPaintContext *ZLEwlLibraryImplementation::createContext() {
	ZLEwlPaintContext *pc = new ZLEwlPaintContext();
	return (ZLPaintContext *)pc;
}

static char *cursor_keys[] = { "Up", "Right", "Down", "Left", "Up", "Right", "Down", "Left" };
static char *jog_keys[] = { "Prior", "Next", "Next", "Prior", "Next", "Prior", "Prior", "Next" };

struct _key {
	int keynum;
	const char *keyname;
	char **rotkeys;
};

static struct _key _keys[] = {
	{ 9, "Escape", NULL },
	{ 10, "1", NULL },
	{ 11, "2", NULL },
	{ 12, "3", NULL },
	{ 13, "4", NULL },
	{ 14, "5", NULL },
	{ 15, "6", NULL },
	{ 16, "7", NULL },
	{ 17, "8", NULL },
	{ 18, "9", NULL },
	{ 19, "0", NULL },
	{ 36, "Return", NULL },
	{ 64, "Alt_L", NULL },
	{ 65, " ", NULL },
	{ 82, "-", NULL },
	{ 86, "+", NULL },
	{ 119, "Delete", NULL },
	{ 111, "Up", &cursor_keys[0] },
	{ 112, "Prior", &jog_keys[0] },
	{ 113, "Left", &cursor_keys[3] },
	{ 114, "Right", &cursor_keys[1] },
	{ 116, "Down", &cursor_keys[2] },
	{ 117, "Next", &jog_keys[4] },
	{ 124, "XF86PowerOff", NULL },
	{ 147, "Menu", NULL },
//	{ 161, "XF86RotateWindows", NULL },
	{ 172, "XF86AudioPlay", NULL },
	{ 225, "XF86Search", NULL },
	{ 0, NULL, NULL }
};

bool _fbreader_closed;

bool isConfigure(xcb_generic_event_t *e) {
	return (e->response_type & ~0x80) == XCB_CONFIGURE_NOTIFY;
}

bool isExpose(xcb_generic_event_t *e) {
	return (e->response_type & ~0x80) == XCB_EXPOSE;
}

void sigint_handler(int)
{
	exit(1);
}

void sigusr1_handler(int)
{
	if(!(window && connection))
		return;

	if(!in_main_loop)
		ecore_main_loop_quit();

	// raise window
	uint32_t value_list = XCB_STACK_MODE_ABOVE;
	xcb_configure_window(connection, window, XCB_CONFIG_WINDOW_STACK_MODE, &value_list);
	xcb_flush(connection);

	int fifo = open(FBR_FIFO, O_RDONLY);
	if(!fifo)
		return;

	char buf[NAME_MAX];
	char *p = buf;
	int ret;
	int len = NAME_MAX - 1;
	while(len > 0 && (ret = read(fifo, p, len)) > 0) {
		len -= ret;
		p += ret;
	}
	*p = '\0';
	close(fifo);

	std::string filename(buf);
	if(filename.empty())
		return;

	FBReader *f = (FBReader*)myapplication;
	if(!f->myModel->book()->filePath().compare(filename))
		return;

	shared_ptr<Book> book;
	f->createBook(filename, book);
	if (!book.isNull()) {
		cover_image_file = "";

		f->openBook(book);
		f->refreshWindow();
		set_properties();
	}
}

void main_loop(ZLApplication *application)
{
	xcb_generic_event_t  *e;
	xcb_visibility_t visibility;

	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t *reply = NULL;

	xcb_atom_t wm_protocols_atom, wm_delete_window_atom;

	cookie = xcb_intern_atom_unchecked(connection, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
	reply = xcb_intern_atom_reply(connection, cookie, NULL);
	wm_delete_window_atom = reply->atom;
	free(reply);

	cookie = xcb_intern_atom_unchecked(connection, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
	reply = xcb_intern_atom_reply(connection, cookie, NULL);
	wm_protocols_atom = reply->atom;
	free(reply);

	static bool alt_pressed = false;

	std::map<int, struct _key *> kmap;
	int i = 0;

	while(_keys[i].keynum) {
		kmap.insert(std::make_pair(_keys[i].keynum, &_keys[i]));
		i++;
	}

	std::vector<xcb_generic_event_t *> efifo;

	_fbreader_closed = false;
	while (!_fbreader_closed) {
		in_main_loop = true;

		if(efifo.empty()) {
			e = xcb_wait_for_event(connection);
			efifo.push_back(e);
		}

		while(e = xcb_poll_for_event(connection))
			efifo.push_back(e);

		if(xcb_connection_has_error(connection)) {
			fprintf(stderr, "Connection to server closed\n");
			break;
		}

		if(efifo.empty())
			continue;
		else {
			e = efifo.front();
			efifo.erase(efifo.begin());
		}

		if (e) {
			switch (e->response_type & ~0x80) {	
				case XCB_KEY_PRESS:
					{
						xcb_key_press_event_t *ev = (xcb_key_press_event_t *)e;

						if(!alt_pressed && kmap[ev->detail]->keyname == "Alt_L")
							alt_pressed = true;

						break;
					}
				case XCB_KEY_RELEASE:
					{
						xcb_key_release_event_t *ev = (xcb_key_release_event_t *)e;

						if(alt_pressed && kmap[ev->detail]->keyname == "Alt_L") {
							alt_pressed = false;
							continue;
						}

						//printf("ev->detail: %d %s\n", ev->detail, kmap[ev->detail].c_str());
						std::string pressed_key;

						struct _key *s_key = kmap[ev->detail];
						if(s_key->rotkeys != NULL)
							pressed_key = get_rotated_key(s_key->rotkeys);
						else
							pressed_key = s_key->keyname;

						in_main_loop = false;
						if(alt_pressed)
							application->doActionByKey(std::string("Alt+") + pressed_key);
						else
							application->doActionByKey(pressed_key);
						in_main_loop = true;

						break;
					}
				case XCB_VISIBILITY_NOTIFY:
					{
						xcb_visibility_notify_event_t *v = (xcb_visibility_notify_event_t *)e;
						visibility = (xcb_visibility_t)v->state;

						break;
					}
				case XCB_EXPOSE:
					{
						if(count_if(efifo.begin(), efifo.end(), isExpose) > 0)
							break;

						xcb_expose_event_t *expose = (xcb_expose_event_t *)e;

						if(visibility != XCB_VISIBILITY_FULLY_OBSCURED) {
							((FBReader*)application)->_refreshWindow();
						}
						
						break;
					}
				case XCB_CONFIGURE_NOTIFY:
					{
						if(count_if(efifo.begin(), efifo.end(), isConfigure) > 0)
							break;

						xcb_configure_notify_event_t *conf = (xcb_configure_notify_event_t *)e;
						ZLEwlViewWidget *view = (ZLEwlViewWidget*)application->myViewWidget;
						if(view->width() != conf->width || view->height() != conf->height) {
							view->resize(conf->width, conf->height);
							((FBReader*)application)->showBookTextView();
						}

						break;
					}
				case XCB_CLIENT_MESSAGE:
					{
						xcb_client_message_event_t *msg = (xcb_client_message_event_t *)e;
						if((msg->type == wm_protocols_atom) &&
								(msg->format == 32) &&
								(msg->data.data32[0] == (uint32_t)wm_delete_window_atom)) {
							_fbreader_closed = true;
						}
                        break;
					}
                case XCB_UNMAP_WINDOW:
                    {
                        struct sigaction act;

                        act.sa_handler = sigint_handler;
                        sigemptyset(&act.sa_mask);
                        act.sa_flags = 0;
                        sigaction(SIGINT, &act, NULL);

                        act.sa_handler = sigusr1_handler;
                        sigemptyset(&act.sa_mask);
                        act.sa_flags = 0;
                        sigaction(SIGUSR1, &act, NULL);

                        break;
                    }
			}
			free (e);
		}
	}
	//delete_timer();
}

static struct atom {
	char *name;
	xcb_atom_t atom;
} atoms[] = {
	"UTF8_STRING", 0,
	"ACTIVE_DOC_AUTHOR", 0,
	"ACTIVE_DOC_TITLE", 0,
	"ACTIVE_DOC_FILENAME", 0,
	"ACTIVE_DOC_FILEPATH", 0,
	"ACTIVE_DOC_SERIES", 0,
	"ACTIVE_DOC_SERIES_NUMBER", 0,
	"ACTIVE_DOC_TYPE", 0,
	"ACTIVE_DOC_SIZE", 0,
	"ACTIVE_DOC_CURRENT_POSITION", 0,
	"ACTIVE_DOC_CURRENT_PAGE", 0,
	"ACTIVE_DOC_PAGES_COUNT", 0,
	"ACTIVE_DOC_WINDOW_ID", 0,
	"ACTIVE_DOC_COVER_IMAGE", 0,
};

static void init_properties()
{
	if(!connection)
		return;

	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t *reply = NULL;

	int atoms_cnt = sizeof(atoms) / sizeof(struct atom);
	for(int i = 0; i < atoms_cnt; i++) {
		cookie = xcb_intern_atom_unchecked(connection, 0, strlen(atoms[i].name), atoms[i].name);
		reply = xcb_intern_atom_reply(connection, cookie, NULL);
		atoms[i].atom = reply->atom;
		free(reply);
	}
}

void set_properties()
{
	if(!(window && connection))
		return;

	FBReader *f = (FBReader*)myapplication;
	shared_ptr<Book> book = f->myModel->book();
	std::string fileName = book->filePath();

#define set_prop_str(__i__, __prop__) \
	xcb_change_property(connection, \
			XCB_PROP_MODE_REPLACE, \
			window, \
			atoms[(__i__)].atom, \
			atoms[0].atom, \
			8, \
			strlen((__prop__)), \
			(__prop__));

#define set_prop_int(__i__, __prop__) \
	{ \
	int i = (__prop__); \
	xcb_change_property(connection, \
			XCB_PROP_MODE_REPLACE, \
			window, \
			atoms[(__i__)].atom, \
			INTEGER, \
			32, \
			1, \
			(unsigned char*)&i); \
	}

	std::string authors;
	for(int j = 0; j < book->authors().size(); j++) {
		if(!authors.empty())
			authors += ", ";

		authors += book->authors().at(j)->name();
	}

	set_prop_str(1, authors.c_str());
	set_prop_str(2, book->title().c_str());
	set_prop_str(3, ZLFile::fileNameToUtf8(ZLFile(fileName).name(false)).c_str());
	set_prop_str(4, ZLFile::fileNameToUtf8(ZLFile(fileName).path()).c_str());
	set_prop_str(5, book->seriesTitle().c_str());
	set_prop_int(6, book->indexInSeries());
/*	set_prop_int(9, f->bookTextView().positionIndicator()->textPosition());
	set_prop_int(10, f->bookTextView().positionIndicator()->currentPage());
	set_prop_int(11, f->bookTextView().positionIndicator()->pagesCount());
*/

	xcb_change_property(connection,
			XCB_PROP_MODE_REPLACE,
			screen->root,
			atoms[12].atom,
			WINDOW,
			sizeof(xcb_window_t) * 8,
			1,
			(unsigned char*)&window);

	xcb_change_property(connection,
			XCB_PROP_MODE_REPLACE,
			window,
			WM_NAME,
			atoms[0].atom,
			8,
			strlen("FBReader"),
			"FBReader");

	xcb_change_property(connection,
			XCB_PROP_MODE_REPLACE,
			window,
			WM_CLASS,
			STRING,
			8,
			strlen("FBReader") * 2 + 2,
			"FBReader\0FBReader");

	xcb_change_property(connection,
			XCB_PROP_MODE_REPLACE,
			window,
			atoms[13].atom,
			STRING,
			8,
			cover_image_file.length(),
			cover_image_file.c_str());

	xcb_flush(connection);
}

void delete_properties()
{
	xcb_delete_property(connection,
			screen->root,
			atoms[12].atom);

	xcb_flush(connection);
}

void update_position_property()
{
	if(!atoms[9].atom)
		return;

	FBReader *f = (FBReader*)myapplication;
	set_prop_int(9, f->bookTextView().positionIndicator()->textPosition());
	set_prop_int(10, f->bookTextView().positionIndicator()->currentPage());
	set_prop_int(11, f->bookTextView().positionIndicator()->pagesCount());
}

void ZLEwlLibraryImplementation::run(ZLApplication *application) {
	struct sigaction act;

	ZLDialogManager::Instance().createApplicationWindow(application);

	myapplication = application;
	application->initWindow();
	set_busy_cursor(true);

	act.sa_handler = sigint_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);

	act.sa_handler = sigusr1_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGUSR1, &act, NULL);

	init_properties();
	set_properties();

	set_busy_cursor(false);
	main_loop(application);
	unlink(PIDFILE);
	delete_properties();
	delete application;
}
