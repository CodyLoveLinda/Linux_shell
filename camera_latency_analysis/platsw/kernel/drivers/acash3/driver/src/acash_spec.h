/* 
 * Copyright (C) 2022 AutoX, Inc. 
 */

#ifndef _AUTOX_CASH_SPEC_H_
#define	_AUTOX_CASH_SPEC_H_
#include "qdma_mod.h"

typedef struct autox_dma_queue {
	unsigned int count;
	unsigned int begin;
	unsigned int end;
} autox_dma_queue_t;

typedef struct autox_queue_spec
{
	unsigned int count;

	autox_dma_queue_t video;
	autox_dma_queue_t event;
	autox_dma_queue_t can;
	autox_dma_queue_t test;
} autox_queue_spec_t;

typedef struct autox_reg_addr {
	unsigned int addr;
	unsigned int count;
} autox_reg_addr_t;

typedef struct autox_scratch_reg_addr_spec {
	unsigned int start_addr;
	unsigned int count;

	autox_reg_addr_t fpga_ver;
	autox_reg_addr_t io_area;
	autox_reg_addr_t video_state;
	autox_reg_addr_t cmd_status;
	autox_reg_addr_t can_cmd_status;
	autox_reg_addr_t video_req_cnt;
	autox_reg_addr_t video_resp_cnt;
	autox_reg_addr_t can_req_cnt;
	autox_reg_addr_t can_resp_cnt;
	autox_reg_addr_t act_md5_value;
	autox_reg_addr_t backup_md5_value;
	autox_reg_addr_t cmd_source;
	autox_reg_addr_t debug_flag;
} autox_scratch_reg_addr_spec_t;

typedef enum autox_spec_type {
	AUTOX_SPEC_8_CAMERAS = 0,
	AUTOX_SPEC_16_CAMERAS = 1,
	AUTOX_SPEC_TYPE_MAX
} autox_spec_type_t;

typedef struct autox_spec {
	struct xlnx_pci_dev	*xpdev;
	autox_spec_type_t type;
	autox_queue_spec_t queue;
	autox_scratch_reg_addr_spec_t scratch_reg;
} autox_spec_t;

int autox_spec_init(struct xlnx_pci_dev *xpdev);
void autox_spec_uninit(struct xlnx_pci_dev *xpdev);
#endif