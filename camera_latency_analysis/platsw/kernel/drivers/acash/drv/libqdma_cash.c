#define pr_fmt(fmt) KBUILD_MODNAME ":%s: " fmt, __func__

#include "libqdma_cash.h"
#include "qdma_mod.h"
#include "libqdma_export.h"

#include "libqdma_config.h"
#include "xdev.h" //probably needed
#include "version.h"

#include "linux/cash_scratch_registers.h"
#include "linux/cash_host_cmds.h"
#include "linux/cash_fpga_events.h"
#include "qdma_access/qdma_platform.h"
#include <linux/pci.h>
#include <linux/delay.h>

#undef pr_error_cash
#define pr_err_cash(...) {pr_err(__VA_ARGS__); pr_err("@%s:%d", __FILE__, __LINE__);}

// static
// void 
// display_time(char* msg, struct timeval* t) {
// 	struct tm broken;
// 	time_to_tm(t->tv_sec, 0, &broken);
// 	pr_info("%s%d:%d:%d:%ld\n", msg, broken.tm_hour, broken.tm_min, 
// 	                         broken.tm_sec, t->tv_usec);
// }

int cash_qdma_get_handles(unsigned int idx
	, unsigned long * dev_hndl
	, unsigned long * video_qhndls
	, unsigned int    video_qhndls_count
	, unsigned long * host_cmd_qhndl
	, unsigned long * fpga_event_qhndl) {
	struct xlnx_pci_dev *xpdev 
		= xpdev_find_by_idx(idx, NULL, 0);
	int i = 0;
	struct xlnx_qdata *qdata = NULL;
	if (!xpdev) {
		pr_err("xpdev_find_by_idx failed for pci address:%x", idx);
		return -EINVAL;
	}
	if (dev_hndl) *dev_hndl = xpdev->dev_hndl;
	for (;i < video_qhndls_count && i < xpdev->qmax; ++i) {
		qdata = xpdev->qdata + i + xpdev->qmax;
		video_qhndls[i] = qdata->qhndl;
	}
	if (host_cmd_qhndl) {
		*host_cmd_qhndl 
			= xpdev->qdata[CASH_HOST_CMD_QDMA_QUEUE_ID].qhndl;
	}
	if (fpga_event_qhndl) {
		*fpga_event_qhndl 
			= xpdev->qdata[CASH_FPGA_EVENT_QDMA_QUEUE_ID + xpdev->qmax]
				.qhndl;
	}

	return 0;
}

int initialize_cash_video(unsigned long dev_hndl) {
	return write_cash_user_reg(dev_hndl, CASH_HW_IRQ_MASK, 0xffffffff);
}

int clear_cash_video_status(unsigned long dev_hndl, uint8_t video_stream_id
	, uint32_t req_init_count) {
	uint32_t req_addr = 
		CASH_SCRATCH_REG_V_REQ_COUNT_ADDR + 4 * video_stream_id;
	uint32_t resp_addr =
		CASH_SCRATCH_REG_V_RESP_COUNT_ADDR + 4 * video_stream_id;
	uint32_t video_state_shift = 4 * video_stream_id;
	uint32_t video_state, c1, c2;
	video_state = 
		(read_cash_user_reg(dev_hndl, CASH_SCRATCH_REG_VIDEO_STATE_ADDR)
		 >> video_state_shift) & 0xf;
	pr_debug("video_state=0x%x", video_state);
	if (video_state != CASH_VIDEO_STATE_READY) return EHOSTDOWN;
	if (write_cash_user_reg(dev_hndl, req_addr, req_init_count)
		|| write_cash_user_reg(dev_hndl, resp_addr, 0)) {
		return EIO;
	}
	msleep_interruptible(100);
	c1 = read_cash_user_reg(dev_hndl, req_addr);
	c2 = read_cash_user_reg(dev_hndl, resp_addr); 

	if (c1 == req_init_count && c2 <= 4) {
		return 0;
	}
	return EAGAIN;
}

