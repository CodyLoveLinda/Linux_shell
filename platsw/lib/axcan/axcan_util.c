#include "axcan_util.h"
#include "axcan.h"

axcan_rxq_t rxq[AXCAN_MAX_DEV];
pthread_mutex_t rxq_lock[AXCAN_MAX_DEV];
pthread_mutex_t debug_lock = PTHREAD_MUTEX_INITIALIZER;
uint64_t rxq_full[AXCAN_MAX_DEV] = {0};
uint64_t unknown_qevent[AXCAN_MAX_DEV] = {0};
uint64_t roflw[AXCAN_MAX_DEV] = {0};
uint64_t rcount[AXCAN_MAX_DEV] = {0};
uint64_t woflw[AXCAN_MAX_DEV] = {0};
uint64_t wcount[AXCAN_MAX_DEV] = {0};
uint16_t last_can_id[AXCAN_MAX_DEV] = {0};
uint16_t expected_can_id[AXCAN_MAX_DEV] = {0};

static int axcan_rx_buf_init(axcan_rxq_t *rxq, size_t capacity, size_t sz)
{
    rxq->buffer = calloc(capacity, sz);
    if (!rxq->buffer) {
        perror("can rxq malloc error");
        return AXCAN_ERR;
    }
    rxq->buffer_end = (char *)rxq->buffer + capacity * sz;
    rxq->capacity = capacity;
    rxq->count = 0;
    rxq->sz = sz;
    rxq->head = rxq->buffer;
    rxq->tail = rxq->buffer;
    return AXCAN_OK;
}

static int axcan_rx_buf_free(axcan_rxq_t *rxq)
{
    if (rxq->buffer)
        free(rxq->buffer);

    return AXCAN_OK;
}

static int axcan_rxq_push_back(axcan_rxq_t *rxq, const void *item)
{
    if (rxq->count == rxq->capacity) {
        return AXCAN_ERR;
    }

    memcpy(rxq->head, item, rxq->sz);
    rxq->head = (char *)rxq->head + rxq->sz;
    if (rxq->head == rxq->buffer_end)
        rxq->head = rxq->buffer;
    rxq->count++;
    return AXCAN_OK;
}

int axcan_rxq_pop_front(axcan_rxq_t *rxq, void *item)
{
    if (rxq->count == 0)
        return AXCAN_ERR;

    memcpy(item, rxq->tail, rxq->sz);
    rxq->tail = (char *)rxq->tail + rxq->sz;
    if (rxq->tail == rxq->buffer_end)
        rxq->tail = rxq->buffer;
    rxq->count--;
    return AXCAN_OK;
}

#ifdef PERF
static struct timespec diff(struct timespec start, struct timespec end)
{
	struct timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1E9+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}
#endif

/*
 * Check if transition from current state to proposed state is valid.
 * Also sanity check for handle.
 */
int axcan_check_state(axcan_ihdl_t *ihdl, int pstate)
{
    int cstate, err = AXCAN_OK;

    if (!ihdl) {
        return (AXCAN_HDL_INVALID);
    }

    cstate = ihdl->dev_state;
    switch (pstate) {
    case AXCAN_DEV_OPEN:
        if (cstate != AXCAN_DEV_UNINIT) {
            err = AXCAN_DEV_INVALID;
        }
        break;

    case AXCAN_DEV_BAUD_SET:
    case AXCAN_DEV_LOOPBACK:
        if (!(cstate & AXCAN_DEV_OPEN)) {
            err = AXCAN_DEV_INVALID;
        }
        break;

    case AXCAN_DEV_START:
        if (!(cstate & AXCAN_DEV_BAUD_SET)) {
            err = AXCAN_DEV_INVALID;
        }
        break;

    case AXCAN_DEV_ACTIVE:
    case AXCAN_DEV_STOP:
        if (!(cstate & AXCAN_DEV_START)) {
            err = AXCAN_DEV_INVALID;
        }
        break;

    case AXCAN_DEV_CLOSE:
        if (!(cstate & AXCAN_DEV_OPEN)) {
            err = AXCAN_DEV_INVALID;
        }
        break;
    default:
        break;
    }

    return (err);
}

