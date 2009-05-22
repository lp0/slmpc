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

#include <math.h>
#include <stdio.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "config.h"
#include "debug.h"
#include "slmpc.h"
#include "comms.h"
#include "tray.h"

int comms_init(struct slmpc_data *data) {
	INT ret;
	DWORD err;
#if HAVE_GETADDRINFO

	odprintf("comms[init]: node=%s service=%s", data->node, data->service);

	data->hbuf[0] = 0;
	data->sbuf[0] = 0;
	data->addrs_res = NULL;
	data->addrs_cur = NULL;

	data->hints.ai_flags = 0;
	data->hints.ai_family = AF_UNSPEC;
	data->hints.ai_socktype = SOCK_STREAM;
	data->hints.ai_protocol = IPPROTO_TCP;
	data->hints.ai_addrlen = 0;
	data->hints.ai_addr = NULL;
	data->hints.ai_canonname = NULL;
	data->hints.ai_next = NULL;

	SetLastError(0);
	ret = getaddrinfo(data->node, data->service, &data->hints, &data->addrs_res);
	err = GetLastError();
	odprintf("getaddrinfo: %d (%d)", ret, err);
	if (ret != 0) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Unable to resolve node \"%s\" service \"%s\" (%d)", data->node, data->service, ret);
		return 1;
	}

	if (data->addrs_res == NULL) {
		odprintf("no results");
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "No results resolving node \"%s\" service \"%s\"", data->node, data->service);
		return 1;
	}

	data->addrs_cur = data->addrs_res;
#else
	int sa4_len = sizeof(data->sa4);
	int sa6_len = sizeof(data->sa6);

	odprintf("comms[init]: node=%s service=%s", data->node, data->service);

	data->family = AF_UNSPEC;
	data->sa = NULL;
	data->sa_len = 0;

	SetLastError(0);
	ret = WSAStringToAddress(data->node, AF_INET, NULL, (LPSOCKADDR)&data->sa4, &sa4_len);
	err = GetLastError();
	odprintf("WSAStringToAddress[IPv4]: %d (%ld)", ret, err);
	if (ret == 0) {
		data->family = AF_INET;
		data->sa4.sin_family = AF_INET;
		data->sa4.sin_port = htons(strtoul(data->service, NULL, 10));

		data->sa = (struct sockaddr*)&data->sa4;
		data->sa_len = sa4_len;
	}

	SetLastError(0);
	ret = WSAStringToAddress(data->node, AF_INET6, NULL, (LPSOCKADDR)&data->sa6, &sa6_len);
	err = GetLastError();
	odprintf("WSAStringToAddress[IPv6]: %d (%ld)", ret, err);
	if (ret == 0) {
		data->family = AF_INET6;
		data->sa6.sin6_family = AF_INET6;
		data->sa6.sin6_port = htons(strtoul(data->service, NULL, 10));

		data->sa = (struct sockaddr*)&data->sa6;
		data->sa_len = sa6_len;
	}

	odprintf("family=%d", data->family);
	if (data->family == AF_UNSPEC) {
		mbprintf(TITLE, MB_OK|MB_ICONERROR, "Unable to connect: Invalid IP \"%s\"", data->node);
		return 1;
	}
#endif

	data->s = INVALID_SOCKET;
	return 0;
}

void comms_destroy(struct slmpc_data *data) {
	odprintf("comms[destroy]");

#if HAVE_GETADDRINFO
	if (data->addrs_res != NULL) {
		freeaddrinfo(data->addrs_res);
		data->addrs_res = NULL;
	}
#endif

	comms_disconnect(data);
}

void comms_disconnect(struct slmpc_data *data) {
	INT ret;
	DWORD err;

	odprintf("comms[disconnect]");

	data->status.conn = NOT_CONNECTED;

	if (data->s != INVALID_SOCKET) {
		SetLastError(0);
		ret = closesocket(data->s);
		err = GetLastError();
		odprintf("closesocket: %d (%ld)", ret, err);

		data->s = INVALID_SOCKET;
	}
}

