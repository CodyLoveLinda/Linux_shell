/*
 * Copyright (C) 2020 AutoX, Inc.
 */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#include "axcan.h"
#include "axcan_util.h"

pthread_t axcan_rx_thr[AXCAN_MAX_DEV];
volatile int axcan_rx_running[AXCAN_MAX_DEV] = {0};

const char *axcan_get_libversion(void)
{
    return LIB_VER;
}

int axcan_open(uint32_t dev_index, uint32_t flags,
           uint64_t tx_to, uint64_t rx_to, axcan_hdl_t *hdl)
{
    axcan_ihdl_t *ihdl;

    if (dev_index >= AXCAN_MAX_DEV) {
        AXCAN_LOG_ERR("can device %d out of range: [0:%d]\n", dev_index,
                       AXCAN_MAX_DEV);
        return AXCAN_PARAM_INVALID;
    }

    ihdl = (axcan_ihdl_t *)calloc(sizeof(axcan_ihdl_t), 1);

    if (!ihdl) {
        AXCAN_LOG_ERR("malloc() failure: %s\n", strerror(errno));
        return AXCAN_NO_BUFFERS;
    }

    *hdl = (uintptr_t)(const void *)ihdl;

    ihdl->dev_index = dev_index;

    ihdl->dev_state = AXCAN_DEV_OPEN;

    ihdl->tx_to = tx_to;
    ihdl->rx_to = rx_to;

    ihdl->tx_qsize = AXCAN_MAX_TX_MSG;
    ihdl->rx_qsize = AXCAN_MAX_RX_MSG;

    (void) flags;
 
    snprintf(ihdl->dev_path, sizeof(ihdl->dev_path), "/dev/%s%u",
             AXCAN_DEV_NAME, dev_index);

    if ((ihdl->fd = open(ihdl->dev_path, O_RDWR | O_NONBLOCK)) < 0) {
        AXCAN_LOG_ERR("%s open() failure: %s\n", ihdl->dev_path,
                      strerror(errno));
        return AXCAN_DEV_ERR;
    }

    axcan_reset(*hdl);

    axcan_rx_running[ihdl->dev_index] = 1;

    if (pthread_create(&axcan_rx_thr[dev_index], NULL, axcan_rx_func, ihdl)) {
        AXCAN_LOG_ERR("axcan_rx_thr create failed\n");
    return AXCAN_ERR;
    }

    AXCAN_LOG_DBG("%s open() success. fd %d\n", ihdl->dev_path, ihdl->fd);

    return AXCAN_OK;
}

int axcan_close(axcan_hdl_t hdl)
{
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;

    if ((axcan_check_state(ihdl, AXCAN_DEV_CLOSE) != AXCAN_OK)) {
        return AXCAN_PARAM_INVALID;
    }

    AXCAN_LOG_DBG("%s: dev_index %d, dev_state %d, fd %d\n", __func__,
                  ihdl->dev_index, ihdl->dev_state, ihdl->fd);

    if (axcan_qdma_send(ihdl, CASH3_CAN_CMD_DEV_STOP, NULL) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_DEV_STOP failed\n",
                      __func__, ihdl->dev_index);
    }

    axcan_rx_running[ihdl->dev_index] = 0;

    if (axcan_qdma_send(ihdl, CASH3_CAN_CMD_TEST, NULL) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_TEST failed\n",
                      __func__, ihdl->dev_index);
    }

    pthread_join(axcan_rx_thr[ihdl->dev_index], NULL);

    close(ihdl->fd);

    ihdl->dev_state = AXCAN_DEV_CLOSE;

    free(ihdl);

    return AXCAN_OK;
}

int axcan_set_baudrate(axcan_hdl_t hdl, enum axcan_baudrate_t curr_rate)
{
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;
    int ret = AXCAN_OK;

    if ((ret = axcan_check_state(ihdl, AXCAN_DEV_BAUD_SET)) != AXCAN_OK) {
        return AXCAN_PARAM_INVALID;
    }

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_BAUDRATE_SET, &curr_rate))
                    != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_BAUDRATE_SET %d "
                      "failed\n", __func__, ihdl->dev_index, curr_rate);
        return ret;
    }

    ihdl->dev_state |= AXCAN_DEV_BAUD_SET;

    ihdl->baudrate = curr_rate;

    return AXCAN_OK;
}

