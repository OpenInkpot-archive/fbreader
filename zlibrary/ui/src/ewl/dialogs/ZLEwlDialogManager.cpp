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

#include "ZLEwlDialogManager.h"

shared_ptr<ZLDialog> ZLEwlDialogManager::createDialog(const ZLResourceKey &key __attribute__ ((__unused__))) const {
	return NULL;
}

shared_ptr<ZLOptionsDialog> ZLEwlDialogManager::createOptionsDialog(const ZLResourceKey &id __attribute__ ((__unused__)), shared_ptr<ZLRunnable> applyAction __attribute__ ((__unused__)), bool showApplyButton __attribute__ ((__unused__))) const {
	return NULL;
}

void ZLEwlDialogManager::informationBox(const ZLResourceKey &key __attribute__ ((__unused__)), const std::string &message __attribute__ ((__unused__))) const {
}

void ZLEwlDialogManager::errorBox(const ZLResourceKey &key __attribute__ ((__unused__)), const std::string &message __attribute__ ((__unused__))) const {
}

int ZLEwlDialogManager::questionBox(const ZLResourceKey &key __attribute__ ((__unused__)), const std::string &message __attribute__ ((__unused__)), const ZLResourceKey &button0 __attribute__ ((__unused__)), const ZLResourceKey &button1 __attribute__ ((__unused__)), const ZLResourceKey &button2 __attribute__ ((__unused__))) const {
	return 0;
}

int ZLEwlDialogManager::internalBox(const char *icon __attribute__ ((__unused__)), const ZLResourceKey &key __attribute__ ((__unused__)), const std::string &message __attribute__ ((__unused__)), const ZLResourceKey &button0 __attribute__ ((__unused__)), const ZLResourceKey &button1 __attribute__ ((__unused__)), const ZLResourceKey &button2 __attribute__ ((__unused__))) const {
	return 0;
}

bool ZLEwlDialogManager::selectionDialog(const ZLResourceKey &key __attribute__ ((__unused__)), ZLTreeHandler &handler __attribute__ ((__unused__))) const {
	return false;
}

shared_ptr<ZLProgressDialog> ZLEwlDialogManager::createProgressDialog(const ZLResourceKey &key __attribute__ ((__unused__))) const {
	return NULL;
}

void ZLEwlDialogManager::wait(const ZLResourceKey &key __attribute__ ((__unused__)), ZLRunnable &runnable __attribute__ ((__unused__))) const {
	runnable.run();
}

bool ZLEwlDialogManager::isClipboardSupported(ClipboardType __attribute__ ((__unused__))) const {
	return false;
}

void ZLEwlDialogManager::setClipboardText(const std::string &text __attribute__ ((__unused__)), ClipboardType type __attribute__ ((__unused__))) const {
}