int comms_connect(HWND hWnd, struct slmpc_data *data) {
	struct tray_status *status = &data->status;
	struct tcp_keepalive ka_get;
	struct tcp_keepalive ka_set = {
		.onoff = 1,
		.keepalivetime = 5000, /* 5 seconds */
		.keepaliveinterval = 5000 /* 5 seconds */
	};
	int timeout = 5000; /* 5 seconds */
	INT ret;
	DWORD retd;
	DWORD err;

	odprintf("comms[connect]");

	if (!data->running)
		return 0;

	if (data->s != INVALID_SOCKET)
		return 0;

	status->conn = NOT_CONNECTED;
	tray_update(hWnd, data);

#if HAVE_GETADDRINFO
	if (data->addrs_cur == NULL && data->addrs_res != NULL) {
		freeaddrinfo(data->addrs_res);
		data->addrs_res = NULL;
	}

	if (data->addrs_res == NULL) {
		SetLastError(0);
		ret = getaddrinfo(data->node, data->service, &data->hints, &data->addrs_res);
		err = GetLastError();
		odprintf("getaddrinfo: %d (%ld)", ret, err);
		if (ret != 0) {
			ret = snprintf(status->msg, sizeof(status->msg), "Unable to resolve node \"%s\" service \"%s\" (%d)", data->node, data->service, ret);
			if (ret < 0)
				status->msg[0] = 0;
			tray_update(hWnd, data);

			return 1;
		}

		if (data->addrs_res == NULL) {
			odprintf("no results");
			ret = snprintf(status->msg, sizeof(status->msg), "No results resolving node \"%s\" service \"%s\"", data->node, data->service);
			if (ret < 0)
				status->msg[0] = 0;
			tray_update(hWnd, data);

			return 1;
		}

		data->addrs_cur = data->addrs_res;
	}

	SetLastError(0);
	ret = getnameinfo(data->addrs_cur->ai_addr, data->addrs_cur->ai_addrlen, data->hbuf, sizeof(data->hbuf), data->sbuf, sizeof(data->sbuf), NI_NUMERICHOST|NI_NUMERICSERV);
	err = GetLastError();
	odprintf("getnameinfo: %d (%ld)", ret, err);
	if (ret == 0) {
		odprintf("trying to connect to node \"%s\" service \"%s\"", data->hbuf, data->sbuf);
	} else {
		data->hbuf[0] = 0;
		data->sbuf[0] = 0;
	}
#else
	odprintf("trying to connect to node \"%s\" service \"%s\"", data->node, data->service);
#endif

	SetLastError(0);
#if HAVE_GETADDRINFO
	data->s = socket(data->addrs_cur->ai_family, SOCK_STREAM, IPPROTO_TCP);
#else
	data->s = socket(data->family, SOCK_STREAM, IPPROTO_TCP);
#endif
	err = GetLastError();
	odprintf("socket: %d (%ld)", data->s, err);

	if (data->s == INVALID_SOCKET) {
		ret = snprintf(status->msg, sizeof(status->msg), "Unable to create socket (%ld)", err);
		if (ret < 0)
			status->msg[0] = 0;
		tray_update(hWnd, data);

#if HAVE_GETADDRINFO
		data->addrs_cur = data->addrs_cur->ai_next;
#endif
		return 1;
	}

	SetLastError(0);
	ret = setsockopt(data->s, SOL_SOCKET, SO_RCVTIMEO, (void*)&timeout, sizeof(timeout));
	err = GetLastError();
	odprintf("setsockopt: %d (%ld)", ret, err);
	if (ret != 0) {
		ret = snprintf(status->msg, sizeof(status->msg), "Unable to set socket timeout (%ld)", err);
		if (ret < 0)
			status->msg[0] = 0;
		tray_update(hWnd, data);

		SetLastError(0);
		ret = closesocket(data->s);
		err = GetLastError();
		odprintf("closesocket: %d (%ld)", ret, err);
		data->s = INVALID_SOCKET;

#if HAVE_GETADDRINFO
		data->addrs_cur = data->addrs_cur->ai_next;
#endif
		return 1;
	}

	SetLastError(0);
	ret = WSAIoctl(data->s, SIO_KEEPALIVE_VALS, (void*)&ka_set, sizeof(ka_set), (void*)&ka_get, sizeof(ka_get), &retd, NULL, NULL);
	err = GetLastError();
	odprintf("WSAIoctl: %d, %d (%ld)", ret, retd, err);
	if (ret != 0) {
		ret = snprintf(status->msg, sizeof(status->msg), "Unable to set socket keepalive options (%ld)", err);
		if (ret < 0)
			status->msg[0] = 0;
		tray_update(hWnd, data);

		SetLastError(0);
		ret = closesocket(data->s);
		err = GetLastError();
		odprintf("closesocket: %d (%ld)", ret, err);
		data->s = INVALID_SOCKET;

#if HAVE_GETADDRINFO
		data->addrs_cur = data->addrs_cur->ai_next;
#endif
		return 1;
	}

	SetLastError(0);
	ret = WSAAsyncSelect(data->s, hWnd, WM_APP_SOCK, FD_CONNECT|FD_READ|FD_CLOSE);
	err = GetLastError();
	odprintf("WSAAsyncSelect: %d (%ld)", ret, err);
	if (ret != 0) {
		ret = snprintf(status->msg, sizeof(status->msg), "Unable to async select on socket (%ld)", err);
		if (ret < 0)
			status->msg[0] = 0;
		tray_update(hWnd, data);

		SetLastError(0);
		ret = closesocket(data->s);
		err = GetLastError();
		odprintf("closesocket: %d (%ld)", ret, err);
		data->s = INVALID_SOCKET;

#if HAVE_GETADDRINFO
		data->addrs_cur = data->addrs_cur->ai_next;
#endif
		return 1;
	}

	status->conn = CONNECTING;
#if HAVE_GETADDRINFO
	if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
		ret = snprintf(status->msg, sizeof(status->msg), "node \"%s\" service \"%s\" (%ld)", data->hbuf, data->sbuf, err);
	} else {
#endif
		ret = snprintf(status->msg, sizeof(status->msg), "node \"%s\" service \"%s\" (%ld)", data->node, data->service, err);
#if HAVE_GETADDRINFO
	}
