/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <nrf_socket.h>
#include <net/socket.h>
#include <stdio.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <dk_buttons_and_leds.h>

#ifdef CONFIG_SUPL_CLIENT_LIB
#include <supl_os_client.h>
#include <supl_session.h>
#include "supl_support.h"
#include <math.h> // used for pow function
#endif

#define AT_XFACTORYRESET	"AT\%XFACTORYRESET=0" // This command is only supported in mfw v1.3.x
#define AT_XSYSTEMMODE      "AT\%XSYSTEMMODE=1,0,1,0"
#define AT_ACTIVATE_GPS     "AT+CFUN=31"
#define AT_ACTIVATE_LTE     "AT+CFUN=21"
#define AT_DEACTIVATE_LTE   "AT+CFUN=20"
#define AT_XMODEMTRACE     	"AT\%XMODEMTRACE=1,2"

#define GNSS_INIT_AND_START 1
#define GNSS_STOP           2
#define GNSS_RESTART        3

#define AT_CMD_SIZE(x) (sizeof(x) - 1)

#ifdef CONFIG_BOARD_NRF9160DK_NRF9160NS
#define AT_MAGPIO      "AT\%XMAGPIO=1,0,0,1,1,1574,1577"
#ifdef CONFIG_GPS_SAMPLE_ANTENNA_ONBOARD
#define AT_COEX0       "AT\%XCOEX0=1,1,1565,1586"
#elif CONFIG_GPS_SAMPLE_ANTENNA_EXTERNAL
#define AT_COEX0       "AT\%XCOEX0"
#endif
#endif /* CONFIG_BOARD_NRF9160DK_NRF9160NS */

#ifdef CONFIG_BOARD_THINGY91_NRF9160NS
#define AT_MAGPIO      "AT\%XMAGPIO=1,1,1,7,1,746,803,2,698,748,2,1710,2200," \
			"3,824,894,4,880,960,5,791,849,7,1565,1586"
#ifdef CONFIG_GPS_SAMPLE_ANTENNA_ONBOARD
#define AT_COEX0       "AT\%XCOEX0=1,1,1565,1586"
#elif CONFIG_GPS_SAMPLE_ANTENNA_EXTERNAL
#define AT_COEX0       "AT\%XCOEX0"
#endif
#endif /* CONFIG_BOARD_THINGY91_NRF9160NS */

static const char update_indicator[] = {'\\', '|', '/', '-'};
static const char *const at_commands[] = {
	AT_XFACTORYRESET,
	AT_XSYSTEMMODE,
#if defined(CONFIG_BOARD_NRF9160DK_NRF9160NS) || \
	defined(CONFIG_BOARD_THINGY91_NRF9160NS)
	AT_MAGPIO,
	AT_COEX0,
#endif
#if defined(CONFIG_NRF_MODEM_LIB_TRACE_ENABLED)
	AT_XMODEMTRACE,
#endif
	AT_ACTIVATE_GPS
};

static int                   gnss_fd;
static char                  nmea_strings[10][NRF_GNSS_NMEA_MAX_LEN];
static uint32_t                 nmea_string_cnt;

static bool                  got_fix;
static uint64_t                 fix_timestamp;
static uint64_t					ttff;
static nrf_gnss_data_frame_t last_pvt;

#ifdef CONFIG_SUPL_CLIENT_LIB
static double init_inj_lat;
static double init_inj_long;
#endif

K_SEM_DEFINE(lte_ready, 0, 1);

void nrf_modem_recoverable_error_handler(uint32_t error)
{
	printf("Err: %lu\n", (unsigned long)error);
}

static int setup_modem(void)
{
	for (int i = 0; i < ARRAY_SIZE(at_commands); i++) {

		if (at_cmd_write(at_commands[i], NULL, 0, NULL) != 0) {
			return -1;
		}
	}

	return 0;
}

#ifdef CONFIG_SUPL_CLIENT_LIB
/* Accepted network statuses read from modem */
static const char status1[] = "+CEREG: 1";
static const char status2[] = "+CEREG:1";
static const char status3[] = "+CEREG: 5";
static const char status4[] = "+CEREG:5";

static void wait_for_lte(void *context, const char *response)
{
	if (!memcmp(status1, response, AT_CMD_SIZE(status1)) ||
		!memcmp(status2, response, AT_CMD_SIZE(status2)) ||
		!memcmp(status3, response, AT_CMD_SIZE(status3)) ||
		!memcmp(status4, response, AT_CMD_SIZE(status4))) {
		k_sem_give(&lte_ready);
	}
}

