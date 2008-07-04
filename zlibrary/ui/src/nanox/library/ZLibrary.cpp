/*
 * Copyright (C) 2008 Alexander Egorov <lunohod@gmx.de>
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

#include <ZLApplication.h>
#include <ZLibrary.h>

#include "../../../../core/src/unix/library/ZLibraryImplementation.h"

#include "../../unix/message/ZLUnixMessage.h"
#include "../../../../core/src/unix/xmlconfig/XMLConfig.h"
#include "../../../../core/src/unix/iconv/IConvEncodingConverter.h"

#include "../filesystem/ZLNXFSManager.h"
#include "../time/ZLNXTime.h"
#include "../view/ZLNXPaintContext.h"
#include "../dialogs/ZLNXDialogManager.h"
#include "../image/ZLNXImageManager.h"

#include "../../../../../fbreader/src/fbreader/FBReaderActions.h"

class ZLApplication;
ZLApplication *mainApplication;

extern xcb_connection_t *connection;

class ZLNXLibraryImplementation : public ZLibraryImplementation {
	private:
		void init(int &argc, char **&argv);
		ZLPaintContext *createContext();
		void run(ZLApplication *application);
};

void initLibrary() {
	//printf("initLibrary\n");
	new ZLNXLibraryImplementation();
}

void ZLNXLibraryImplementation::init(int &argc, char **&argv) {
	//printf("init\n");
	ZLibrary::parseArguments(argc, argv);

	XMLConfigManager::createInstance();
	ZLNXFSManager::createInstance();
	ZLNXTimeManager::createInstance();
	ZLNXDialogManager::createInstance();
	ZLUnixCommunicationManager::createInstance();
	ZLNXImageManager::createInstance();
	ZLEncodingCollection::instance().registerProvider(new IConvEncodingConverterProvider());
}

ZLPaintContext *ZLNXLibraryImplementation::createContext() {
	return new ZLNXPaintContext();
}

void ZLNXLibraryImplementation::run(ZLApplication *application) {
	//printf("ZLNXLibraryImplementation::rrun\n");
	ZLDialogManager::instance().createApplicationWindow(application);

	application->initWindow();

//key press events
	bool end = false;
	xcb_generic_event_t  *e;
	while (!end) {
		e = xcb_wait_for_event(connection);
		if (e) {
			switch (e->response_type & ~0x80) {
				case XCB_KEY_RELEASE: 
					{
						xcb_key_release_event_t *ev;

						ev = (xcb_key_release_event_t *)e;

						//printf("ev->detail: %d\n", ev->detail);
						switch (ev->detail) {
							/* ESC */
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
					}
			}
			free (e);
		}
	}

	delete application;
}
