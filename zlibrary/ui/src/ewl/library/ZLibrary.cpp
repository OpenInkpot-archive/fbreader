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

#include <sys/time.h>

#include <ewl/Ewl.h>

#include <ZLApplication.h>
#include <ZLibrary.h>

#include "../../../../core/src/unix/library/ZLibraryImplementation.h"

#include "../filesystem/ZLEwlFSManager.h"
#include "../time/ZLEwlTime.h"
#include "../dialogs/ZLEwlDialogManager.h"
#include "../image/ZLEwlImageManager.h"
#include "../view/ZLEwlPaintContext.h"
#include "../util/ZLEwlUtil.h"
#include "../../unix/message/ZLUnixMessage.h"
#include "../../../../core/src/util/ZLKeyUtil.h"
#include "../../../../core/src/unix/xmlconfig/XMLConfig.h"
#include "../../../../core/src/unix/iconv/IConvEncodingConverter.h"
#include "../../../../../fbreader/src/fbreader/FBReader.h"
#include "../../../../../fbreader/src/fbreader/BookTextView.h"

extern xcb_connection_t *connection;

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

void main_loop(ZLApplication *application)
{
	xcb_generic_event_t  *e;
	bool end = false;

//	init_timer();

	while (!end) {
//		set_timer();
		e = xcb_wait_for_event(connection);
//		busy();
		if (e) {
			switch (e->response_type & ~0x80) {
				case XCB_KEY_RELEASE:
					{
						xcb_key_release_event_t *ev;
						ev = (xcb_key_release_event_t *)e;

//						application->doActionByKey(ZLKeyUtil::keyName(ev->detail, ev->detail, ev->state));

						//printf("ev->detail: %d\n", ev->detail);

/*						switch (ev->detail) {
							// ESC
							case 9:
								application->doAction(ActionCode::CANCEL);
								end = true;
								break;

							case 19:
							case 90:
							case 111:
								application->doAction(ActionCode::LARGE_SCROLL_FORWARD);
								break;

							case 18:
							case 81:
							case 116:
								application->doAction(ActionCode::LARGE_SCROLL_BACKWARD);
								break;
						}
*/
					}
			}
			free (e);
		}
	}
	//delete_timer();
}

void ZLEwlLibraryImplementation::run(ZLApplication *application) {
	ZLDialogManager::instance().createApplicationWindow(application);
	application->initWindow();

	FBReader *f = (FBReader*)application;
	f->bookTextView().gotoPage(1);
	f->refreshWindow();
	shared_ptr<ZLView> view = f->currentView();


/*	struct timeval t1, t2;

	gettimeofday(&t1, NULL);
	for(int i = 0; i < 100; i++)
		((FBView&)*view).scrollAndUpdatePage(true, ZLTextView::NO_OVERLAPPING, 0);
	gettimeofday(&t2, NULL);

	int usec_diff = t2.tv_usec - t1.tv_usec;
	int sec_diff = t2.tv_sec - t1.tv_sec;
	if(usec_diff < 0) {
		sec_diff--;
		usec_diff = 1000000 + usec_diff;
	}

	printf("%d %d, %d %d\n", t1.tv_sec, t1.tv_usec, t2.tv_sec, t2.tv_usec);
	printf("100 pages: %d.%06d\n", sec_diff, usec_diff);
	exit(0);
*/	

	ewl_main();
//	main_loop(application);
	delete application;
}
