/*
 * This file captures the basic CASH3 FPGA registers
 *
 */

#ifndef __CASH3_CONF_H__
#define __CASH3_CONF_H__
/* FPGA buffers address */
#define CASH3_VIDEO_BUFFER_DEPTH  	    4

#define CASH3_VIDEO_BUFFER_SIZE_1_RGB_2MP 	(6  * 1024 * 1024) // Video RGB_2MP, 1920x1080x3=6220800, 6MB
#define CASH3_VIDEO_BUFFER_SIZE_2_YUYV_2MP 	(4  * 1024 * 1024) // Video YUYV_2MP, 1920x1080x2=4147200, 4MB
#define CASH3_VIDEO_BUFFER_SIZE_3_RGB_8MP 	(24 * 1024 * 1024) // Video RGB_8MP, 3840x2160x3=24883200, 24MB
#define CASH3_VIDEO_BUFFER_SIZE_4_YUYV_8MP 	(16 * 1024 * 1024) // Video YUYV_8MP, 3840x2160x2=16588800, 16MB
#define CASH3_VIDEO_BUFFER_SIZE_5_RAW_8MP 	(12 * 1024 * 1024) // Video RAW_8MP, 3840x2160x1.5=12441600, 12MB
#define CASH3_VIDEO_BUFFER_SIZE_6_UYVY_2MP 	(4  * 1024 * 1024) // Video UYVY_2MP, 1920x1080x2=4147200, 4MB
#define CASH3_VIDEO_BUFFER_SIZE_7_UYVY_8MP 	(16 * 1024 * 1024) // Video UYVY_8MP, 3840x2160x2=16588800, 16MB

#define CASH3_VIDEO_QUEUE_COUNT_MAX  	    16
#define CASH3_FPGA_VIDEO_BUF_ADDR       0x880000000  // Video streams buffer
#define CASH3_FPGA_EVENT_BUF_ADDR       0x860000000  // Event buffer (4 bufs:4KB size, 6MB gap)
#define CASH3_FPGA_EVENT_BUF_SIZE       (4 * 1024)
#define CASH3_FPGA_EVENT_BUF_STRIDE     CASH3_VIDEO_BUFFER_SIZE_1_RGB_2MP

#define CASH3_FPGA_CAN_BUF_ADDR         0x862000000  // CAN buffer (6 cmd bufs and 6*24 event bufs)
#define CASH3_FPGA_CAN_BUF_SIZE         0x00A00000   // CAN buffer (6 cmd bufs and 6*24 event bufs)

#define CASH3_FPGA_CANRX_EXT_BADDR      0x810000000  // Extended buf, used as ring buffer with struct cash3_cmsg
#define CASH3_FPGA_CANRX_EXT_BSIZE      (1 * 1024 * 1024)
#define CASH3_FPGA_CANRX_EXT_MAXLEN     256          // must be 2^n

#define CASH3_FPGA_IOR_BUF_ADDR         0x870000000  // SW update buffer
#define CASH3_FPGA_SWUPD_BUF_ADDR       0x870000000  // SW update buffer

#define CASH3_CAN_MAX                   6

#define CASH3_FPGA_CAN_TX_BUF_ADDR      CASH3_FPGA_CAN_BUF_ADDR
#define CASH3_FPGA_CAN_TX_QUEUE_COUNT   CASH3_CAN_MAX
#define CASH3_FPGA_CAN_TX_BUFFER_SIZE   (1024 * 1024)


#define CASH3_FPGA_CAN_RX_BUF_ADDR      (CASH3_FPGA_CAN_BUF_ADDR + 0x00A00000)
#define CASH3_FPGA_CAN_RX_QUEUE_COUNT   CASH3_CAN_MAX
#define CASH3_FPGA_CAN_RX_BUF_DEPTH     4
#define CASH3_FPGA_CAN_RX_BUFFER_SIZE   (4 * 1024)
#define CASH3_FPGA_CAN_RX_QUEUE_SIZE    (CASH3_FPGA_CAN_RX_BUFFER_SIZE \
                                       * CASH3_FPGA_CAN_RX_BUF_DEPTH)