int axcan_qdma_send(axcan_ihdl_t *ihdl, enum cash3_can_cmd subcmd, void *p)
{
    struct cash3_host_cmd_t cmd;
    struct cash3_host_cmd_can_payload *cmd_payload;
    struct cash3_can_cmd_status *s;
    struct cash3_can_msgs *bmsgs;
    uint32_t dev_index = ihdl->dev_index;
    uint64_t backreference = 0xdeadbeefbeefdead;
    char buf[CASH3_FPGA_EVENT_SIZE];
    int rc;
#ifdef PERF
    struct timespec start,end;
#endif
#ifdef DEBUG
    struct timespec ts;
#endif

    memset(&cmd, 0, sizeof(cmd));

    memset(&buf, 0, CASH3_FPGA_EVENT_SIZE);

    cmd.head.cmd = CASH3_HOST_CMD_CAN_TRAFFIC;

    cmd_payload = (struct cash3_host_cmd_can_payload *)&cmd.payload;
    cmd_payload->can_subcmd = subcmd;

    switch (subcmd) {
    case CASH3_CAN_CMD_TEST:
        cmd_payload->test.backReference = backreference;
	break;
    case CASH3_CAN_CMD_TX_TIMEOUT_SET:
    case CASH3_CAN_CMD_RX_TIMEOUT_SET:
    case CASH3_CAN_CMD_ADD_FILTER:
    case CASH3_CAN_CMD_DEL_FILTER:
    case CASH3_CAN_CMD_BAUDRATE_SET:
        cmd_payload->val = *(uint64_t *)p;
        break;
    case CASH3_CAN_CMD_SEND:
        bmsgs = (struct cash3_can_msgs *) p;
        if (bmsgs->num_msg_tx > CASH3_CAN_BULK_MSG_MAX) {
            AXCAN_LOG_ERR("%s %s subcmd=%d num_msgs=%d > max allowed=%d\n",
		          __func__, ihdl->dev_path, subcmd, bmsgs->num_msg_tx,
                          CASH3_CAN_BULK_MSG_MAX);
	   return AXCAN_PARAM_INVALID;
        }

        memcpy(&cmd_payload->data, bmsgs, sizeof(struct cash3_can_msgs));


#ifdef DEBUG
        clock_gettime(CLOCK_REALTIME, &ts);
        pthread_mutex_lock(&debug_lock);
#endif

	for (uint32_t i = 0; i < bmsgs->num_msg_tx; i++, wcount[dev_index]++) {
#ifdef PERF
            if (wcount[dev_index] == 1) {
                clock_gettime(CLOCK_REALTIME, &start);
            } else if (wcount[dev_index] == AXCAN_PERF_MSG_CNT) {
                clock_gettime(CLOCK_REALTIME, &end);
                struct timespec d = diff(start, end);
                AXCAN_LOG_ERR("%s can%d tx %ld msgs in (%ld.%06ld) secs."
                              " total tx %ld msgs\n", __func__, dev_index,
                              wcount[dev_index], d.tv_sec, d.tv_nsec / 1000,
                              wcount[dev_index]+woflw[dev_index]*AXCAN_PERF_MSG_CNT);
                woflw[dev_index] += 1;
                wcount[dev_index] = 0;
            }
#endif
#ifdef DEBUG
            AXCAN_LOG_DBG("(%ld.%06ld) can%d write%ld %x#", ts.tv_sec,
			    ts.tv_nsec / 1000, dev_index, wcount[dev_index]+
                            woflw[dev_index]*AXCAN_PERF_MSG_CNT,
			    bmsgs->msgs[i].msg.can_id);

            for (int j = 0; j < bmsgs->msgs[i].msg.can_dlc; j++) {
                AXCAN_LOG_DBG("%02x", bmsgs->msgs[i].msg.data[j]);
            }

            fflush(stdout);
#endif
	}
        break;

    case CASH3_CAN_CMD_GET_CAN_STATE:
    case CASH3_CAN_CMD_GET_NET_STATS:
    case CASH3_CAN_CMD_GET_CAN_STATS:
    case CASH3_CAN_CMD_CLEAR_STATS:
    case CASH3_CAN_CMD_DUMP_HW_REGS:

    default:

        break;
    }

    if ((rc = write(ihdl->fd, &cmd, sizeof(cmd))) == AXCAN_OK) {
#ifdef DEBUG
        AXCAN_LOG_DBG("...success!!\n");
        fflush(stdout);
        pthread_mutex_unlock(&debug_lock);
#endif
        return AXCAN_OK;
    }

    s = (struct cash3_can_cmd_status *) &rc;

    AXCAN_LOG_ERR("%s: write error dev_index %d, subcmd %d failed. "
                  "tx_result: %d can_state: %d rx_oflw: %d tx_full: %d "
                  "txb_full: %d num_msg_sent: %d errno:%s rc:%d\n",
                  __func__, ihdl->dev_index, subcmd, s->tx_result,
                  s->can_state, s->rx_oflw, s->tx_full, s->txb_full,
                  s->num_msg_sent, strerror(errno), rc);
#ifdef DEBUG
   AXCAN_LOG_DBG("...failed!! rc=%d\n", rc);
   fflush(stdout);
   pthread_mutex_unlock(&debug_lock);
#endif

    ihdl->state = *s;

    if (subcmd == CASH3_CAN_CMD_SEND) {
        ((struct cash3_can_msgs *)p)->num_msg_tx = rc;
    }

    return AXCAN_ERR;
}

