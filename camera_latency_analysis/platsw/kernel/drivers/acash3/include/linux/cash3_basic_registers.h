/*
 * This file captures the basic CASH3 FPGA registers
 *
 */

#ifndef __CASH3_BASIC_REGISTERS_H__
#define __CASH3_BASIC_REGISTERS_H__
#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif
typedef uint32_t hwint;
typedef uint8_t  hwint8;
typedef uint16_t hwint16;
typedef uint32_t hwint32;
#include "hw/pcie_ep.h"

#define CASH3_HW_IRQ_MASK0_ADDR		pcie_ep_frame_ctrl_HW_IRQ_MASK_0_ADDRESS  //host write 0xffffffff to mask all
#define CASH3_HW_IRQ_MASK1_ADDR		pcie_ep_frame_ctrl_HW_IRQ_MASK_1_ADDRESS  //host write 0xffffffff to mask all
#define CASH3_FPGA_IRQ_STATUS_ADDR	pcie_ep_frame_ctrl_FPGA_IRQ_STATUS_ADDRESS  //host write to it causes an interrupt
					            //to FPGA CPU - restricted by MASK
#define CASH3_FPGA_IRQ_MASK_ADDR	pcie_ep_frame_ctrl_FPGA_IRQ_MASK_ADDRESS  //restricting mask for the above
#define CASH3_FPGA_IRQ_CLEAR_ADDR	pcie_ep_frame_ctrl_FPGA_IRQ_CLEAR_ADDRESS //mask indicating which bit to clear
					            //in CASH3_FPGA_IRQ_STATUS_ADDR
#define CASH3_FPGA_IRQ_CMD_ADDR		pcie_ep_frame_ctrl_FPGA_IRQ_CMD_ADDRESS  //FPGA write 1 to clear the interrupt

/* spare status register are used for HW to report information/status to SW */
#define CASH3_SPARE_STATUS_0_REG 	(pcie_ep_frame_ctrl_spare_OFFSET + pcie_ep_frame_ctrl_SPARE_STATUS_OFFSET)

#endif