#endif
	if (ret < 0)
		status->msg[0] = 0;
	tray_update(hWnd, data);

	SetLastError(0);
#if HAVE_GETADDRINFO
	ret = connect(data->s, data->addrs_cur->ai_addr, data->addrs_cur->ai_addrlen);
#else
	ret = connect(data->s, data->sa, data->sa_len);
#endif
	err = GetLastError();
	odprintf("connect: %d (%ld)", ret, err);
	if (ret == 0 || err == WSAEWOULDBLOCK) {
		return 0;
	} else {
		status->conn = NOT_CONNECTED;
#if HAVE_GETADDRINFO
		if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
			ret = snprintf(status->msg, sizeof(status->msg), "Error connecting to node \"%s\" service \"%s\"", data->hbuf, data->sbuf);
		} else {
#endif
			ret = snprintf(status->msg, sizeof(status->msg), "Error connecting to node \"%s\" service \"%s\"", data->node, data->service);
#if HAVE_GETADDRINFO
		}
#endif
		if (ret < 0)
			status->msg[0] = 0;
		tray_update(hWnd, data);

		SetLastError(0);
		ret = closesocket(data->s);
		err = GetLastError();
		odprintf("closesocket: %d (%ld)", ret, err);
		data->s = INVALID_SOCKET;

#if HAVE_GETADDRINFO
		data->addrs_cur = data->addrs_cur->ai_next;
#endif
		return 1;
	}
}

