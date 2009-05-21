/*
 * Copyright ©2009  Simon Arlott
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#include "debug.h"

#if DEBUG > 0
void odprintf(const char *fmt, ...) {
		char buf[4096] = {};
		int ret;
		va_list args;

		va_start(args, fmt);
		ret = vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		if (ret < 0)
			OutputDebugString("Error in odprintf()");
		else
			OutputDebugString(buf);
}

void mbprintf(const char *title, int flags, const char *fmt, ...) {
		char buf[4096] = {};
		int ret;
		va_list args;

		va_start(args, fmt);
		ret = vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);

		if (ret < 0)
			MessageBox(NULL, "Error in mbprintf()", title, flags);
		else
			MessageBox(NULL, buf, title, flags);
}
#endif
