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
#include "debug.h"
#include "slmpc.h"
#include "keyboard.h"

struct slmpc_data *_data = NULL;

int kbd_init(struct slmpc_data *data) {
	HHOOK ret;
	DWORD err;

	odprintf("kbd[init]");

	_data = data;

	SetLastError(0);
	ret = SetWindowsHookEx(WH_KEYBOARD_LL, kbd_hook, data->hInstance, 0);
	err = GetLastError();
	odprintf("SetWindowsHookEx: %p (%d)", ret, err);
	if (ret == NULL)
		return 1;

	data->kbd_hook = ret;

	return 0;
}

LRESULT CALLBACK kbd_hook(int code, WPARAM wParam, LPARAM lParam) {
	struct slmpc_data *data = _data;
	KBDLLHOOKSTRUCT *event;

	event = (PKBDLLHOOKSTRUCT)lParam;

	if (_data == NULL || event == NULL)
		return CallNextHookEx(NULL, code, wParam, lParam);
	
	if (event->vkCode == VK_SCROLL) {
		BOOL ret;
		DWORD err;

		SetLastError(0);
		ret = PostMessage(data->hWnd, WM_APP_KBD, 0, KBD_MSG_CHECK);
		err = GetLastError();
		odprintf("PostMessage: %s (%d)", ret == TRUE ? "TRUE" : "FALSE", err);
	}

	return CallNextHookEx(_data->kbd_hook, code, wParam, lParam);
}

void kbd_destroy(struct slmpc_data *data) {
	BOOL ret;
	DWORD err;

	odprintf("kbd[destroy]");

	SetLastError(0);
	ret = UnhookWindowsHookEx(data->kbd_hook);
	err = GetLastError();
	odprintf("UnhookWindowsHookEx: %s (%d)", ret == TRUE ? "TRUE" : "FALSE", err);

	_data = NULL;
}

enum sl_status kbd_get(void) {
	SHORT ret;
	DWORD err;

	odprintf("kbd[get]");

	SetLastError(0);
	ret = GetKeyState(VK_SCROLL);
	err = GetLastError();
	odprintf("GetKeyState: %d (%d)", ret, err);
	if (err != 0)
		return SL_UNKNOWN;

	return (ret & 1) == 0 ? SL_OFF : SL_ON;
}

enum sl_status kbd_set(enum sl_status status) {
	UINT ret;
	DWORD err;
	INPUT keys[2];
	enum sl_status current;

	odprintf("kbd[set]: status=%d", status);

	keys[0].type = INPUT_KEYBOARD;
	keys[0].ki.wVk = VK_SCROLL;
	keys[0].ki.wScan = 0;
	keys[0].ki.dwFlags = 0;

	keys[1].type = INPUT_KEYBOARD;
	keys[1].ki.wVk = VK_SCROLL;
	keys[1].ki.wScan = 0;
	keys[1].ki.dwFlags = KEYEVENTF_KEYUP;

	current = kbd_get();
	switch (current) {
	case SL_ON:
	case SL_OFF:
		if (current == status)
			return status;

		SetLastError(0);
		ret = SendInput(2, (LPINPUT)&keys, sizeof(INPUT));
		err = GetLastError();
		odprintf("SendInput: %d (%d)", ret, err);
		if (ret == 0 || err == 0)
			current = status;
		break;

	default:
		break;
	}

	return current;
}