int axcan_get_baudrate(axcan_hdl_t hdl, enum axcan_baudrate_t *rate)
{
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;
    int ret = AXCAN_OK;

    if ((ret = axcan_check_state(ihdl, AXCAN_DEV_BAUD_SET)) != AXCAN_OK) {
        return AXCAN_PARAM_INVALID;
    }

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_BAUDRATE_GET, rate)) !=
                     AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_BAUDRATE_GET %d "
                      "failed\n", __func__, ihdl->dev_index, *rate);
        return ret;
    }

    if (ihdl->baudrate != *rate) {
        AXCAN_LOG_ERR("%s: dev_index %d, baudrate %d and cached rate %d "
                      "disagree\n", __func__, ihdl->dev_index, *rate, 
                      ihdl->baudrate);
        return AXCAN_DEV_ERR;
    }

    return AXCAN_OK;
}

int axcan_set_loopback(axcan_hdl_t hdl)
{
    int ret = AXCAN_OK;
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;

    if ((ret = axcan_check_state(ihdl, AXCAN_DEV_LOOPBACK)) != AXCAN_OK) {
        return AXCAN_HDL_INVALID;
    }

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_LOOPBACK_SET, NULL)) !=
                   AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_LOOPBACK_SET "
                      "failed\n", __func__, ihdl->dev_index);
        return ret;
    }

    ihdl->dev_state &= ~AXCAN_DEV_NORMAL;

    ihdl->dev_state |= AXCAN_DEV_LOOPBACK;

    return AXCAN_OK;
}

int axcan_unset_loopback(axcan_hdl_t hdl)
{
    int ret = AXCAN_OK;
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;

    if ((ret = axcan_check_state(ihdl, -1)) != AXCAN_OK) {
        return AXCAN_PARAM_INVALID;
    }


    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_LOOPBACK_UNSET, NULL)) !=
                   AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_LOOPBACK_UNSET "
                      "failed\n", __func__, ihdl->dev_index);
        return ret;
    }

    ihdl->dev_state &= ~AXCAN_DEV_LOOPBACK;

    ihdl->dev_state |= AXCAN_DEV_NORMAL;

    return AXCAN_OK;
}

int axcan_start(axcan_hdl_t hdl)
{
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;
    int ret = AXCAN_OK;

    if ((ret = axcan_check_state(ihdl, AXCAN_DEV_START)) != AXCAN_OK) {
        return AXCAN_PARAM_INVALID;
    }

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_DEV_START, NULL)) !=
                     AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_DEV_START "
                      "failed\n", __func__, ihdl->dev_index);
        return ret;
    }

    ihdl->dev_state |= AXCAN_DEV_START;

    return AXCAN_OK;
}

int axcan_stop(axcan_hdl_t hdl)
{
    int ret = AXCAN_OK;
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;

    if ((ret = axcan_check_state(ihdl, AXCAN_DEV_STOP)) != AXCAN_OK) {
        return AXCAN_PARAM_INVALID;
    }

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_DEV_STOP, NULL)) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_DEV_STOP failed\n",
                      __func__, ihdl->dev_index);
        return ret;
    }

    ihdl->dev_state |= AXCAN_DEV_STOP;

    ihdl->dev_state &= ~AXCAN_DEV_START;

    return AXCAN_OK;
}

int axcan_reset(axcan_hdl_t hdl)
{
    int ret = AXCAN_OK;
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_DEV_RESET, NULL)) !=
                   AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_DEV_RESET "
                      "failed\n", __func__, ihdl->dev_index);
        return ret;
    }

    return AXCAN_OK;
}

int axcan_id_add(axcan_hdl_t hdl, uint32_t id)
{
    int ret = AXCAN_OK;
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_ADD_FILTER, &id)) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_ID_ADD failed\n",
                      __func__, ihdl->dev_index);
        return ret;
    }

    return AXCAN_OK;
}

int axcan_id_del(axcan_hdl_t hdl, uint32_t id)
{
    int ret = AXCAN_OK;
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_DEL_FILTER, &id)) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_ID_DEL failed\n",
                      __func__, ihdl->dev_index);
        return ret;
    }

    return AXCAN_OK;
}

int axcan_test(axcan_hdl_t hdl)
{
    int ret = AXCAN_OK;
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;

    for (int count = 0; count < AXCAN_TEST_CMD_MAX; count++) {

        printf("sending CAN_CMD_TEST %d time \n", count); 

        if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_TEST, NULL)) != AXCAN_OK) {
            AXCAN_LOG_ERR("%s: dev_index %d, write CASH3_CAN_CMD_TEST failed\n",
                           __func__, ihdl->dev_index);
            return ret;
        }
    }

    return AXCAN_OK;
}

