/*
 * Copyright (C) 2020 AutoX, Inc.
 */

#ifndef __AAXCAN_API_H__
#define __AAXCAN_API_H__

#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AXCAN_EXTENDED_FRAME 0x20000000
#define AXCAN_20B_BASE 0x20000000

#define AXCAN_MAX_DEV 9
#define AXCAN_MAX_BULK_MSG 64
#define AXCAN_MAX_TX_MSG (AXCAN_MAX_BULK_MSG * 1)
#define AXCAN_MAX_RX_MSG 16384

/* Channel states */
#define AXCAN_DEV_UNINIT -1
#define AXCAN_DEV_OPEN (1 << 0)
#define AXCAN_DEV_CLOSE (1 << 1)
#define AXCAN_DEV_BAUD_SET (1 << 2)
#define AXCAN_DEV_NORMAL (1 << 3)
#define AXCAN_DEV_LOOPBACK (1 << 4)
#define AXCAN_DEV_CONFIG (1 << 5)
#define AXCAN_DEV_START (1 << 6)
#define AXCAN_DEV_STOP (1 << 7)
#define AXCAN_DEV_ACTIVE (1 << 8)
#define AXCAN_DEV_RECVD (1 << 9)

typedef uint64_t AXCAN_CTRL_STATE;
typedef uint64_t AXCAN_BUS_STATISTIC;
typedef uint64_t CAN_IF_STATUS;
typedef uint64_t axcan_hdl_t;
typedef axcan_hdl_t AXCAN_HANDLE;
extern uint64_t unknown_qevent[AXCAN_MAX_DEV];
extern pthread_t axcan_rx_thr[AXCAN_MAX_DEV];
extern pthread_mutex_t rxq_lock[AXCAN_MAX_DEV];
extern volatile int axcan_rx_running[AXCAN_MAX_DEV];

/*
 * AXCAN message definition
 */
typedef struct axcan_msg {
  uint32_t id; /* AXCAN-ID (11-/29-bit) or Event-ID        [Tx, Rx] */
  uint8_t len; /* Bit 0-3 = Data Length Code             [Tx, Rx] */
  /* Bit 4   = RTR (Classical AXCAN)          [Tx, Rx] */
  /*         = No_BRS (AXCAN FD)              [Tx, Rx] */
  /* Bit 5   = No_Data (Object Mode)        [    Rx] */
  /*         = Interaction Data (FIFO Mode)          */
  /* Bit 6   = Reserved                     [Tx, Rx] */
  /* Bit 7   = Type(AXCAN FD / Classical AXCAN) [Tx, Rx] */
  uint8_t msg_lost; /* counter for lost Rx messages */
  uint8_t reserved[2];
  uint8_t data[8];           /* 8 data-bytes */
  struct timespec timestamp; /* timestamp of this message */
} axcan_msg_t;

/*
 * AXCAN error code
 */
enum axcan_err_code {
  AXCAN_PARAM_INVALID = -12,
  AXCAN_HDL_INVALID,
  AXCAN_DEV_INVALID,
  AXCAN_DEV_ERR,
  AXCAN_DEV_BUSY,
  AXCAN_TIMEOUT,
  AXCAN_FAIL,
  AXCAN_NOT_SUPPORTED,
  AXCAN_NOT_IMPLEMENTED,
  AXCAN_INVALID,
  AXCAN_NO_BUFFERS,
  AXCAN_ERR,
  AXCAN_OK, /* 0 */
  AXCAN_PARTIAL_OK,
  AXCAN_RX_TIMEOUT,
  AXCAN_TX_TIMEOUT,
  AXCAN_TX_ERROR,
  AXCAN_CONTR_OFF_BUS,
  AXCAN_CONTR_BUSY,
  AXCAN_CONTR_NO_ID_ENABLED,
  AXCAN_ID_ALREADY_ENABLED,
  AXCAN_ID_NOT_ENABLED,
  AXCAN_MESSAGE_LOST,
};

/*
 * CAN operational and error states
 */
enum axcan_state {
  AXCAN_STATE_ERROR_ACTIVE = 0,	/* RX/TX error count < 96 */
  AXCAN_STATE_ERROR_WARNING,	/* RX/TX error count < 128 */
  AXCAN_STATE_ERROR_PASSIVE,	/* RX/TX error count < 256 */
  AXCAN_STATE_BUS_OFF,		/* RX/TX error count >= 256 */
  AXCAN_STATE_STOPPED,		/* Device is stopped */
  AXCAN_STATE_SLEEPING,		/* Device is sleeping */
  AXCAN_STATE_MAX
};

/*
 * CAN device statistics
 */
struct axcan_device_stats {
  uint32_t bus_error;	/* Bus errors */
  uint32_t error_warning;	/* Changes to error warning state */
  uint32_t error_passive;	/* Changes to error passive state */
  uint32_t bus_off;		/* Changes to bus off state */
  uint32_t arbitration_lost; /* Arbitration lost errors */
  uint32_t restarts;		/* CAN controller re-starts */
};


typedef enum axcan_baudrate_t {
  AXCAN_BAUDRATE_1M,
  AXCAN_BAUDRATE_500K,
  AXCAN_BAUDRATE_250K,
  AXCAN_BAUDRATE_150K,
  AXCAN_BAUDRATE_NUM
} AXCAN_BITRATE;

/* Returns can library version. */
const char *axcan_get_libversion(void);

/* Returns error message corresponding to the given error code. */
const char *axcan_get_err_msg(int err_code);

int axcan_test(axcan_hdl_t hdl);
int axcan_open(uint32_t dev_index, uint32_t flags, 
             uint64_t tx_to, uint64_t rx_to, axcan_hdl_t *hdl);
int axcan_close(axcan_hdl_t hdl);
int axcan_start(axcan_hdl_t hdl);
int axcan_stop(axcan_hdl_t hdl);
int axcan_reset(axcan_hdl_t hdl);
int axcan_id_add(axcan_hdl_t hdl, uint32_t id);
int axcan_id_del(axcan_hdl_t hdl, uint32_t id);
int axcan_get_stats(axcan_hdl_t hdl);
int axcan_clear_stats(axcan_hdl_t hdl);
int axcan_set_loopback(axcan_hdl_t hdl);
int axcan_unset_loopback(axcan_hdl_t hdl);
int axcan_set_baudrate(axcan_hdl_t hdl, enum axcan_baudrate_t rate);
int axcan_get_baudrate(axcan_hdl_t hdl, enum axcan_baudrate_t *rate);
int axcan_recv(axcan_hdl_t hdl, axcan_msg_t *buf, int32_t num_msg);
int axcan_send(axcan_hdl_t hdl, axcan_msg_t *buf, int32_t num_msg);
int axcan_send_hi_pri(axcan_hdl_t hdl, axcan_msg_t *buf);
int axcan_get_state(axcan_hdl_t hdl);
int axcan_dump_debug_regs(axcan_hdl_t hdl);
int axcan_get_err_counter(axcan_hdl_t hdl, uint8_t *rx_err, uint8_t *tx_err);
int axcan_trigger_dummy(uint32_t dev_index);

#ifdef __cplusplus
}
#endif
#endif /* __AAXCAN_API_H__ */
