/* 
 * Copyright (C) 2020 AutoX, Inc.
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "axcan.h"

#define	VERSION			"0.1"
#define	RX_THREAD_WAIT_MS	10	/* in ms */

#define	TEST_STD_FRAME_ID	0x72
#define	TEST_EXT_FRAME_ID	0x18ff84a9
#define	TEST_DATA_LEN		8
int gnum_msg = 4;
int g_loop_cnt = 2048;
uint64_t g_tx_qsize = 16384; /* max tx msg queue size */
uint64_t g_rx_qsize = 16384; /* max rx msg queue size */
uint64_t g_txto = 10; /* tx timeout in ms */
uint64_t g_rxto = 100; /* rx timeout in ms */
uint64_t g_tx_sleep = 1; /* tx timeout in ms */
uint64_t g_val = 0xAB;
int g_err = 0;
uint32_t g_rx_id = 1;
int g_debug = 0;
int g_ext_frame = 0;
int baudrate = AXCAN_BAUDRATE_500K;
uint32_t lost = 0;

void *rx_func(void *arg);
void print_usage(char *name);
void print_status(axcan_hdl_t hdl, int id, const char *prefix);
int configure(axcan_hdl_t *hdl, int channel_id);

pthread_mutex_t plock;
static int g_num_recv = 0;
static volatile int g_rx_running = 0;
static int rx_err = 0;
static axcan_hdl_t *g_hdl = 0;

#define	xprintf(...)	\
	pthread_mutex_lock(&plock);	\
	printf(__VA_ARGS__);		\
	pthread_mutex_unlock(&plock);

static void int_handler()
{
	printf("Program interrupted.\n");
	g_rx_running = 0;
}

