/*
 * This file captures the CASH3 FPGA scratch register interface
 *
 */

#ifndef __CASH3_SCRATCH_REGISTERS_H__
#define __CASH3_SCRATCH_REGISTERS_H__

#include "cash3_basic_registers.h"
#include "cash3_conf.h"
#ifndef __KERNEL__
#include <stdint.h>
#else
#include <linux/types.h>
#endif
#include "hw/pcie_ep.h"

//                      qdma bypass
// +----------------+   FPGA event Q +------------------+
// |                <----------------+                  |
// |                |                |                  |
// |    host        |                |     FPGA         |
// |                +--------------->+                  |
// |                |    non-bypass  |                  |
// +--------------+-+    host cmd Q  +---+--------------+
//                |                      ^
//                |                      |
//                +----------------------+
//                    trigger interrupt

/**
 * @brief	steps to use scratch register to communicate. NOTE: The registers writes
 * or reads are not protected and it is the user's responsibility to synch between
 * threads.
 * It is recommended they be used in a single thread way on the host - which means
 * using a single thread to write host commands to FPGA and a single thread (or
 * the same thread) to read from FPGA event queue
 * @detail refer to the diagram above.
 * 		from host to initialize communication to FPGA CPU - sending host commands:
 * 			- host writes to FPGA IO area a host cmd (see cash3_host_cmds.h) or multple cmds
 * 				of the same type (following a single cmd head) via the qdma non-bypass queue
 * 			- host writes to CASH3_FPGA_IRQ_STATUS_ADDR using the mask CASH3_HOST_CMD_PENDING
 * 				to generate an interrupt to FPGA CPU
 * 						- FPGA CPU gets an interrupt, check if CASH3_FPGA_IRQ_STATUS_ADDR
 * 							is CASH3_HOST_CMD_PENDING, otherwise ignore
 * 						- FPGA CPU writes CASH3_HOST_CMD_PENDING to CASH3_FPGA_IRQ_CLEAR_ADDR
 * 							to indicate the interrupt to clear
 * 						- FPGA CPU writes 1 to CASH3_FPGA_IRQ_CMD_ADDR
 * 							and clears interrupt
 * 						- FPGA CPU honour the cmds stored in the IO area
 * 						- FPGA CPU report back the cmd execution status at
 * 							CASH3_SCRATCH_REG_CMD_STATUS_ADDR[8] array, each 32 bits in the array
 * 							is for one commnad in the current cmd head transfer, it is formatted :
 * 								lower 8 bits: Linux error number - positive or 0;
 * 								higher 24 bits: cmd specific auxiliary error indicators
 * 			- ONLY NOW host can safely send the next command - NO COMMANDS OVERLAPPING
 *
 * 		from FPGA to intialize communication to host - report FPGA events
 * 			similarly to how to transfer video frames to hosts, each event is fixed 512B in size
 * 			so the sender and receiver know how much to ask for each transfer
 *
 * 			- host MUST ensure the FPGA event qdma queue is being read continousely
 * 				, which means there is always a host read request pending from FPGA's point
 * 				of view. This garantees all FPGA events are reported back to hosts and no
 * 				buffer overflow
 * 						- When FPGA detects something to report to host as event, CPU writes an
 * 							event (512B) to CASH3_FPGA_EVENT_QDMA_QUEUE_ID bypass qdma queue
 * 						- trigger the qdma transfer
 * 						* since there is only one event queue, there might be extra synchronization
 * 							considerations compared to the video transfer scenario.
 */

/**
 * @enum - cash3_fpga_irq_mask
 * @brief	mark bits in CASH3_FPGA_IRQ_STATUS_ADDR
 * CASH3_SCRATCH_REG_VIDEO_STATUS_ADDR is the starting address of the 1st in
 * an array of this struct
 */

enum cash3_fpga_irq_mask_en {
	CASH3_HOST_CMD_PENDING 		= 1u << 0,	//FPGA to check IO area for a new host cmd
	CASH3_HOST_CAN_0_CMD_PENDING 	= 1u << 1,	//FPGA to check IO area for a host CAN0 cmd
	CASH3_HOST_CAN_1_CMD_PENDING 	= 1u << 2,
	CASH3_HOST_CAN_2_CMD_PENDING 	= 1u << 3,
	CASH3_HOST_CAN_3_CMD_PENDING 	= 1u << 4,
	CASH3_HOST_CAN_4_CMD_PENDING 	= 1u << 5,
	CASH3_HOST_CAN_5_CMD_PENDING 	= 1u << 6,
};

