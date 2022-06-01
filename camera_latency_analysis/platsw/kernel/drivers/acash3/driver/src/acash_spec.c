/* 
 * Copyright (C) 2022 AutoX, Inc. 
 */

#include "linux/cash3_scratch_registers.h"
#include "linux/cash3_conf.h"

#include "acash_spec.h"
#include "acash_utils.h"
#include "libqdma_cash3.h"
typedef struct autox_spec_ops {
	autox_spec_type_t type;
	void (*handle)(autox_spec_t *aspec);
} autox_spec_ops_t;


static void autox_queue_spec_8_cameras_init(autox_spec_t *aspec)
{
	// queue specification
	aspec->queue.count = CASH_DMA_QUEUE_COUNT_C8;
	
	aspec->queue.video.count = CASH_DMA_VIDEO_QUEUE_COUNT_C8;
	aspec->queue.video.begin = CASH_DMA_VIDEO_QUEUE_BEGIN_C8;
	aspec->queue.video.end = CASH_DMA_VIDEO_QUEUE_END_C8;

	aspec->queue.event.count = CASH_DMA_EVENT_COUNT_C8;
	aspec->queue.event.begin = CASH_DMA_EVENT_QUEUE_BEGIN_C8;
	aspec->queue.event.end = CASH_DMA_EVENT_QUEUE_END_C8;

	aspec->queue.can.count = CASH_DMA_CAN_QUEUE_COUNT_C8;
	aspec->queue.can.begin = CASH_DMA_CAN_QUEUE_BEGIN_C8;
	aspec->queue.can.end = CASH_DMA_CAN_QUEUE_END_C8;

	aspec->queue.test.count = CASH_DMA_TEST_QUEUE_COUNT_C8;
	aspec->queue.test.begin = CASH_DMA_TEST_QUEUE_BEGIN_C8;
	aspec->queue.test.end = CASH_DMA_TEST_QUEUE_END_C8;
}

static void autox_scratch_reg_8_cameras_init(autox_spec_t *aspec)
{
	// scratch register specification
	aspec->scratch_reg.start_addr = CASH3_SCRATCH_REG_START_ADDR;
	aspec->scratch_reg.count = CASH3_SCRATCH_REG_COUNT;

	aspec->scratch_reg.fpga_ver.addr = CASH3_SCRATCH_REG_FPGA_VER_ADDR;
	aspec->scratch_reg.fpga_ver.count = 1;

	aspec->scratch_reg.io_area.addr = CASH3_SCRATCH_REG_IO_AREA_LOW_ADDR;
	aspec->scratch_reg.io_area.count = 1;

	aspec->scratch_reg.video_state.addr = CASH3_SCRATCH_REG_VIDEO_STATE_ADDR;
	aspec->scratch_reg.video_state.count = 1;

	aspec->scratch_reg.cmd_status.addr = CASH3_SCRATCH_REG_CMD_STATUS_ADDR_C8;
	aspec->scratch_reg.cmd_status.count = 1;

	aspec->scratch_reg.can_cmd_status.addr = CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C8;
	aspec->scratch_reg.can_cmd_status.count = CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_COUNT;

	aspec->scratch_reg.video_req_cnt.addr = CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR_C8;
	aspec->scratch_reg.video_req_cnt.count = 8;

	aspec->scratch_reg.video_resp_cnt.addr = CASH3_SCRATCH_REG_V_RESP_COUNT_ADDR_C8;
	aspec->scratch_reg.video_resp_cnt.count = 8;

	aspec->scratch_reg.can_req_cnt.addr = CASH3_SCRATCH_REG_CAN_REQ_COUNT_ADDR_C8;
	aspec->scratch_reg.can_req_cnt.count = 6;

	aspec->scratch_reg.can_resp_cnt.addr = CASH3_SCRATCH_REG_CAN_RESP_COUNT_ADDR_C8;
	aspec->scratch_reg.can_resp_cnt.count = 6;

	aspec->scratch_reg.act_md5_value.addr = CASH3_SCRATCH_REG_ACT_MD5_ADDR;
	aspec->scratch_reg.act_md5_value.count = 4;

	aspec->scratch_reg.backup_md5_value.addr = CASH3_SCRATCH_REG_BACKUP_MD5_ADDR;
	aspec->scratch_reg.backup_md5_value.count = 4;

	aspec->scratch_reg.cmd_source.addr = CASH3_SCRATCH_REG_CMD_SOURCE_ADDR;
	aspec->scratch_reg.cmd_source.count = 1;

	aspec->scratch_reg.debug_flag.addr = CASH3_SCRATCH_REG_DEBUG_FLAG_ADDR;
	aspec->scratch_reg.debug_flag.count = 1;
}

static void autox_spec_8_cameras_init(autox_spec_t *aspec)
{
	autox_queue_spec_8_cameras_init(aspec);
	autox_scratch_reg_8_cameras_init(aspec);
}

