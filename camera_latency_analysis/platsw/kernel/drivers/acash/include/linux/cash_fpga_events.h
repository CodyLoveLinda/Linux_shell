/*
 * This file captures the CASH FPGA events reported to hosts
 *
 * To keep released version backward compatible with older released version, follow
 * this guideline when changing released commands struct:
 * 
 * mark deleted fields as reserved and always add new fields at the end and reduce 
 * the reserved field size at the end to keep the cmd struct size unchanged (64B)
 * 
 * 32bit value needs to land at word boundary, and 16but valuye on 2 byte boundary
 */
#ifndef __CASH_FPGA_EVENTS_H__
#define __CASH_FPGA_EVENTS_H__
#include "cash_scratch_registers.h"


#define CASH_FPGA_EVENT_QDMA_QUEUE_ID	8
#define CASH_FPGA_EVENT_SIZE			4096

/**
 * @enum - cash_fpga_event_en
 * @brief	event tags - do not reused the released tag numbers - keep adding
 * at the end instead
 */
enum cash_fpga_event_en {
	CASH_FPGA_EVENT_VIDEO_STATUS_CHANGED 		= 1,
	CASH_FPGA_EVENT_IMAGE_FLASHED				= 2,
	CASH_FPGA_EVENT_HEALTH_REPORT				= 3,
	CASH_FPGA_EVENT_ERROR_REPORT				= 4,
	CASH_FPGA_EVENT_CAN_TRAFFIC					= 5,
	CASH_FPGA_EVENT_DUMMY						= 0xffff, //no more events
};

/**
 * @struct - cash_fpga_event_head_t
 * @brief	every event start with this header
 */
struct cash_fpga_event_head_t {
	enum cash_fpga_event_en event : 16;	//what event
	uint16_t event_count;			//event of the same type can be reported
									//together following a single head, 
									//typically, one for each video stream
									//here is the count of them
	uint32_t reserved;				
} __attribute__((packed));


struct cash_fpga_event_t {
	struct cash_fpga_event_head_t head;
	uint8_t payload[1024 - sizeof(struct cash_fpga_event_head_t)];
} __attribute__((packed));

/**
 * @struct - cash_fpga_event_image_flashed_t
 * @brief	FPGA image is flashed on FPGA file system
 */
struct cash_fpga_event_image_flashed_t {
	uint32_t flash_image_crc_checksum;	//crc checksum of the flashed image
} __attribute__((packed));

/**
 * @enum - cash_video_status_event_en
 * @brief	used to show the new status of a single video queue (stream)
 * FPGA writes it and host reads it
 */
enum cash_video_status_event_en {
	CASH_VIDEO_STATUS_EVENT_NOT_READY = 0,
	CASH_VIDEO_STATUS_EVENT_READY,
};

/**
 * @struct - cash_fpga_event_video_status_changed_t
 * @brief	a video stream status has changed
 */
struct cash_fpga_event_video_status_changed_t {
	uint8_t video_stream_id;
	uint8_t reserved;
	enum cash_video_status_event_en change : 16;	
	int errno;							//see Linux errno
	char comment[24];					//comment about that change
	uint8_t reserved2[64];
} __attribute__((packed));

/**
 * @struct - 
 * @brief	
 */
struct cash_fpga_event_fd_link_status_t {
	uint8_t cam_id;
	uint8_t ser_general_status;		//DS90UB953_GENERAL_STATUS
	uint8_t ser_device_sts;			//DS90UB953_DEVICE_STS	
	uint8_t ser_csi_err_status;		//DS90UB953_CSI_ERR_STATUS
	
	uint8_t ser_crc_err_cnt1;		//DS90UB953_CRC_ERR_CNT1
	uint8_t ser_crc_err_cnt2;		//DS90UB953_CRC_ERR_CNT2
	uint8_t ser_csi_err_dlane01;		//DS90UB953_CSI_ERR_DLANE01
	uint8_t ser_csi_err_dlane23;		//DS90UB953_CSI_ERR_DLANE23

	uint8_t ser_csi_err_clk_lane;		//DS90UB953_CSI_ERR_CLK_LANE
	uint8_t ser_csi_err_cnt;		//DS90UB953_CSI_ERR_CNT
	uint8_t des_device_sts; 		//DS90UB954_DEVICE_STS
	uint8_t des_csi_rx_status;		//DS90UB954_CSI_RX_STS

