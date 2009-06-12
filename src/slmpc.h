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

#define TITLE "slmpc\0"
#define DEFAULT_SERVICE "6600"

#define WM_APP_NET  (WM_APP+0)
#define WM_APP_TRAY (WM_APP+1)
#define WM_APP_SOCK (WM_APP+2)
#define WM_APP_KBD  (WM_APP+3)

#define NET_MSG_CONNECT 0
#define KBD_MSG_CHECK 1

#define RETRY_TIMER_ID 1

enum conn_status {
	NOT_CONNECTED,
	CONNECTING,
	CONNECTED
};

enum play_status {
	MPD_UNKNOWN,
	MPD_PLAYING,
	MPD_PAUSED,
	MPD_STOPPED
};

enum cmd_status {
	MPC_NONE,
	MPC_CONNECT,
	MPC_PASSWORD,
	MPC_STATUS,
	MPC_IDLE,
	MPC_NOIDLE,
	MPC_PLAY,
	MPC_PAUSE
};

enum sl_status {
	SL_UNKNOWN,
	SL_OFF,
	SL_ON
};

struct tray_status {
	enum conn_status conn;
	enum play_status play;
	char msg[512];
};

struct slmpc_data {
	HWND hWnd;
	HINSTANCE hInstance;
	int running;

	char *node;
	char *service;
	char *password;

#if HAVE_GETADDRINFO
	char hbuf[NI_MAXHOST];
	char sbuf[NI_MAXSERV];
	struct addrinfo hints;
	struct addrinfo *addrs_res;
	struct addrinfo *addrs_cur;
#else
	struct sockaddr_in sa4;
	struct sockaddr_in6 sa6;
	int family;
	struct sockaddr *sa;
	int sa_len;
#endif
	SOCKET s;

	HHOOK kbd_hook;

	char parse_buf[512];
	unsigned int parse_pos;

	HBITMAP hbmMask;

	UINT taskbarCreated;
	NOTIFYICONDATA niData;
	int tray_ok;
	struct tray_status status;
	enum cmd_status cmd;
	enum cmd_status pending_cmd;
	enum sl_status sl_status;
};

void slmpc_shutdown(struct slmpc_data *data, int status);
