/*
 * This file captures the CASH3 host commands to FPGA, the communicaion protocle is
 * documented in cash3_scratch_registers.h
 *
 * To keep released version backward compatible with older released version, follow
 * this guideline when changing released commands struct:
 *
 * mark deleted fields as reserved and always add new fields at the end and reduce
 * the reserved field size at the end to keep the cmd struct size unchanged (64B)
 *
 * 32bit value needs to land at word boundary, and 16but valuye on 2 byte boundary
 *
 *
 */
#ifndef __CASH3_HOST_CMDS_H__
#define __CASH3_HOST_CMDS_H__
#include "cash3_conf.h"
#ifndef __KERNEL__
#include <stdint.h>
#else
#include <linux/types.h>
#endif
#define CASH3_HOST_CMD_QDMA_QUEUE_ID_C8  8
#define CASH3_HOST_CMD_QDMA_QUEUE_ID_C16 16

/**
 * @enum - cash3_host_cmd_en
 * @brief	cmd tags - do not reused the released cmd tag numbers - keep adding
 * at the end instead
 */
enum cash3_host_cmd_en {
	CASH3_HOST_CMD_VIDEO_SET_VIDEO_PARAMS	 = 1,
	CASH3_HOST_CMD_TRIGGER_DUMMY_EVENT       = 2,
	CASH3_HOST_CMD_BACKUP_FPGA_IMAGE_FLASH	 = 3,
	CASH3_HOST_CMD_ENABLE_BACKUP_FPGA_IMAGE  = 4,
	CASH3_HOST_CMD_STOP_APP                  = 5,
	CASH3_HOST_CMD_INQUIRY                   = 6,
	CASH3_HOST_CMD_REQ_HEALTH_REPORT         = 7,
	CASH3_HOST_CMD_START_STOP_CAMERA         = 8,  //1 - start, 2 - stop
	CASH3_HOST_CMD_DEBUG                     = 9,
	CASH3_HOST_CMD_CAN_TRAFFIC               = 10,
	CASH3_HOST_CMD_REQ_MIPI_REPORT           = 11,
	CASH3_HOST_CMD_UPDATE_UPLOAD_FILE = 12,
	CASH3_HOST_CMD_UPDATE_FILE_VERSION = 13,
	CASH3_HOST_CMD_UPDATE_ACTIVE_FILE = 14,
};

struct cash3_host_cmd_head_t {
	enum cash3_host_cmd_en cmd : 16;	//what cmd
	uint16_t reserved;
} __attribute__((packed));

/**
 * @struct - cash3_host_cmd_t
 * @brief	a shell struct holding actual cmd(s)
 */
struct cash3_host_cmd_t {
	struct cash3_host_cmd_head_t head;
	uint8_t payload[4096 - sizeof(struct cash3_host_cmd_head_t)];
} __attribute__((packed));

/**
 * @struct - cash3_host_cmd_backup_fpga_image_flash_t
 * @brief	FPGA needs to verify the image before committing to the
 * file system
 */
struct cash3_host_cmd_backup_fpga_image_flash_t {
	uint32_t reserved;
	uint32_t flash_image_size;		//the byte size of the image - below
	uint8_t flash_image[1];			//actual image starts here
} __attribute__((packed));


/**
 * @struct - cash3_host_cmd_update_t
 * @brief	FPGA needs to verify the file before committing to the
 * file system
 */
struct cash3_host_cmd_update_t {
	uint32_t type;
	uint32_t data_size;
	uint8_t data[1];
} __attribute__((packed));


/**
 * @struct - cash3_host_cmd_enable_backup_fpga_image_t
 * @brief	make the backup image primary (used for next boot up)
 * and the current primary backup - basically a swap
 */
struct cash3_host_cmd_enable_backup_fpga_image_t {
	uint8_t reserved[64];
} __attribute__((packed));


/**
 * @struct - cash3_host_cmd_request_health_report_t
 * @brief	 Host sends this command to ask FPGA send health report with count
 */
struct cash3_host_cmd_request_health_report_t {
	uint32_t report_count;
} __attribute__((packed));

/**
 * @struct - cash3_host_cmd_request_mipi_report_t
 * @brief	 Host sends this command to ask FPGA send health report with count
 */
struct cash3_host_cmd_request_mipi_report_t {
	uint32_t report_count;
	uint32_t defectChanThreshAdj;
} __attribute__((packed));

/**
 * @struct - cash3_host_cmd_set_video_params_t
 */
struct cash3_host_cmd_set_video_params_t {
	uint8_t video_stream_id;
	// setting 0 in any parameters below indicates no change from existing
	// setting for those parameters
	uint8_t format;					//1 RGB, 2 YUV
	uint8_t delay_ms;
	uint8_t fps;
	uint16_t exposure;
	uint8_t reserved[2];
} __attribute__((packed));

/**
 * @struct - cash3_host_cmd_trigger_dummy_event
 * @brief	 Host sends this command to ask FPGA to send back a dummy event
 */
struct cash3_host_cmd_trigger_dummy_event_t {
	uint32_t reserved;	//align the following
	uint64_t backReference; //this value is going to come back in the dummy event
	uint64_t reserved2[2];  //this value is used by FPGA internally
} __attribute__((packed));

enum CAMERA_OPS {
	CASH3_HOST_CMD_START_CAMERA = 1,
	CASH3_HOST_CMD_STOP_CAMERA  = 2,
};


/**
 * @struct - cash3_host_cmd_start_stop_camera_t
 * @brief	 Host sends this command to ask FPGA to start/stop camera streaming
 */
struct cash3_host_cmd_start_stop_streaming_t {
	uint16_t cam_id;
	uint16_t op;					//1 - start, 2 - stop
} __attribute__((packed));

/**
 * @struct - cash3_host_cmd_debug_t
 * @brief	 Host sends this command to ask FPGA to do debugging
 */
struct cash3_host_cmd_debug_t {
	uint32_t dbg_flag;
} __attribute__((packed));


#endif
