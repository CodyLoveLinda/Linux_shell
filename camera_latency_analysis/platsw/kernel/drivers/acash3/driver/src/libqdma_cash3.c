#define pr_fmt(fmt) KBUILD_MODNAME ":%s: " fmt, __func__

#include "libqdma_cash3.h"
#include "qdma_mod.h"
#include "libqdma_export.h"

#include "libqdma_config.h"
#include "xdev.h" //probably needed
#include "version.h"
#include "acash_video.h"
#include "acash_spec.h"

#include "linux/cash3_scratch_registers.h"
#include "linux/cash3_host_cmds.h"
#include "linux/cash3_fpga_events.h"

#include "qdma_access/qdma_platform.h"
#include <linux/pci.h>
#include <linux/delay.h>

#undef pr_error_cash3
#define pr_err_cash3(...) {pr_err(__VA_ARGS__); pr_err("@%s:%d", __FILE__, __LINE__);}

// static
// void 
// display_time(char* msg, struct timeval* t) {
// 	struct tm broken;
// 	time_to_tm(t->tv_sec, 0, &broken);
// 	pr_info("%s%d:%d:%d:%ld\n", msg, broken.tm_hour, broken.tm_min, 
// 	                         broken.tm_sec, t->tv_usec);
// }

int cash3_qdma_get_handles(unsigned int idx
	, unsigned long * dev_hndl
	, unsigned long * video_qhndls
	, unsigned int    video_qhndls_count
	, unsigned long * host_cmd_qhndl
	, unsigned long * fpga_event_qhndl) {
	struct xlnx_pci_dev *xpdev 
		= xpdev_find_by_idx(idx, NULL, 0);
	int i = 0;
	struct xlnx_qdata *qdata = NULL;
	unsigned int qidx;

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
		qidx = ((autox_spec_t *)xpdev->spec)->queue.event.begin;
		*host_cmd_qhndl = xpdev->qdata[qidx].qhndl;
	}
	if (fpga_event_qhndl) {
		qidx = ((autox_spec_t *)xpdev->spec)->queue.event.begin;
		*fpga_event_qhndl = xpdev->qdata[qidx + xpdev->qmax].qhndl;
	}

	return 0;
}

int initialize_cash3_video(unsigned long dev_hndl) {
	return 0;
}

int clear_cash3_video_status(unsigned long dev_hndl, uint8_t video_stream_id, uint32_t req_init_count) 
	{
	uint32_t req_addr;
	uint32_t resp_addr;
	uint32_t video_state_shift = 4 * (video_stream_id % 8);
	uint32_t video_state, c1, c2;
	uint32_t video_state_reg;

	struct xlnx_pci_dev *xpdev = dev_get_drvdata(
		&((struct xlnx_dma_dev *)(dev_hndl))->conf.pdev->dev);
	autox_scratch_reg_addr_spec_t *scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;
	
	req_addr = scratch_reg->video_req_cnt.addr + 4 * video_stream_id;
	resp_addr = scratch_reg->video_resp_cnt.addr + 4 * video_stream_id;
	video_state_reg = read_cash3_user_reg(dev_hndl, scratch_reg->video_state.addr + (video_stream_id / 8) * 4);
	video_state = (video_state_reg >> video_state_shift) & 0xf;
	// pr_info(" clear_cash3_video_status   video_state=0x%x \n", video_state);
	if (video_state < CASH3_VIDEO_STATE_READY_RGB_2M
		|| video_state > CASH3_VIDEO_STATE_READY_UYVY_8M)
		return EHOSTDOWN;

	if (write_cash3_user_reg(dev_hndl, req_addr, req_init_count)
		|| write_cash3_user_reg(dev_hndl, resp_addr, 0)) {
		pr_err("write_cash3_user_reg  0x%X IO error",  resp_addr);
		return EIO;
	}

	// tested no need to sleep wait, still leave 1ms for now
	msleep_interruptible(1);
	c1 = read_cash3_user_reg(dev_hndl, req_addr);
	c2 = read_cash3_user_reg(dev_hndl, resp_addr); 
	// pr_info("~~~~~~~~~  clear_cash3_video_status  req_init_count 0x%x c1 0x%x   c2 0x%x \n", req_init_count, c1, c2);
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
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(
		&((struct xlnx_dma_dev *)(dev_hndl))->conf.pdev->dev);
	autox_scratch_reg_addr_spec_t *scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;
	uint32_t addr_low = read_cash3_user_reg(dev_hndl
		, scratch_reg->io_area.addr);
	*bram_addr = ((uint64_t)CASH3_IO_AREA_HIGH << 32ul) + addr_low;
	*size = CASH3_IO_AREA_BYTE_SIZE;
}

