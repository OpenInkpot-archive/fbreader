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

#include <ewl/Ewl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

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

#define FBR_FIFO "/tmp/.FBReader-fifo"

#ifndef NAME_MAX
#define NAME_MAX 4096
#endif

extern xcb_connection_t *connection;
extern xcb_window_t window;
ZLApplication *myapplication;
static bool in_main_loop;

class ZLEwlLibraryImplementation : public ZLibraryImplementation {

private:
	void init(int &argc, char **&argv);
	ZLPaintContext *createContext();
	void run(ZLApplication *application);
};

void initLibrary() {
	new ZLEwlLibraryImplementation();
}

void ZLEwlLibraryImplementation::init(int &argc, char **&argv) {
	int pid;
	FILE *pidof = popen("pidof FBReader", "r");
	if(pidof) {
		while(fscanf(pidof, "%d", &pid) != EOF && pid == getpid());
		pclose(pidof);

		do {
			if(pid <= 0 || pid == getpid())
				break;

			if(mkfifo(FBR_FIFO, 0666) && errno != EEXIST)
				break;

			kill(pid, SIGUSR1);

			int fifo = open(FBR_FIFO, O_WRONLY);
			if(!fifo) {
				unlink(FBR_FIFO);
				break;
			}

			char *p = argv[1];
			int ret;
			int len = strlen(p);
			while(len) {
				ret = write(fifo, p, len);
				if(ret == -1 && errno == EINTR)
					break;

				len -= ret;
				p += ret;
			}

			close(fifo);
			unlink(FBR_FIFO);
			exit(0);
		} while(0);
	}

	if(!ewl_init(&argc, argv)) {
		fprintf(stderr, "Unable to init EWL.\n");
	}		

	ZLibrary::parseArguments(argc, argv);

	XMLConfigManager::createInstance();
	ZLEwlFSManager::createInstance();
	ZLEwlTimeManager::createInstance();
	ZLEwlDialogManager::createInstance();
	ZLUnixCommunicationManager::createInstance();
	ZLEwlImageManager::createInstance();
	ZLEncodingCollection::instance().registerProvider(new IConvEncodingConverterProvider());

	//ZLKeyUtil::setKeyNamesFileName("keynames-xcb.xml");
}

ZLPaintContext *ZLEwlLibraryImplementation::createContext() {
	ZLEwlPaintContext *pc = new ZLEwlPaintContext();
	return (ZLPaintContext *)pc;
}

static struct {
	int keynum;
	char *keyname;
} _keys[] = {
	9, "Escape",
	10, "1",
	11, "2",
	12, "3",
	13, "4",
	14, "5",
	15, "6",
	16, "7",
	17, "8",
	18, "9",
	19, "0",
	36, "Return",
	64, "Alt_L",
	65, " ",
	82, "-",
	86, "+",
	119, "Delete",
	111, "Up",
	112, "Prior",
	113, "Left",
	114, "Right",
	116, "Down",
	117, "Next",
	124, "XF86PowerOff",
	147, "Menu",
	161, "XF86RotateWindows",
	172, "XF86AudioPlay",
	225, "XF86Search",
	0, NULL
};

bool _fbreader_closed;

void main_loop(ZLApplication *application)
{
	xcb_generic_event_t  *e;
	static bool alt_pressed = false;

	std::map<int, std::string> kmap;
	int i = 0;

	while(_keys[i].keynum) {
		kmap.insert(std::make_pair(_keys[i].keynum, _keys[i].keyname));
		i++;
	}


//	init_timer();

	_fbreader_closed = false;
	while (!_fbreader_closed) {
//		set_timer();
		in_main_loop = true;
		e = xcb_wait_for_event(connection);
		if(xcb_connection_has_error(connection)) {
			fprintf(stderr, "Connection to server closed\n");
			break;
		}
//		busy();
		if (e) {
			switch (e->response_type & ~0x80) {	
				case XCB_KEY_PRESS:
					{
						xcb_key_press_event_t *ev = (xcb_key_press_event_t *)e;

						if(!alt_pressed && kmap[ev->detail] == "ALT_L")
							alt_pressed = true;

						break;
					}
				case XCB_KEY_RELEASE:
					{
						xcb_key_release_event_t *ev = (xcb_key_release_event_t *)e;

						if(alt_pressed && kmap[ev->detail] == "ALT_L") {
							alt_pressed = false;
							continue;
						}

						//printf("ev->detail: %d %s\n", ev->detail, kmap[ev->detail].c_str());

						in_main_loop = false;
						if(alt_pressed)
							application->doActionByKey(std::string("Alt+") + kmap[ev->detail]);
						else
							application->doActionByKey(kmap[ev->detail]);
						in_main_loop = true;

						break;
					}
				case XCB_EXPOSE:
					{
						xcb_expose_event_t *expose = (xcb_expose_event_t *)e;

						application->refreshWindow();
						
						break;
					}
				case XCB_CONFIGURE_NOTIFY:
					{
						xcb_configure_notify_event_t *conf = (xcb_configure_notify_event_t *)e;

						//printf("resize: %d %d\n", conf->width, conf->height);	
						ZLEwlViewWidget *view = (ZLEwlViewWidget*)application->myViewWidget;
						if(view->width() != conf->width || view->height() != conf->height) {
							view->resize(conf->width, conf->height);
							application->refreshWindow();
						}
						
						break;
					}
			}
			free (e);
		}
	}
	//delete_timer();
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
	while((ret = read(fifo, p, len)) > 0) {
		len -= ret;
		p += ret;
	}
	*p = '\0';
	close(fifo);

	std::string filename(buf);
	if(filename.empty())
		return;

	FBReader *f = (FBReader*)myapplication;
	if(!f->myModel->fileName().compare(filename))
		return;

	BookDescriptionPtr description;
	f->createDescription(filename, description);
	if (!description.isNull()) {
		f->openBook(description);
		f->refreshWindow();
	}
}

void ZLEwlLibraryImplementation::run(ZLApplication *application) {
	struct sigaction act;

	ZLDialogManager::instance().createApplicationWindow(application);
	application->initWindow();

	myapplication = application;

	act.sa_handler = sigint_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGINT, &act, NULL);

	act.sa_handler = sigusr1_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGUSR1, &act, NULL);

	main_loop(application);
	delete application;
}