#define CASH3_CAN_BULK_MSG_MAX          64 /* FIXME JIRA XPU-216 */
#define CASH3_CAN_DEV_FIRST_16CAM       17
#define CASH3_CAN_DEV_FIRST_8CAM        9

#define MAXQ                             40
#define QDMA_C2H_DEFAULT_BUF_SZ          (4096)
#define XNL_RESP_BUFLEN_MIN              256
#define XNL_ERR_BUFLEN                   64

#define CASH_DMA_QUEUE_COUNT_C8             16
#define CASH_DMA_VIDEO_QUEUE_COUNT_C8       8
#define CASH_DMA_VIDEO_QUEUE_BEGIN_C8       0
#define CASH_DMA_VIDEO_QUEUE_END_C8         7
#define CASH_DMA_EVENT_COUNT_C8             1
#define CASH_DMA_EVENT_QUEUE_BEGIN_C8       8
#define CASH_DMA_EVENT_QUEUE_END_C8         8
#define CASH_DMA_CAN_QUEUE_COUNT_C8         6
#define CASH_DMA_CAN_QUEUE_BEGIN_C8         9
#define CASH_DMA_CAN_QUEUE_END_C8           14
#define CASH_DMA_TEST_QUEUE_COUNT_C8        1
#define CASH_DMA_TEST_QUEUE_BEGIN_C8        16
#define CASH_DMA_TEST_QUEUE_END_C8          16

#define CASH_DMA_QUEUE_COUNT_C16            24
#define CASH_DMA_VIDEO_QUEUE_COUNT_C16      16
#define CASH_DMA_VIDEO_QUEUE_BEGIN_C16      0
#define CASH_DMA_VIDEO_QUEUE_END_C16        15
#define CASH_DMA_EVENT_COUNT_C16            1
#define CASH_DMA_EVENT_QUEUE_BEGIN_C16      16
#define CASH_DMA_EVENT_QUEUE_END_C16        16
#define CASH_DMA_CAN_QUEUE_COUNT_C16        6
#define CASH_DMA_CAN_QUEUE_BEGIN_C16        17
#define CASH_DMA_CAN_QUEUE_END_C16          22
#define CASH_DMA_TEST_QUEUE_COUNT_C16       1
#define CASH_DMA_TEST_QUEUE_BEGIN_C16       24
#define CASH_DMA_TEST_QUEUE_END_C16         24

#ifdef __cplusplus
uint32_t const cash3_qdma_buffer_sizes[] = {
    0,
    CASH3_VIDEO_BUFFER_SIZE_1_RGB_2MP,  // Video RGB_2MP, 1920x1080x3=6220800, 6MB
    CASH3_VIDEO_BUFFER_SIZE_2_YUYV_2MP, // Video YUYV_2MP, 1920x1080x2=4147200, 4MB
    CASH3_VIDEO_BUFFER_SIZE_3_RGB_8MP,  // Video RGB_8MP, 3840x2160x3=24883200, 24MB
    CASH3_VIDEO_BUFFER_SIZE_4_YUYV_8MP, // Video YUYV_8MP, 3840x2160x2=16588800, 16MB
    CASH3_VIDEO_BUFFER_SIZE_5_RAW_8MP,  // Video RAW_8MP, 3840x2160x1.5=12441600, 12MB
    CASH3_VIDEO_BUFFER_SIZE_6_UYVY_2MP, // Video UYVY_2MP, 1920x1080x2=4147200, 4MB
    CASH3_VIDEO_BUFFER_SIZE_7_UYVY_8MP  // Video UYVY_8MP, 3840x2160x2=16588800, 16MB
};
#endif

/* cmd direction: host -> fpga  */

#define CASH3_FPGA_CAN_TX_QUEUE_COUNT                   CASH3_CAN_MAX
#define CASH3_FPGA_CAN_TX_BUFFER_SIZE                   (1024 * 1024)

#endif
