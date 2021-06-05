/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <drivers/uart.h>
#include <string.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/nrf_clock_control.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>

#define AT_XMODEM_TRACE		"AT\%XMODEMTRACE=1,2"
#define AT_CEREG			"AT+CEREG=5"
#define AT_CNEC				"AT+CNEC=24"
#define AT_CSCON			"AT+CSCON=3"
#define AT_CESQ				"AT\%CESQ=1"

static const char *const at_commands[] = {
	AT_XMODEM_TRACE,
	AT_CEREG,
	AT_CNEC,
	AT_CSCON,
	AT_CESQ
};

/**@brief Recoverable modem library error. */
void nrf_modem_recoverable_error_handler(uint32_t err)
{
	printk("Modem library recoverable error: %u\n", err);
}

/* To strictly comply with UART timing, enable external XTAL oscillator */
void enable_xtal(void)
{
	struct onoff_manager *clk_mgr;
	static struct onoff_client cli = {};

	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	sys_notify_init_spinwait(&cli.notify);
	(void)onoff_request(clk_mgr, &cli);
}

/* Run each of the AT Commands defined above one at a time */
static int setup_commands(void)
{
	for (int i = 0; i < ARRAY_SIZE(at_commands); i++) {

		if (at_cmd_write(at_commands[i], NULL, 0, NULL) != 0) {
			return -1;
		}
	}

	return 0;
}

void main(void)
{
	enable_xtal();
	if (setup_commands() != 0) {
		printk("Failed to setup modem\n");
		return -1;
	}
	else{
		printk("The AT host sample started\n");
		printk("Modem trace and notifications for CEREG, CNEC, CSCON, and CESQ are enabled\n");
		return 0;
	}
}