// static 
// unsigned long get_h2c_handle(unsigned long dev_hndl, uint8_t q_id) {
// 	struct qdma_dev_conf conf;
// 	int rv = qdma_device_get_config(dev_hndl, &conf, NULL, 0);
// 	struct device *dev;
// 	struct xlnx_pci_dev *xpdev;
// 	if (rv) return 0;
// 	dev = &conf.pdev->dev;
// 	xpdev = dev_get_drvdata(dev);
// 	return xpdev->qdata[q_id].qhndl;
// }

// static 
// unsigned long get_c2h_handle(unsigned long dev_hndl, uint8_t q_id) {
// 	struct qdma_dev_conf conf;
// 	struct device *dev;
// 	struct xlnx_pci_dev *xpdev;
// 	int rv = qdma_device_get_config(dev_hndl, &conf, NULL, 0);
// 	if (rv) return 0;
// 	dev = &conf.pdev->dev;
// 	xpdev = dev_get_drvdata(dev);
// 	return xpdev->qdata[xpdev->qmax + q_id].qhndl;
//  }

void read_io_area_conf(unsigned long dev_hndl
	, uint64_t* bram_addr, uint32_t* size) {
	uint32_t addr_low = read_cash_user_reg(dev_hndl
		, CASH_SCRATCH_REG_IO_AREA_LOW_ADDR);
	*bram_addr = ((uint64_t)CASH_IO_AREA_HIGH << 32ul) + addr_low;
	*size = CASH_IO_AREA_BYTE_SIZE;
}

ssize_t send_host_cmd(unsigned long dev_hndl
	, struct cash_host_cmd_t* cmd
	, size_t cmd_size
	, bool checkCmdState) {
	struct cash_qdma_request qreq = {0};
	uint64_t ior_addr;
	uint32_t ior_size;
	int i, rv;
	size_t chunk_max = 2 * 1024 * 1024;
	struct qdma_dev_conf conf;
	unsigned long host_cmd_qhndl;
	struct xlnx_pci_dev *xpdev;
	// struct timeval tv;

	if (!cmd->head.cmd) return -EINVAL;

	read_io_area_conf(dev_hndl, &ior_addr, &ior_size);
	if (cmd_size > ior_size) return -EINVAL;
	rv = qdma_device_get_config(dev_hndl, &conf, NULL, 0);
	if (rv) return -rv;
	xpdev = dev_get_drvdata(&conf.pdev->dev);
	rv = cash_qdma_get_handles(xpdev->idx, NULL
		, NULL, 0, &host_cmd_qhndl, NULL);
	if (rv) return -rv;

	qreq.dev_hndl = dev_hndl;
	qreq.qhndl = host_cmd_qhndl;
	if (cmd_size <= chunk_max) {
		qreq.buffer_chunk_count = 1;
		qreq.buffer_chunk_size = cmd_size;
	} else {
		qreq.buffer_chunk_count = (cmd_size + chunk_max - 1) / chunk_max;
		qreq.buffer_chunk_size = chunk_max;
	}
	qreq.pdev = conf.pdev;
	qreq.bram_addr = ior_addr;
	qreq.is_write = true;
	rv = prepare_cash_qdma_request(&qreq);
	if (rv) return -rv;
	// do_gettimeofday(&tv);
	// display_time("after prepare host cmd @", &tv);
	for (i = 0; i < qreq.buffer_chunk_count; ++i) {
		memcpy(qreq.buffer_chunks[i]
			, (char*)cmd + i * qreq.buffer_chunk_size
			, qreq.buffer_chunk_size);
	}
	for (i = 0; i < cmd->head.cmd_count; ++i) {
		rv = write_cash_user_reg(dev_hndl
			, CASH_SCRATCH_REG_CMD_STATUS_ADDR + i * 4
			, 0xffffffff);
		if (0 != rv) {
			pr_err("failed init cmd status regs rv=%d", rv);
			return -EIO;
		}
	}
	rv = send_cash_qdma_request(&qreq);
	unprepare_cash_qdma_request(&qreq);
	pr_debug("cmd send_cash_qdma_request rv=%d", rv);
	if (rv <= 0) return rv;
	{
		int rv;
		rv = write_cash_user_reg(dev_hndl, CASH_FPGA_IRQ_STATUS_ADDR
			, CASH_HOST_CMD_PENDING);
		if (rv) {
			pr_err("failed trigger fpga interrupt rv=%d", rv);
			return -EIO;
		}

		if (checkCmdState) {
			for (i = 0; i < cmd->head.cmd_count;++i) {
				while (0xffffffff == read_cash_user_reg(
					dev_hndl, CASH_SCRATCH_REG_CMD_STATUS_ADDR + i * 4)) {
					pr_debug("cmd status pending ...");
					msleep_interruptible(1);
				}
			}
		}
	}

	return rv;
}

