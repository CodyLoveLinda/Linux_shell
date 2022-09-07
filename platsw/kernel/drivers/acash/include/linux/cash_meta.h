/*
 * This file contains data structure reported from ARM
 *
 */

#ifndef __CASH_META_H__
#define __CASH_META_H__
#include <linux/types.h>

/*
 * Use one of the scratch register to pass debug info from host
   #define CASH_SCRATCH_REG_DEBUG_FLAG_ADDR (CASH_SCRATCH_REG_START_ADDR + 63 * 4)
   bit 31-28 as channel number
   bit 27-26 as pre-fetch number, 1 to 4
   bit 2: 1 to change prefecth frames, 1 to 4, 0 no pre-fetch
   bit 1: 1 to restart MIPI
   bit 0: Set to 1 to log metadata
 */
#define DBG_SET_META_BUF         0x00000001
#define DBG_RESTART_MIPI         0x00000002
#define PREFETCH_ENABLE          0x00000004
#define DBG_PREFETCH_FRAMES      26

/**
 * @enum - cash_meta_state_en
 * @brief state field in metadata
 */
enum cash_meta_state_en {
    CASH_META_NORMAL_FRAME = 1,
    CASH_META_DUMMY_FRAME  = 2,
};

struct cash_video_meta_data_t {
    uint32_t mipi_id;
    uint32_t mipi_timestamp:17;
    uint32_t mipi_frame_idx: 8;
    uint32_t buffer_idx: 2;
    uint32_t state:5;
    uint32_t resp;
    uint32_t req;
    uint32_t curr_ts_sec;
    uint32_t curr_ts_us;
    uint32_t prev_ts_sec;
    uint32_t prev_ts_us;
    uint32_t csi_csr;
    uint32_t csi_isr;
    uint32_t crc_video;
    uint32_t reserved[4];
    uint32_t frame_index;
} __attribute__((packed));

#endif