static int activate_lte(bool activate)
{
	if (activate) {
		if (at_cmd_write(AT_ACTIVATE_LTE, NULL, 0, NULL) != 0) {
			return -1;
		}

		at_notif_register_handler(NULL, wait_for_lte);
		if (at_cmd_write("AT+CEREG=2", NULL, 0, NULL) != 0) {
			return -1;
		}

		k_sem_take(&lte_ready, K_FOREVER);

		at_notif_deregister_handler(NULL, wait_for_lte);
		if (at_cmd_write("AT+CEREG=0", NULL, 0, NULL) != 0) {
			return -1;
		}
	} else {
		if (at_cmd_write(AT_DEACTIVATE_LTE, NULL, 0, NULL) != 0) {
			return -1;
		}
	}

	return 0;
}
#endif

static int gnss_ctrl(uint32_t ctrl)
{
	int retval;

	nrf_gnss_fix_retry_t    fix_retry    = 0;
	nrf_gnss_fix_interval_t fix_interval = 0; // Attempt a single fix
	nrf_gnss_delete_mask_t	delete_mask; // Delete masks set individually below
	nrf_gnss_nmea_mask_t	nmea_mask    = NRF_GNSS_NMEA_GSV_MASK |
					       NRF_GNSS_NMEA_GSA_MASK |
					       NRF_GNSS_NMEA_GLL_MASK |
					       NRF_GNSS_NMEA_GGA_MASK |
					       NRF_GNSS_NMEA_RMC_MASK;
	nrf_gnss_use_case_t		use_case 	= NRF_GNSS_USE_CASE_SINGLE_COLD_START;

	if (ctrl == GNSS_INIT_AND_START) {
		gnss_fd = nrf_socket(NRF_AF_LOCAL,
				     NRF_SOCK_DGRAM,
				     NRF_PROTO_GNSS);

		if (gnss_fd >= 0) {
			printk("GPS Socket created\n");
		} else {
			printk("Could not init socket (err: %d)\n", gnss_fd);
			return -1;
		}

		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_FIX_RETRY,
					&fix_retry,
					sizeof(fix_retry));
		if (retval != 0) {
			printk("Failed to set fix retry value\n");
			return -1;
		}

		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_FIX_INTERVAL,
					&fix_interval,
					sizeof(fix_interval));
		if (retval != 0) {
			printk("Failed to set fix interval value\n");
			return -1;
		}

		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_NMEA_MASK,
					&nmea_mask,
					sizeof(nmea_mask));
		if (retval != 0) {
			printk("Failed to set nmea mask\n");
			return -1;
		}

		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_USE_CASE,
					&use_case,
					sizeof(use_case));
		if (retval != 0) {
			printk("Failed to set use case\n");
			return -1;
		}
	}

	/* Separated GNSS_INIT_AND_START and GNSS_RESTART into 2 sections. GNSS_INIT_AND_START will still
	 * run first before A-GPS to clear existing data and then after A-GPS data is injected,
	 * GNSS_RESTART will start GPS without deleting injected data. GNSS_STOP will take care of clearing
	 * data before attempting the next fix with GNSS_RESTART.
	 */

	if (ctrl == GNSS_INIT_AND_START) {
		delete_mask = 0x7f; // First start deletes everything but TCXO
		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_START,
					&delete_mask,
					sizeof(delete_mask));
		if (retval != 0) {
			printk("Failed to start GPS\n");
			return -1;
		}
	}

	if (ctrl == GNSS_RESTART) {
		delete_mask = 0x00; // Restart deletes nothing
		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_START,
					&delete_mask,
					sizeof(delete_mask));
		if (retval != 0) {
			printk("Failed to start GPS\n");
			return -1;
		}
	}

	if (ctrl == GNSS_STOP) {
		delete_mask = 0x7f; // Stop deletes everything but TCXO
		retval = nrf_setsockopt(gnss_fd,
					NRF_SOL_GNSS,
					NRF_SO_GNSS_STOP,
					&delete_mask,
					sizeof(delete_mask));
		if (retval != 0) {
			printk("Failed to stop GPS\n");
			return -1;
		}
	}

	return 0;
}

static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	if (has_changed & button_states & DK_BTN1_MSK) {
		got_fix = false;
	}
}

static int init_app(void)
{
	int retval;

	retval = dk_buttons_init(button_handler);
	if (retval != 0) {
		printk("dk_buttons_init, error: %d", retval);
		return -1;
	}

	if (setup_modem() != 0) {
		printk("Failed to initialize modem\n");
		return -1;
	}

	retval = gnss_ctrl(GNSS_INIT_AND_START);

	return retval;
}