int main(int argc, char **argv)
{
	axcan_hdl_t hdl2;
	pthread_t rx_thr1;
	struct sigaction sa;
	extern char *optarg;
	int qdma_chan, num;
	int c;

	printf("software version: %s, libcan version: %s\n",
	    VERSION, axcan_get_libversion());

	memset(&sa, 0, sizeof (struct sigaction));
	sa.sa_handler = int_handler;
	sigaction(SIGINT, &sa, NULL);

	while ((c= getopt(argc, argv, "hn:t:T:C:r:R:s:S:b:d:L:eD:Q")) != EOF) {
		switch (c) {
		case '?':
		case 'h':
			print_usage(argv[0]);
			return 0;
		case 'b':
			baudrate = optarg[0] - '0';
                        break;
		case 'L':
			g_loop_cnt = strtol(optarg, NULL, 10);
			break;
		case 'n':
			gnum_msg = strtol(optarg, NULL, 10);
			if (gnum_msg > AXCAN_MAX_BULK_MSG) {
				printf("wrong value specified in -n option: "
				    "exceed AXCAN_MAX_BULK_MSG %d\n",
				    AXCAN_MAX_BULK_MSG);
				return -1;
			}
			break;
                case 'Q':
                        g_rx_qsize = strtol(optarg, NULL, 10);
                        break;
		case 't':
			g_txto = strtol(optarg, NULL, 10);
			break;
                case 'T':
                        qdma_chan = optarg[0] - '0';
                        if (qdma_chan != 8) {
                                printf("unsupported qdma device. use only [8]\n");
                                return 0;
                        }
                        return axcan_trigger_dummy(qdma_chan);
                case 'C':
                        qdma_chan = optarg[0] - '0';
                        if ((qdma_chan < 0) || (qdma_chan >= AXCAN_MAX_DEV)) {
                                printf("unsupported can device. use only [0-5]\n");
                                return 0;
                        }
                        axcan_open(qdma_chan, 0, g_txto, g_rxto, &hdl2);
                        axcan_test(hdl2);
			axcan_close(hdl2);
                        return 0;
		case 'r':
			qdma_chan = optarg[0] - '0';
			axcan_open(qdma_chan, 0, g_txto, g_rxto, &hdl2);
			axcan_reset(hdl2);
			axcan_close(hdl2);
			return 0;
		case 'R':
			qdma_chan = optarg[0] - '0';
			axcan_open(qdma_chan, 0, g_txto, g_rxto, &hdl2);
			axcan_dump_debug_regs(hdl2);
			axcan_close(hdl2);
			return 0;
		case 's':
			qdma_chan = optarg[0] - '0';
			axcan_open(qdma_chan, 0, g_txto, g_rxto, &hdl2);
			axcan_get_state(hdl2);
			axcan_get_stats(hdl2);
			axcan_close(hdl2);
			return 0;
		case 'S':
			qdma_chan = optarg[0] - '0';
			axcan_open(qdma_chan, 0, g_txto, g_rxto, &hdl2);
			return axcan_clear_stats(hdl2);
		case 'd':
			g_rx_id = optarg[0] - '0';
			break;
		case 'D':
			g_debug = 1;
			break;
		case 'e':
			g_ext_frame = 1;
			break;
		default:
			printf("unsupported argument\n");
			print_usage(argv[0]);
			return -1;
		}
	}

	num = (gnum_msg <= 0) ? 1 : gnum_msg;
	g_rxto = g_txto * num + RX_THREAD_WAIT_MS;
	printf("baudrate = 500kbps, tx_to = %" PRIu64 "ms, "
	    "gnum_msg = %" PRId32", rx_channel_id = %" PRIu32 "\n",
	    g_txto, gnum_msg, g_rx_id);

	if (configure(&hdl2, g_rx_id)) {
		axcan_close(hdl2);
		printf("configure Rx channel %" PRIu32 " failed.\n", g_rx_id);
		return -1;
	}

	if (pthread_create(&rx_thr1, NULL, rx_func, (void *) hdl2) != 0) {
		printf("rx_thr create failed\n");
		goto err;
	}

	pthread_join(rx_thr1, NULL);

	if (axcan_stop(hdl2) != 0) {
		printf("axcan_stop hdl2 failed\n");
	}

	if (g_err || lost) {
		printf("TEST FAILED: Lost %d msgs\n", lost);
	} else {
		printf("TEST PASSED\n");
	}
err:
	if (axcan_close(hdl2) != 0) {
		printf("axcan_close hdl2 failed\n");
	}

	return rx_err;
}

int configure(axcan_hdl_t *hdl, int channel_id)
{
	int rtn;

	rtn = axcan_open(channel_id, 0, g_txto, g_rxto, hdl);
	if (rtn != 0) {
		printf("axcan_open failed for channel %d: %s (%d)\n",
		    channel_id, axcan_get_err_msg(rtn), rtn);
		goto err1;
	}

	g_hdl = hdl;
	printf("axcan_open success for channel %d\n", channel_id);

	rtn = axcan_set_baudrate(*hdl, baudrate);
	if (rtn  != 0) {
		printf("axcan_set_baudrate failed for channel %d: %s (%d)\n",
		    channel_id, axcan_get_err_msg(rtn), rtn);
		goto err;
	}
	printf("axcan_set_baudrate 500kbps success\n");

	rtn = axcan_start(*hdl);
	if (rtn != 0) {
		printf("axcan_start failed for channel %d: %s (%d)\n",
		    channel_id, axcan_get_err_msg(rtn), rtn);
		goto err;
	}
	printf("axcan_start success\n");

	return 0;

err:
	rtn = axcan_close(*hdl);
	if (rtn != 0) {
		printf("axcan_close failed for channel %d: %s (%d)\n",
		    channel_id, axcan_get_err_msg(rtn), rtn);
	}
err1:
	return -1;
}