static void autox_queue_spec_16_cameras_init(autox_spec_t *aspec)
{
	// queue specification
	aspec->queue.count = CASH_DMA_QUEUE_COUNT_C16;
	
	aspec->queue.video.count = CASH_DMA_VIDEO_QUEUE_COUNT_C16;
	aspec->queue.video.begin = CASH_DMA_VIDEO_QUEUE_BEGIN_C16;
	aspec->queue.video.end = CASH_DMA_VIDEO_QUEUE_END_C16;

	aspec->queue.event.count = CASH_DMA_EVENT_COUNT_C16;
	aspec->queue.event.begin = CASH_DMA_EVENT_QUEUE_BEGIN_C16;
	aspec->queue.event.end = CASH_DMA_EVENT_QUEUE_END_C16;

	aspec->queue.can.count = CASH_DMA_CAN_QUEUE_COUNT_C16;
	aspec->queue.can.begin = CASH_DMA_CAN_QUEUE_BEGIN_C16;
	aspec->queue.can.end = CASH_DMA_CAN_QUEUE_END_C16;

	aspec->queue.test.count = CASH_DMA_TEST_QUEUE_COUNT_C16;
	aspec->queue.test.begin = CASH_DMA_TEST_QUEUE_BEGIN_C16;
	aspec->queue.test.end = CASH_DMA_TEST_QUEUE_END_C16;
}

static void autox_scratch_reg_16_cameras_init(autox_spec_t *aspec)
{
	// scratch register specification
	aspec->scratch_reg.start_addr = CASH3_SCRATCH_REG_START_ADDR;
	aspec->scratch_reg.count = CASH3_SCRATCH_REG_COUNT;

	aspec->scratch_reg.fpga_ver.addr = CASH3_SCRATCH_REG_FPGA_VER_ADDR;
	aspec->scratch_reg.fpga_ver.count = 1;

	aspec->scratch_reg.io_area.addr = CASH3_SCRATCH_REG_IO_AREA_LOW_ADDR;
	aspec->scratch_reg.io_area.count = 1;

	aspec->scratch_reg.video_state.addr = CASH3_SCRATCH_REG_VIDEO_STATE_ADDR;
	aspec->scratch_reg.video_state.count = 2;

	aspec->scratch_reg.cmd_status.addr = CASH3_SCRATCH_REG_CMD_STATUS_ADDR_C16;
	aspec->scratch_reg.cmd_status.count = 1;

	aspec->scratch_reg.can_cmd_status.addr = CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C16;
	aspec->scratch_reg.can_cmd_status.count = CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_COUNT;

	aspec->scratch_reg.video_req_cnt.addr = CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR_C16;
	aspec->scratch_reg.video_req_cnt.count = 16;

	aspec->scratch_reg.video_resp_cnt.addr = CASH3_SCRATCH_REG_V_RESP_COUNT_ADDR_C16;
	aspec->scratch_reg.video_resp_cnt.count = 16;

	aspec->scratch_reg.can_req_cnt.addr = CASH3_SCRATCH_REG_CAN_REQ_COUNT_ADDR_C16;
	aspec->scratch_reg.can_req_cnt.count = 6;

	aspec->scratch_reg.can_resp_cnt.addr = CASH3_SCRATCH_REG_CAN_RESP_COUNT_ADDR_C16;
	aspec->scratch_reg.can_resp_cnt.count = 6;

	aspec->scratch_reg.act_md5_value.addr = CASH3_SCRATCH_REG_ACT_MD5_ADDR;
	aspec->scratch_reg.act_md5_value.count = 4;

	aspec->scratch_reg.backup_md5_value.addr = CASH3_SCRATCH_REG_BACKUP_MD5_ADDR;
	aspec->scratch_reg.backup_md5_value.count = 4;

	aspec->scratch_reg.cmd_source.addr = CASH3_SCRATCH_REG_CMD_SOURCE_ADDR;
	aspec->scratch_reg.cmd_source.count = 1;

	aspec->scratch_reg.debug_flag.addr = CASH3_SCRATCH_REG_DEBUG_FLAG_ADDR;
	aspec->scratch_reg.debug_flag.count = 1;
}

static void autox_spec_16_cameras_init(autox_spec_t *aspec)
{
	autox_queue_spec_16_cameras_init(aspec);
	autox_scratch_reg_16_cameras_init(aspec);
}

static autox_spec_ops_t spec_ops[] = {
	{AUTOX_SPEC_8_CAMERAS, autox_spec_8_cameras_init},
	{AUTOX_SPEC_16_CAMERAS, autox_spec_16_cameras_init}
};

int autox_spec_init(struct xlnx_pci_dev *xpdev)
{
	int i = 0;
	int op_array_size = sizeof(spec_ops) / sizeof(autox_spec_ops_t);
	autox_spec_t *aspec = NULL;
	unsigned int fw_ver;
	
	aspec = (autox_spec_t *)kzalloc(sizeof(autox_spec_t), GFP_KERNEL);
	if (!aspec) {
		ALOG_ERR(xpdev, "failed to alloc mem for specification\n");
		return -1;
	}

	qdma_device_read_user_register(xpdev, CASH3_SCRATCH_REG_FPGA_VER_ADDR, &fw_ver);
	if ((fw_ver & 0x3FF) < 200)
		aspec->type = AUTOX_SPEC_8_CAMERAS;
	else
		aspec->type = AUTOX_SPEC_16_CAMERAS;

	for (i = 0; i < op_array_size; i++) {
		if (aspec->type == spec_ops[i].type) {
			xpdev->spec = aspec;
			aspec->xpdev = xpdev;
			spec_ops[i].handle(aspec);
			return 0;
		}
	}
	
	// no type matched
	kfree(aspec);
	return -1;
}

void autox_spec_uninit(struct xlnx_pci_dev *xpdev)
{
	if (xpdev->spec) {
		kfree(xpdev->spec);
		xpdev->spec = NULL;
	}
}
