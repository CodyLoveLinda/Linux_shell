#include "qdma_mod.h"
#include "libqdma_cash3.h"
#include "acash_video.h"
#include "acash_spec.h"

#include "qdma_access/qdma_platform.h"
#include "linux/cash3_scratch_registers.h"
#include "linux/cash3_host_cmds.h"
#include "linux/cash3_fpga_events.h"
#include "linux/cash3_meta.h"
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <linux/crc32.h>
#include <linux/version.h>

// #undef CASH3_VIDEO_QUEUE_COUNT
// #define CASH3_VIDEO_QUEUE_COUNT 3

// static u64 const VIDEO_BUFFER_START_BRAM_ADDR = 0x801000000ul;
static uint32_t const TEST_IO_Q_ID_C8 = 16;
static uint32_t const TEST_IO_Q_ID_C16 = 24;
static int cash3_qdma_test_error = 0;

#undef QDMA_CASH3_EXPECT
#define QDMA_CASH3_EXPECT(x) if (!(x)) {\
	pr_err("QDMA_CASH3_EXPECT %s is false @%s:%d\n", (#x), __FILE__, __LINE__); \
	cash3_qdma_test_error |= -1;} else {\
	pr_debug("QDMA_CASH3_CHECKED %s @%s:%d\n", (#x), __FILE__, __LINE__); \
    }

#undef QDMA_CASH3_ASSERT
#define QDMA_CASH3_ASSERT(x) if (!(x)) {\
    pr_err("QDMA_CASH3_ASSERT %s is false @%s:%d\n", (#x), __FILE__, __LINE__); \
    cash3_qdma_test_error |= -1; return;} else {\
	pr_debug("QDMA_CASH3_CHECKED %s @%s:%d\n", (#x), __FILE__, __LINE__); \
    }

// #undef CASH3_VIDEO_QUEUE_COUNT
// #define CASH3_VIDEO_QUEUE_COUNT 2

// static
// int log_cash3_fpga_events_func(void* dev) {
// 	while (!kthread_should_stop()) {
// 		unsigned long dev_hndl = (unsigned long)dev;
// 		struct cash3_fpga_event_t event;
// 		if (0 <= get_fpga_event(dev_hndl, &event)) {
// 			switch (event.head.event) {
// 				case CASH3_FPGA_EVENT_VIDEO_STATUS_CHANGED: {
// 					pr_info("CASH3_FPGA_EVENT_VIDEO_STATUS_CHANGED\n");
// 				} break;
// 				case CASH3_FPGA_EVENT_IMAGE_FLASHED: {
// 					pr_info("CASH3_FPGA_EVENT_IMAGE_FLASHED\n");
// 				} break;
// 			}
// 		} else {
// 			pr_debug("no event read");
// 		}
// 		schedule();
// 	}
// 	pr_info("CASH3_FPGA_EVENT read stopping\n");
// 	do_exit(0);
// }

// __attribute__ ((unused))
// static 
// struct task_struct *
// log_cash3_fpga_events(unsigned long dev_hndl) {
// 	if (log_cash3_fpga_events_thread == NULL) {
// 		char const our_thread[]="fpga_event_log";
// 		log_cash3_fpga_events_thread 
// 			= kthread_create(log_cash3_fpga_events_func, (void*)dev_hndl
// 			, our_thread);
// 		wake_up_process(log_cash3_fpga_events_thread);
// 	}
// 	return log_cash3_fpga_events_thread;
// }

__attribute__ ((unused))
static
void dump_regs(unsigned long dev_hndl) {
{
	uint32_t reg_addr = 0x0;
	for (; reg_addr <= 0xfc; reg_addr += 4) {
		pr_info("0x%08X => %08x", reg_addr, qdma_reg_read((void*)dev_hndl, reg_addr));
	}
}
{
	uint32_t reg_addr = 0x100;
	for (; reg_addr <= 0x1cc; reg_addr += 4) {
		pr_info("0x%08X => %08x", reg_addr, qdma_reg_read((void*)dev_hndl, reg_addr));
	}
}
{
	uint32_t reg_addr = 0x204;
	for (; reg_addr <= 0x2c4; reg_addr += 4) {
		pr_info("0x%08X => %08x", reg_addr, qdma_reg_read((void*)dev_hndl, reg_addr));
	}
}
{
	uint32_t reg_addr = 0x800;
	for (; reg_addr <= 0x844; reg_addr += 4) {
		pr_info("0x%08X => %08x", reg_addr, qdma_reg_read((void*)dev_hndl, reg_addr));
	}
}
{
	uint32_t reg_addr = 0xa00;
	for (; reg_addr <= 0xbf4; reg_addr += 4) {
		pr_info("0x%08X => %08x", reg_addr, qdma_reg_read((void*)dev_hndl, reg_addr));
	}
}
{
	uint32_t reg_addr = 0xe00;
	for (; reg_addr <= 0xe28; reg_addr += 4) {
		pr_info("0x%08X => %08x", reg_addr, qdma_reg_read((void*)dev_hndl, reg_addr));
	}
}
{
	uint32_t reg_addr = 0x1000;
	for (; reg_addr <= 0x10e8; reg_addr += 4) {
		pr_info("0x%08X => %08x", reg_addr, qdma_reg_read((void*)dev_hndl, reg_addr));
	}
}
{
	uint32_t reg_addr = 0x1200;
	for (; reg_addr <= 0x12ec; reg_addr += 4) {
		pr_info("0x%08X => %08x", reg_addr, qdma_reg_read((void*)dev_hndl, reg_addr));
	}
}	
}


//#if KERNEL_VERSION(5, 0, 0) <= LINUX_VERSION_CODE		
//static
//void ktime_get_ts64(struct timespec64 *tv) {
//	struct timespec64 ts;
//	ktime_get_real_ts64(&ts);
//	tv->tv_sec = ts.tv_sec;
//	tv->tv_usec = ts.tv_nsec / 1000;
//}
//#endif

static
void 
display_time(char* msg, struct timespec64* t) {
	struct tm broken;

	time64_to_tm(t->tv_sec, 0, &broken);

	pr_info("%s%d:%d:%d:%ld\n", msg, broken.tm_hour, broken.tm_min, 
	                         broken.tm_sec, t->tv_nsec);
}

/******************************************************************************/
/**
 * set_cash3_checksum() -  setup checksum (XOR) on an array of memory chunks
 * @chunks: chunk array
 * @chunk_count: array length
 * @chunk_size: each chunk byte size
 * 
 * @return	the XOR checksum which is calculated by XOR all words except the
 * last and put the last as the last word in last chunk
 *****************************************************************************/
static
uint32_t 
set_cash3_checksum(void** chunks, size_t chunk_count, size_t chunk_size) {
	size_t i, j;
	uint32_t checksum = 0, *addr = 0;
	for (i = 0; i < chunk_count; ++i) {
		addr = chunks[i];
		for (j = 0; j < chunk_size / sizeof(uint32_t); ++j) {
			if (i != chunk_count - 1
				|| j != chunk_size / sizeof(uint32_t) - 1) {
				checksum ^= *(addr++);
			}
		}
	}
	*addr = checksum;
	return checksum;
}

/******************************************************************************/
/**
 * verify_cash3_checksum() -  verify checksum (XOR) on an array of memory chunks
 * It cannot be 0 to avoid empty buffer related bug
 * @chunks: chunk array
 * @chunk_count: array length
 * @chunk_size: each chunk byte size
 * 
 * @return	the XOR checksum
 *****************************************************************************/
static
uint32_t 
verify_cash3_checksum(void** chunks, size_t chunk_count, size_t chunk_size) {
	size_t i, j;
	uint32_t checksum = 0, *addr = 0;
	for (i = 0; i < chunk_count; ++i) {
		addr = chunks[i];
		for (j = 0; j < chunk_size / sizeof(uint32_t); ++j) {
			checksum ^= *(addr++);
		}
	}
	QDMA_CASH3_EXPECT(0 == checksum);
	if (checksum == 0) {
		QDMA_CASH3_EXPECT(0 != *(addr - 1));
	}
	return *(addr - 1);
}

/******************************************************************************/
/**
 * setup_tests() -  prepare to run test cases
 * 
 *****************************************************************************/
static
void
setup_tests(struct device *dev) {
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(dev);
	autox_queue_spec_t *queue_spec = &((autox_spec_t *)xpdev->spec)->queue;
	autox_scratch_reg_addr_spec_t *scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;
	uint32_t i;
	int rv;
	uint32_t verInt = 
		read_cash3_user_reg(xpdev->dev_hndl, scratch_reg->fpga_ver.addr);
	struct cash3_fpga_manifest_t* ver = (void*)&verInt;
	// uint32_t crc_buf[] = {0x1234, 0x5678, 0x9abc, 0xdef0, 0x1234, 0x5678, 0xbeef, 0xdead};

	pr_info("cash3_fw_ver=%d file_fw_ver=%d, hw_ver=%d.%d"
		, ver->fw_ver
		, ver->backup_fw_ver
		, ver->hw_major_ver
		, ver->hw_minor_ver);

	// QDMA_CASH3_EXPECT(NULL != log_cash3_fpga_events(xpdev->dev_hndl));
	QDMA_CASH3_EXPECT(0 == initialize_cash3_video(xpdev->dev_hndl));
	for (i = 0; i < queue_spec->video.count; ++i) {
		rv = clear_cash3_video_status(xpdev->dev_hndl, i, 0);
		pr_info("video stream status %d = %d", i, rv);
		QDMA_CASH3_ASSERT(0 == rv);
	}

}

/******************************************************************************/
/**
 * tear_down_tests() -  undo setup_tests
 *****************************************************************************/
static
void
tear_down_tests(struct device *dev) {
	// struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	// if (log_cash3_fpga_events_thread != NULL) {
	// 	QDMA_CASH3_ASSERT(!kthread_stop(log_cash3_fpga_events_thread));
	// 	log_cash3_fpga_events_thread = NULL;
	// }
	// uninitialize_cash3_video(xpdev->dev_hndl);
}

/******************************************************************************/
/**
 * test cases starting 
 * 
 *****************************************************************************/
static 
void 
test_read_register(struct device *dev) {
	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	autox_scratch_reg_addr_spec_t *scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;
	uint32_t res = qdma_reg_read((void*)xpdev->dev_hndl, 0);
	pr_debug("res = %x", res);
	/**
	*structure of res:
	* identifier               : bit20~bit31:DMA subsystem for PCIe identifier,reset_value= 12'h1fd
	* Config_block_identifier  : bit16~bit19:Config Identifier,reset_value=3
	* Reserved                 : bit8 ~bit15:Reserved,reset_value=0
	* version                  : bit0 ~bit7 :version number, 0(Vivado 2019) 1(Vivado 2020)
	*/
	QDMA_CASH3_EXPECT(res == 0x1fD30000 || res == 0x1fD30001);

	res = read_cash3_user_reg(xpdev->dev_hndl, 0);
	pr_debug("hw ver = 0x%x", res);

	QDMA_CASH3_EXPECT(0 
		== write_cash3_user_reg(xpdev->dev_hndl
			, scratch_reg->cmd_status.addr, 0xffffffff));
	
	QDMA_CASH3_EXPECT(0xffffffff 
		== read_cash3_user_reg(xpdev->dev_hndl
			, scratch_reg->cmd_status.addr));
}

static 
void 
test_allocate_mem(struct device *dev) {
	size_t count = 2 * 1024 * 1024;
	dma_addr_t dma_hndl;
	void* res = dma_alloc_coherent(dev, count, &dma_hndl, GFP_KERNEL);
	QDMA_CASH3_EXPECT(res != NULL);
	dma_free_coherent(dev, count, res, dma_hndl);
	res = kmalloc(count, GFP_KERNEL|GFP_DMA);
	QDMA_CASH3_EXPECT(res != NULL);
	kfree(res);
}

static 
void 
test_cash3_qdma_get_handles(struct device *dev) {
	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	unsigned long dev_hndl, expected_qhndl, host_cmd_qhndl
		, fpga_event_qhndl
		, video_qhndls[CASH3_VIDEO_QUEUE_COUNT_MAX] = {0};
	QDMA_CASH3_EXPECT(0 == cash3_qdma_get_handles(
		xpdev->idx, &dev_hndl, video_qhndls, 1, &host_cmd_qhndl
		, &fpga_event_qhndl));
	QDMA_CASH3_EXPECT(0 != host_cmd_qhndl);
	QDMA_CASH3_EXPECT(0 != fpga_event_qhndl);

	QDMA_CASH3_EXPECT(xpdev->dev_hndl == dev_hndl);
	expected_qhndl = xpdev->qdata[xpdev->qmax].qhndl;
	QDMA_CASH3_EXPECT(expected_qhndl == video_qhndls[0]);
}

static 
void 
test_prepare_unprepare_qdma(struct device *dev) {
	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	int rv = 0;
	struct cash3_qdma_request* qreq = kmalloc(sizeof(*qreq), GFP_KERNEL);

	memset(qreq, 0, sizeof(*qreq));
	qreq->pdev = xpdev->pdev;
	qreq->buffer_chunk_count = 100;
	qreq->buffer_chunk_size = 2 * 1024 * 1024;
	qreq->is_write=true;

	rv = cash3_qdma_get_handles(xpdev->idx, &qreq->dev_hndl, NULL, 0
		, &qreq->qhndl, NULL);
	QDMA_CASH3_ASSERT(rv == 0);
	rv = prepare_cash3_qdma_request(qreq);
	QDMA_CASH3_ASSERT(rv == 0);
	unprepare_cash3_qdma_request(qreq);
	kfree(qreq);
}

static 
void 
test_prepare_unprepare_video(struct device *dev) {
	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	int rv = 0;
	struct cash3_qdma_video_request* vreq = kmalloc(sizeof(*vreq), GFP_KERNEL);

	memset(vreq, 0, sizeof(*vreq));
	vreq->pdev = xpdev->pdev;
	rv = cash3_qdma_get_handles(xpdev->idx, &vreq->dev_hndl, &vreq->qhndl, 1
		, NULL, NULL);
	QDMA_CASH3_EXPECT(rv == 0);
	vreq->avideo = xpdev->videos;
	rv = prepare_cash3_qdma_video_request(vreq);
	pr_debug("prepare_cash3_qdma_video_request= %d", rv);
	QDMA_CASH3_ASSERT(rv == 0);
	QDMA_CASH3_EXPECT(vreq->buffer_chunk_size == 2 * 1024 * 1024);

	unprepare_cash3_qdma_video_request(vreq);
	kfree(vreq);
}

static
void
test_io_area(struct device *dev) {
	struct cash3_qdma_request qreq = {0};
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(dev);
	int i;
	uint32_t test_queue_idx = TEST_IO_Q_ID_C8;

	uint32_t checksum;
	uint64_t ior_addr;
	uint32_t ior_size;
	unsigned long dev_hndl, host_cmd_qhndl;
	struct timespec64 tv, tv2;

	if (xpdev->video_cnt > 8) {
		test_queue_idx = TEST_IO_Q_ID_C16;
	}

	read_io_area_conf(xpdev->dev_hndl, &ior_addr, &ior_size);

	QDMA_CASH3_EXPECT(ior_size > 100 * 1024 * 1024);
	QDMA_CASH3_EXPECT(ior_addr);
	QDMA_CASH3_EXPECT(0 == cash3_qdma_get_handles(
		xpdev->idx, &dev_hndl, NULL, 0, &host_cmd_qhndl
		, NULL));
	QDMA_CASH3_EXPECT(dev_hndl == xpdev->dev_hndl);
	pr_info("ior_addr=%llx", ior_addr);
	QDMA_CASH3_ASSERT(0 != host_cmd_qhndl);

	qreq.dev_hndl = xpdev->dev_hndl;
	qreq.buffer_chunk_count = 100;
	qreq.buffer_chunk_size = 2 * 1024 * 1024;
	qreq.pdev = xpdev->pdev;
	qreq.bram_addr = ior_addr;
	qreq.qhndl = host_cmd_qhndl;
	qreq.is_write = true;
	QDMA_CASH3_EXPECT(0 == prepare_cash3_qdma_request(&qreq));
	pr_debug("ior_addr 2=%llx", ior_addr);
	for (i = 0; i < qreq.buffer_chunk_count; ++i) {
		get_random_bytes(qreq.buffer_chunks[i], qreq.buffer_chunk_size);
	}
	checksum = set_cash3_checksum(qreq.buffer_chunks
		, qreq.buffer_chunk_count, qreq.buffer_chunk_size);
	QDMA_CASH3_EXPECT(qreq.buffer_chunk_count * qreq.buffer_chunk_size 
		== send_cash3_qdma_request(&qreq));
	// dump_regs(dev_hndl);

	// now clear and read it back
	for (i = 0; i < qreq.buffer_chunk_count; ++i) {
		memset(qreq.buffer_chunks[i], 0, qreq.buffer_chunk_size);
	}
	qreq.is_write = false;
	qreq.qhndl = xpdev->qdata[test_queue_idx  + xpdev->qmax].qhndl;
	QDMA_CASH3_ASSERT(0 != qreq.qhndl);
	QDMA_CASH3_EXPECT(0 == prepare_cash3_qdma_request(&qreq));

	for (i = 0; i < 100; ++i) {
		ktime_get_ts64(&tv);
		QDMA_CASH3_EXPECT(qreq.buffer_chunk_count * qreq.buffer_chunk_size 
			== send_cash3_qdma_request(&qreq));
		ktime_get_ts64(&tv2);

		display_time("test_io_area sync read@", &tv);
		display_time("test_io_area sync recv@", &tv2);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 18, 0)
		pr_info("test_io_area sync us=%lld", ((tv2.tv_sec - tv.tv_sec) * NSEC_PER_SEC + (tv2.tv_nsec - tv.tv_nsec) / NSEC_PER_USEC));
#else
		pr_info("test_io_area sync us=%ld", ((tv2.tv_sec - tv.tv_sec) * NSEC_PER_SEC + (tv2.tv_nsec - tv.tv_nsec) / NSEC_PER_USEC));
#endif

		QDMA_CASH3_EXPECT(checksum == verify_cash3_checksum(qreq.buffer_chunks
			, qreq.buffer_chunk_count, qreq.buffer_chunk_size));
	}
	unprepare_cash3_qdma_request(&qreq);
}

static 
void 
test_sync_read_from_video_queue_zero(struct device *dev) {
	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	int rv = 0;
	unsigned long dev_hndl, qhndl, video_qhndls[1] = {0};
	struct cash3_qdma_video_request vreq = {0};
	uint32_t count = 0;
	struct timespec64 tv, tv2;
	int i;

	vreq.pdev = xpdev->pdev;
	/* Clear the registers of req and resp will cause cam0 to slow down by one frame */
	// crv = clear_cash3_video_status(xpdev->dev_hndl, 0, 4);
	// pr_info("video stream status %d = %d", 0, rv);

	rv = cash3_qdma_get_handles(xpdev->idx, &dev_hndl, video_qhndls, 1
		, NULL, NULL);
	QDMA_CASH3_EXPECT(rv == 0);
	qhndl = video_qhndls[0];
	vreq.dev_hndl = dev_hndl;
	vreq.qhndl = video_qhndls[0];
	vreq.avideo = xpdev->videos;
	rv = prepare_cash3_qdma_video_request(&vreq);
	pr_debug("prepare_cash3_qdma_video_request= %d", rv);
	QDMA_CASH3_ASSERT(rv == 0);
	count = vreq.buffer_chunk_count * vreq.buffer_chunk_size;
	// qdma_sw_sg_dump(vreq.reserved->sgl);
	msleep_interruptible(200);

	for (i = 0; i < 4; i++) {
		ktime_get_ts64(&tv);
		QDMA_CASH3_EXPECT(count == send_cash3_qdma_video_request(&vreq));
		ktime_get_ts64(&tv2);
		display_time("test_sync_read_from_video_queue_zero sent@", &tv);
		display_time("test_sync_read_from_video_queue_zero recv@", &tv2); 
		msleep_interruptible(100);
	}

	QDMA_CASH3_EXPECT( 0 == ((uint32_t*)(vreq.buffer_chunks[0]))[0]);
	unprepare_cash3_qdma_video_request(&vreq);
}

static 
void
aio_done_async_read_from_video_queue_zero(
	unsigned long user_data, unsigned int bytes_done, int err) {
	struct cash3_qdma_video_request** vreq_ptr = (void*)user_data;
	struct cash3_qdma_video_request* vreq = *vreq_ptr;

	pr_debug("%s: bytes_done=%d, err=%d, buffer_chunk_count=%ld, buffer_chunk_size=%ld"
		, __FUNCTION__, bytes_done, err, vreq->buffer_chunk_count, vreq->buffer_chunk_size);
	QDMA_CASH3_EXPECT(err == 0);
	QDMA_CASH3_EXPECT(bytes_done == vreq->buffer_chunk_count *
		vreq->buffer_chunk_size);
	pr_debug("==free1===\n");
	unprepare_cash3_qdma_video_request(vreq);
	pr_debug("==free2===\n");
	kfree(vreq);
	*vreq_ptr = NULL;
}

static 
void 
test_async_read_from_video_queue_zero(struct device *dev) {
	struct cash3_qdma_video_request* vreq = NULL;
	{
		struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
		int rv = 0;
		unsigned long dev_hndl, qhndl, video_qhndls[1] = {0};
		vreq = kzalloc(sizeof(*vreq), GFP_KERNEL);
		vreq->user_data = (unsigned long)&vreq;
		vreq->aio_done = aio_done_async_read_from_video_queue_zero;
		vreq->pdev = xpdev->pdev;

		rv = cash3_qdma_get_handles(xpdev->idx, &dev_hndl, video_qhndls, 1
			, NULL, NULL);
		qhndl = video_qhndls[0];
		vreq->dev_hndl = dev_hndl;
		vreq->qhndl = video_qhndls[0];
		vreq->avideo = xpdev->videos;
		QDMA_CASH3_EXPECT(0 == prepare_cash3_qdma_video_request(vreq));

		pr_debug("==ready to submit===\n");
		QDMA_CASH3_EXPECT(0 == send_cash3_qdma_video_request(vreq));

		pr_debug("==done submit===\n");
	}
	msleep_interruptible(1000);
	QDMA_CASH3_EXPECT(vreq == NULL);
	pr_debug("vreq=%p\n", vreq);
}

static 
void 
test_async_read_large_from_video_queue_zero(struct device *dev) {
	struct cash3_qdma_video_request* vreq = NULL;
	{
		struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
		int rv = 0;
		unsigned long dev_hndl, qhndl, video_qhndls[1] = {0};
		vreq = kzalloc(sizeof(*vreq), GFP_KERNEL);
		vreq->user_data = (unsigned long)&vreq;
		vreq->aio_done = aio_done_async_read_from_video_queue_zero;
		vreq->pdev = xpdev->pdev;

		rv = cash3_qdma_get_handles(xpdev->idx, &dev_hndl, video_qhndls, 1
			, NULL, NULL);
		qhndl = video_qhndls[0];
		vreq->dev_hndl = dev_hndl;
		vreq->qhndl = video_qhndls[0];
		vreq->avideo = xpdev->videos;
		QDMA_CASH3_EXPECT(0 == prepare_cash3_qdma_video_request(vreq));

		pr_debug("==ready to submit===\n");
		QDMA_CASH3_EXPECT(0 == send_cash3_qdma_video_request(vreq));

		pr_debug("==done submit===\n");
	}
	msleep_interruptible(1000);
	QDMA_CASH3_EXPECT(vreq == NULL);
	pr_debug("vreq=%p\n", vreq);
}

static 
void
aio_done_async_read_multi_from_video_queue_zero(
	unsigned long user_data, unsigned int bytes_done, int err) {
	struct cash3_qdma_video_request** vreq_ptr = (void*)user_data;
	struct cash3_qdma_video_request* vreq = *vreq_ptr;

	pr_debug("%s: bytes_done=%d, err=%d, buffer_chunk_count=%ld, buffer_chunk_size=%ld"
		, __FUNCTION__, bytes_done, err, vreq->buffer_chunk_count, vreq->buffer_chunk_size);
	QDMA_CASH3_EXPECT(err == 0);
	QDMA_CASH3_EXPECT(bytes_done == vreq->buffer_chunk_count * vreq->buffer_chunk_size);
	QDMA_CASH3_EXPECT( 0 == ((uint32_t*)(vreq->buffer_chunks[0]))[0]);
	unprepare_cash3_qdma_video_request(vreq);
	kfree(vreq);
	*vreq_ptr = NULL;
}

static 
void 
test_async_multiple_read_from_video_queue_zero(struct device *dev) {
	int test_frame_count = 29;
	while (test_frame_count--) {
		struct cash3_qdma_video_request* vreq = NULL;
		struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
		int rv = 0;
		unsigned long dev_hndl, qhndl, video_qhndls[1] = {0};
		vreq = kzalloc(sizeof(*vreq), GFP_KERNEL);
		vreq->user_data = (unsigned long)&vreq;
		vreq->aio_done = aio_done_async_read_multi_from_video_queue_zero;
		vreq->pdev = xpdev->pdev;

		rv = cash3_qdma_get_handles(xpdev->idx, &dev_hndl, video_qhndls, 1
			, NULL, NULL);
		qhndl = video_qhndls[0];
		vreq->dev_hndl = dev_hndl;
		vreq->qhndl = video_qhndls[0];
		vreq->avideo = xpdev->videos;
		QDMA_CASH3_ASSERT(0 == prepare_cash3_qdma_video_request(vreq));

		pr_debug("==ready to submit===\n");
		QDMA_CASH3_ASSERT(0 == send_cash3_qdma_video_request(vreq));

		pr_debug("==done submit===\n");

		msleep_interruptible(200);
		QDMA_CASH3_ASSERT(vreq == NULL);
	}
}

static 
void
aio_done_async_multiple_read_from_video_queue_zero_reuse_req(
	unsigned long user_data, unsigned int bytes_done, int err) {
	struct cash3_qdma_video_request** vreq_ptr = (void*)user_data;
	struct cash3_qdma_video_request* vreq = *vreq_ptr;

	pr_debug("%s: bytes_done=%d, err=%d, buffer_chunk_count=%ld, buffer_chunk_size=%ld"
		, __FUNCTION__, bytes_done, err, vreq->buffer_chunk_count, vreq->buffer_chunk_size);
	QDMA_CASH3_EXPECT(err == 0);
	QDMA_CASH3_EXPECT(bytes_done == vreq->buffer_chunk_count * vreq->buffer_chunk_size);
	*vreq_ptr = NULL;
}

static 
void 
test_async_multiple_read_from_video_queue_zero_reuse_req(struct device *dev) {
	int test_frame_count = 29;
	{
		struct cash3_qdma_video_request * volatile vreq = 0, *vreq_keep;
		struct xlnx_pci_dev *xpdev = dev_get_drvdata(dev);
		int rv = 0;
		unsigned long dev_hndl, qhndl, video_qhndls[1] = {0};
		vreq = kzalloc(sizeof(*vreq), GFP_KERNEL);
		vreq->user_data = (unsigned long)&vreq;
		vreq->aio_done = aio_done_async_multiple_read_from_video_queue_zero_reuse_req;
		vreq->pdev = xpdev->pdev;

		rv = cash3_qdma_get_handles(xpdev->idx, &dev_hndl, video_qhndls, 1
			, NULL, NULL);
		qhndl = video_qhndls[0];
		vreq->dev_hndl = dev_hndl;
		vreq->qhndl = video_qhndls[0];
		vreq->avideo = xpdev->videos;
		QDMA_CASH3_EXPECT(0 == prepare_cash3_qdma_video_request(vreq));

		vreq_keep = vreq;
		while (test_frame_count--) {
			vreq = vreq_keep;
			pr_debug("==ready to submit===\n");
			QDMA_CASH3_EXPECT(0 == send_cash3_qdma_video_request(vreq));
			pr_debug("==done submit===\n");

			msleep_interruptible(200);
			QDMA_CASH3_EXPECT(vreq == NULL);
		}
		unprepare_cash3_qdma_video_request(vreq_keep);
		kfree(vreq);
	}
}

struct my_video_request {
	struct cash3_qdma_video_request vreq;
};

static 
void
aio_done_async_multiple_read_from_all_video_queues_reuse_req(
	unsigned long user_data, unsigned int bytes_done, int err) {
	struct my_video_request** my_vreq_ptr = (void*)user_data;
	struct cash3_qdma_video_request* vreq = &((*my_vreq_ptr)->vreq);

	QDMA_CASH3_EXPECT(err == 0);
	QDMA_CASH3_EXPECT(bytes_done == vreq->buffer_chunk_count * vreq->buffer_chunk_size);
	*my_vreq_ptr = NULL;
}

static 
void 
async_multiple_read_from_all_video_queues_reuse_req(struct device *dev
	, int test_frame_count) {
	int i; 
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(dev);
	autox_queue_spec_t *queue_spec = &((autox_spec_t *)xpdev->spec)->queue;
	uint32_t qid;
	unsigned long dev_hndl, video_qhndls[CASH3_VIDEO_QUEUE_COUNT_MAX] = {0};
	struct my_video_request * volatile my_vreqs[CASH3_VIDEO_QUEUE_COUNT_MAX] = {0}
		, *my_vreqs_keep[CASH3_VIDEO_QUEUE_COUNT_MAX] = {0};
	
	QDMA_CASH3_ASSERT(0 == cash3_qdma_get_handles(
		xpdev->idx, &dev_hndl, video_qhndls, queue_spec->video.count
		, NULL, NULL));
	for (qid = 0; qid < queue_spec->video.count; ++qid) {
		struct cash3_qdma_video_request *vreq = 0;
		my_vreqs[qid] = kzalloc(sizeof(*my_vreqs[qid]), GFP_KERNEL);
		QDMA_CASH3_ASSERT(0 != my_vreqs[qid]);
		vreq = &my_vreqs[qid]->vreq;
		vreq->user_data = (unsigned long)(&(my_vreqs[qid]));
		vreq->aio_done = 
			aio_done_async_multiple_read_from_all_video_queues_reuse_req;
		vreq->pdev = xpdev->pdev;
		vreq->dev_hndl = dev_hndl;
		vreq->qhndl = video_qhndls[qid];
		vreq->avideo = ((autox_video_t *)xpdev->videos) + qid;
		QDMA_CASH3_ASSERT(0 == prepare_cash3_qdma_video_request(vreq));

		my_vreqs_keep[qid] = my_vreqs[qid];
	}
	for (i = 0; i < test_frame_count; ++i) {
		for (qid = 0; qid != queue_spec->video.count; ++qid) {
			struct cash3_qdma_video_request *vreq = &my_vreqs_keep[qid]->vreq;
			my_vreqs[qid] = my_vreqs_keep[qid];
			pr_debug("==ready to submit for qid %d ===\n", qid);
			QDMA_CASH3_EXPECT(0 == send_cash3_qdma_video_request(vreq));
			pr_debug("==done submit for qid %d ===\n", qid);
		}

		for (qid = 0; qid < queue_spec->video.count; ++qid) {
			while (my_vreqs[qid]) msleep_interruptible(1);
			QDMA_CASH3_EXPECT(my_vreqs[qid] == NULL);
		}

		if ((i + 1) % 100 == 0) {
			pr_info("%d streams, done_frame_count_per_stream = %d", queue_spec->video.count, (i + 1));
		}
	}

	for (qid = 0; qid < queue_spec->video.count; ++qid) {
		unprepare_cash3_qdma_video_request(&my_vreqs_keep[qid]->vreq);
		kfree(my_vreqs_keep[qid]);
	}
}

static 
void 
test_async_multiple_read_from_all_video_queues_reuse_req(struct device *dev) {
	async_multiple_read_from_all_video_queues_reuse_req(dev
		, 37);
}

static 
void 
test_async_multiple_read_from_all_video_queues_reuse_req_overnight(struct device *dev) {
	async_multiple_read_from_all_video_queues_reuse_req(dev
		, 12 * 60 * 60 * 10);
}

static 
void 
test_async_multiple_read_from_all_video_queues_reuse_req_rate_check(struct device *dev) {
	struct timespec64 end, start;
	ktime_get_ts64(&start);
	async_multiple_read_from_all_video_queues_reuse_req(dev
		, 60 * 10);
	ktime_get_ts64(&end);
	QDMA_CASH3_EXPECT(end.tv_sec - start.tv_sec < 62);
}

static 
void
aio_done_async_multiple_display_all_video_queues_reuse_req(
	unsigned long user_data, unsigned int bytes_done, int err) {
	struct my_video_request** my_vreq_ptr = (void*)user_data;
	struct cash3_qdma_video_request* vreq = &((*my_vreq_ptr)->vreq);

	QDMA_CASH3_EXPECT(err == 0);
	QDMA_CASH3_EXPECT(bytes_done == vreq->buffer_chunk_count * vreq->buffer_chunk_size);

	*my_vreq_ptr = NULL;
}

static 
void
test_async_multiple_display_all_video_queues_reuse_req(struct device *dev) {
	int i, test_frame_count = 160;
	uint32_t display_size = 10;
	// int i, test_frame_count = 16 * 60 * 60 * 12;
	struct xlnx_pci_dev *xpdev = dev_get_drvdata(dev);
	autox_queue_spec_t *queue_spec = &((autox_spec_t *)xpdev->spec)->queue;
	uint32_t qid;
	unsigned long dev_hndl, video_qhndls[CASH3_VIDEO_QUEUE_COUNT_MAX] = {0};
	struct my_video_request * volatile my_vreqs[CASH3_VIDEO_QUEUE_COUNT_MAX] = {0}
		, *my_vreqs_keep[CASH3_VIDEO_QUEUE_COUNT_MAX] = {0};

	QDMA_CASH3_ASSERT(0 == cash3_qdma_get_handles(
		xpdev->idx, &dev_hndl, video_qhndls, queue_spec->video.count
		, NULL, NULL));
	for (qid = 0; qid < queue_spec->video.count; ++qid) {
		struct cash3_qdma_video_request *vreq = 0;
		my_vreqs[qid] = kzalloc(sizeof(*my_vreqs[qid]), GFP_KERNEL);
		QDMA_CASH3_ASSERT(0 != my_vreqs[qid]);
		vreq = &my_vreqs[qid]->vreq;
		vreq->user_data = (unsigned long)(&(my_vreqs[qid]));
		vreq->aio_done = 
			aio_done_async_multiple_display_all_video_queues_reuse_req;
		vreq->pdev = xpdev->pdev;
		vreq->dev_hndl = dev_hndl;
		vreq->qhndl = video_qhndls[qid];
		vreq->avideo = ((autox_video_t *)xpdev->videos) + qid;
		QDMA_CASH3_ASSERT(0 == prepare_cash3_qdma_video_request(vreq));

		my_vreqs_keep[qid] = my_vreqs[qid];
	}
	for (i = 0; i < test_frame_count; ++i) {
		for (qid = 0; qid != queue_spec->video.count; ++qid) {
			struct cash3_qdma_video_request *vreq = &my_vreqs_keep[qid]->vreq;
			my_vreqs[qid] = my_vreqs_keep[qid];
			pr_debug("==ready to submit for qid %d ===\n", qid);
			QDMA_CASH3_EXPECT(0 == send_cash3_qdma_video_request(vreq));
			pr_debug("==done submit for qid %d ===\n", qid);
		}

		msleep_interruptible(200);
		for (qid = 0; qid < queue_spec->video.count; ++qid) {
			int i, j;
			struct cash3_qdma_video_request *vreq = &my_vreqs_keep[qid]->vreq;
			QDMA_CASH3_EXPECT(my_vreqs[qid] == NULL);
			pr_info("qid %d starting \n", qid);
			for (j = 0; j < vreq->buffer_chunk_count; ++j) {
				uint32_t* from =  vreq->buffer_chunks[j];
				for (i = 0; j == 0 && i < display_size; ++i) {
					pr_info("%08x ", from[i]);
				}
				if (j == 0) pr_info("---");
				for (i = display_size - 1; 
					j == vreq->buffer_chunk_count - 1 && i != -1; 
					--i) {
					pr_info("%08x ", from[vreq->buffer_chunk_size / sizeof(*from) - i
						- (6 * 1024 * 1024 - 1920 * 1080 * 3) / 4
						]);
				}
			}
			pr_info("qid %d ended \n", qid);
		}

		if ((i + 1) % 100 == 0) {
			pr_info("done_frame_count = %d", (i + 1) * queue_spec->video.count);
		}
	}

	for (qid = 0; qid < queue_spec->video.count; ++qid) {
		unprepare_cash3_qdma_video_request(&my_vreqs_keep[qid]->vreq);
		kfree(my_vreqs_keep[qid]);
	}
}

static 
void 
test_cmd_event_cycle(struct device *dev) {
	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	int rv = 0;
	unsigned long dev_hndl, cmd_qhndl, event_qhndl;

	//reset frame request size to be 6MB
	//since most tests expect that
	char mem[512] = {0};
	struct cash3_host_cmd_t *cmd = (void*)mem;
	struct cash3_host_cmd_trigger_dummy_event_t* trigger 
		= (void*)cmd->payload;
	struct timespec64 tv;

	struct cash3_qdma_request qreq = {0};
	struct cash3_fpga_event_t* event;
	struct timespec64 tv2;
	int i = 0;


	rv = cash3_qdma_get_handles(xpdev->idx, &dev_hndl, NULL, 0
		, &cmd_qhndl, &event_qhndl);
	QDMA_CASH3_ASSERT(rv == 0);
	QDMA_CASH3_ASSERT(event_qhndl != 0);
	QDMA_CASH3_ASSERT(cmd_qhndl != 0);
	
	cmd->head.cmd = CASH3_HOST_CMD_TRIGGER_DUMMY_EVENT;

	qreq.pdev = xpdev->pdev;
	qreq.buffer_chunk_count = 1;
	qreq.buffer_chunk_size = 4 * 1024;
	qreq.dev_hndl = dev_hndl;
	qreq.qhndl = event_qhndl;
	rv = prepare_cash3_qdma_request(&qreq);
	QDMA_CASH3_ASSERT(rv == 0);
	event = qreq.buffer_chunks[0];


	trigger->backReference = 0;

	for (; i < 100; ++i) {
		ktime_get_ts64(&tv);
		rv = send_host_cmd(xpdev->dev_hndl
			, cmd
			, sizeof(mem)
			, false);
		pr_debug("send_host_cmd rv=%d", rv);
		QDMA_CASH3_EXPECT(0 < rv);
		rv = send_cash3_qdma_request(&qreq);
		pr_info("send_cash3_qdma_request rv=%d", rv);
		QDMA_CASH3_EXPECT(qreq.buffer_chunk_size == rv);
		ktime_get_ts64(&tv2);

		display_time("cash3_host_cmd_trigger_dummy_event_t sent@", &tv);
		display_time("cash3_fpga_event_dummy_t             recv@", &tv2);
		QDMA_CASH3_EXPECT(CASH3_FPGA_EVENT_DUMMY == event->head.event);
		pr_debug("event->head.event=%d", event->head.event);
		{
			struct cash3_fpga_event_dummy_t* dummy = (void*)&event->payload;
			QDMA_CASH3_EXPECT(dummy->backReference == trigger->backReference);
			pr_debug("dummy->backReference %llx=?trigger->backReference %llx"
				, dummy->backReference, trigger->backReference);
		}
		trigger->backReference++;
	}
	unprepare_cash3_qdma_request(&qreq);
}

static 
void
aio_done_dummy_event_read(
	unsigned long user_data, unsigned int bytes_done, int err) {
	struct timespec64 tv;
	ktime_get_ts64(&tv);
	display_time("aio_done_dummy_event_read                 recv@", &tv);
}

static 
void 
test_cmd_event_cycle_timing(struct device *dev) {
	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	int rv = 0;
	unsigned long dev_hndl, cmd_qhndl, event_qhndl;
	autox_scratch_reg_addr_spec_t *scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;

	//reset frame request size to be 6MB
	//since most tests expect that
	char mem[512] = {0};
	struct cash3_host_cmd_t *cmd = (void*)mem;
	struct cash3_host_cmd_trigger_dummy_event_t* trigger 
		= (void*)cmd->payload;
	struct timespec64 tv, tv_sent, tv_fr;

	struct cash3_qdma_request qreq = {0};
	struct cash3_fpga_event_t* event;
	int i = 0;


	rv = cash3_qdma_get_handles(xpdev->idx, &dev_hndl, NULL, 0
		, &cmd_qhndl, &event_qhndl);
	QDMA_CASH3_ASSERT(rv == 0);
	QDMA_CASH3_ASSERT(event_qhndl != 0);
	QDMA_CASH3_ASSERT(cmd_qhndl != 0);
	
	cmd->head.cmd = CASH3_HOST_CMD_TRIGGER_DUMMY_EVENT;

	qreq.pdev = xpdev->pdev;
	qreq.buffer_chunk_count = 1;
	qreq.buffer_chunk_size = 4 * 1024;
	qreq.dev_hndl = dev_hndl;
	qreq.qhndl = event_qhndl;
	qreq.aio_done = aio_done_dummy_event_read;
	qreq.user_data = (unsigned long)&qreq;
	rv = prepare_cash3_qdma_request(&qreq);
	QDMA_CASH3_ASSERT(rv == 0);
	event = qreq.buffer_chunks[0];


	trigger->backReference = 0;

	for (; i < 100; ++i) {
		rv = send_cash3_qdma_request(&qreq);
		pr_info("send_cash3_qdma_request rv=%d", rv);
		QDMA_CASH3_EXPECT(rv == 0); //async
		// msleep_interruptible(100);

		QDMA_CASH3_EXPECT(0 
			== write_cash3_user_reg(xpdev->dev_hndl, scratch_reg->start_addr + 62 * 4, 0x0));

		ktime_get_ts64(&tv);
		rv = send_host_cmd(xpdev->dev_hndl
			, cmd
			, sizeof(mem)
			, false);
		ktime_get_ts64(&tv_sent);
		while(0 == read_cash3_user_reg(xpdev->dev_hndl, scratch_reg->start_addr + 62 * 4)) {
			msleep_interruptible(1);
		}
		ktime_get_ts64(&tv_fr);

		pr_debug("send_host_cmd rv=%d", rv);
		QDMA_CASH3_EXPECT(0 < rv);

		display_time("cash3_host_cmd_trigger_dummy_event_t   pre-sent@", &tv);
		display_time("cash3_host_cmd_trigger_dummy_event_t  post-sent@", &tv_sent);
		display_time("frame done                                    @", &tv_fr);
		msleep_interruptible(500); //got to be done
		QDMA_CASH3_EXPECT(CASH3_FPGA_EVENT_DUMMY == event->head.event);
		pr_debug("event->head.event=%d", event->head.event);
		{
			struct cash3_fpga_event_dummy_t* dummy = (void*)&event->payload;
			QDMA_CASH3_EXPECT(dummy->backReference == trigger->backReference);
			pr_debug("dummy->backReference %llx=?trigger->backReference %llx"
				, dummy->backReference, trigger->backReference);
		}
		trigger->backReference++;
	}
	unprepare_cash3_qdma_request(&qreq);
}

static 
void 
test_sync_cmd_event_cycle_timing(struct device *dev) {
	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	int rv = 0;
	unsigned long dev_hndl, cmd_qhndl, event_qhndl;

	char mem[512] = {0};
	struct cash3_host_cmd_t *cmd = (void*)mem;
	struct cash3_host_cmd_trigger_dummy_event_t* trigger 
		= (void*)cmd->payload;
	struct timespec64 tv, tv2;

	struct cash3_qdma_request qreq = {0};
	struct cash3_fpga_event_t* event;
	int i = 0;


	rv = cash3_qdma_get_handles(xpdev->idx, &dev_hndl, NULL, 0
		, &cmd_qhndl, &event_qhndl);
	QDMA_CASH3_ASSERT(rv == 0);
	QDMA_CASH3_ASSERT(event_qhndl != 0);
	QDMA_CASH3_ASSERT(cmd_qhndl != 0);
	
	cmd->head.cmd = CASH3_HOST_CMD_TRIGGER_DUMMY_EVENT;

	qreq.pdev = xpdev->pdev;
	qreq.buffer_chunk_count = 1;
	qreq.buffer_chunk_size = 4 * 1024;
	qreq.dev_hndl = dev_hndl;
	qreq.qhndl = event_qhndl;
	// qreq.aio_done = aio_done_dummy_event_read;
	qreq.user_data = (unsigned long)&qreq;
	rv = prepare_cash3_qdma_request(&qreq);
	QDMA_CASH3_ASSERT(rv == 0);
	event = qreq.buffer_chunks[0];


	trigger->backReference = 0;
	
	QDMA_CASH3_ASSERT(0 
		== read_cash3_user_reg(xpdev->dev_hndl, CASH3_FPGA_IRQ_MASK_ADDR));
	// for (i = 0; i < 1; ++i) {
	//     rv = send_host_cmd(xpdev->dev_hndl
	// 		, cmd
	// 		, sizeof(mem)
	// 		, true);
	// }
//	msleep_interruptible(100); //got to be done
	for (i = 0; i < 100; ++i) {
		ktime_get_ts64(&tv);
		rv = send_host_cmd(xpdev->dev_hndl
			, cmd
			, sizeof(mem)
			, false);
		pr_debug("send_host_cmd rv=%d", rv);
		QDMA_CASH3_EXPECT(0 < rv);
		// msleep_interruptible(5); //got to be done

		rv = send_cash3_qdma_request(&qreq);
		ktime_get_ts64(&tv2);
		pr_info("send_cash3_qdma_request rv=%d", rv);
		QDMA_CASH3_EXPECT(rv > 0); //async

		display_time("cash3_host_cmd_trigger_dummy_event_t   pre-sent@", &tv);
		display_time("cash3_host_cmd_trigger_dummy_event_t       recv@", &tv2);
		{
			int64_t cycle_us = ((tv2.tv_sec - tv.tv_sec) * NSEC_PER_SEC + (tv2.tv_nsec - tv.tv_nsec)) / NSEC_PER_USEC;
			pr_info("cycle us=%lld", cycle_us);
			QDMA_CASH3_EXPECT(cycle_us < 500);
		}
		msleep_interruptible(200); //got to be done
		QDMA_CASH3_EXPECT(CASH3_FPGA_EVENT_DUMMY == event->head.event);
		pr_debug("event->head.event=%d", event->head.event);
		{
			struct cash3_fpga_event_dummy_t* dummy = (void*)&event->payload;
			QDMA_CASH3_EXPECT(dummy->backReference == trigger->backReference);
			pr_debug("dummy->backReference %llx=?trigger->backReference %llx"
				, dummy->backReference, trigger->backReference);
		}
		trigger->backReference++;
	}
	unprepare_cash3_qdma_request(&qreq);
}

static 
void 
test_qdma_timing(struct device *dev) {
	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	int rv = 0;
	unsigned long dev_hndl;
	struct timespec64 tv;

	struct cash3_qdma_request qreq = {0};
	int i = 0;
	uint64_t ior_addr;
	uint32_t ior_size;
	uint32_t test_queue_idx = TEST_IO_Q_ID_C8;

	if (xpdev->video_cnt > 8) {
		test_queue_idx = TEST_IO_Q_ID_C16;
	}

	rv = cash3_qdma_get_handles(xpdev->idx, &dev_hndl, NULL, 0
		, NULL, NULL);
	QDMA_CASH3_ASSERT(rv == 0);
	qreq.pdev = xpdev->pdev;
	qreq.buffer_chunk_count = 1;
	qreq.buffer_chunk_size = 4 * 1024;
	qreq.dev_hndl = dev_hndl;
	// qreq.qhndl = event_qhndl;

	read_io_area_conf(xpdev->dev_hndl, &ior_addr, &ior_size);
	QDMA_CASH3_EXPECT(ior_size > 100 * 1024 * 1024);
	QDMA_CASH3_EXPECT(ior_addr);
	qreq.bram_addr = ior_addr;
	qreq.qhndl = xpdev->qdata[test_queue_idx  + xpdev->qmax].qhndl;
	
	qreq.aio_done = aio_done_dummy_event_read;
	qreq.user_data = (unsigned long)&qreq;
	rv = prepare_cash3_qdma_request(&qreq);
	QDMA_CASH3_ASSERT(rv == 0);


	// trigger->backReference = 0;

	for (; i < 100; ++i) {
		ktime_get_ts64(&tv);
		rv = send_cash3_qdma_request(&qreq);
		display_time("cash3_host_cmd_trigger_dummy_event_t   pre-sent@", &tv);
		QDMA_CASH3_EXPECT(rv == 0); //async
		msleep_interruptible(600); //got to be done
	    // QDMA_CASH3_EXPECT(CASH3_FPGA_EVENT_DUMMY == event->hea
	}
	unprepare_cash3_qdma_request(&qreq);
}


// static
// void sync_request(struct device * dev, uint8_t video_id
// 	, size_t count, uint32_t frame_request_size) {
// 	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
// 	int rv = 0;
// 	unsigned long dev_hndl, qhndl, video_qhndls[CASH3_VIDEO_QUEUE_COUNT] = {0};
// 	struct cash3_qdma_video_request vreq = {0};
// 	vreq.pdev = xpdev->pdev;
// 	vreq.buffer_chunk_count = 2;
// 	vreq.buffer_chunk_size = frame_request_size / 2;

// 	rv = cash3_qdma_get_handles(xpdev->idx, &dev_hndl
// 		, video_qhndls, CASH3_VIDEO_QUEUE_COUNT, NULL, NULL);
// 	QDMA_CASH3_EXPECT(rv == 0);
// 	qhndl = video_qhndls[video_id];
// 	vreq.dev_hndl = dev_hndl;
// 	vreq.qhndl = video_qhndls[video_id];
// 	rv = prepare_cash3_qdma_video_request(&vreq);
// 	pr_debug("prepare_cash3_qdma_video_request= %d", rv);
// 	QDMA_CASH3_EXPECT(rv == 0);
// 	// qdma_sw_sg_dump(vreq.reserved->sgl);
// 	while (count--) {
// 		QDMA_CASH3_EXPECT(frame_request_size 
// 			== send_cash3_qdma_video_request(&vreq));
// 	} 
// 	QDMA_CASH3_EXPECT(video_id == ((uint32_t*)(vreq.buffer_chunks[0]))[0]);
// 	unprepare_cash3_qdma_video_request(&vreq);
// }

// static 
// void
// test_set_frame_request_size_for_0_1(struct device *dev) {
// 	struct xlnx_pci_dev *xpdev = dev_get_drvdata(dev);
// 	char mem[512] = {0};
// 	struct cash3_host_cmd_t *cmd = (void*)mem;
// 	struct cash3_host_cmd_set_video_params_t* set_params 
// 		= (void*)cmd->payload;
// 	uint32_t frs0 = 1920*1080*3 + 4* 1024;
// 	uint32_t frs1 = 6 * 1024 * 1024;
// 	cmd->head.cmd = CASH3_HOST_CMD_VIDEO_SET_VIDEO_PARAMS;
// 	cmd->head.cmd_count = 2;
// 	set_params->video_stream_id = 0;
// 	set_params->frame_request_size = frs0;
// 	set_params++;
// 	set_params->video_stream_id = 1;
// 	set_params->frame_request_size = frs1;

// 	QDMA_CASH3_ASSERT(0 < send_host_cmd(xpdev->dev_hndl
// 		, cmd
// 		, sizeof(mem)
// 		, true)
// 	);
// 	QDMA_CASH3_ASSERT(0 == read_cash3_user_reg(xpdev->dev_hndl
// 		, CASH3_SCRATCH_REG_CMD_STATUS_ADDR));
// 	QDMA_CASH3_ASSERT(0 == read_cash3_user_reg(xpdev->dev_hndl
// 		, CASH3_SCRATCH_REG_CMD_STATUS_ADDR + 4));

// 	sync_request(dev, 0, 4, frs0);
// 	sync_request(dev, 1, 8, frs1);

// 	//put things back
// 	set_params = (void*)cmd->payload;
// 	cmd->head.cmd = CASH3_HOST_CMD_VIDEO_SET_VIDEO_PARAMS;
// 	cmd->head.cmd_count = 1;
// 	set_params->video_stream_id = 0;
// 	set_params->frame_request_size = frs1;

// 	QDMA_CASH3_ASSERT(0 < send_host_cmd(xpdev->dev_hndl
// 		, cmd
// 		, sizeof(mem)
// 		, true)
// 	);
// }

struct sync_video_stream_t {
	struct device *dev;
	uint32_t count;
	uint16_t v_id;
};

static
int sync_video_stream_func(void* param_in) {
	struct sync_video_stream_t* param = (void*)param_in;
	struct device *dev = param->dev;
	uint32_t v_id = param->v_id;
	uint32_t const count = param->count;

	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	autox_queue_spec_t *queue_spec = &((autox_spec_t *)xpdev->spec)->queue;
	int rv = 0;
	unsigned long dev_hndl, qhndl, video_qhndls[CASH3_VIDEO_QUEUE_COUNT_MAX] = {0};
	struct cash3_qdma_video_request vreq = {0};
	struct timespec64 tv, tv2;
	int i, j;
	char* dest_mem = vmalloc(6 * 1024 * 1024);

	QDMA_CASH3_EXPECT(dest_mem != NULL);

	vreq.pdev = xpdev->pdev;

	rv = clear_cash3_video_status(xpdev->dev_hndl, 0, 4);
	pr_info("video stream status %d = %d", 0, rv);

	rv = cash3_qdma_get_handles(xpdev->idx, &dev_hndl, video_qhndls, queue_spec->video.count
		, NULL, NULL);
	QDMA_CASH3_EXPECT(rv == 0);
	qhndl = video_qhndls[v_id];
	vreq.dev_hndl = dev_hndl;
	vreq.qhndl = qhndl;
	vreq.avideo = ((autox_video_t *)xpdev->videos) + v_id;
	rv = prepare_cash3_qdma_video_request(&vreq);
	pr_debug("v_id=%d, prepare_cash3_qdma_video_request= %d", v_id, rv);
	QDMA_CASH3_EXPECT(rv == 0);

	ktime_get_ts64(&tv);
	for (i = 0; i < count; i++) {
		struct cash3_video_meta_data_t* meta = vreq.buffer_chunks[0];
		uint32_t ts_gap_us = (meta->curr_ts_sec - meta->prev_ts_sec) * 1000000 
			+ meta->curr_ts_us - meta->prev_ts_us;
		ssize_t send_count = send_cash3_qdma_video_request(&vreq);
		QDMA_CASH3_EXPECT(6 * 1024 * 1024 == send_count);
		if (6 * 1024 * 1024 != send_count) {
			pr_err("v_id %d exiting abnormally", v_id);
			return -1;
		}
		QDMA_CASH3_EXPECT(v_id == meta->mipi_id);
		QDMA_CASH3_EXPECT(ts_gap_us < 120000);
		if  (ts_gap_us >= 120000) {
		pr_err("gap %d too big mipi_id=%d mipi_frame_idx=%d resp=%d req=%d curr_ts_sec=%d curr_ts_us=%d prev_ts_sec=%d prev_ts_us=%d"
			, ts_gap_us
			, meta->mipi_id
			, meta->mipi_frame_idx
			, meta->resp
			, meta->req
			, meta->curr_ts_sec
			, meta->curr_ts_us
			, meta->prev_ts_sec
			, meta->prev_ts_us
			);
		}
		for (j = 0; j < vreq.buffer_chunk_count; ++j) {
			memcpy(dest_mem + j * vreq.buffer_chunk_size, vreq.buffer_chunks[j], vreq.buffer_chunk_size);
		}
		// display_time("test_sync_read_from_video_queue_zero sent@", &tv);
		// display_time("test_sync_read_from_video_queue_zero recv@", &tv2);
		if ((i + 1) % 10 == 0) {
			ktime_get_ts64(&tv2);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 18, 0)
			pr_info("stream %d, done_frame_count = %d, took %lld", v_id, (i + 1)
				, ((tv2.tv_sec - tv.tv_sec) * NSEC_PER_SEC + (tv2.tv_nsec - tv.tv_nsec)) / NSEC_PER_USEC);
#else
			pr_info("stream %d, done_frame_count = %d, took %ld", v_id, (i + 1)
				, ((tv2.tv_sec - tv.tv_sec) * NSEC_PER_SEC + (tv2.tv_nsec - tv.tv_nsec)) / NSEC_PER_USEC);
#endif
			ktime_get_ts64(&tv);
		}
	}
	unprepare_cash3_qdma_video_request(&vreq);
	vfree(dest_mem);
	do_exit(0);
}


static 
void 
test_sync_threads(struct device *dev) {
	struct sync_video_stream_t params[CASH3_VIDEO_QUEUE_COUNT_MAX];
	uint16_t v_id = 0;
	struct xlnx_pci_dev * xpdev = dev_get_drvdata(dev);
	uint32_t count = 10 * 60 * 2;
	autox_queue_spec_t *queue_spec = &((autox_spec_t *)xpdev->spec)->queue;

	for (;v_id < queue_spec->video.count; ++v_id) {
		char const our_thread[]="fpga_event_log";
		struct task_struct* thrd;
		struct sync_video_stream_t* param = &params[v_id];
		param->dev = dev;
		param->count = count;
		param->v_id = v_id;
		thrd = kthread_create(sync_video_stream_func, params + v_id
			, our_thread);
		wake_up_process(thrd);
	}
	msleep_interruptible(count * 1200 / 10 + 5000); //wait until all thrd finish
}

/************************************end of test cases********************************************/
typedef void (*test_case_t)(struct device *);
struct test_case_entry_t {
	char const* name;
	test_case_t test_case;
};

#undef ADD_TEST
#define ADD_TEST(tf) {#tf, tf,}

static struct test_case_entry_t const tests[] = {
	ADD_TEST(setup_tests), 		//keep it the 1st - it should always be run once before running actual test cases
/*1*/	ADD_TEST(test_read_register),
/*2*/	ADD_TEST(test_allocate_mem),
/*3*/	ADD_TEST(test_cash3_qdma_get_handles),
/*4*/	ADD_TEST(test_prepare_unprepare_qdma),
/*5*/	ADD_TEST(test_prepare_unprepare_video),
/*6*/	ADD_TEST(test_io_area),
/*7*/	ADD_TEST(test_sync_read_from_video_queue_zero),
/*8*/	ADD_TEST(test_async_read_from_video_queue_zero),
/*9*/	ADD_TEST(test_async_read_large_from_video_queue_zero),
/*10*/	ADD_TEST(test_async_multiple_read_from_video_queue_zero),
/*11*/	ADD_TEST(test_async_multiple_read_from_video_queue_zero_reuse_req),
/*12*/	ADD_TEST(test_async_multiple_read_from_all_video_queues_reuse_req),
/*13*/  ADD_TEST(test_async_multiple_read_from_all_video_queues_reuse_req_rate_check),
/*14*/	ADD_TEST(test_sync_cmd_event_cycle_timing),
// /*13*/	ADD_TEST(test_set_frame_request_size_for_0_1),
	ADD_TEST(tear_down_tests),	//keep it the last after all regular test cases - it should always be run once after running actual test cases

		// start of special (non-auto-run) test cases
/*11*/  ADD_TEST(test_cmd_event_cycle),
		ADD_TEST(test_cmd_event_cycle_timing),
		ADD_TEST(test_qdma_timing),
		ADD_TEST(test_async_multiple_read_from_all_video_queues_reuse_req_overnight),
		ADD_TEST(test_async_multiple_display_all_video_queues_reuse_req),
		ADD_TEST(test_sync_threads),
};

static unsigned int const test_case_count = sizeof(tests) / sizeof(struct test_case_entry_t);


ssize_t show_libqdma_cash3_test_cases(struct device *dev,
	struct device_attribute *attr, char *buf) {
	unsigned int i = 0;
	char* buf_old = buf;
	for (; i < test_case_count; ++i) {
		buf += sprintf(buf, "%d) %s\n", i, tests[i].name);
	}
	return buf - buf_old;
}

ssize_t run_libqdma_cash3_test_case(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count) {
	unsigned int test_case_id = 0;
	int err = 0;
	err = kstrtouint(buf, 0, &test_case_id);
	if (err < 0 || test_case_id >= test_case_count) {
		pr_info("test id %d does not exist\n", test_case_id);
		return count;
	}

	pr_info("running test id %d\n", test_case_id);
	cash3_qdma_test_error = 0;
	tests[test_case_id].test_case(dev);
	pr_info("test %d) ==> %d  %s\n", test_case_id, cash3_qdma_test_error, tests[test_case_id].name);
	return count;
}