#define CASH3_SCRATCH_REG_START_ADDR 			pcie_ep_frame_ctrl_scratch_OFFSET
#define CASH3_SCRATCH_REG_COUNT 					64

/**
 * @struct - cash3_fpga_versions_t
 * @brief	used to describe the versions of FPGA
 */
struct cash3_fpga_manifest_t {
	uint16_t fw_ver : 10;					//running firmware version
	uint16_t hw_major_ver : 6;				//running FPGA (hw) major version
	uint16_t backup_fw_ver : 10;			//firmware version on file as a backup
	uint16_t hw_minor_ver : 6;				//running FPGA (hw) minor version
}  __attribute__((packed));
#define CASH3_SCRATCH_REG_FPGA_VER_ADDR		(CASH3_SCRATCH_REG_START_ADDR + 0 * 4)

/**
 * @struct - cash3_io_area_low_t
 * @brief	IO area is a continuous memory area on FPGA used for host IO
 * that a host side non-bypass queue can read and write
 * FPGA writes it and host reads it. This struct holds the low 32bit address
 * of the starting address
 */
struct cash3_io_area_t {
	uint32_t bram_addr;				//board address low
}  __attribute__((packed));
#define CASH3_IO_AREA_HIGH		0x00000008	//board address high
#define CASH3_IO_AREA_BYTE_SIZE	(256 * 1024 * 1024)
#define CASH3_SCRATCH_REG_IO_AREA_LOW_ADDR	(CASH3_SCRATCH_REG_START_ADDR + 1 * 4)

/**
 * @enum - cash3_video_status_en
 * @brief	used to show the new status of a single video queue (stream)
 * FPGA writes it and host reads it
 */
// update new status for camera link
enum cash3_video_state_en {
  CASH3_VIDEO_STATE_NOT_READY = 0,	//host should not send cmds
  CASH3_VIDEO_STATE_READY_RGB_2M,       //1920x1080x3=6220800
  CASH3_VIDEO_STATE_READY_YUYV_2M,      //1920x1080x2=4147200
  CASH3_VIDEO_STATE_READY_RGB_8M,       //3840x2160x3=24883200
  CASH3_VIDEO_STATE_READY_YUYV_8M,      //3840x2160x2=16588800
  CASH3_VIDEO_STATE_READY_RAW_8M,       // 3840x2160x1.5=12441600
  CASH3_VIDEO_STATE_READY_UYVY_2M,      //1920x1080x2=4147200
  CASH3_VIDEO_STATE_READY_UYVY_8M,      //3840x2160x2=16588800
};

/**
 * @struct - cash3_video_status_t
 * @brief	the current state of a video stream
 * CASH3_SCRATCH_REG_VIDEO_STATE_ADDR is the starting address of an array
 * of this struct.
 */
struct cash3_video_status_t {
	enum cash3_video_state_en state  : 4;
	enum cash3_video_state_en state2 : 4;
}  __attribute__((packed));
#define CASH3_SCRATCH_REG_VIDEO_STATE_ADDR	(CASH3_SCRATCH_REG_START_ADDR + 2 * 4)

/**
 * @struct - cash3_cmd_status_t
 * @brief	last command execution result. Since there could be up to 8 commands
 * of the same type, CASH3_SCRATCH_REG_CMD_STATUS_ADDR is the starting address
 * of an 8 dimension array. It is important to set them to be 0xffffffff before
 * sending the new command
 */
struct cash3_cmd_status_t {
	uint8_t err_no;				//see Linux errno encoding - 0xffffffff means pending
	uint32_t err_details : 24;	//cmd specific details
}  __attribute__((packed));
#define CASH3_SCRATCH_REG_CMD_STATUS_ADDR_C8    (CASH3_SCRATCH_REG_START_ADDR + 3 * 4)
#define CASH3_SCRATCH_REG_CMD_STATUS_ADDR_C16   (CASH3_SCRATCH_REG_START_ADDR + 4 * 4)
/** 6 registers for cmd status for cmd sent to CAN queues
 */
#define CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C8    (CASH3_SCRATCH_REG_START_ADDR + 4 * 4)
#define CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C16   (CASH3_SCRATCH_REG_START_ADDR + 5 * 4)
#define CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_COUNT	6