static void print_satellite_stats(nrf_gnss_data_frame_t *pvt_data)
{
	uint8_t  tracked          = 0;
	uint8_t  in_fix           = 0;
	uint8_t  unhealthy        = 0;

	for (int i = 0; i < NRF_GNSS_MAX_SATELLITES; ++i) {

		if ((pvt_data->pvt.sv[i].sv > 0) &&
		    (pvt_data->pvt.sv[i].sv < 33)) {

			tracked++;

			if (pvt_data->pvt.sv[i].flags &
					NRF_GNSS_SV_FLAG_USED_IN_FIX) {
				in_fix++;
			}

			if (pvt_data->pvt.sv[i].flags &
					NRF_GNSS_SV_FLAG_UNHEALTHY) {
				unhealthy++;
			}
		}
	}

	printk("Tracking: %d Using: %d Unhealthy: %d\n", tracked,
							 in_fix,
							 unhealthy);
}

static void print_gnss_stats(nrf_gnss_data_frame_t *pvt_data)
{
	if (pvt_data->pvt.flags & NRF_GNSS_PVT_FLAG_DEADLINE_MISSED) {
		printk("GNSS notification deadline missed\n");
	}
	if (pvt_data->pvt.flags & NRF_GNSS_PVT_FLAG_NOT_ENOUGH_WINDOW_TIME) {
		printk("GNSS operation blocked by insufficient time windows\n");
	}
}

static void print_fix_data(nrf_gnss_data_frame_t *pvt_data)
{
	printf("Longitude:  %f\n", pvt_data->pvt.longitude);
	printf("Latitude:   %f\n", pvt_data->pvt.latitude);
	printf("Altitude:   %f\n", pvt_data->pvt.altitude);
	printf("Speed:      %f\n", pvt_data->pvt.speed);
	printf("Heading:    %f\n", pvt_data->pvt.heading);
	printk("Date:       %02u-%02u-%02u\n", pvt_data->pvt.datetime.year,
					       pvt_data->pvt.datetime.month,
					       pvt_data->pvt.datetime.day);
	printk("Time (UTC): %02u:%02u:%02u\n", pvt_data->pvt.datetime.hour,
					       pvt_data->pvt.datetime.minute,
					      pvt_data->pvt.datetime.seconds);
	#ifdef CONFIG_SUPL_CLIENT_LIB
		printf("Initially injected location: %3.8f, %3.8f \n", init_inj_lat, init_inj_long);
	#endif
	printf("Fix location: %3.8f, %3.8f \n", pvt_data->pvt.latitude, pvt_data->pvt.longitude);
}

static void print_nmea_data(void)
{
	for (int i = 0; i < nmea_string_cnt; ++i) {
		printk("%s", nmea_strings[i]);
	}
}

int process_gps_data(nrf_gnss_data_frame_t *gps_data)
{
	int retval;

	retval = nrf_recv(gnss_fd,
			  gps_data,
			  sizeof(nrf_gnss_data_frame_t),
			  NRF_MSG_DONTWAIT);

	if (retval > 0) {

		switch (gps_data->data_id) {
		case NRF_GNSS_PVT_DATA_ID:
			memcpy(&last_pvt,
			       gps_data,
			       sizeof(nrf_gnss_data_frame_t));
			nmea_string_cnt = 0;
			got_fix = false;

			if (gps_data->pvt.flags
					& NRF_GNSS_PVT_FLAG_FIX_VALID_BIT) {

				got_fix = true;
				ttff = (k_uptime_get() - fix_timestamp) / 1000;
				fix_timestamp = k_uptime_get();
			}
			break;

		case NRF_GNSS_NMEA_DATA_ID:
			if (nmea_string_cnt < 10) {
				memcpy(nmea_strings[nmea_string_cnt++],
				       gps_data->nmea,
				       retval);
			}
			break;

		case NRF_GNSS_AGPS_DATA_ID:
#ifdef CONFIG_SUPL_CLIENT_LIB
			printk("\033[1;1H");
			printk("\033[2J");
			printk("New AGPS data requested, contacting SUPL server, flags %d\n",
			       gps_data->agps.data_flags);
			gnss_ctrl(GNSS_STOP);
			activate_lte(true);
			printk("Established LTE link\n");
			if (open_supl_socket() == 0) {
				printf("Starting SUPL session\n");
				supl_session(&gps_data->agps);
				printk("Done\n");
				close_supl_socket();
			}
			activate_lte(false);
			gnss_ctrl(GNSS_RESTART);
			// k_msleep(2000); // Not needed

			// Reset fix_timestamp so the TTFF timer starts after A-GPS data is acquired
			fix_timestamp = k_uptime_get(); 
#endif
			break;

		default:
			break;
		}
	}

	return retval;
}