ssize_t send_host_cmd(unsigned long dev_hndl
	, struct cash3_host_cmd_t* cmd
	, size_t cmd_size
	, bool checkCmdState) {
	struct cash3_qdma_request qreq = {0};
	uint64_t ior_addr;
	uint32_t ior_size;
	int i, rv;
	size_t chunk_max = 2 * 1024 * 1024;
	struct qdma_dev_conf conf;
	unsigned long host_cmd_qhndl;
	struct xlnx_pci_dev *xpdev;
	// struct timeval tv;
	autox_scratch_reg_addr_spec_t *scratch_reg = NULL;
	int loop = 0;
	
	if (!cmd->head.cmd) return -EINVAL;

	read_io_area_conf(dev_hndl, &ior_addr, &ior_size);
	if (cmd_size > ior_size) return -EINVAL;
	rv = qdma_device_get_config(dev_hndl, &conf, NULL, 0);
	if (rv) 
		return -rv;
	xpdev = dev_get_drvdata(&conf.pdev->dev);
	scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;

	rv = cash3_qdma_get_handles(xpdev->idx, NULL
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
	rv = prepare_cash3_qdma_request(&qreq);
	if (rv) return -rv;
	// do_gettimeofday(&tv);
	// display_time("after prepare host cmd @", &tv);
	for (i = 0; i < qreq.buffer_chunk_count; ++i) {
		memcpy(qreq.buffer_chunks[i]
			, (char*)cmd + i * qreq.buffer_chunk_size
			, qreq.buffer_chunk_size);
	}

	rv = write_cash3_user_reg(dev_hndl, scratch_reg->cmd_status.addr, 0xffffffff);
	if (0 != rv) {
		pr_err("failed init cmd status regs rv=%d", rv);
		return -EIO;
	}
	rv = send_cash3_qdma_request(&qreq);
	unprepare_cash3_qdma_request(&qreq);
	pr_debug("cmd send_cash3_qdma_request rv=%d", rv);

	if (rv > 0)
	{
		int rv;
		rv = write_cash3_user_reg(dev_hndl, CASH3_FPGA_IRQ_STATUS_ADDR
			, CASH3_HOST_CMD_PENDING);

		if (rv) {
			pr_err("failed trigger fpga interrupt rv=%d", rv);
			return -EIO;
		}

		if (checkCmdState) {
			while (0xffffffff == read_cash3_user_reg(dev_hndl, scratch_reg->cmd_status.addr))
			{
				pr_debug("cmd status pending ...");
				loop++;
				yield();
			}
		}
		pr_info(" cmd status pending loop  %d ...  rv %d\n", loop, rv);
	}

	return rv;
}

// ssize_t get_fpga_event(unsigned long dev_hndl
// 	, struct cash3_fpga_event_t* event) {
// 	struct cash3_qdma_request* qreq;
// 	int rv;
// 	struct qdma_dev_conf conf;
// 	struct xlnx_pci_dev *xpdev;

// 	rv = qdma_device_get_config(dev_hndl, &conf, NULL, 0);
// 	if (rv) return -rv;
// 	xpdev = dev_get_drvdata(&conf.pdev->dev);
// 	qreq = xpdev->fpga_event_queue_request;
// 	rv = send_cash3_qdma_request(qreq);
// 	if (rv == qreq->buffer_chunk_size) {
// 		memcpy(event, qreq->buffer_chunks[0]
// 			, qreq->buffer_chunk_size);
// 	}
// 	return rv;
// }

struct qdma_request const CASH3_VIDEO_READ_REQUEST_DEFAULT = {
	.timeout_ms = 1 * 1000, //1 sec
};

struct qdma_request const CASH3_QDMA_REQUEST_DEFAULT = {
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
	struct cash3_qdma_video_request* vreq = (void*)req->uld_data;
	vreq->aio_done(vreq->user_data, bytes_done, err);
	return 0;
}

static uint32_t const DATA_DESC_PAYLOAD_SIZE = 60 * 1024;

static 
int setup_sgl_for_video_chunk(
	struct cash3_qdma_video_request* vreq, uint32_t chunk_idx, struct qdma_sw_sg ** sgl_ptr) {
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

static
int calculate_video_mem_usage(unsigned long dev_hndl
	, uint16_t qid
	, size_t* buffer_chunk_count
	, size_t* buffer_chunk_size) 
{
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(&((struct xlnx_dma_dev *)(dev_hndl))->conf.pdev->dev);
	autox_scratch_reg_addr_spec_t *scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;
	uint32_t video_state_shift =  4 * (qid % 8);
	uint32_t video_state = 0;
	uint32_t video_state_addr = 0;
	uint32_t video_state_reg = 0;
	uint32_t const cash3_qdma_buffer_sizes[] = {
	    0,
	    CASH3_VIDEO_BUFFER_SIZE_1_RGB_2MP,  // Video RGB_2MP, 1920x1080x3=6220800, 6MB
	    CASH3_VIDEO_BUFFER_SIZE_2_YUYV_2MP, // Video YUYV_2MP, 1920x1080x2=4147200, 4MB
	    CASH3_VIDEO_BUFFER_SIZE_3_RGB_8MP,  // Video RGB_8MP, 3840x2160x3=24883200, 24MB
	    CASH3_VIDEO_BUFFER_SIZE_4_YUYV_8MP, // Video YUYV_8MP, 3840x2160x2=16588800, 16MB
	    CASH3_VIDEO_BUFFER_SIZE_5_RAW_8MP,  // Video RAW_8MP, 3840x2160x1.5=12441600, 12MB
	    CASH3_VIDEO_BUFFER_SIZE_6_UYVY_2MP, // Video UYVY_2MP, 1920x1080x2=4147200, 4MB
	    CASH3_VIDEO_BUFFER_SIZE_7_UYVY_8MP  // Video UYVY_8MP, 3840x2160x2=16588800, 16MB
	};
	uint32_t const sizes_count 
		= sizeof(cash3_qdma_buffer_sizes) / sizeof(cash3_qdma_buffer_sizes[0]);

	video_state_addr = scratch_reg->video_state.addr + (qid / 8) * 4;
	video_state_reg = read_cash3_user_reg(dev_hndl, video_state_addr);
	video_state = (video_state_reg >> video_state_shift) & 0xf;
	pr_info("  calculate_video_mem_usage   video_state_reg 0x%x  qid %d  video_state %x\n", 
			 video_state_reg, qid, video_state);

	if (!video_state || video_state >= sizes_count) return EHOSTDOWN;
	*buffer_chunk_size = (2 * 1024 * 1024);
	*buffer_chunk_count = cash3_qdma_buffer_sizes[video_state] / *buffer_chunk_size;
	pr_debug("buffer_chunk_size=%ld buffer_chunk_count=%ld\n", *buffer_chunk_size, *buffer_chunk_count);
	return 0;
}

int prepare_cash3_qdma_video_request(struct cash3_qdma_video_request* vreq) {
	int chunk_idx = 0;
	int rv = 0;
	uint16_t qid;
	unsigned int desc_count = 0;
	struct qdma_sw_sg * sgl, * sgl_work;
	struct qdma_request *req = NULL;
	autox_video_t *avideo = NULL;
	autox_queue_spec_t *queue_spec = NULL;

	//pr_info("   ~~~~~~  prepare_cash3_qdma_video_request   vreq %p \n", vreq);
	if (vreq == NULL)
		return -ENODEV;

	avideo = (autox_video_t* )vreq->avideo;
	//pr_info("   ~~~~~~  prepare_cash3_qdma_video_request   avideo %p \n", avideo);
	if (avideo)
		mutex_lock(&avideo->mlock);

	{// figure out the q_id from qhdnl
		struct qdma_dev_conf qdev_conf;
		struct device *dev;
		struct xlnx_pci_dev *xpdev;
		rv = qdma_device_get_config(vreq->dev_hndl, &qdev_conf, NULL, 0);
		if (rv) 
		{
			if (avideo)
				mutex_unlock(&avideo->mlock);
			return -ENODEV;
		}

		dev = &qdev_conf.pdev->dev;
		xpdev = dev_get_drvdata(dev);
		queue_spec = &((autox_spec_t *)xpdev->spec)->queue;

		for (qid = xpdev->qmax
			; qid < xpdev->qmax + queue_spec->video.count
			; ++qid) {
			if (xpdev->qdata[qid].qhndl == vreq->qhndl) break;
		}
		if (qid == xpdev->qmax + queue_spec->video.count)
		{
			if (avideo)
				mutex_unlock(&avideo->mlock);
			return -ENODEV;
		}
		vreq->qid_reserved = qid - xpdev->qmax;
		rv = calculate_video_mem_usage(vreq->dev_hndl, vreq->qid_reserved, 
				&vreq->buffer_chunk_count, &vreq->buffer_chunk_size);
		if (rv) 
		{
			if (avideo)
				mutex_unlock(&avideo->mlock);
			return -ENODEV;
		}
		desc_count = 
			(vreq->buffer_chunk_count * vreq->buffer_chunk_size + (DATA_DESC_PAYLOAD_SIZE - 1))
				/ DATA_DESC_PAYLOAD_SIZE + 3; //over allocate by 3
	}

	if (vreq->reserved)
	{
		if (avideo)
			mutex_unlock(&avideo->mlock);
		unprepare_cash3_qdma_video_request(vreq);
		if (avideo)
			mutex_lock(&avideo->mlock);
	} 
	else if (vreq->buffer_chunks || vreq->buffer_chunk_dma_hndls) {
		pr_err_cash3("need to set NULL for buffer_chunks and buffer_chunk_dma_hndls");
		if (avideo)
			mutex_unlock(&avideo->mlock);
		return -EINVAL;
	}

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req) {
		pr_err_cash3("cannot allocate req");
		if (avideo)
			mutex_unlock(&avideo->mlock);
		return -ENOMEM;
	}
	vreq->buffer_chunks 
		= kzalloc(vreq->buffer_chunk_count * sizeof(void*), GFP_KERNEL);
	vreq->buffer_chunk_dma_hndls 
		= kzalloc(vreq->buffer_chunk_count * sizeof(dma_addr_t), GFP_KERNEL);
	sgl = kzalloc(desc_count * (sizeof(struct qdma_sw_sg)), GFP_KERNEL);

	if (!sgl || !vreq->buffer_chunks || !vreq->buffer_chunk_dma_hndls) {
		pr_err_cash3(
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
		pr_err_cash3("cannot sgl_map");
		goto recover_pages;
	}

	*req = CASH3_VIDEO_READ_REQUEST_DEFAULT;	
	req->count = vreq->buffer_chunk_count * vreq->buffer_chunk_size;
	req->sgcnt = sgl_work - sgl;
	req->sgl = sgl;
	req->dma_mapped = 1;
	vreq->reserved = req;
	if (vreq->aio_done) {
		req->uld_data = (unsigned long)vreq;
		req->fp_done = fp_done_video;
	}
	if (avideo)
		mutex_unlock(&avideo->mlock);
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
	if (avideo)
		mutex_unlock(&avideo->mlock);
	return rv;
}

void unprepare_cash3_qdma_video_request(struct cash3_qdma_video_request* vreq) {

	uint32_t chunk_idx;
	autox_video_t *avideo = NULL;;
	if (vreq == NULL)
		return;

	avideo = (autox_video_t* )vreq->avideo;
	if(avideo)
		mutex_lock(&avideo->mlock);

	chunk_idx = vreq->buffer_chunk_count;
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

	if(avideo)
		mutex_unlock(&avideo->mlock);
}


ssize_t send_cash3_qdma_video_request(struct cash3_qdma_video_request* vreq)
{
	struct xlnx_pci_dev *xpdev;
	uint32_t req_reg_addr = 0;
	struct qdma_dev_conf conf;
	autox_scratch_reg_addr_spec_t *scratch_reg = NULL;
	int rv = qdma_device_get_config(vreq->dev_hndl, &conf, NULL, 0);
	if (rv) return -rv;
	xpdev = dev_get_drvdata(&conf.pdev->dev);

	scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;
	req_reg_addr = scratch_reg->video_req_cnt.addr +
		4 * vreq->qid_reserved;
	// struct timeval tv1, tv2;
	pr_debug("req_reg_addr=%x", req_reg_addr);

	// do_gettimeofday(&tv1);
	if (write_cash3_user_reg(vreq->dev_hndl
		, req_reg_addr
		, read_cash3_user_reg(vreq->dev_hndl, req_reg_addr) + 1)) {
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
	struct cash3_qdma_request* qreq = (void*)req->uld_data;
	qreq->aio_done(qreq->user_data, bytes_done, err);
	return 0;
}

static 
int setup_sgl_for_qdma_chunk(
	struct cash3_qdma_request* qreq, uint32_t chunk_idx, struct qdma_sw_sg ** sgl_ptr) {
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

int prepare_cash3_qdma_request(struct cash3_qdma_request* qreq) {
	int chunk_idx = 0;
	int rv;
	unsigned int desc_count
		= (qreq->buffer_chunk_count * qreq->buffer_chunk_size + (DATA_DESC_PAYLOAD_SIZE - 1)) 
			/ DATA_DESC_PAYLOAD_SIZE + 3; //over allocate by 3
	struct qdma_sw_sg * sgl, * sgl_work;
	struct qdma_request *req = NULL;

	if (qreq->reserved) {
		unprepare_cash3_qdma_request(qreq);
	} else if (qreq->buffer_chunks || qreq->buffer_chunk_dma_hndls) {
		pr_err_cash3(
			"need to set NULL for buffer_chunks and buffer_chunk_dma_hndls");
			return -EINVAL;
	}

	req = kzalloc(sizeof(*req), GFP_KERNEL);
	if (!req) {
		pr_err_cash3("cannot allocate req");
		return -ENOMEM;
	}

	qreq->buffer_chunks
		= kzalloc(qreq->buffer_chunk_count * sizeof(void*), GFP_KERNEL);
	qreq->buffer_chunk_dma_hndls 
		= kzalloc(qreq->buffer_chunk_count * sizeof(dma_addr_t), GFP_KERNEL);
	sgl = kzalloc(desc_count * (sizeof(struct qdma_sw_sg)), GFP_KERNEL);

	if (!sgl || !qreq->buffer_chunks || !qreq->buffer_chunk_dma_hndls) {
		pr_err_cash3(
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
		pr_err_cash3("cannot sgl_map");
		goto recover_pages;
	}

	*req = CASH3_QDMA_REQUEST_DEFAULT;	
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


void unprepare_cash3_qdma_request(struct cash3_qdma_request* qreq) {
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

ssize_t send_cash3_qdma_request(struct cash3_qdma_request* qreq) {
	return qdma_request_submit(qreq->dev_hndl, qreq->qhndl, qreq->reserved);
}

uint32_t read_cash3_user_reg(unsigned long dev_hndl, uint32_t reg_addr) {
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(
		&((struct xlnx_dma_dev *)(dev_hndl))->conf.pdev->dev);
	u32 value;
	int rv = qdma_device_read_user_register(xpdev, reg_addr, &value);
	if (rv) {
		pr_err_cash3("cannot read user reg, rv=%d", rv);
		return 0xffffffff;
	}
	return value;
}

int write_cash3_user_reg(unsigned long dev_hndl, uint32_t reg_addr, uint32_t value) {
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(
		&((struct xlnx_dma_dev *)(dev_hndl))->conf.pdev->dev);
	return qdma_device_write_user_register(xpdev, reg_addr, value);
}