int comms_activity(HWND hWnd, struct slmpc_data *data, SOCKET s, WORD sEvent, WORD sError) {
	struct tray_status *status = &data->status;
	INT ret;
	DWORD err;

	odprintf("comms[activity]: s=%p sEvent=%d sError=%d", s, sEvent, sError);

	if (!data->running)
		return 0;

	if (data->s != s)
		return 0;

	switch (sEvent) {
	case FD_CONNECT:
		odprintf("FD_CONNECT %s", status->conn == CONNECTING ? "OK" : "?");
		if (status->conn != CONNECTING)
			return 0;

		if (sError == 0) {
			status->conn = CONNECTED;
			status->play = MPD_UNKNOWN;
			data->cmd = MPC_CONNECT;
			data->pending_cmd = MPC_NONE;
			status->msg[0] = 0;
			tray_update(hWnd, data);

#if HAVE_GETADDRINFO
			freeaddrinfo(data->addrs_res);
			data->addrs_res = NULL;
#endif
			data->parse_pos = 0;
			return 0;
		} else {
			status->conn = NOT_CONNECTED;
#if HAVE_GETADDRINFO
			if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
				ret = snprintf(status->msg, sizeof(status->msg), "Error connecting to node \"%s\" service \"%s\" (%d)", data->hbuf, data->sbuf, sError);
			} else {
#endif
				ret = snprintf(status->msg, sizeof(status->msg), "Error connecting to node \"%s\" service \"%s\" (%d)", data->node, data->service, sError);
#if HAVE_GETADDRINFO
			}
#endif
			if (ret < 0)
				status->msg[0] = 0;
			tray_update(hWnd, data);

			SetLastError(0);
			ret = closesocket(data->s);
			err = GetLastError();
			odprintf("closesocket: %d (%ld)", ret, err);

			data->s = INVALID_SOCKET;
#if HAVE_GETADDRINFO
			data->addrs_cur = data->addrs_cur->ai_next;
#endif
			return 1;
		}

	case FD_READ:
		odprintf("FD_READ %s", status->conn == CONNECTED ? "OK" : "?");
		if (status->conn != CONNECTED)
			return 0;

		if (sError == 0) {
			char recv_buf[128];

			SetLastError(0);
			ret = recv(data->s, recv_buf, sizeof(recv_buf), 0);
			err = GetLastError();
			odprintf("recv: %d (%ld)", ret, err);
			if (ret <= 0) {
				status->conn = NOT_CONNECTED;
#if HAVE_GETADDRINFO
				if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
					ret = snprintf(status->msg, sizeof(status->msg), "Error reading from node \"%s\" service \"%s\" (%ld)", data->hbuf, data->sbuf, err);
				} else {
#endif
					ret = snprintf(status->msg, sizeof(status->msg), "Error reading from node \"%s\" service \"%s\" (%ld)", data->node, data->service, err);
#if HAVE_GETADDRINFO
				}
#endif
				if (ret < 0)
					status->msg[0] = 0;
				tray_update(hWnd, data);

				SetLastError(0);
				ret = closesocket(data->s);
				err = GetLastError();
				odprintf("closesocket: %d (%ld)", ret, err);

				data->s = INVALID_SOCKET;
				return 1;
			} else if (err == WSAEWOULDBLOCK) {
				return 0;
			} else {
				int size, i;

				size = ret;
				for (i = 0; i < size; i++) {
					/* find a newline and parse the buffer */
					if (recv_buf[i] == '\n') {
						comms_parse(hWnd, data);

						/* clear buffer */
						data->parse_pos = 0;

					/* buffer overflow */
					} else if (data->parse_pos == sizeof(data->parse_buf)/sizeof(char) - 1) {
						odprintf("parse: sender overflowed buffer waiting for '\\n'");
						data->parse_buf[0] = 0;
						data->parse_pos++;

					/* ignore */
					} else if (data->parse_pos > sizeof(data->parse_buf)/sizeof(char) - 1) {

					/* append to buffer */
					} else {
						data->parse_buf[data->parse_pos++] = recv_buf[i];
						data->parse_buf[data->parse_pos] = 0;
					}
				}
				return 0;
			}
		} else {
			status->conn = NOT_CONNECTED;
#if HAVE_GETADDRINFO
			if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
				ret = snprintf(status->msg, sizeof(status->msg), "Error reading from node \"%s\" service \"%s\" (%d)", data->hbuf, data->sbuf, sError);
			} else {
#endif
				ret = snprintf(status->msg, sizeof(status->msg), "Error reading from node \"%s\" service \"%s\" (%d)", data->node, data->service, sError);
#if HAVE_GETADDRINFO
			}
