/*
 * Copyright (C) 2004-2010 Geometer Plus <contact@geometerplus.com>
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

#include <stdio.h>
#include <sys/timeb.h>
#include <time.h>

#include <ZLTime.h>

#include "ZLUnixTime.h"

ZLTime ZLUnixTimeManager::currentTime() const {
	struct timeb timeB;
	ftime(&timeB);
	return ZLTime(timeB.time, timeB.millitm);
}

short ZLUnixTimeManager::hoursBySeconds(long seconds) const {
	if(localtime(&seconds)->tm_year < 109)
		return -1;
	else
		return localtime(&seconds)->tm_hour;
}

short ZLUnixTimeManager::minutesBySeconds(long seconds) const {
	if(localtime(&seconds)->tm_year < 109)
		return -1;
	else
		return localtime(&seconds)->tm_min;
}

short ZLUnixTimeManager::yearBySeconds(long seconds) const {
	return localtime(&seconds)->tm_year + 1900;
}

short ZLUnixTimeManager::monthBySeconds(long seconds) const {
	return localtime(&seconds)->tm_mon + 1;
}

short ZLUnixTimeManager::dayOfMonthBySeconds(long seconds) const {
	return localtime(&seconds)->tm_mday;
}