static void axcan_dump_qdma_msg(uint32_t * from)
{
    (void) from;
    for (int i = 0; i < 16; ++i) {
        if (i%8 == 0) {
            AXCAN_LOG_DBG("\n");
        }
        AXCAN_LOG_DBG("0x%08x ", from[i]);
    }
    AXCAN_LOG_DBG("\n");
}

uint64_t axcan_qdma_recv(axcan_ihdl_t *ihdl, char *buf)
{
    struct cash3_fpga_event_t *event;
    struct cash3_fpga_event_can_payload *event_payload;
    struct cash3_can_msgs *data;
    struct cash3_can_net_stats *net_stats;
    struct cash3_can_dev_stats *can_stats;
    uint32_t i, dev_index = ihdl->dev_index;
    uint64_t backreference = 0xdeadbeefbeefdead;
    uint32_t* from = (uint32_t*)buf;
    int rc;
#ifdef PERF
    struct timespec start,end;
#endif
#ifdef DEBUG
    struct timespec ts;
#endif

    memset(&event, 0, sizeof(event));
    memset(buf, 0xdeadbeef, CASH3_FPGA_EVENT_SIZE);

    if ((rc = read(ihdl->fd, buf, CASH3_FPGA_EVENT_SIZE)) > 0) {

        event = (struct cash3_fpga_event_t *)buf;

        if (event->head.event == CASH3_FPGA_EVENT_CAN_TRAFFIC) {

            event_payload = 
                    (struct cash3_fpga_event_can_payload *) &event->payload;

            if (event_payload->can_subcmd == CASH3_CAN_CMD_TEST) {

                if (event_payload->test.backReference != backreference) {

                    AXCAN_LOG_ERR( "CASH3_CAN_CMD_TEST backReference unexpected "
                            "%ld\n", event_payload->test.backReference);

                } else {

                    AXCAN_LOG_ERR( "CASH3_CAN_CMD_TEST passed\n");

                }

            } else if (event_payload->can_subcmd == CASH3_CAN_CMD_BAUDRATE_GET) {

                AXCAN_LOG_ERR("CASH3_CAN_CMD_BAUDRATE_GET returns %d\n",
                               event_payload->val);

            } else if (event_payload->can_subcmd == CASH3_CAN_CMD_RECV) {

                data = &event_payload->data;

                if (data->num_msg_rx > AXCAN_MAX_BULK_MSG) {
                    AXCAN_LOG_ERR("CASH3_CAN_CMD_RECV garbage num_msg_rx %d\n",
                                   data->num_msg_rx);
                    axcan_dump_qdma_msg(from);
                    return rc;
                }
                else{
#ifdef DEBUG
                    AXCAN_LOG_ERR("CASH3_CAN_CMD_RECV bulk msg recvd %d msgs\n",
                             data->num_msg_rx);
#endif
                }

                pthread_mutex_lock(&rxq_lock[dev_index]);

                for (i = 0; i < data->num_msg_rx; i++, rcount[dev_index]++) {

		    if (data->msgs[i].msg.can_dlc > 8) {
                        AXCAN_LOG_ERR("CASH3_CAN_CMD_RECV garbage can_dlc %d\n",
                             data->msgs[i].msg.can_dlc);
                        axcan_dump_qdma_msg(from);
                        continue;
                    }

                    last_can_id[dev_index] = data->msgs[i].msg.can_id;

#ifdef DEBUG_CANID
                    /* The very first can_id from Kvaser can be random.
                     * initialize expected_can_id to whatever is received.
                     * From then onwards, it must increment by 1
                     */
                    if (expected_can_id[dev_index] == AXCAN_MAX_CAN_ID) {
                        expected_can_id[dev_index] = last_can_id[dev_index];
                    }

                    if (last_can_id[dev_index] != expected_can_id[dev_index]) {
                        AXCAN_LOG_ERR("CASH3_CAN_CMD_RECV invalid can_id "
                            "received %d expected %d\n",
                        last_can_id[dev_index],expected_can_id[dev_index]);
                        axcan_dump_qdma_msg(from);
                        //continue; /* FIXME */
                        pthread_mutex_unlock(&rxq_lock[dev_index]);
			axcan_stop(ihdl); /* FIXME */
			pthread_exit(NULL);
                    }

                    if (++expected_can_id[dev_index] > AXCAN_MAX_CAN_ID) {
                        expected_can_id[dev_index] = 0;
                    }
#endif
                    rc = axcan_rxq_push_back(&rxq[dev_index], &data->msgs[i]);

                    if (rc != AXCAN_OK) {
                        rxq_full[dev_index] += 1;

                        if ((rxq_full[dev_index] == 1) || 
                                    (rxq_full[dev_index] >> 8)) {
                            AXCAN_LOG_ERR("%s can%d rxq_full overflow %ld msgs\n",
                                           __func__, dev_index, 
                                           rxq_full[dev_index]);
                            if (rxq_full[dev_index] != 1)
                                rxq_full[dev_index] = 0;
                        }
                    }

#ifdef PERF
                    if (rcount[dev_index] == 1) {
                        clock_gettime(CLOCK_REALTIME, &start);
                    } else if (rcount[dev_index] == AXCAN_PERF_MSG_CNT) {
                        clock_gettime(CLOCK_REALTIME, &end);
                        struct timespec d = diff(start, end);
                        AXCAN_LOG_ERR("%s can%d rx %ld msgs in (%ld.%06ld) secs."
                                      " total rx %ld msgs\n", __func__, dev_index,
                                       rcount[dev_index], d.tv_sec, d.tv_nsec / 1000,
                                       rcount[dev_index]+roflw[dev_index]*AXCAN_PERF_MSG_CNT);
                        roflw[dev_index] += 1;
                        rcount[dev_index] = 0;
                    }
#endif

#ifdef DEBUG
                    clock_gettime(CLOCK_REALTIME, &ts);

                    pthread_mutex_lock(&debug_lock);
                    AXCAN_LOG_DBG("(%ld.%06ld) can%d read%ld %x#",
                                  ts.tv_sec, ts.tv_nsec / 1000, dev_index,
                                  rcount[dev_index]+roflw[dev_index]*AXCAN_PERF_MSG_CNT,
                                  data->msgs[i].msg.can_id);

                    for (int j = 0; j < data->msgs[i].msg .can_dlc; j++) {
                        AXCAN_LOG_DBG("%02x", data->msgs[i].msg.data[j]);
                    }

                    AXCAN_LOG_DBG("\n");
                    pthread_mutex_unlock(&debug_lock);
#endif
                }

                pthread_mutex_unlock(&rxq_lock[dev_index]);

            } else if (event_payload->can_subcmd == CASH3_CAN_CMD_GET_CAN_STATS) {
                can_stats = &event_payload->can_stats;
                memcpy(&ihdl->can_stats, can_stats, sizeof(struct cash3_can_dev_stats));
                AXCAN_LOG_DBG("%s %s CASH3_CAN_CMD_GET_CAN_STATS bus_error=%d "\
                              "error_warning=%d error_passive=%d bus_off=%d "\
                              "arbitration_lost=%d restarts=%d\n",
                               __func__, ihdl->dev_path, can_stats->bus_error,
                               can_stats->error_warning, can_stats->error_passive,
                               can_stats->bus_off, can_stats->arbitration_lost,
                               can_stats->restarts);
            } else if (event_payload->can_subcmd == CASH3_CAN_CMD_GET_NET_STATS) {
                net_stats = &event_payload->net_stats;
                memcpy(&ihdl->net_stats, net_stats, sizeof(struct cash3_can_net_stats));
                AXCAN_LOG_DBG("%s %s CASH3_CAN_CMD_GET_NET_STATS tx_bytes=%d "\
                              "tx_packets=%d tx_errors=%d tx_ack_errors=%d "\
                              "tx_bit_errors=%d tx_fifo_full_errors=%d "
                              "tx_hpb_fifo_full_errors=%d " \
                              "rx_bytes=%d rx_packets=%d rx_errors=%d rx_oflw_errors=%d"\
                              "rx_stuff_errors=%d rx_form_errors=%d rx_crc_errors=%d",
                               __func__, ihdl->dev_path, net_stats->tx_bytes,
                               net_stats->tx_packets, net_stats->tx_errors,
                               net_stats->tx_ack_errors, net_stats->tx_bit_errors,
                               net_stats->tx_fifo_full_errors, net_stats->tx_hpb_fifo_full_errors,
                               net_stats->rx_bytes, net_stats->rx_packets, net_stats->rx_errors,
                               net_stats->rx_oflw_errors, net_stats->rx_stuff_errors,
                               net_stats->rx_form_errors, net_stats->rx_crc_errors);
            } else {
                AXCAN_LOG_DBG("%s unsupported CAN_TRAFFIC subcmd = %d\n", __func__, 
                              event_payload->can_subcmd);
            }

        } else {

            unknown_qevent[dev_index] += 1;

            AXCAN_LOG_DBG("%s %s unknown fpga event=%d rc=0x%x errno=%s\n",
                           __func__, ihdl->dev_path, event->head.event, 
                           rc, strerror(errno));
            axcan_dump_qdma_msg(from);
       }
    } else {

        AXCAN_LOG_DBG("%s %s qdma read rc=%d\n", __func__, ihdl->dev_path, rc);
    }

    return rc;
}