// ssize_t get_fpga_event(unsigned long dev_hndl
// 	, struct cash_fpga_event_t* event) {
// 	struct cash_qdma_request* qreq;
// 	int rv;
// 	struct qdma_dev_conf conf;
// 	struct xlnx_pci_dev *xpdev;

// 	rv = qdma_device_get_config(dev_hndl, &conf, NULL, 0);
// 	if (rv) return -rv;
// 	xpdev = dev_get_drvdata(&conf.pdev->dev);
// 	qreq = xpdev->fpga_event_queue_request;
// 	rv = send_cash_qdma_request(qreq);
// 	if (rv == qreq->buffer_chunk_size) {
// 		memcpy(event, qreq->buffer_chunks[0]
// 			, qreq->buffer_chunk_size);
// 	}
// 	return rv;
// }

struct qdma_request const CASH_VIDEO_READ_REQUEST_DEFAULT = {
	.timeout_ms = 1 * 1000, //1 sec
};

struct qdma_request const CASH_QDMA_REQUEST_DEFAULT = {
	.timeout_ms = 1 * 1000, //1 sec
};

extern 
int sgl_map(struct pci_dev *pdev, struct qdma_sw_sg *sgl, unsigned int sgcnt,
	enum dma_data_direction dir);

extern 
void sgl_unmap(struct pci_dev *pdev, struct qdma_sw_sg *sg, unsigned int sgcnt,
		 enum dma_data_direction dir);

static
int fp_done_video(struct qdma_request *req, unsigned int bytes_done,
	int err) {
	struct cash_qdma_video_request* vreq = (void*)req->uld_data;
	vreq->aio_done(vreq->user_data, bytes_done, err);
	return 0;
}

static uint32_t const DATA_DESC_PAYLOAD_SIZE = 60 * 1024;

static 
int setup_sgl_for_video_chunk(
	struct cash_qdma_video_request* vreq, uint32_t chunk_idx, struct qdma_sw_sg ** sgl_ptr) {
	size_t cs;
	char* ptr;
	struct qdma_sw_sg * sgl = *sgl_ptr;

	vreq->buffer_chunks[chunk_idx] = 
		dma_alloc_coherent(&vreq->pdev->dev, vreq->buffer_chunk_size
			, vreq->buffer_chunk_dma_hndls + chunk_idx, GFP_KERNEL);
		// kmalloc(vreq->chunk_size, GFP_KERNEL|GFP_DMA);

	if (!vreq->buffer_chunks[chunk_idx]) {
		pr_err("dma_alloc_coherent failed");
		return -ENOMEM;
	}
	memset(vreq->buffer_chunks[chunk_idx], 0, vreq->buffer_chunk_size);
	cs = vreq->buffer_chunk_size;
	ptr = vreq->buffer_chunks[chunk_idx];

	while (cs) {
		uint32_t offset = offset_in_page(ptr);
		uint32_t nbytes = min_t(uint32_t, DATA_DESC_PAYLOAD_SIZE - offset, cs);

		sgl->pg = virt_to_page(ptr);
		sgl->offset = offset;
		sgl->len = nbytes;
		sgl->dma_addr = 0UL;
		sgl->next = sgl + 1;

		ptr += nbytes;
		cs -= nbytes;
		sgl++;
	}
	*sgl_ptr = sgl;
	return 0;
}