void *rx_func(void *arg)
{
	axcan_hdl_t hdl = (axcan_hdl_t)arg;
	int c;
	uint32_t num_msg;
	axcan_msg_t *buf;
	struct timespec ts;

	buf = malloc(sizeof(axcan_msg_t) * 1024 * 1024);
	if (!buf) {
		perror("malloc");
		goto err;
	}

	xprintf("rx thread started, to receive %" PRId32 " msgs\n", gnum_msg);

	num_msg = (gnum_msg <= 0) ? 1 : gnum_msg;
	g_rx_running = 1;

	while (g_rx_running) {
#ifdef FIXME
		if ((gnum_msg > 0) && (g_num_recv == gnum_msg)) {
#else
		lost = unknown_qevent[g_rx_id]*num_msg;
		if (g_num_recv >= (g_loop_cnt - lost)) {
#endif
			break;
		}
		if ((c = axcan_recv(hdl, buf, num_msg)) < 0) {
			if (gnum_msg <= 0) {
				if (!g_debug) {
					goto err;
				}
			} else {
				xprintf("can_read failed: %s (%d). total msg recvd %d\n",
				    axcan_get_err_msg(c), c, g_num_recv);
				++rx_err;
				g_err = 1;
				if (!g_debug) {
					goto err;
				}
			}
			continue;
		}

		g_num_recv += c;

		if (g_debug) {
		    clock_gettime(CLOCK_REALTIME, &ts);
		    xprintf("can_read: recvd %d msg, total msg recvd %d, TS: %ld.%06ld\n",
		        c, g_num_recv, ts.tv_sec, ts.tv_nsec / 1000);
		}

		if (c < num_msg) {
			//xprintf("can_read: not enough msg recvd, expect %d\n", num_msg);
			num_msg -= c;
			num_msg = (gnum_msg <= 0) ? 1 : gnum_msg;
		} else {
			num_msg = (gnum_msg <= 0) ? 1 : gnum_msg;
		}
	}

err:
	print_status(hdl, g_rx_id, "can_read");
	if (buf) {
		free(buf);
	}
	pthread_exit(NULL);
}

void print_status(axcan_hdl_t hdl, int id, const char *prefix)
{
	int status;
	uint8_t rx_err, tx_err;

	pthread_mutex_lock(&plock);
	if ((status = axcan_get_state(hdl)) < 0) {
		printf("%s: channel %d, can_get_status() error: %s (%d)\n",
		    prefix, id, axcan_get_err_msg(status), status);
	} else {
		printf("%s: channel %d, status reg 0x%x\n",
		    prefix, id, status);
	}

	if ((status = axcan_get_err_counter(hdl, &rx_err, &tx_err)) < 0) {
		printf("%s: channel %d, can_get_err_counter() error: %s (%d)\n",
		    prefix, id, axcan_get_err_msg(status), status);
	} else {
		printf("%s: channel %d, rx_err_counter %d, tx_err_counter %d\n",
		    prefix, id, rx_err, tx_err);
	}
	pthread_mutex_unlock(&plock);
}

void print_usage(char *name)
{
	printf("Usage: %s -d -n -L -t -b -T -C -e -Q -D -s -S -r -R\n"
	    "\t-d -> Tx and Rx channel id. e.g. -d 10 (Tx on 1 and Rx on 0)\n"
	    "\t-n -> Number of CAN msgs per bulk msg (Maximum %d)\n"
	    "\t-L -> Loop count for msgs to be transmitted\n"
	    "\t-t -> Tx timeout value in ms\n"
	    "\t-b -> CAN baudrate (0 -> 1Mbps, 1 -> 500Kbps, 2 -> 250Kbps,  3 -> 150Kbps)\n"
	    "\t-T -> CAN QDMA Trigger Dummy\n"
	    "\t-C -> CAN Test <canid> \n"
	    "\t-e -> Use extended frames\n"
	    "\t-Q -> Rx Queue size\n"
	    "\t-D -> debug \n"
	    "\t-s -> Get CAN status \n"
	    "\t-S -> Clear CAN statistics \n"
	    "\t-r -> Reset \n"
	    "\t-R -> Dump debug regs \n",
	name, AXCAN_MAX_BULK_MSG);
}