/** 8 registers keeping the number of ever sent (after a reset) video qdma requests
 *
 * Before host sending a video request on video stream n, it MUST increase
 * CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR(n) by 1
 * Host can reset the following CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR(n) and
 * CASH3_SCRATCH_REG_V_RESP_COUNT_ADDR(n) by setting them to be 0,
 * with CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR(n) first
 */
#define CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR_C8    (CASH3_SCRATCH_REG_START_ADDR + 10 * 4)

/** 16 registers keeping the number of ever sent (after a reset) video qdma requests
 *
 * Before host sending a video request on video stream n, it MUST increase
 * CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR(n) by 1
 * Host can reset the following CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR(n) and
 * CASH3_SCRATCH_REG_V_RESP_COUNT_ADDR(n) by setting them to be 0,
 * with CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR(n) first
 */
#define CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR_C16   (CASH3_SCRATCH_REG_START_ADDR + 11 * 4)

/** 8 registers keeping the number of triggered video qdma responses
 * Before FPGA triggers a QDMA transfer for video stream n, it MUST check
 * CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR(n) > CASH3_SCRATCH_REG_V_RESP_COUNT_ADDR(n)
 * If false, drop the video frame and DONOT trigger qdma transfer
 */
#define CASH3_SCRATCH_REG_V_RESP_COUNT_ADDR_C8   (CASH3_SCRATCH_REG_START_ADDR + 18 * 4)

/** 16 registers keeping the number of triggered video qdma responses
 * Before FPGA triggers a QDMA transfer for video stream n, it MUST check
 * CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR(n) > CASH3_SCRATCH_REG_V_RESP_COUNT_ADDR(n)
 * If false, drop the video frame and DONOT trigger qdma transfer
 */
#define CASH3_SCRATCH_REG_V_RESP_COUNT_ADDR_C16  (CASH3_SCRATCH_REG_START_ADDR + 27 * 4)

/** 6 registers keeping the number of ever sent (after a reset) CAN qdma requests
 *
 * Before host sending a CAN frame request on CAN channel n, it MUST increase
 * CASH3_SCRATCH_REG_CAN_REQ_COUNT_ADDR(n) by 1
 * Host can reset the following CASH3_SCRATCH_REG_CAN_REQ_COUNT_ADDR(n) and
 * CASH3_SCRATCH_REG_CAN_RESP_COUNT_ADDR(n) by setting them to be 0,
 * with CASH3_SCRATCH_REG_CAN_REQ_COUNT_ADDR(n) first
 */
#define CASH3_SCRATCH_REG_CAN_REQ_COUNT_ADDR_C8	   (CASH3_SCRATCH_REG_START_ADDR + 26 * 4)
#define CASH3_SCRATCH_REG_CAN_REQ_COUNT_ADDR_C16   (CASH3_SCRATCH_REG_START_ADDR + 43 * 4)

/** 6 registers keeping the number of triggered CAN qdma responses
 * Before FPGA triggers a QDMA transfer for CAN channel n, it MUST check
 * CASH3_SCRATCH_REG_CAN_REQ_COUNT_ADDR(n) > CASH3_SCRATCH_REG_CAN_RESP_COUNT_ADDR(n)
 * If false, DONOT trigger qdma transfer and keep the CAN frame inside internal ringbuf
 */
#define CASH3_SCRATCH_REG_CAN_RESP_COUNT_ADDR_C8    (CASH3_SCRATCH_REG_START_ADDR + 32 * 4)
#define CASH3_SCRATCH_REG_CAN_RESP_COUNT_ADDR_C16   (CASH3_SCRATCH_REG_START_ADDR + 49 * 4)

/* Scratch register used internally to get active file and backup md5 value */
#define CASH3_SCRATCH_REG_ACT_MD5_ADDR (CASH3_SCRATCH_REG_START_ADDR + 54 * 4)
#define CASH3_SCRATCH_REG_BACKUP_MD5_ADDR (CASH3_SCRATCH_REG_START_ADDR + 58 * 4)

/* Scratch register used internally by FW */
#define CASH3_SCRATCH_REG_CMD_SOURCE_ADDR (CASH3_SCRATCH_REG_START_ADDR + 62 * 4)

/* Scratch register used internally to turn debug message on/off */
#define CASH3_SCRATCH_REG_DEBUG_FLAG_ADDR (CASH3_SCRATCH_REG_START_ADDR + 63 * 4)



#endif
