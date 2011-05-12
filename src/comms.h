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

#ifndef SIO_KEEPALIVE_VALS
# define SIO_KEEPALIVE_VALS _WSAIOW(IOC_VENDOR,4)
struct tcp_keepalive {
	u_long onoff;
	u_long keepalivetime;
	u_long keepaliveinterval;
}; 
#endif

#define CMD_TIMEOUT 30000 /* 30 seconds */

int comms_init(struct slmpc_data *data);
void comms_destroy(HWND hWnd, struct slmpc_data *data);
void comms_disconnect(HWND hWnd, struct slmpc_data *data);
int comms_connect(HWND hWnd, struct slmpc_data *data);
int comms_activity(HWND hWnd, struct slmpc_data *data, SOCKET s, WORD sEvent, WORD sError);
int comms_parse(HWND hWnd, struct slmpc_data *data);
int comms_kbd(HWND hWnd, struct slmpc_data *data);
void comms_timeout(HWND hWnd, struct slmpc_data *data);
