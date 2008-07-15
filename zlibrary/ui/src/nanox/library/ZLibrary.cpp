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


bool idle;
void suspendTimer(int signo, siginfo_t* evp, void* ucontext)
{
	if(idle)
		system("echo mem > /sys/power/state");
}

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

	
	struct sigaction sigv;
	struct sigevent sigx;
	struct itimerspec val;
	timer_t t_id;

	sigemptyset(&sigv.sa_mask);
	sigv.sa_flags = SA_SIGINFO;
	sigv.sa_sigaction = suspendTimer;

	if(sigaction(SIGUSR1, &sigv, 0) == -1) {
		perror("sigaction");
		return;
	}

	sigx.sigev_notify = SIGEV_SIGNAL;
	sigx.sigev_signo = SIGUSR1;
	sigx.sigev_value.sival_ptr = (void *)NULL;

	if(timer_create(CLOCK_REALTIME, &sigx, &t_id) == -1) {
		perror("timer_create");
		return;
	}

	val.it_value.tv_sec = 60;
	val.it_value.tv_nsec = 0;
	val.it_interval.tv_sec = 0;
	val.it_interval.tv_nsec = 0;

//key press events
	bool end = false;
	xcb_generic_event_t  *e;
	while (!end) {
		idle = true;
		timer_settime(t_id, 0, &val, 0);
		e = xcb_wait_for_event(connection);
		idle = false;
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

	timer_delete(t_id);
	delete application;
}
