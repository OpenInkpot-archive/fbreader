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

#include "../../../../core/src/unix/message/ZLUnixMessage.h"
#include "../../../../core/src/unix/xmlconfig/XMLConfig.h"
#include "../../../../core/src/unix/iconv/IConvEncodingConverter.h"

#include "../filesystem/ZLNXFSManager.h"
#include "../time/ZLNXTime.h"
#include "../view/ZLNXPaintContext.h"
#include "../dialogs/ZLNXDialogManager.h"


class ZLNXLibraryImplementation : public ZLibraryImplementation {
	private:
		void init(int &argc, char **&argv);
		ZLPaintContext *createContext();
		void run(ZLApplication *application);
};

void initLibrary() {
	printf("initLibrary\n");
	new ZLNXLibraryImplementation();
}

void ZLNXLibraryImplementation::init(int &argc, char **&argv) {
	printf("init\n");
	ZLibrary::parseArguments(argc, argv);

	XMLConfigManager::createInstance();
	ZLNXFSManager::createInstance();
	ZLNXTimeManager::createInstance();
	ZLNXDialogManager::createInstance();
	ZLUnixCommunicationManager::createInstance();
	ZLEncodingCollection::instance().registerProvider(new IConvEncodingConverterProvider());
}

ZLPaintContext *ZLNXLibraryImplementation::createContext() {
	return new ZLNXPaintContext();
}

void ZLNXLibraryImplementation::run(ZLApplication *application) {
	ZLDialogManager::instance().createApplicationWindow(application);

	printf("run\n");
	application->initWindow();

	sleep(3);
	std::string x("largeScrollForward");
	application->doActionByKey(x);

	sleep(10);

	delete application;
}