/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <net/socket.h>
#include <stdio.h>
#include <string.h>
#include <drivers/uart.h>
#include <zephyr.h>

#define RECV_BUF_SIZE 1024

char recv_buf[RECV_BUF_SIZE + 1];

const char *at_commands[] = {
	"AT+CFUN?", "AT+CGMM", "AT+CNUM", "AT+CGDCONT?", "AT%XCBAND=?", "AT+CGSN=1", "AT+CGMR", 
	"AT+CESQ", "AT+CNMI=3,2,0,1", "AT+CNMI?", "AT+CMGS=29\r0001000B916164622635F5000012C8329BFD0699E5EF36C8F99693D3E310\x1A", "AT+CGMR", 
	/* Add more here if needed */
};

int blocking_recv(int fd, uint8_t *buf, uint32_t size, uint32_t flags)
{
	int err;

	do {
		err = recv(fd, buf, size, flags);
	} while (err < 0 && errno == EAGAIN);

	return err;
}

int blocking_send(int fd, uint8_t *buf, uint32_t size, uint32_t flags)
{
	int err;

	do {
		err = send(fd, buf, size, flags);
	} while (err < 0 && errno == EAGAIN);

	return err;
}

int blocking_connect(int fd, struct sockaddr *local_addr, socklen_t len)
{
	int err;

	do {
		err = connect(fd, local_addr, len);
	} while (err < 0 && errno == EAGAIN);

	return err;
}

void app_socket_start(void)
{
	int at_socket_fd = socket(AF_LTE, SOCK_DGRAM, NPROTO_AT);

	printk("Starting simple AT socket application\n\r");

	if (at_socket_fd < 0) {
		printk("Socket err: %d, errno: %d\r\n", at_socket_fd, errno);
	}
	for (int i = 0; i < ARRAY_SIZE(at_commands); i++) {
		printk("%s\r",at_commands[i]);
		int bytes_written = send(at_socket_fd, at_commands[i],
					 strlen(at_commands[i]), 0);
		if (bytes_written > 0) {
			int r_bytes =
				blocking_recv(at_socket_fd, recv_buf,
					      sizeof(recv_buf), MSG_DONTWAIT);
			if (r_bytes > 0) {
				printk("%s", recv_buf);
			}
		}
	}
	printk("Closing socket\n\r");
	(void)close(at_socket_fd);
}

int main(void)
{
	app_socket_start();
	while (1) {
		;
	}
}
