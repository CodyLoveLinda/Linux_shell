/*
 * This file captures the CASH3 traffic cmd and events data structure
 */
#ifndef __CASH3_CAN_TRAFFIC_H__
#define __CASH3_CAN_TRAFFIC_H__

#include "cash3_conf.h"
#include <linux/can.h>

/*
 * CAN cmds
 */
enum cash3_can_cmd {
  CASH3_CAN_CMD_TEST = 0,
  CASH3_CAN_CMD_TX_TIMEOUT_SET,
  CASH3_CAN_CMD_RX_TIMEOUT_SET,
  CASH3_CAN_CMD_DEV_START,
  CASH3_CAN_CMD_DEV_STOP,
  CASH3_CAN_CMD_DEV_RESET,
  CASH3_CAN_CMD_ADD_FILTER,
  CASH3_CAN_CMD_DEL_FILTER,
  CASH3_CAN_CMD_BAUDRATE_SET,
  CASH3_CAN_CMD_BAUDRATE_GET,
  CASH3_CAN_CMD_LOOPBACK_SET,
  CASH3_CAN_CMD_LOOPBACK_UNSET,
  CASH3_CAN_CMD_RECV,
  CASH3_CAN_CMD_SEND,
  CASH3_CAN_CMD_SEND_HIPRI,
  CASH3_CAN_CMD_GET_CAN_STATE,
  CASH3_CAN_CMD_GET_CAN_STATS,
  CASH3_CAN_CMD_GET_NET_STATS,
  CASH3_CAN_CMD_CLEAR_STATS,
  CASH3_CAN_CMD_DUMP_HW_REGS,
  CASH3_CAN_CMD_CMD_MAX
};

/*
 * CAN Rx/Tx msg
 */
struct cash3_cmsg {
  struct can_frame msg;
  struct timespec timestamp; //64-bit Alignment is critical
} __attribute__((packed));

struct cash3_can_msgs {
  unsigned int num_msg_tx;  // req no. of bulk msgs to be sent/received
  unsigned int num_msg_rx;  // actual no. bulk msg successfully sent/received
  struct cash3_cmsg msgs[CASH3_CAN_BULK_MSG_MAX];
                              // bulk mode for better performance
} __attribute__((packed));

/*
 * CAN error and status
 */
struct cash3_can_status_err {
  unsigned int can_status;
  unsigned int can_err_status;
  unsigned int can_err_count;
  int can_err;
} __attribute__((packed));

/*
 * CAN baudrate
 */
enum cash3_can_baudrate {
  CASH3_CAN_BAUDRATE_1M,
  CASH3_CAN_BAUDRATE_500K,
  CASH3_CAN_BAUDRATE_250K,
  CASH3_CAN_BAUDRATE_150K,
  CASH3_CAN_BAUDRATE_MAX
};

/*
 * CAN statistics
 */
enum cash3_can_statistics {
  CASH3_CAN_STATS_PIO_RX = 0,
  CASH3_CAN_STATS_PIO_TX,
  CASH3_CAN_STATS_PIO_TX_HI,
  CASH3_CAN_STATS_USR_RX_WAIT,
  CASH3_CAN_STATS_USR_RX_WAIT_INT,
  CASH3_CAN_STATS_USR_RX_TIMEOUT,
  CASH3_CAN_STATS_USR_RX_PARTIAL,
  CASH3_CAN_STATS_BUS_OFF,
  CASH3_CAN_STATS_STATUS_ERR,
  CASH3_CAN_STATS_RX_IP_FIFO_OVF,
  CASH3_CAN_STATS_RX_USR_FIFO_OVF,
  CASH3_CAN_STATS_TX_TIMEOUT,
  CASH3_CAN_STATS_TX_LP_FIFO_FULL,
  CASH3_CAN_STATS_RX_USR_FIFO_FULL,
  CASH3_CAN_STATS_TX_HP_FIFO_FULL,
  CASH3_CAN_STATS_CRC_ERR,
  CASH3_CAN_STATS_FRAME_ERR,
  CASH3_CAN_STATS_STUFF_ERR,
  CASH3_CAN_STATS_BIT_ERR,
  CASH3_CAN_STATS_ACK_ERR,
  CASH3_CAN_STATS_MAX
};

#define CASH3_CAN_LABEL_LEN 64

/*
 * CAN statistics
 */
static const char cash3_can_stats_label[CASH3_CAN_STATS_MAX][CASH3_CAN_LABEL_LEN] = {
    "PIO Rx",
    "PIO Tx",
    "PIO Tx Hi",
    "User Rx wait",
    "User Rx wait intr",
    "User Rx timeout",
    "User Rx partial",
    "Bus off",
    "Status error",
    "Rx IP fifo overflow",
    "Rx user fifo overflow",
    "Tx timeout",
    "Tx LP fifo full",
    "Rx user fifo full",
    "Tx HP fifo full",
    "CRC error",
    "Frame error",
    "Stuff error",
    "Bit error",
    "Ack error"
};