int prepare_cash_qdma_video_request(struct cash_qdma_video_request* vreq) {
	int chunk_idx = 0;
	int rv = 0;
	uint16_t qid;
	unsigned int desc_count
		= (vreq->buffer_chunk_count * vreq->buffer_chunk_size + (DATA_DESC_PAYLOAD_SIZE - 1))
			/ DATA_DESC_PAYLOAD_SIZE + 3; //over allocate by 3
	struct qdma_sw_sg * sgl, * sgl_work;
	struct qdma_request *req = NULL;

	{// figure out the q_id from qhdnl
		struct qdma_dev_conf qdev_conf;
		struct device *dev;
		struct xlnx_pci_dev *xpdev;
		rv = qdma_device_get_config(vreq->dev_hndl, &qdev_conf, NULL, 0);
		if (rv) return -ENODEV;

		dev = &qdev_conf.pdev->dev;
		xpdev = dev_get_drvdata(dev);
		for (qid = xpdev->qmax
			; qid < xpdev->qmax + CASH_VIDEO_QUEUE_COUNT
			; ++qid) {
			if (xpdev->qdata[qid].qhndl == vreq->qhndl) break;
		}
		if (qid == xpdev->qmax + CASH_VIDEO_QUEUE_COUNT) {
			return -ENODEV;
		}
		vreq->qid_reserved = qid - xpdev->qmax;
	}

	if (vreq->reserved) {
		unprepare_cash_qdma_video_request(vreq);
	} else if (vreq->buffer_chunks || vreq->buffer_chunk_dma_hndls) {
		pr_err_cash(
			"need to set NULL for buffer_chunks and buffer_chunk_dma_hndls");
			return -EINVAL;
	}

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req) {
		pr_err_cash("cannot allocate req");
		return -ENOMEM;
	}
	vreq->buffer_chunks 
		= kzalloc(vreq->buffer_chunk_count * sizeof(void*), GFP_KERNEL);
	vreq->buffer_chunk_dma_hndls 
		= kzalloc(vreq->buffer_chunk_count * sizeof(dma_addr_t), GFP_KERNEL);
	sgl = kzalloc(desc_count * (sizeof(struct qdma_sw_sg)), GFP_KERNEL);

	if (!sgl || !vreq->buffer_chunks || !vreq->buffer_chunk_dma_hndls) {
		pr_err_cash(
			"cannot allocate sgl, buffer_chunks and buffer_chunk_dma_hndls");
		rv = -ENOMEM;
		goto recover_req;
	}
	sgl_work = sgl;
	for (chunk_idx = 0; chunk_idx < vreq->buffer_chunk_count; ++chunk_idx) {
		if ((rv = setup_sgl_for_video_chunk(vreq, chunk_idx, &sgl_work))) {
			goto recover_chunks;
		}
		pr_debug("%d, sgl_work - sgl = %ld", chunk_idx, sgl_work - sgl);
	}
	pr_debug("sgl_work - sgl = %ld", sgl_work - sgl);
	sgl[sgl_work - sgl - 1].next = NULL;

	if ((rv = sgl_map(vreq->pdev, sgl, sgl_work - sgl, DMA_FROM_DEVICE))) {
		pr_err_cash("cannot sgl_map");
		goto recover_pages;
	}

	*req = CASH_VIDEO_READ_REQUEST_DEFAULT;	
	req->count = vreq->buffer_chunk_count * vreq->buffer_chunk_size;
	req->sgcnt = sgl_work - sgl;
	req->sgl = sgl;
	req->dma_mapped = 1;
	vreq->reserved = req;
	if (vreq->aio_done) {
		req->uld_data = (unsigned long)vreq;
		req->fp_done = fp_done_video;
	}
	return 0;

recover_chunks:
	while(chunk_idx--) {
		kfree(vreq->buffer_chunks[chunk_idx]);
		vreq->buffer_chunks[chunk_idx] = NULL;
	}
recover_pages:
	kfree(sgl);
recover_req:
	kfree(vreq->buffer_chunks);
	kfree(vreq->buffer_chunk_dma_hndls);
	kfree(req);

	return rv;
}

void unprepare_cash_qdma_video_request(struct cash_qdma_video_request* vreq) {
	uint32_t chunk_idx = vreq->buffer_chunk_count;
	sgl_unmap(vreq->pdev, vreq->reserved->sgl, vreq->reserved->sgcnt, DMA_FROM_DEVICE);
	vreq->reserved->dma_mapped = 0;

	while(chunk_idx--) {
		dma_free_coherent(&vreq->pdev->dev, vreq->buffer_chunk_size
			, vreq->buffer_chunks[chunk_idx], vreq->buffer_chunk_dma_hndls[chunk_idx]);
		vreq->buffer_chunks[chunk_idx] = NULL;
		vreq->buffer_chunk_dma_hndls[chunk_idx] = 0UL;
	}

	kfree(vreq->buffer_chunks);
	kfree(vreq->buffer_chunk_dma_hndls);
	kfree(vreq->reserved->sgl);
	kfree(vreq->reserved);
	vreq->reserved = NULL;
	vreq->buffer_chunks = NULL;
	vreq->buffer_chunk_dma_hndls = NULL;
}

