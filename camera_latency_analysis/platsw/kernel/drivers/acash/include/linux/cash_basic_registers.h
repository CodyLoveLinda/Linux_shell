/*
 * This file captures the basic CASH FPGA registers
 *
 */

#ifndef __CASH_BASIC_REGISTERS_H__
#define __CASH_BASIC_REGISTERS_H__
#include <linux/types.h>

#define CASH_VIDEO_BUFFER_DEPTH  				4
#define CASH_VIDEO_BUFFER_SIZE 					(6 * 1024 * 1024)
#define CASH_VIDEO_QUEUE_COUNT  				8
#define CASH_HW_IRQ_MASK						0x00000010  //host write 0xffffffff to mask all
#define CASH_FPGA_IRQ_STATUS_ADDR				0x00000030 	//host write to it causes an interrupt
															//to FPGA CPU - restricted by MASK
#define CASH_FPGA_IRQ_MASK_ADDR					0x00000034	//restricting mask for the above
#define CASH_FPGA_IRQ_CLEAR_ADDR				0x00000038	//mask indicating which bit to clear 
															//in CASH_FPGA_IRQ_STATUS_ADDR
#define CASH_FPGA_IRQ_CMD_ADDR					0x0000003c	//FPGA write 1 to clear the interrupt

/* FPGA buffers address */
#define CASH_FPGA_VIDEO_BUF_ADDR                0x801000000  // Vidoe streams buffer (8streams)
#define CASH_FPGA_EVENT_BUF_ADDR                0x860000000  // Event buffer (4bufs:4KB size, 6MB gap)
#define CASH_FPGA_CAN_BUF_ADDR                  0x862000000  // CAN buffer
#define CASH_FPGA_SWUPD_BUF_ADDR                0x870000000  // SW update buffer

#endif
