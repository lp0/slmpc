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

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "config.h"

int kbd_init(HINSTANCE hInstance);
LRESULT CALLBACK kbd_hook(int code, WPARAM wParam, LPARAM lParam);
void kbd_destroy(void);
enum sl_status kbd_get(void);
enum sl_status kbd_set(enum sl_status status);
