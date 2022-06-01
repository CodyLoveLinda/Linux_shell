/*
 * Copyright (C) 2020 AutoX, Inc.
 */
#ifndef __AXCAN_UTIL_H__
#define __AXCAN_UTIL_H__

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#include <linux/can/netlink.h>

#include "cash3_conf.h"
#include "cash3_can_traffic.h"
#include "cash3_fpga_events.h"
#include "cash3_host_cmds.h"
#include <errno.h>
#include <axcan.h>

extern pthread_mutex_t debug_lock;

#ifdef DEBUG
#define AXCAN_LOG_DBG(s...) \
  {                       \
    printf(s);            \
    syslog(LOG_ERR, s);   \
  }
#else
#define AXCAN_LOG_DBG(s...) \
  do {                    \
  } while (0)
#endif
#define AXCAN_LOG_ERR(s...) \
  { printf(s); syslog(LOG_ERR, s); }

#define AXCAN_ERRNO_MASK 0xFF
#define AXCAN_ERRNO_BITS 8
#define AXCAN_TEST_CMD_MAX 10
#define AXCAN_PERF_MSG_CNT 10000
#define AXCAN_MAX_CAN_ID 255

#define AXCAN_DEV_NAME "axcan"
#define AXCAN_DEV_1_NAME "qdmab8000-MM-"
#define AXCAN_DEV_2_NAME "qdmab9000-MM-"
#define AXCAN_DEV_NAME_LEN	256

typedef struct axcan_ihdl {
  int dev_index;
  int dev_state;
  char dev_path[AXCAN_DEV_NAME_LEN];
  int fd;
  uint32_t baudrate;
  uint32_t tx_qsize; /* max tx msgs in queue */
  uint32_t rx_qsize; /* max rx msgs  in queue */
  uint32_t tx_to;    /* tx timeout in ms */
  uint32_t rx_to;    /* rx timeout in ms */
  struct cash3_can_cmd_status state;
  struct cash3_can_net_stats net_stats;
  struct cash3_can_dev_stats can_stats;
} axcan_ihdl_t;

typedef struct axcan_rxq {
  void *buffer;      // data buffer
  void *buffer_end;  // end of data buffer
  size_t capacity;   // maximum number of items in the buffer
  size_t count;      // number of items in the buffer
  size_t sz;         // size of each item in the buffer
  void *head;        // pointer to head
  void *tail;        // pointer to tail
} axcan_rxq_t;

extern axcan_rxq_t rxq[AXCAN_MAX_DEV];

int axcan_qdma_send(axcan_ihdl_t *hdl, enum cash3_can_cmd subcmd, void *p);
uint64_t axcan_qdma_recv(axcan_ihdl_t *hdl, char *buf);
int axcan_check_state(axcan_ihdl_t *ihdl, int pstate);
int axcan_rxq_pop_front(axcan_rxq_t *rxq, void *item);
void *axcan_rx_func(void *arg);

#endif  // __AXCAN_UTIL_H__