ssize_t send_cash_qdma_video_request(struct cash_qdma_video_request* vreq) {
	uint32_t req_reg_addr 
		= CASH_SCRATCH_REG_V_REQ_COUNT_ADDR + 4 * vreq->qid_reserved;
	// struct timeval tv1, tv2;
	pr_debug("req_reg_addr=%x", req_reg_addr);

	// do_gettimeofday(&tv1);
	if (write_cash_user_reg(vreq->dev_hndl
		, req_reg_addr
		, read_cash_user_reg(vreq->dev_hndl, req_reg_addr) + 1)) {
		return -EIO;
	}
	// do_gettimeofday(&tv2);
	// display_time("before counter_inc@", &tv1);
	// display_time("after  counter_inc@", &tv2);
	// pr_info("counter_inc us=%ld", (tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec);
	return qdma_request_submit(vreq->dev_hndl, vreq->qhndl, vreq->reserved);
}

static
int fp_done(struct qdma_request *req, unsigned int bytes_done,
	int err) {
	struct cash_qdma_request* qreq = (void*)req->uld_data;
	qreq->aio_done(qreq->user_data, bytes_done, err);
	return 0;
}

static 
int setup_sgl_for_qdma_chunk(
	struct cash_qdma_request* qreq, uint32_t chunk_idx, struct qdma_sw_sg ** sgl_ptr) {
	size_t cs;
	char* ptr;
	struct qdma_sw_sg * sgl = *sgl_ptr;

	qreq->buffer_chunks[chunk_idx] = 
		dma_alloc_coherent(&qreq->pdev->dev, qreq->buffer_chunk_size
			, qreq->buffer_chunk_dma_hndls + chunk_idx, GFP_KERNEL);

	if (!qreq->buffer_chunks[chunk_idx]) {
		pr_err("dma_alloc_coherent failed");
		return -ENOMEM;
	}
	memset(qreq->buffer_chunks[chunk_idx], 0, qreq->buffer_chunk_size);
	cs = qreq->buffer_chunk_size;
	ptr = qreq->buffer_chunks[chunk_idx];

	while (cs) {
		uint32_t offset = offset_in_page(ptr);
		uint32_t nbytes = min_t(uint32_t, DATA_DESC_PAYLOAD_SIZE - offset, cs);

		sgl->pg = virt_to_page(ptr);
		sgl->offset = offset;
		sgl->len = nbytes;
		sgl->dma_addr = 0UL;
		sgl->next = sgl + 1;

		ptr += nbytes;
		cs -= nbytes;
		sgl++;
	}
	*sgl_ptr = sgl;
	return 0;
}

