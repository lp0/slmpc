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

#ifndef SIO_KEEPALIVE_VALS
# define SIO_KEEPALIVE_VALS _WSAIOW(IOC_VENDOR,4)
struct tcp_keepalive {
	u_long onoff;
	u_long keepalivetime;
	u_long keepaliveinterval;
}; 
#endif

int comms_init(struct slmpc_data *data);
void comms_destroy(struct slmpc_data *data);
void comms_disconnect(struct slmpc_data *data);
int comms_connect(HWND hWnd, struct slmpc_data *data);
int comms_activity(HWND hWnd, struct slmpc_data *data, SOCKET s, WORD sEvent, WORD sError);
int comms_parse(HWND hWnd, struct slmpc_data *data);
int comms_kbd(HWND hWnd, struct slmpc_data *data);