#ifdef CONFIG_SUPL_CLIENT_LIB
int inject_agps_type(void *agps,
		     size_t agps_size,
		     nrf_gnss_agps_data_type_t type,
		     void *user_data)
{
	ARG_UNUSED(user_data);

	if (type == NRF_GNSS_AGPS_LOCATION) {
		/* Dump location data */
		nrf_gnss_agps_data_location_t *loc = agps;
		printk("latitude: %d\n", loc->latitude);
		printk("longitude: %d\n", loc->longitude);
		printk("altitude: %d\n", loc->altitude);
		printk("unc_semimajor: %u\n", loc->unc_semimajor);
		printk("unc_semiminor: %u\n", loc->unc_semiminor);
		printk("orientation_major: %u\n", loc->orientation_major);
		printk("unc_altitude: %u\n", loc->unc_altitude);
		printk("confidence: %u\n", loc->confidence);

		double tmp1 = pow(2,23);
		double tmp2 = tmp1/90;
		init_inj_lat = loc->latitude/tmp2;

		tmp1 = pow(2,24);
		tmp2 = tmp1/360;
		init_inj_long = loc->longitude/tmp2;

		printf("Initially injected location: %3.8f, %3.8f \n", init_inj_lat, init_inj_long);
	}

	int retval = nrf_sendto(gnss_fd,
				agps,
				agps_size,
				0,
				&type,
				sizeof(type));

	if (retval != 0) {
		printk("Failed to send AGNSS data, type: %d (err: %d)\n",
		       type,
		       errno);
		return -1;
	}

	printk("Injected AGPS data, flags: %d, size: %d\n", type, agps_size);

	return 0;
}
#endif

int main(void)
{
	nrf_gnss_data_frame_t gps_data;
	uint8_t		      cnt = 0;

#ifdef CONFIG_SUPL_CLIENT_LIB
	static struct supl_api supl_api = {
		.read       = supl_read,
		.write      = supl_write,
		.handler    = inject_agps_type,
		.logger     = supl_logger,
		.counter_ms = k_uptime_get
	};

	//AGPS data frame to request all data
	static nrf_gnss_agps_data_frame_t agps_data = {
		0xffffffff, // ephe
		0xffffffff, // alm
		0x3b // flags
	};
#endif

	printk("Starting GPS application\n");

	if (init_app() != 0) {
		return -1;
	}

#ifdef CONFIG_SUPL_CLIENT_LIB
	int rc = supl_init(&supl_api);

	if (rc != 0) {
		return rc;
	}
#endif

	printk("Getting GPS data...\n");

	while (1) {

		do {
			/* Loop until we don't have more
			 * data to read
			 */
		} while (process_gps_data(&gps_data) > 0);

		if (IS_ENABLED(CONFIG_GPS_SAMPLE_NMEA_ONLY)) {
			print_nmea_data();
			nmea_string_cnt = 0;
		} else {
			printk("\033[1;1H");
			printk("\033[2J");
			print_satellite_stats(&last_pvt);
			print_gnss_stats(&last_pvt);
			printk("---------------------------------\n");

			if (!got_fix) {
				printk("Seconds since last fix: %lld\n",
				       (k_uptime_get() - fix_timestamp) / 1000);
				cnt++;
				printk("Searching [%c]\n",
				       update_indicator[cnt%4]);
			} else {
				print_fix_data(&last_pvt);
				printk("\nTime to get fix: %lld seconds\n", ttff);
				printk("---------------------------------");
				printk("\nNMEA strings for GPS fix:\n\n");
				print_nmea_data();

				printk("\nPress Button 1 to attempt another fix.\n");
				gnss_ctrl(GNSS_STOP);
				
				// wait here for user to press button 1 to attempt another a fix
				while (got_fix) {
					k_msleep(500);
				}
#ifdef CONFIG_SUPL_CLIENT_LIB
				/* Fetch APGS data and start next round */
				printk("Establishing LTE link...\n");
				activate_lte(true);
				printk("Established LTE link\n");
				if (open_supl_socket() == 0) {
					printf("Starting SUPL session\n");
					supl_session(&agps_data);
					printk("Done\n");
					close_supl_socket();
				}
				activate_lte(false);
#endif
				got_fix = false;
				fix_timestamp = k_uptime_get();
				gnss_ctrl(GNSS_RESTART);
				continue;
			}

			printk("\nNMEA strings:\n\n");
			print_nmea_data();
			printk("---------------------------------");
		}

		k_msleep(500);
	}

	return 0;
}