int axcan_get_stats(axcan_hdl_t hdl)
{
    int ret = AXCAN_OK;
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;
    struct cash3_can_net_stats *net_stats = &ihdl->net_stats;
    struct cash3_can_dev_stats *can_stats = &ihdl->can_stats;

    if (axcan_check_state(ihdl, AXCAN_DEV_ACTIVE) != AXCAN_OK) {
        return AXCAN_DEV_ERR;
    }

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_GET_CAN_STATS, NULL)) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_GET_CAN_STATS "
                "failed\n", __func__, ihdl->dev_index);
        return ret;
    }

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_GET_NET_STATS, NULL)) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_GET_NET_STATS "
                "failed\n", __func__, ihdl->dev_index);
        return ret;
    }

    printf("%s %s bus_error=%d error_warning=%d error_passive=%d bus_off=%d "\
           "arbitration_lost=%d restarts=%d\n",
           __func__, ihdl->dev_path, can_stats->bus_error,
           can_stats->error_warning, can_stats->error_passive,
           can_stats->bus_off, can_stats->arbitration_lost,
           can_stats->restarts);

    printf("%s %s tx_bytes=%d tx_packets=%d tx_errors=%d tx_ack_errors=%d "\
           "tx_bit_errors=%d tx_fifo_full_errors=%d tx_hpb_fifo_full_errors=%d "\
           "rx_bytes=%d rx_packets=%d rx_errors=%d rx_oflw_errors=%d"\
           "rx_stuff_errors=%d rx_form_errors=%d rx_crc_errors=%d\n",
           __func__, ihdl->dev_path, net_stats->tx_bytes,
           net_stats->tx_packets, net_stats->tx_errors,
           net_stats->tx_ack_errors, net_stats->tx_bit_errors,
           net_stats->tx_fifo_full_errors, net_stats->tx_hpb_fifo_full_errors,
           net_stats->rx_bytes, net_stats->rx_packets, net_stats->rx_errors,
           net_stats->rx_oflw_errors, net_stats->rx_stuff_errors,
           net_stats->rx_form_errors, net_stats->rx_crc_errors);

    return AXCAN_OK;
}

int axcan_clear_stats(axcan_hdl_t hdl)
{
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;

    if (axcan_check_state(ihdl, AXCAN_DEV_ACTIVE) != AXCAN_OK) {
        return AXCAN_DEV_ERR;
    }

    if (axcan_qdma_send(ihdl, CASH3_CAN_CMD_CLEAR_STATS, NULL) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_CLEAR_STATS "
                "failed\n", __func__, ihdl->dev_index);
        return AXCAN_ERR;
    }

    return AXCAN_OK;
}

int axcan_recv(axcan_hdl_t hdl, axcan_msg_t *buf, int32_t num_msg)
{
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;
    uint32_t num_recv = 0;

    if (axcan_check_state(ihdl, AXCAN_DEV_ACTIVE) != AXCAN_OK) {
        return AXCAN_DEV_ERR;
    }

    pthread_mutex_lock(&rxq_lock[ihdl->dev_index]);

    for (int i = 0; i < num_msg; i++, num_recv++) {
        if (axcan_rxq_pop_front(&rxq[ihdl->dev_index], &buf[i]))
            break;
    }

    pthread_mutex_unlock(&rxq_lock[ihdl->dev_index]);

    return num_recv;
}

int axcan_send(axcan_hdl_t hdl, axcan_msg_t *buf, int32_t num_msg)
{
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;
    struct cash3_can_msgs bmsgs = {0};

    if (axcan_check_state(ihdl, AXCAN_DEV_ACTIVE) != AXCAN_OK) {
        return AXCAN_PARAM_INVALID;
    }

    memset(&bmsgs.msgs, 0, num_msg * sizeof(axcan_msg_t));
    memcpy(&bmsgs.msgs, buf, num_msg * sizeof(axcan_msg_t));

    bmsgs.num_msg_tx = num_msg;

    if (axcan_qdma_send(ihdl, CASH3_CAN_CMD_SEND, &bmsgs) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_SEND "
                "failed\n", __func__, ihdl->dev_index);
        return AXCAN_ERR;
    }

    return bmsgs.num_msg_tx;
}