	uint8_t des_csi_err_cnt;		//DS90UB954_CSI_ERR_COUNTER
	uint8_t des_port_sts1;			//DS90UB954_RX_PORT_STS1
	uint8_t des_port_sts2;			//DS90UB954_RX_PORT_STS2
	uint8_t des_sensor_sts_0;		//DS90UB954_SENSOR_STS_0

	uint8_t des_sensor_sts_3;		//DS90UB954_SENSOR_STS_3
	uint8_t des_rx_par_err_hi;		//DS90UB954_RX_PAR_ERR_HI
	uint8_t des_rx_par_err_lo;		//DS90UB954_RX_PAR_ERR_LO
	uint8_t des_aeq_status;			//DS90UB954_AEQ_STATUS

	uint8_t reserved2[4];
} __attribute__((packed));

struct cash_mipi_intc_status_t {
	uint32_t isr;
	uint32_t ipr;
	uint32_t ier;
	uint32_t mer;
};
struct cash_mipi_top_status_t {
	uint32_t fpgaVersion;
	uint32_t status;
};

struct cash_mipi_frmwr_status_t {
	uint32_t control;
	uint32_t iis;
};

struct cash_mipi_csc_status_t {
	uint32_t control;
	uint32_t iis;
};

struct cash_mipi_crs_status_t {
	uint32_t control;
	uint32_t iis;
};
struct cash_mipi_tpg_status_t {
	uint32_t control;
	uint32_t iis;
};

struct cash_mipi_csi_status_t {
	uint32_t ccr;
	uint32_t pcr;
	uint32_t csr;
	uint32_t isr;
};

struct cash_mipi_log_t {
	uint32_t frame_processed;
	uint32_t frame_skipped;
	uint32_t frame_lost;
	uint32_t frame_dummy;
};
/**
 * @struct - 
 * @brief	
 */
struct cash_fpga_event_mipi_status_t {
	uint32_t mipi_id:8;
	uint32_t state:8;				//ACTIVE, IDLE, DEFECTED
	uint32_t reserved:16;
	struct cash_mipi_log_t log;
	struct cash_mipi_intc_status_t intc;
	struct cash_mipi_top_status_t top;
	struct cash_mipi_frmwr_status_t frmwr;
	struct cash_mipi_crs_status_t crs;
	struct cash_mipi_csc_status_t csc;
	struct cash_mipi_tpg_status_t tpg;
	struct cash_mipi_csi_status_t csi;
} __attribute__((packed));
/**
 * @struct - 
 * @brief	
 */
struct cash_fpga_event_fpga_status_t {
	uint32_t video_state;
	uint32_t reserved;
} __attribute__((packed));
/**
 * @struct - 
 * @brief	
 */
struct cash_fpga_event_misc_status_t {
	uint32_t core_temperature;
	uint32_t reserved;
} __attribute__((packed));

/**
 * @struct - 
 * @brief	
 */
struct cash_fpga_event_health_status_t {
	uint64_t ts_sec;
	uint64_t ts_us;
	struct cash_fpga_event_misc_status_t misc_status;
	struct cash_fpga_event_fpga_status_t fpga_status;
	struct cash_fpga_event_fd_link_status_t fd_link_status[CASH_VIDEO_QUEUE_COUNT];
	struct cash_fpga_event_mipi_status_t mipi_status[CASH_VIDEO_QUEUE_COUNT];
} __attribute__((packed));

/**
 * @struct - 
 * @brief	
 */
struct cash_fpga_event_dummy_t {
	uint64_t backReference;		//see cash_host_cmd_trigger_dummy_event_t
	uint64_t fw_duration_us; 	//microseconds spent in fw
	uint64_t fw_duration_sec; 	//sec spent in fw, used by FPGA
} __attribute__((packed));


// /**
//  * @struct - cash_fpga_event_cmd_failed_t
//  * @brief	a host cmd previously issued failed
//  */
// struct cash_fpga_event_cmd_failed_t {
// 	uint64_t cmd_ref_num;		//refer to the failed host cmd
// 	uint8_t video_stream_id;	//further identify where the error is
// 	int errno;					//see Linux errno
// 	char comments[24];			//null terminated comment
// 	uint8_t reserved[64];
// } __attribute__((packed));

#endif
