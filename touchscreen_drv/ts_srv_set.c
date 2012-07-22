/*
 * This is a binary for sending data via socket to the TouchPad's ts_srv
 * touchscreen driver to change settings.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
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
 *
 * The code was written from scratch by Dees_Troy dees_troy at
 * yahoo
 *
 * Copyright (c) 2012 CyanogenMod Touchpad Project.
 *
 *
 */

/* Standalone binary for setting mode of operation for the touchscreen
 * Run the binary and supply 1 argument to indicate the mode of operation
 * F = Finger
 * S = Stylus
 * M = return current Mode
 */

#define LOG_TAG "ts_srv_set"
#include <cutils/log.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

#define TS_SOCKET_LOCATION "/dev/socket/tsdriver"
#define TS_SOCKET_TIMEOUT 500000
#define SOCKET_BUFFER_SIZE 1

int receive_ts_mode(int ts_fd) {
	// Receives the mode from touchscreen socket
	struct timeval seltmout;
	fd_set fdset;
	int sel_ret, recv_ret;
	char recv_str[SOCKET_BUFFER_SIZE];

	seltmout.tv_sec = 0;
	seltmout.tv_usec = TS_SOCKET_TIMEOUT;
	FD_ZERO(&fdset);
	FD_SET(ts_fd, &fdset);
	sel_ret = select(ts_fd + 1, &fdset, NULL, NULL, &seltmout);
	if (sel_ret == 0) {
		ALOGE("Unable to retrieve current mode - timeout\n");
		return -40;
	} else {
		recv_ret = recv(ts_fd, recv_str, SOCKET_BUFFER_SIZE, 0);
		if (recv_ret > 0) {
			if ((int)recv_str[0] == 0)
				printf("Finger mode\n");
			else if ((int)recv_str[0] == 1)
				printf("Stylus mode\n");
			else {
				ALOGI("Unknown mode '%i'\n", (int)recv_str[0]);
				return -60;
			}
			return 0;
		} else {
			ALOGE("Error receiving mode\n");
			return -50;
		}
	}
}

int send_ts_socket(char *send_data) {
	// Connects to the touchscreen socket
	struct sockaddr_un unaddr;
	int ts_fd, len;

	ts_fd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (ts_fd >= 0) {
		unaddr.sun_family = AF_UNIX;
		strcpy(unaddr.sun_path, TS_SOCKET_LOCATION);
		len = strlen(unaddr.sun_path) + sizeof(unaddr.sun_family);
		if (connect(ts_fd, (struct sockaddr *)&unaddr, len) >= 0) {
			int send_ret;
			send_ret = send(ts_fd, send_data, sizeof(*send_data), 0);
			if (send_ret <= 0) {
				ALOGE("Unable to send data to socket\n");
				return -30;
			} else {
				if ((strcmp(send_data, "F") == 0)) {
					ALOGI("Touchscreen set for finger mode\n");
					return 0;
				} else if ((strcmp(send_data, "S") == 0)) {
					ALOGI("Touchscreen set for stylus mode\n");
					return 0;
				} else {
					// Get the current mode
					return receive_ts_mode(ts_fd);
				}
			}
		} else {
			ALOGE("Unable to connect socket\n");
			return -20;
		}
		close(ts_fd);
	} else {
		ALOGE("Unable to create socket\n");
		return -10;
	}
}

int main(int argc, char** argv)
{
	if (argc != 2 || strlen(argv[1]) != 1 ||
		(strcmp(argv[1], "F") != 0 && strcmp(argv[1], "S") != 0 &&
		strcmp(argv[1], "M") != 0)) {
		printf("Please supply exactly 1 argument:\n");
		printf("F to set finger mode\n");
		printf("S to set stylus mode\n");
		printf("M to display the current setting\n");
		printf("This is used to set the mode of operation for the\n");
		printf("touchscreen driver on the TouchPad\n");
		return -1;
	} else
		return send_ts_socket(argv[1]);
}