int axcan_send_hi_pri(axcan_hdl_t hdl, axcan_msg_t *buf)
{
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;
    struct cash3_can_msgs bmsgs = {0};

    if (axcan_check_state(ihdl, AXCAN_DEV_ACTIVE) != AXCAN_OK) {
        return AXCAN_PARAM_INVALID;
    }

    memcpy(&bmsgs.msgs, buf, sizeof(axcan_msg_t));

    bmsgs.num_msg_tx = 1;

    if (axcan_qdma_send(ihdl, CASH3_CAN_CMD_SEND_HIPRI, &bmsgs) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_SEND_HIPRI "
                "failed. \n", __func__, ihdl->dev_index);
        return AXCAN_ERR;
    }
  
    return bmsgs.num_msg_tx;
}

int axcan_dump_debug_regs(axcan_hdl_t hdl)
{
    struct cash3_can_stats stats;
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;

    if (axcan_check_state(ihdl, AXCAN_DEV_ACTIVE) != AXCAN_OK) {
        return AXCAN_HDL_INVALID;
    }

    memset(&stats, 0, sizeof(struct cash3_can_stats));

    if (axcan_qdma_send(ihdl, CASH3_CAN_CMD_DUMP_HW_REGS, &stats) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_DUMP_HW_REGS "
                "failed\n", __func__, ihdl->dev_index);
        return AXCAN_ERR;
    }

    return AXCAN_OK;
}

int axcan_get_state(axcan_hdl_t hdl)
{
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;
    struct cash3_can_cmd_status *s = &ihdl->state;
    int ret = AXCAN_OK;

    if (axcan_check_state(ihdl, AXCAN_DEV_ACTIVE) != AXCAN_OK) {
        return AXCAN_DEV_ERR;
    }

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_GET_CAN_STATE, NULL)) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_GET_CAN_STATE "
                "failed\n", __func__, ihdl->dev_index);
        return ret;
    }

    printf("%s: dev_index %d, tx_result: %d can_state: %d rx_oflw: %d tx_full: %d "
                  "txb_full: %d num_msg_sent: %d\n", __func__, ihdl->dev_index,
                  s->tx_result, s->can_state, s->rx_oflw, s->tx_full, s->txb_full,
                  s->num_msg_sent);

    return AXCAN_OK;
}

int axcan_get_err_counter(axcan_hdl_t hdl, uint8_t *rx_err, uint8_t *tx_err)
{
    (void) hdl;
    (void) rx_err;
    (void) tx_err;
#ifdef TBD
    struct cash3_can_status_err st_err;
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)hdl;
    int ret = AXCAN_OK;

    if ((ret = axcan_check_state(ihdl, -1)) != AXCAN_OK) {
        return AXCAN_HDL_INVALID;
    }

    if ((ret = axcan_qdma_send(ihdl, CASH3_CAN_CMD_GET_STATUS_ERR,
                 &st_err)) != AXCAN_OK) {
        AXCAN_LOG_ERR("%s: dev_index %d, CASH3_CAN_CMD_GET_STATUS_ERR "
                "failed\n", __func__, ihdl->dev_index);
        return ret;
    }

    *tx_err = (st_err.axcan_err_count & 0xFF);
    *rx_err = (st_err.axcan_err_count & 0xFFFF) >> 8;
#endif
    return AXCAN_OK;
}

const char *axcan_get_err_msg(int err_code)
{
    switch (err_code) {
    case AXCAN_PARAM_INVALID:
        return "invalid parameter";
    case AXCAN_HDL_INVALID:
        return "invalid can handle";
    case AXCAN_DEV_INVALID:
        return "invalid can device";
    case AXCAN_DEV_ERR:
        return "can device error";
    case AXCAN_DEV_BUSY:
        return "can device busy";
    case AXCAN_TIMEOUT:
        return "can timeout";
    case AXCAN_FAIL:
        return "can operation failed";
    case AXCAN_NOT_SUPPORTED:
        return "can operation not supported";
    case AXCAN_NOT_IMPLEMENTED:
        return "can operation not implemented";
    case AXCAN_INVALID:
        return "can invalid error";
    case AXCAN_NO_BUFFERS:
        return "failed to allocate can buffers";
    case AXCAN_ERR:
        return "generic can error, check error log";
    case AXCAN_OK:
        return "OK"; /* 0 */
    case AXCAN_PARTIAL_OK:
        return "can IO partial completion";
    default:
        return "undefined error code";
    }
}