#endif
			if (ret < 0)
				status->msg[0] = 0;
			tray_update(hWnd, data);

			SetLastError(0);
			ret = closesocket(data->s);
			err = GetLastError();
			odprintf("closesocket: %d (%ld)", ret, err);

			data->s = INVALID_SOCKET;
			return 1;
		}

	case FD_CLOSE:
		odprintf("FD_CLOSE %s", status->conn == CONNECTED ? "OK" : "?");
		if (status->conn != CONNECTED)
			return 0;

		status->conn = NOT_CONNECTED;
#if HAVE_GETADDRINFO
		if (data->hbuf[0] != 0 && data->sbuf[0] != 0) {
			ret = snprintf(status->msg, sizeof(status->msg), "Lost connection to node \"%s\" service \"%s\" (%d)", data->hbuf, data->sbuf, sError);
		} else {
#endif
			ret = snprintf(status->msg, sizeof(status->msg), "Lost connection to node \"%s\" service \"%s\" (%d)", data->node, data->service, sError);
#if HAVE_GETADDRINFO
		}
#endif
		if (ret < 0)
			status->msg[0] = 0;
		tray_update(hWnd, data);

		data->s = INVALID_SOCKET;
		return 1;

	default:
		return 0;
	}
}

int comms_send(SOCKET s, const char *data) {
	INT ret;
	DWORD err;
	int len = strlen(data);

	SetLastError(0);
	ret = send(s, data, len, 0);
	err = GetLastError();
	odprintf("send: %d (%ld)", ret, err);

	if (err != 0)
		return err;
	if (ret != len)
		return -1;
	return 0;
}

