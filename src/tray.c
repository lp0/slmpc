/*
 * Copyright ©2009  Simon Arlott
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <math.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "config.h"
#include "debug.h"
#include "icon.h"
#include "slmpc.h"
#include "comms.h"
#include "tray.h"

#include "connecting.xbm"
#include "not_connected.xbm"

#include "unknown.xbm"
#include "playing.xbm"
#include "paused.xbm"
#include "stopped.xbm"

int tray_init(struct slmpc_data *data) {
	UINT ret;
	DWORD err;

	odprintf("tray[init]");

	data->status.conn = NOT_CONNECTED;
	data->tray_ok = 0;

	SetLastError(0);
	ret = RegisterWindowMessage(TEXT("TaskbarCreated"));
	err = GetLastError();
	odprintf("RegisterMessageWindow: %u (%ld)", ret, err);
	if (ret == 0) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Unable to register TaskbarCreated message (%ld)", err);
		return 1;
	} else {
		data->taskbarCreated = ret;
		return 0;
	}
}

void tray_reset(HWND hWnd, struct slmpc_data *data) {
	odprintf("tray[reset]");

	/* Try this anyway... */
	tray_remove(hWnd, data);

	/* Assume it has been removed */
	data->tray_ok = 0;

	/* Add it again */
	tray_add(hWnd, data);
	tray_update(hWnd, data);
}

void tray_add(HWND hWnd, struct slmpc_data *data) {
	NOTIFYICONDATA *niData;
	BOOL ret;
	DWORD err;

	odprintf("tray[add]");

	if (!data->tray_ok) {
		niData = &data->niData;
		niData->cbSize = sizeof(NOTIFYICONDATA);
		niData->hWnd = hWnd;
		niData->uID = TRAY_ID;
		niData->uFlags = NIF_MESSAGE|NIF_TIP;
		niData->uCallbackMessage = WM_APP_TRAY;
		niData->hIcon = NULL;
		niData->szTip[0] = 0;
		niData->uVersion = NOTIFYICON_VERSION;

		SetLastError(0);
		ret = Shell_NotifyIcon(NIM_ADD, niData);
		err = GetLastError();
		odprintf("Shell_NotifyIcon[ADD]: %s (%ld)", ret == TRUE ? "TRUE" : "FALSE", err);
		if (ret == TRUE)
			data->tray_ok = 1;

		SetLastError(0);
		ret = Shell_NotifyIcon(NIM_SETVERSION, niData);
		err = GetLastError();
		odprintf("Shell_NotifyIcon[SETVERSION]: %s (%ld)", ret == TRUE ? "TRUE" : "FALSE", err);
		if (ret != TRUE)
			niData->uVersion = 0;
	}
}

void tray_remove(HWND hWnd, struct slmpc_data *data) {
	NOTIFYICONDATA *niData = &data->niData;
	BOOL ret;
	DWORD err;
	(void)hWnd;

	odprintf("tray[remove]");

	if (data->tray_ok) {
		SetLastError(0);
		ret = Shell_NotifyIcon(NIM_DELETE, niData);
		err = GetLastError();
		odprintf("Shell_NotifyIcon[DELETE]: %s (%ld)", ret == TRUE ? "TRUE" : "FALSE", err);
		if (ret == TRUE)
			data->tray_ok = 0;
	}
}