void *axcan_rx_func(void *arg)
{
    axcan_ihdl_t *ihdl = (axcan_ihdl_t *)arg;
    uint32_t dev_index = ihdl->dev_index;
    char *buf;

    axcan_rx_buf_init(&rxq[dev_index], ihdl->rx_qsize, sizeof(axcan_msg_t));

    buf = calloc(CASH3_FPGA_EVENT_SIZE, sizeof(char));

    expected_can_id[dev_index] = AXCAN_MAX_CAN_ID;

    pthread_mutex_init(&rxq_lock[dev_index], NULL);

    while (axcan_rx_running[dev_index]) {
        axcan_qdma_recv(ihdl, buf);
    }

    AXCAN_LOG_ERR("stopping axcan_rx_func thread %d total rx_count %ld "
                  "total tx_count %ld last_can_id %d\n", dev_index,
                  roflw[dev_index]*AXCAN_PERF_MSG_CNT + rcount[dev_index],
                  woflw[dev_index]*AXCAN_PERF_MSG_CNT + wcount[dev_index],
                  last_can_id[dev_index]);

    pthread_mutex_destroy(&rxq_lock[dev_index]);

    axcan_rx_buf_free(&rxq[dev_index]);

    free(buf);

    pthread_exit(NULL);
}

int axcan_trigger_dummy(uint32_t dev_index)
{
    char dev_path[AXCAN_DEV_NAME_LEN];
    struct cash3_host_cmd_t cmd;
    struct cash3_host_cmd_trigger_dummy_event_t *cmd_payload;
    struct cash3_fpga_event_t *event;
    struct cash3_fpga_event_dummy_t *event_payload;
    char buf[CASH3_FPGA_EVENT_SIZE] = { 0 };
    int fpga_fd, rc;
    size_t count = 10;

    if (dev_index >= AXCAN_MAX_DEV) {
        AXCAN_LOG_ERR("can device %d out of range: [0:%d]\n", dev_index,
                       AXCAN_MAX_DEV);
        return AXCAN_PARAM_INVALID;
    }

    snprintf(dev_path, sizeof(dev_path), "/dev/%s%u", AXCAN_DEV_NAME,
         dev_index);

    AXCAN_LOG_DBG("sending trigger dummy to %s \n", dev_path); 

    if ((fpga_fd = open(dev_path, O_RDWR | O_NONBLOCK)) == -1) {
        AXCAN_LOG_ERR("%s open() failure: %s\n", dev_path, strerror(errno));
        return AXCAN_PARAM_INVALID;
    }

    AXCAN_LOG_DBG("%s open() success. fd %d\n", dev_path, fpga_fd);

    memset(&cmd, 0, sizeof(cmd));

    cmd.head.cmd = CASH3_HOST_CMD_TRIGGER_DUMMY_EVENT;

    cmd_payload = (struct cash3_host_cmd_trigger_dummy_event_t *)&cmd.payload;
    cmd_payload->backReference = 0xdeadbeefbeefdead;

    while (count--) {
        cmd_payload->backReference += 1;

        if (0 != write(fpga_fd, &cmd, sizeof(cmd))) {
            AXCAN_LOG_ERR("IO error: %s\n", strerror(errno));
            rc = AXCAN_DEV_ERR;
            goto err;
        }

        AXCAN_LOG_DBG("write success %s \n", dev_path);

        rc = read(fpga_fd, buf, sizeof(buf));

        if (rc > 0) {
            event = (struct cash3_fpga_event_t *)buf;

            AXCAN_LOG_DBG("read success %s \n", dev_path);

            switch (event->head.event) {
            case CASH3_FPGA_EVENT_DUMMY:
                AXCAN_LOG_DBG("CASH3_FPGA_EVENT_DUMMY received\n\n");
                event_payload = (struct cash3_fpga_event_dummy_t *)&event->payload;
                if (event_payload->backReference != cmd_payload->backReference) {
                    AXCAN_LOG_ERR( "CASH3_FPGA_EVENT_DUMMY backReference unexpected "
                        "%ld\n", event_payload->backReference);
                }
                rc = AXCAN_OK;
            break;

            default:
                AXCAN_LOG_DBG("unexpected FPGA Event %d received\n\n", event->head.event);
                rc = AXCAN_OK;
            }
        } else {
            AXCAN_LOG_ERR("FPGA Event IO error: %s\n", strerror(errno));
            rc = AXCAN_DEV_ERR;
        }
    }

err:
    close(fpga_fd);

    return rc;
}
