/*
 * This file captures the CASH FPGA scratch register interface
 *
 */

#ifndef __CASH_SCRATCH_REGISTERS_H__
#define __CASH_SCRATCH_REGISTERS_H__

#include "cash_basic_registers.h"
#include <linux/types.h>

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
 * 			- host writes to FPGA IO area a host cmd (see cash_host_cmds.h) or multple cmds
 * 				of the same type (following a single cmd head) via the qdma non-bypass queue
 * 			- host writes to CASH_FPGA_IRQ_STATUS_ADDR using the mask CASH_HOST_CMD_PENDING
 * 				to generate an interrupt to FPGA CPU
 * 						- FPGA CPU gets an interrupt, check if CASH_FPGA_IRQ_STATUS_ADDR
 * 							is CASH_HOST_CMD_PENDING, otherwise ignore
 * 						- FPGA CPU writes CASH_HOST_CMD_PENDING to CASH_FPGA_IRQ_CLEAR_ADDR
 * 							to indicate the interrupt to clear
 * 						- FPGA CPU writes 1 to CASH_FPGA_IRQ_CMD_ADDR 
 * 							and clears interrupt
 * 						- FPGA CPU honour the cmds stored in the IO area
 * 						- FPGA CPU report back the cmd execution status at 
 * 							CASH_SCRATCH_REG_CMD_STATUS_ADDR[8] array, each 32 bits in the array
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
 * 							event (512B) to CASH_FPGA_EVENT_QDMA_QUEUE_ID bypass qdma queue
 * 						- trigger the qdma transfer
 * 						* since there is only one event queue, there might be extra synchronization 
 * 							considerations compared to the video transfer scenario.
 */

/**
 * @enum - cash_fpga_irq_mask
 * @brief	mark bits in CASH_FPGA_IRQ_STATUS_ADDR
 * CASH_SCRATCH_REG_VIDEO_STATUS_ADDR is the starting address of the 1st in
 * an array of this struct
 */

enum cash_fpga_irq_mask_en {
	CASH_HOST_CMD_PENDING = 1 << 0,		//FPGA to check IO area for a new host cmd
};

#define CASH_SCRATCH_REG_START_ADDR 			0x00001100
#define CASH_SCRATCH_REG_COUNT 					64

/**
 * @struct - cash_fpga_versions_t
 * @brief	used to describe the versions of FPGA
 */
struct cash_fpga_manifest_t {
	uint16_t fw_ver : 10;					//running firmware version
	uint16_t hw_major_ver : 6;				//running FPGA (hw) major version
	uint16_t backup_fw_ver : 10;			//firmware version on file as a backup
	uint16_t hw_minor_ver : 6;				//running FPGA (hw) minor version
}  __attribute__((packed));
#define CASH_SCRATCH_REG_FPGA_VER_ADDR		(CASH_SCRATCH_REG_START_ADDR + 0 * 4)

/**
 * @struct - cash_io_area_low_t
 * @brief	IO area is a continuous memory area on FPGA used for host IO
 * that a host side non-bypass queue can read and write
 * FPGA writes it and host reads it. This struct holds the low 32bit address
 * of the starting address
 */
struct cash_io_area_t {
	uint32_t bram_addr;				//board address low
}  __attribute__((packed));
#define CASH_IO_AREA_HIGH		0x00000008	//board address high
#define CASH_IO_AREA_BYTE_SIZE	(256 * 1024 * 1024)
#define CASH_SCRATCH_REG_IO_AREA_LOW_ADDR	(CASH_SCRATCH_REG_START_ADDR + 1 * 4)

/**
 * @enum - cash_video_status_en
 * @brief	used to show the new status of a single video queue (stream)
 * FPGA writes it and host reads it
 */
enum cash_video_state_en {
	CASH_VIDEO_STATE_NOT_READY = 0,		//host should not send cmds
	CASH_VIDEO_STATE_READY 				//FPGA is ready for host cmds
};
/**
 * @struct - cash_video_status_t
 * @brief	the current state of a video stream
 * CASH_SCRATCH_REG_VIDEO_STATE_ADDR is the starting address of an array
 * of this struct. 
 */ 
struct cash_video_status_t {
	enum cash_video_state_en state  : 4;
	enum cash_video_state_en state2 : 4;
}  __attribute__((packed));
#define CASH_SCRATCH_REG_VIDEO_STATE_ADDR	(CASH_SCRATCH_REG_START_ADDR + 2 * 4)

/**
 * @struct - cash_cmd_status_t
 * @brief	last command execution result. Since there could be up to 8 commands
 * of the same type, CASH_SCRATCH_REG_CMD_STATUS_ADDR is the starting address
 * of an 8 dimension array. It is important to set them to be 0xffffffff before 
 * sending the new command
 */
struct cash_cmd_status_t {
	uint8_t errno;				//see Linux errno encoding - 0xffffffff means pending
	uint32_t err_details : 24;	//cmd specific details
}  __attribute__((packed));
#define CASH_SCRATCH_REG_CMD_STATUS_ADDR	(CASH_SCRATCH_REG_START_ADDR + 3 * 4)
#define CASH_SCRATCH_REG_CMD_STATUS_COUNT	8

/** 8 registers keeping the number of ever sent (after a reset) video qdma requests
 *
 * Before host sending a video request on video stream n, it MUST increase
 * CASH_SCRATCH_REG_V_REQ_COUNT_ADDR(n) by 1
 * Host can reset the following CASH_SCRATCH_REG_V_REQ_COUNT_ADDR(n) and 
 * CASH_SCRATCH_REG_V_RESP_COUNT_ADDR(n) by setting them to be 0,
 * with CASH_SCRATCH_REG_V_REQ_COUNT_ADDR(n) first
 */
#define CASH_SCRATCH_REG_V_REQ_COUNT_ADDR	(CASH_SCRATCH_REG_START_ADDR + 11 * 4)

/** 8 registers keeping the number of triggered video qdma responses
 * Before FPGA triggers a QDMA transfer for video stream n, it MUST check
 * CASH_SCRATCH_REG_V_REQ_COUNT_ADDR(n) > CASH_SCRATCH_REG_V_RESP_COUNT_ADDR(n)
 * If false, drop the video frame and DONOT trigger qdma transfer
 */
#define CASH_SCRATCH_REG_V_RESP_COUNT_ADDR	(CASH_SCRATCH_REG_START_ADDR + 19 * 4)

/* Scratch register used internally to measure frame timing */
#define CASH_SCRATCH_TIMING_FLAG_ADDR (CASH_SCRATCH_REG_START_ADDR + 62 * 4)

/* Scratch register used internally to turn debug message on/off */
#define CASH_SCRATCH_REG_DEBUG_FLAG_ADDR (CASH_SCRATCH_REG_START_ADDR + 63 * 4)

#endif