int prepare_cash_qdma_request(struct cash_qdma_request* qreq) {
	int chunk_idx = 0;
	int rv;
	unsigned int desc_count
		= (qreq->buffer_chunk_count * qreq->buffer_chunk_size + (DATA_DESC_PAYLOAD_SIZE - 1)) 
			/ DATA_DESC_PAYLOAD_SIZE + 3; //over allocate by 3
	struct qdma_sw_sg * sgl, * sgl_work;
	struct qdma_request *req = NULL;

	if (qreq->reserved) {
		unprepare_cash_qdma_request(qreq);
	} else if (qreq->buffer_chunks || qreq->buffer_chunk_dma_hndls) {
		pr_err_cash(
			"need to set NULL for buffer_chunks and buffer_chunk_dma_hndls");
			return -EINVAL;
	}

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req) {
		pr_err_cash("cannot allocate req");
		return -ENOMEM;
	}

	qreq->buffer_chunks
		= kzalloc(qreq->buffer_chunk_count * sizeof(void*), GFP_KERNEL);
	qreq->buffer_chunk_dma_hndls 
		= kzalloc(qreq->buffer_chunk_count * sizeof(dma_addr_t), GFP_KERNEL);
	sgl = kzalloc(desc_count * (sizeof(struct qdma_sw_sg)), GFP_KERNEL);

	if (!sgl || !qreq->buffer_chunks || !qreq->buffer_chunk_dma_hndls) {
		pr_err_cash(
			"cannot allocate sgl, buffer_chunks and buffer_chunk_dma_hndls");
		rv = -ENOMEM;
		goto recover_req;
	}
	sgl_work = sgl;
	for (chunk_idx = 0; chunk_idx < qreq->buffer_chunk_count; ++chunk_idx) {
		if ((rv = setup_sgl_for_qdma_chunk(qreq, chunk_idx, &sgl_work))) {
			goto recover_chunks;
		}
	}
	sgl[sgl_work - sgl - 1].next = NULL;

	if ((rv = sgl_map(qreq->pdev, sgl, sgl_work - sgl
		, qreq->is_write?DMA_TO_DEVICE:DMA_FROM_DEVICE))) {
		pr_err_cash("cannot sgl_map");
		goto recover_pages;
	}

	*req = CASH_QDMA_REQUEST_DEFAULT;	
	req->timeout_ms += (sgl_work - sgl) / 1000 * 1000;
	req->count = qreq->buffer_chunk_count * qreq->buffer_chunk_size;
	req->sgcnt = sgl_work - sgl;
	req->sgl = sgl;
	req->write = qreq->is_write?1:0;
	req->ep_addr = qreq->bram_addr;
	req->dma_mapped = 1;
	qreq->reserved = req;
	if (qreq->aio_done) {
		req->uld_data = (unsigned long)qreq;
		req->fp_done = fp_done;
	}

	return 0;

recover_chunks:
	while(chunk_idx--) {
		kfree(qreq->buffer_chunks[chunk_idx]);
		qreq->buffer_chunks[chunk_idx] = NULL;
	}
recover_pages:
	kfree(qreq->buffer_chunks);
	kfree(qreq->buffer_chunk_dma_hndls);
	kfree(sgl);
recover_req:
	kfree(req);

	return rv;
}


void unprepare_cash_qdma_request(struct cash_qdma_request* qreq) {
	uint32_t chunk_idx = qreq->buffer_chunk_count;
	sgl_unmap(qreq->pdev, qreq->reserved->sgl, qreq->reserved->sgcnt, DMA_FROM_DEVICE);
	qreq->reserved->dma_mapped = 0;

	while(chunk_idx--) {
		dma_free_coherent(&qreq->pdev->dev, qreq->buffer_chunk_size
			, qreq->buffer_chunks[chunk_idx], qreq->buffer_chunk_dma_hndls[chunk_idx]);
		qreq->buffer_chunks[chunk_idx] = NULL;
		qreq->buffer_chunk_dma_hndls[chunk_idx] = 0UL;
	}

	kfree(qreq->buffer_chunks);
	kfree(qreq->buffer_chunk_dma_hndls);
	kfree(qreq->reserved->sgl);
	kfree(qreq->reserved);
	qreq->reserved = NULL;
	qreq->buffer_chunks = NULL;
	qreq->buffer_chunk_dma_hndls = NULL;
}

ssize_t send_cash_qdma_request(struct cash_qdma_request* qreq) {
	return qdma_request_submit(qreq->dev_hndl, qreq->qhndl, qreq->reserved);
}

uint32_t read_cash_user_reg(unsigned long dev_hndl, uint32_t reg_addr) {
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(
		&((struct xlnx_dma_dev *)(dev_hndl))->conf.pdev->dev);
	u32 value;
	int rv = qdma_device_read_user_register(xpdev, reg_addr, &value);
	if (rv) {
		pr_err_cash("cannot read user reg, rv=%d", rv);
		return 0xffffffff;
	}
	return value;
}

int write_cash_user_reg(unsigned long dev_hndl, uint32_t reg_addr, uint32_t value) {
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(
		&((struct xlnx_dma_dev *)(dev_hndl))->conf.pdev->dev);
	return qdma_device_write_user_register(xpdev, reg_addr, value);
}