int comms_parse(HWND hWnd, struct slmpc_data *data) {
	struct tray_status *status = &data->status;
	char msg_type[64];
	int ret;
	(void)hWnd;
	(void)status;

	odprintf("comms[parse]: \"%s\"", data->parse_buf);

	if (sscanf(data->parse_buf, "%64s", msg_type) == 1) {
		if (!strcmp(msg_type, "OK")) {
			switch (data->cmd) {	
			case MPC_CONNECT:
				if (data->password[0] != 0) {
					odprintf("comms[parse]: connected, sending password");

					ret = comms_send(data->s, "password ");
					ret |= comms_send(data->s, data->password);
					ret |= comms_send(data->s, "\n");
					if (ret) {
						ret = snprintf(status->msg, sizeof(status->msg), "Error sending password (%d)", ret);
						if (ret < 0)
							status->msg[0] = 0;
						return -1;
					}

					data->cmd = MPC_PASSWORD;
					break;
				}
				odprintf("comms[parse]: connected, requesting status");

			case MPC_PASSWORD:
				if (data->cmd == MPC_PASSWORD)
					odprintf("comms[parse]: authenticated, requesting status");

				ret = comms_send(data->s, "status\n");
				if (ret) {
					ret = snprintf(status->msg, sizeof(status->msg), "Error requesting status (%d)", ret);
					if (ret < 0)
						status->msg[0] = 0;
					return -1;
				}

				data->cmd = MPC_STATUS;
				break;

			case MPC_STATUS:
				odprintf("comms[parse]: status received, going idle");

				ret = comms_send(data->s, "idle player\n");
				if (ret) {
					ret = snprintf(status->msg, sizeof(status->msg), "Error requesting idle mode (%d)", ret);
					if (ret < 0)
						status->msg[0] = 0;
					return -1;
				}

				data->cmd = MPC_IDLE;
				break;

			case MPC_IDLE:
				odprintf("comms[parse]: resume from idle");

				switch (data->pending_cmd) {
				case MPC_NONE:
					odprintf("comms[parse]: no command pending, going idle");

					ret = comms_send(data->s, "idle player\n");
					if (ret) {
						ret = snprintf(status->msg, sizeof(status->msg), "Error requesting idle mode (%d)", ret);
						if (ret < 0)
							status->msg[0] = 0;
						return -1;
					}

					data->cmd = MPC_IDLE;
					break;

				case MPC_STATUS:
					odprintf("comms[parse]: pending command to request status");

					ret = comms_send(data->s, "status\n");
					if (ret) {
						ret = snprintf(status->msg, sizeof(status->msg), "Error requesting status (%d)", ret);
						if (ret < 0)
							status->msg[0] = 0;
						return -1;
					}

					data->cmd = MPC_STATUS;
					break;
				}

				break;
			}
		} else if (!strcmp(msg_type, "ACK")) {
			switch (data->cmd) {
			case MPC_CONNECT:
				ret = snprintf(status->msg, sizeof(status->msg), "Session start failed (%s)", data->parse_buf);
				if (ret < 0)
					status->msg[0] = 0;
				return -1;

			case MPC_PASSWORD:
				ret = snprintf(status->msg, sizeof(status->msg), "Authentication failed (%s)", data->parse_buf);
				if (ret < 0)
					status->msg[0] = 0;
				return -1;

			case MPC_STATUS:
				ret = snprintf(status->msg, sizeof(status->msg), "Status request failed (%s)", data->parse_buf);
				if (ret < 0)
					status->msg[0] = 0;
				return -1;

			case MPC_IDLE:
				ret = snprintf(status->msg, sizeof(status->msg), "Idle command failed (%s)", data->parse_buf);
				if (ret < 0)
					status->msg[0] = 0;
				return -1;

			case MPC_NOIDLE:
				ret = snprintf(status->msg, sizeof(status->msg), "Idle abort failed (%s)", data->parse_buf);
				if (ret < 0)
					status->msg[0] = 0;
				return -1;
			}
			return -1;
		} else {
			switch (data->cmd) {
			case MPC_NONE:
				odprintf("comms[parse]: no command running?");
				ret = snprintf(status->msg, sizeof(status->msg), "Internal error, got data but no command was running");
				if (ret < 0)
					status->msg[0] = 0;
				return -1;

			case MPC_CONNECT:
				odprintf("comms[parse]: ignoring pre-connect message");
				break;

			case MPC_PASSWORD:
				odprintf("comms[parse]: ignoring password response message");
				break;

			case MPC_IDLE:
				if (!strcmp(data->parse_buf, "changed: player")) {
					if (data->pending_cmd == MPC_NONE) {
						odprintf("comms[parse]: player change, queuing status request");
						data->pending_cmd = MPC_STATUS;
					} else {
						odprintf("comms[parse]: player change, ignoring");
					}
				}
				break;

			case MPC_NOIDLE:
				odprintf("comms[parse]: ignoring idle response");
				break;

			case MPC_STATUS:
				if (!strcmp(msg_type, "state:")) {
					if (!strcmp(data->parse_buf, "state: stop")) {
						odprintf("comms[parse]: updating state (STOPPED)");
						status->play = MPD_STOPPED;
						return 1;
					} else if (!strcmp(data->parse_buf, "state: play")) {
						odprintf("comms[parse]: updating state (PLAYING)");
						status->play = MPD_PLAYING;
						return 1;
					} else if (!strcmp(data->parse_buf, "state: pause")) {
						odprintf("comms[parse]: updating state (PAUSED)");
						status->play = MPD_PAUSED;
						return 1;
					} else {
						odprintf("comms[parse]: updating state (UNKNOWN)");
						status->play = MPD_UNKNOWN;
						return 1;
					}
				}
				break;

			case MPC_PLAY:
				odprintf("comms[parse]: ignoring play response");
				break;

			case MPC_PAUSE:
				odprintf("comms[parse]: ignoring pause response");
				break;
			}
		}
	}

	return 0;
}