void tray_update(HWND hWnd, struct slmpc_data *data) {
	struct tray_status *status = &data->status;
	NOTIFYICONDATA *niData = &data->niData;
	HICON oldIcon;
	unsigned int fg, bg;
	BOOL ret;
	DWORD err;

	if (!data->tray_ok) {
		tray_add(hWnd, data);

		if (!data->tray_ok)
			return;
	}

	odprintf("tray[update]: conn=%d play=%d msg=\"%s\"", status->conn, status->play, status->msg);

	fg = icon_syscolour(COLOR_BTNTEXT);
	bg = icon_syscolour(COLOR_3DFACE);

	switch (status->conn) {
	case NOT_CONNECTED:
		if (not_connected_width < ICON_WIDTH || not_connected_height < ICON_HEIGHT)
			icon_wipe(bg);

		icon_blit(0, 0, 0, fg, bg, 0, 0, not_connected_width, not_connected_height, not_connected_bits);

		if (status->msg[0] != 0)
			ret = snprintf(niData->szTip, sizeof(niData->szTip), "Not Connected: %s", status->msg);
		else
			ret = snprintf(niData->szTip, sizeof(niData->szTip), "Not Connected");
		if (ret < 0)
			niData->szTip[0] = 0;
		break;

	case CONNECTING:
		if (connecting_width < ICON_WIDTH || connecting_height < ICON_HEIGHT)
			icon_wipe(bg);

		icon_blit(0, 0, 0, fg, bg, 0, 0, connecting_width, connecting_height, connecting_bits);

		if (status->msg[0] != 0)
			ret = snprintf(niData->szTip, sizeof(niData->szTip), "Connecting to %s", status->msg);
		else
			ret = snprintf(niData->szTip, sizeof(niData->szTip), "Connecting");
		if (ret < 0)
			niData->szTip[0] = 0;
		break;

	case CONNECTED:
		switch (status->play) {
		case MPD_UNKNOWN:
			if (unknown_width < ICON_WIDTH || unknown_height < ICON_HEIGHT)
				icon_wipe(bg);

			icon_blit(0, 0, 0, fg, bg, 0, 0, unknown_width, unknown_height, unknown_bits);

			if (status->msg[0] != 0)
				ret = snprintf(niData->szTip, sizeof(niData->szTip), "Connected to %s", status->msg);
			else
				ret = snprintf(niData->szTip, sizeof(niData->szTip), "Connected");
			if (ret < 0)
				niData->szTip[0] = 0;
			break;

		case MPD_PLAYING:
			if (playing_width < ICON_WIDTH || playing_height < ICON_HEIGHT)
				icon_wipe(bg);

			icon_blit(0, 0, 0, fg, bg, 0, 0, playing_width, playing_height, playing_bits);

			if (status->msg[0] != 0)
				ret = snprintf(niData->szTip, sizeof(niData->szTip), "Playing: %s", status->msg);
			else
				ret = snprintf(niData->szTip, sizeof(niData->szTip), "Playing");
			if (ret < 0)
				niData->szTip[0] = 0;
			break;

		case MPD_PAUSED:
			if (paused_width < ICON_WIDTH || paused_height < ICON_HEIGHT)
				icon_wipe(bg);

			icon_blit(0, 0, 0, fg, bg, 0, 0, paused_width, paused_height, paused_bits);

			if (status->msg[0] != 0)
				ret = snprintf(niData->szTip, sizeof(niData->szTip), "Paused: %s", status->msg);
			else
				ret = snprintf(niData->szTip, sizeof(niData->szTip), "Paused");
			if (ret < 0)
				niData->szTip[0] = 0;
			break;

		case MPD_STOPPED:
			if (stopped_width < ICON_WIDTH || stopped_height < ICON_HEIGHT)
				icon_wipe(bg);

			icon_blit(0, 0, 0, fg, bg, 0, 0, stopped_width, stopped_height, stopped_bits);

			if (status->msg[0] != 0)
				ret = snprintf(niData->szTip, sizeof(niData->szTip), "Stopped %s", status->msg);
			else
				ret = snprintf(niData->szTip, sizeof(niData->szTip), "Stopped");
			if (ret < 0)
				niData->szTip[0] = 0;
			break;
		}
		break;

	default:
		return;
	}

	oldIcon = niData->hIcon;

	niData->uFlags &= ~NIF_ICON;
	niData->hIcon = icon_create();
	if (niData->hIcon != NULL)
		niData->uFlags |= NIF_ICON;

	SetLastError(0);
	ret = Shell_NotifyIcon(NIM_MODIFY, niData);
	err = GetLastError();
	odprintf("Shell_NotifyIcon[MODIFY]: %s (%ld)", ret == TRUE ? "TRUE" : "FALSE", err);
	if (ret != TRUE)
		tray_remove(hWnd, data);

	if (oldIcon != NULL)
		icon_destroy(niData->hIcon);
}

BOOL tray_activity(HWND hWnd, struct slmpc_data *data, WPARAM wParam, LPARAM lParam) {
	(void)hWnd;

	odprintf("tray[activity]: wParam=%ld lParam=%ld", wParam, lParam);

	if (wParam != TRAY_ID)
		return FALSE;

	switch (data->niData.uVersion) {
		case NOTIFYICON_VERSION:
			switch (lParam) {
			case WM_CONTEXTMENU:
				slmpc_shutdown(hWnd, data, EXIT_SUCCESS);
				return TRUE;

			default:
				return FALSE;
			}

		case 0:
			switch (lParam) {
			case WM_RBUTTONUP:
				slmpc_shutdown(hWnd, data, EXIT_SUCCESS);
				return TRUE;

			default:
				return FALSE;
			}

	default:
		return FALSE;
	}
}