/*
 * CAN stats
 */
struct cash3_can_stats {
  struct {
    unsigned int devnum;
    unsigned int stats[CASH3_CAN_STATS_MAX];
  } chs[CASH3_CAN_MAX + 1];
} __attribute__((packed));

/**
 * @struct - cash3_host_cmd_can_filter_t
 * @brief	 Host sends this command to set acceptance filters
 * struct can_filter {
 *      canid_t can_id;
 *      canid_t can_mask;
 * };
 */
struct cash3_host_cmd_can_filter_t {
  uint32_t  filterId;         // 0-3
  struct    can_filter filter;
} __attribute__((packed));

/**
 * @struct - cash3_host_cmd_trigger_dummy_event
 * @brief	 Host sends this command to ask FPGA to send back a dummy event
 */
struct cash3_host_cmd_can_cmd_test_t {
  uint64_t backReference; // this value is going to come back in the dummy event
  uint64_t ticks[2];  // this value is used by FPGA internally
} __attribute__((packed));

/*
 * CAN host cmd payload
 */
struct cash3_host_cmd_can_payload {
  enum cash3_can_cmd can_subcmd : 8; // what can subcmd
  uint32_t rsvd : 24;

  union {
    uint32_t flush_flag; //For start cmd it is flag to flush tx/rx FIFO
    uint32_t val;
    struct cash3_can_msgs data;
    struct cash3_host_cmd_can_cmd_test_t test;
    struct cash3_host_cmd_can_filter_t filter;
  };
} __attribute__((packed));

/*
 * CAN debug reg
 */
struct cash3_can_dbg_reg {
  int reg;
  char reg_desc[CASH3_CAN_LABEL_LEN];
} __attribute__((packed));

/*
 * CAN debug reg
 */
enum cash3_can_dbg_regs {
  CAN_IP_SRR,
  CAN_IP_MSR,
  CAN_IP_BRPR,
  CAN_IP_BTR,
  CAN_IP_ECR,
  CAN_IP_ESR,
  CAN_IP_SR,
  CAN_IP_ISR,
  CAN_IP_IER,
  CAN_IP_AFR,
  CAN_IP_DBG_REG_MAX
};

struct cash3_can_net_stats {
  unsigned int tx_bytes;
  unsigned int tx_packets;

  unsigned int tx_errors;
  unsigned int tx_ack_errors;

  unsigned int tx_bit_errors;
  unsigned int tx_fifo_full_errors;

  unsigned int tx_hpb_fifo_full_errors;
  unsigned int rx_bytes;

  unsigned int rx_packets;
  unsigned int rx_errors;

  unsigned int rx_oflw_errors;
  unsigned int rx_stuff_errors;

  unsigned int rx_form_errors;
  unsigned int rx_crc_errors;

  unsigned int qdma_no_freebuf;
  unsigned int rx_ringbuf_overflow;

  unsigned int dma_out_msgs;
  unsigned int reserved;
};

/*
 * CAN debug reg
 */
struct cash3_can_dbg_reg_dump {
  unsigned int reg_val[CAN_IP_DBG_REG_MAX];
} __attribute__((packed));

/*
 * CAN device statistics
 */
struct cash3_can_dev_stats {
  __u32 bus_error;        /* Bus errors */
  __u32 error_warning;    /* Changes to error warning state */
  __u32 error_passive;    /* Changes to error passive state */
  __u32 bus_off;          /* Changes to bus off state */
  __u32 arbitration_lost; /* Arbitration lost errors */
  __u32 restarts;         /* CAN controller re-starts */
};

struct cash3_error_report {
  uint32_t state;
  uint32_t reserved;
  struct cash3_can_dev_stats can_stats;
  struct cash3_can_net_stats net_stats;
  struct cash3_can_dbg_reg_dump dbg_regs;
};

/* event direction: host <- fpga */
/*
 * CAN fpga event payload
 */
struct cash3_fpga_event_can_payload {
  enum cash3_can_cmd can_subcmd : 8; // what can subcmd
  uint32_t rsvd : 24;
  uint32_t rsvd2;

  union {
    uint32_t val;
    struct cash3_can_msgs data;
    struct cash3_host_cmd_can_cmd_test_t test;
    struct cash3_can_dev_stats can_stats;
    struct cash3_can_status_err status;
    struct cash3_can_dbg_reg_dump dbg_regs;
    struct cash3_can_net_stats net_stats;
    struct cash3_error_report report;
  };
} __attribute__((packed));

struct cash3_can_cmd_status {
  uint32_t tx_result : 8;
  uint32_t can_state : 3;
  uint32_t rx_oflw:    1;
  uint32_t tx_full:    1;
  uint32_t txb_full:   1;
  uint32_t num_msg_sent: 6;
  uint32_t reserved: 12;
} __attribute__((packed));

#endif
