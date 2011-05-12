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

#define TRAY_ID 1

#define COLOUR_WHITE 0xffffffff
#define COLOUR_BLACK 0x00000000

int tray_init(struct slmpc_data *data);
void tray_reset(HWND hWnd, struct slmpc_data *data);
void tray_add(HWND hWnd, struct slmpc_data *data);
void tray_update(HWND hWnd, struct slmpc_data *data);
BOOL tray_activity(HWND hWnd, struct slmpc_data *data, WPARAM wParam, LPARAM lParam);
void tray_remove(HWND hWnd, struct slmpc_data *data);
