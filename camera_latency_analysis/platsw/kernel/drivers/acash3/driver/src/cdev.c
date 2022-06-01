/*
 * This file is part of the Xilinx DMA IP Core driver for Linux
 *
 * Copyright (c) 2017-2020,  Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ":%s: " fmt, __func__

#include "cdev.h"

#include <asm/cacheflush.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/aio.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/delay.h>
#if KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
#include <linux/uio.h>
#endif

#include "libqdma_cash3.h"
#include "qdma_mod.h"
#include "libqdma/xdev.h"
#include "acash_spec.h"

#include "../../include/linux/cash3_scratch_registers.h"
#include "../../include/linux/cash3_fpga_events.h"
#include "../../include/linux/cash3_host_cmds.h"
#include "../../include/linux/cash3_api.h"
extern 
int write_cash3_user_reg(unsigned long dev_hndl, uint32_t reg_addr, uint32_t value);
 

/*
 * @struct - xlnx_phy_dev
 * @brief	xilinx board device data members
 */
struct xlnx_phy_dev {
	struct list_head list_head;	/**< board list */
	unsigned int major;	/**< major number per board */
	unsigned int device_bus;	/**< PCIe device bus number per board */
	unsigned int dma_device_index;
};

static LIST_HEAD(xlnx_phy_dev_list);
static DEFINE_MUTEX(xlnx_phy_dev_mutex);

struct cdev_async_io {
	ssize_t res2;
	unsigned long req_count;
	unsigned long cmpl_count;
	unsigned long err_cnt;
	struct qdma_io_cb *qiocb;
	struct qdma_request **reqv;
	struct kiocb *iocb;
	struct work_struct wrk_itm;
};

enum qdma_cdev_ioctl_cmd {
	QDMA_CDEV_IOCTL_NO_MEMCPY,
	QDMA_CDEV_IOCTL_CASH3_MANIFEST = CASH3_QDMA_CDEV_IOCTL_MANIFEST,
	QDMA_CDEV_IOCTL_CMDS
};

struct class *qdma_class;
static struct kmem_cache *cdev_cache;

static ssize_t cdev_gen_read_write(struct file *file, char __user *buf,
		size_t count, loff_t *pos, bool write);
static void unmap_user_buf(struct qdma_io_cb *iocb, bool write);
static inline void iocb_release(struct qdma_io_cb *iocb);

static inline void xlnx_phy_dev_list_remove(struct xlnx_phy_dev *phy_dev)
{
	if (!phy_dev)
		return;

	mutex_lock(&xlnx_phy_dev_mutex);
	list_del(&phy_dev->list_head);
	mutex_unlock(&xlnx_phy_dev_mutex);
}

static inline void xlnx_phy_dev_list_add(struct xlnx_phy_dev *phy_dev)
{
	if (!phy_dev)
		return;

	mutex_lock(&xlnx_phy_dev_mutex);
	list_add_tail(&phy_dev->list_head, &xlnx_phy_dev_list);
	mutex_unlock(&xlnx_phy_dev_mutex);
}

static int qdma_req_completed(struct qdma_request *req,
		       unsigned int bytes_done, int err)
{
	struct qdma_io_cb *qiocb = container_of(req,
						struct qdma_io_cb,
						req);
	struct cdev_async_io *caio = NULL;
	bool free_caio = false;
	ssize_t res, res2;


	if (qiocb) {
		caio = (struct cdev_async_io *)qiocb->private;
	} else {
		pr_err("Invalid Data structure. Probable memory corruption");
		return -EINVAL;
	}

	if (!caio) {
		pr_err("Invalid Data structure. Probable memory corruption");
		return -EINVAL;
	}

	unmap_user_buf(qiocb, req->write);
	iocb_release(qiocb);
	caio->res2 |= (err < 0) ? err : 0;
	if (caio->res2)
		caio->err_cnt++;
	caio->cmpl_count++;
	if (caio->cmpl_count == caio->req_count) {
		res = caio->cmpl_count - caio->err_cnt;
		res2 = caio->res2;
#if KERNEL_VERSION(4, 1, 0) <= LINUX_VERSION_CODE
		caio->iocb->ki_complete(caio->iocb, res, res2);
#else
		aio_complete(caio->iocb, res, res2);
#endif
		kfree(caio->qiocb);
		free_caio = true;
	}
	if (free_caio)
		kmem_cache_free(cdev_cache, caio);

	return 0;
}

/*
 * character device file operations
 */
static int cdev_gen_open(struct inode *inode, struct file *file)
{
	struct qdma_cdev *xcdev = container_of(inode->i_cdev, struct qdma_cdev,
						cdev);
	file->private_data = xcdev;

	if (xcdev->fp_open_extra)
		return xcdev->fp_open_extra(xcdev);

	return 0;
}

static int cdev_gen_close(struct inode *inode, struct file *file)
{
	struct qdma_cdev *xcdev = (struct qdma_cdev *)file->private_data;

	if (xcdev && xcdev->fp_close_extra)
		return xcdev->fp_close_extra(xcdev);

	return 0;
}

static loff_t cdev_gen_llseek(struct file *file, loff_t off, int whence)
{
	struct qdma_cdev *xcdev = (struct qdma_cdev *)file->private_data;

	loff_t newpos = 0;

	switch (whence) {
	case 0: /* SEEK_SET */
		newpos = off;
		break;
	case 1: /* SEEK_CUR */
		newpos = file->f_pos + off;
		break;
	case 2: /* SEEK_END, @TODO should work from end of address space */
		newpos = UINT_MAX + off;
		break;
	default: /* can't happen */
		return -EINVAL;
	}
	if (newpos < 0)
		return -EINVAL;
	file->f_pos = newpos;

	pr_debug("%s: pos=%lld\n", xcdev->name, (signed long long)newpos);

	return newpos;
}

static long cdev_gen_ioctl(struct file *file, unsigned int cmd,
			unsigned long arg)
{
	struct qdma_cdev *xcdev = (struct qdma_cdev *)file->private_data;
	struct xlnx_pci_dev *xpdev = xcdev->xcb->xpdev;
	autox_scratch_reg_addr_spec_t *scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;
	int ret = 0;

	switch (cmd) {
	case QDMA_CDEV_IOCTL_NO_MEMCPY:
		get_user(xcdev->no_memcpy, (unsigned char *)arg);
		return 0;
	case QDMA_CDEV_IOCTL_CASH3_MANIFEST: {
		struct cash3_qdma_cdev_ioctl_manifest_t res;
		struct cash3_fpga_manifest_t mani;
		*(uint32_t*)(void*)&mani 
			= read_cash3_user_reg(xpdev->dev_hndl, scratch_reg->fpga_ver.addr);
		res.fw_ver = mani.fw_ver;
		res.hw_major_ver = mani.hw_major_ver;
		res.backup_fw_ver = mani.backup_fw_ver;
		res.hw_minor_ver = mani.hw_minor_ver;
		res.pci_devfn = xpdev->pdev->devfn;
		res.pci_vendor = xpdev->pdev->vendor;
		res.pci_device = xpdev->pdev->device;
		res.pci_subsystem_vendor = xpdev->pdev->subsystem_vendor;
		res.pci_subsystem_device = xpdev->pdev->subsystem_device;
  		res.api_ver_maj = CASH3_API_VER_MAJ;
  		res.api_ver_min = CASH3_API_VER_MIN;
		ret = copy_to_user((unsigned char *)arg, &res, sizeof(res));
		if (ret) {
			pr_err("%s copy to user failed.\n", xcdev->name);
			return -EFAULT;
		}
		return 0;
	} break;
	default:
		break;
	}
	if (xcdev->fp_ioctl_extra)
		return xcdev->fp_ioctl_extra(xcdev, cmd, arg);

	pr_err("%s ioctl NOT supported.\n", xcdev->name);
	return -EINVAL;
}

/*
 * cdev r/w
 */
static inline void iocb_release(struct qdma_io_cb *iocb)
{
	if (iocb->pages)
		iocb->pages = NULL;
	kfree(iocb->sgl);
	iocb->sgl = NULL;
	iocb->buf = NULL;
}

static void unmap_user_buf(struct qdma_io_cb *iocb, bool write)
{
	int i;

	if (!iocb->pages || !iocb->pages_nr)
		return;

	for (i = 0; i < iocb->pages_nr; i++) {
		if (iocb->pages[i]) {
			if (!write)
				set_page_dirty_lock(iocb->pages[i]);
			put_page(iocb->pages[i]);
		} else
			break;
	}

	if (i != iocb->pages_nr)
		pr_err("sgl pages %d/%u.\n", i, iocb->pages_nr);

	iocb->pages_nr = 0;
}

static int map_user_buf_to_sgl(struct qdma_io_cb *iocb, bool write)
{
	unsigned long len = iocb->len;
	char *buf = iocb->buf;
	struct qdma_sw_sg *sg;
	unsigned int pg_off = offset_in_page(buf);
	unsigned int pages_nr = (len + pg_off + PAGE_SIZE - 1) >> PAGE_SHIFT;
	int i;
	int rv;

	if (len == 0)
		pages_nr = 1;
	if (pages_nr == 0)
		return -EINVAL;

	iocb->pages_nr = 0;
	sg = kmalloc(pages_nr * (sizeof(struct qdma_sw_sg) +
			sizeof(struct page *)), GFP_KERNEL);
	if (!sg) {
		pr_err("sgl allocation failed for %u pages", pages_nr);
		return -ENOMEM;
	}
	memset(sg, 0, pages_nr * (sizeof(struct qdma_sw_sg) +
			sizeof(struct page *)));
	iocb->sgl = sg;

	iocb->pages = (struct page **)(sg + pages_nr);
	rv = get_user_pages_fast((unsigned long)buf, pages_nr, 1/* write */,
				iocb->pages);
	/* No pages were pinned */
	if (rv < 0) {
		pr_err("unable to pin down %u user pages, %d.\n",
			pages_nr, rv);
		goto err_out;
	}
	/* Less pages pinned than wanted */
	if (rv != pages_nr) {
		pr_err("unable to pin down all %u user pages, %d.\n",
			pages_nr, rv);
		iocb->pages_nr = rv;
		rv = -EFAULT;
		goto err_out;
	}

	for (i = 1; i < pages_nr; i++) {
		if (iocb->pages[i - 1] == iocb->pages[i]) {
			pr_err("duplicate pages, %d, %d.\n",
				i - 1, i);
			iocb->pages_nr = pages_nr;
			rv = -EFAULT;
			goto err_out;
		}
	}

	sg = iocb->sgl;
	for (i = 0; i < pages_nr; i++, sg++) {
		unsigned int offset = offset_in_page(buf);
		unsigned int nbytes = min_t(unsigned int, PAGE_SIZE - offset,
						len);
		struct page *pg = iocb->pages[i];

		flush_dcache_page(pg);

		sg->next = sg + 1;
		sg->pg = pg;
		sg->offset = offset;
		sg->len = nbytes;
		sg->dma_addr = 0UL;

		buf += nbytes;
		len -= nbytes;
	}

	iocb->sgl[pages_nr - 1].next = NULL;
	iocb->pages_nr = pages_nr;
	return 0;

err_out:
	unmap_user_buf(iocb, write);
	iocb_release(iocb);

	return rv;
}

static ssize_t cdev_gen_read_write(struct file *file, char __user *buf,
		size_t count, loff_t *pos, bool write)
{
	struct qdma_cdev *xcdev = (struct qdma_cdev *)file->private_data;
	struct qdma_io_cb iocb;
	struct qdma_request *req = &iocb.req;
	ssize_t res = 0;
	int rv;
	unsigned long qhndl;

	if (!xcdev) {
		pr_err("file 0x%p, xcdev NULL, 0x%p,%llu, pos %llu, W %d.\n",
			file, buf, (u64)count, (u64)*pos, write);
		return -EINVAL;
	}

	if (!xcdev->fp_rw) {
		pr_err("file 0x%p, %s, NO rw, 0x%p,%llu, pos %llu, W %d.\n",
			file, xcdev->name, buf, (u64)count, (u64)*pos, write);
		return -EINVAL;
	}
	if (write == 0) pr_debug("q %d read started ", xcdev->qidx);
	qhndl = write ? xcdev->h2c_qhndl : xcdev->c2h_qhndl;

	pr_debug("%s, priv 0x%lx: buf 0x%p,%llu, pos %llu, W %d.\n",
		xcdev->name, qhndl, buf, (u64)count, (u64)*pos,
		write);

	memset(&iocb, 0, sizeof(struct qdma_io_cb));
	iocb.buf = buf;
	iocb.len = count;
	rv = map_user_buf_to_sgl(&iocb, write);
	if (rv < 0) {
		pr_err("cannot map user buffer for IO");
		return rv;
	}

	req->sgcnt = iocb.pages_nr;
	req->sgl = iocb.sgl;
	req->write = write ? 1 : 0;
	req->dma_mapped = 0;
	req->udd_len = 0;
	req->ep_addr = (u64)*pos;
	req->count = count;
	req->timeout_ms = 0; //no timeput
	req->fp_done = NULL;		/* blocking */
	req->h2c_eot = 1;		/* set to 1 for STM tests */

	res = xcdev->fp_rw(xcdev->xcb->xpdev->dev_hndl, qhndl, req);

	unmap_user_buf(&iocb, write);
	iocb_release(&iocb);
	if (res < 0) {
		pr_err("qdma returns %ld", res);
	}

	if (write == 0) pr_debug("q %d read =%lu", xcdev->qidx, res);
	return res;
}

struct cash3_q_params {
	loff_t write_offset;
	uint32_t cmd_status_register;
	size_t read_size;
	uint32_t cmd_pending_mask;
};

static struct cash3_q_params 
cash3_queues_C8[] = {
	//q 0 - 7 are not used for cmd, video queues
	  {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}

	// fpga level cmd q
	, {CASH3_FPGA_IOR_BUF_ADDR, CASH3_SCRATCH_REG_CMD_STATUS_ADDR_C8, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CMD_PENDING}

	//can cmd q starts
	, {CASH3_FPGA_CAN_BUF_ADDR + 0 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C8 + 0 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_0_CMD_PENDING}
	, {CASH3_FPGA_CAN_BUF_ADDR + 1 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C8 + 1 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_1_CMD_PENDING}
	, {CASH3_FPGA_CAN_BUF_ADDR + 2 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C8 + 2 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_2_CMD_PENDING}
	, {CASH3_FPGA_CAN_BUF_ADDR + 3 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C8 + 3 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_3_CMD_PENDING}
	, {CASH3_FPGA_CAN_BUF_ADDR + 4 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C8 + 4 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_4_CMD_PENDING}
	, {CASH3_FPGA_CAN_BUF_ADDR + 5 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C8 + 5 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_5_CMD_PENDING}
};

static struct cash3_q_params 
cash3_queues_C16[] = {
	//q 0 - 7 are not used for cmd, video queues
	  {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}
	, {0, 0, 0, 0}

	// fpga level cmd q
	, {CASH3_FPGA_IOR_BUF_ADDR, CASH3_SCRATCH_REG_CMD_STATUS_ADDR_C16, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CMD_PENDING}

	//can cmd q starts
	, {CASH3_FPGA_CAN_BUF_ADDR + 0 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C16 + 0 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_0_CMD_PENDING}
	, {CASH3_FPGA_CAN_BUF_ADDR + 1 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C16 + 1 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_1_CMD_PENDING}
	, {CASH3_FPGA_CAN_BUF_ADDR + 2 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C16 + 2 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_2_CMD_PENDING}
	, {CASH3_FPGA_CAN_BUF_ADDR + 3 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C16 + 3 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_3_CMD_PENDING}
	, {CASH3_FPGA_CAN_BUF_ADDR + 4 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C16 + 4 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_4_CMD_PENDING}
	, {CASH3_FPGA_CAN_BUF_ADDR + 5 * CASH3_FPGA_CAN_TX_BUFFER_SIZE, CASH3_SCRATCH_REG_CAN_CMD_STATUS_ADDR_C16 + 5 * 4, CASH3_FPGA_EVENT_SIZE, CASH3_HOST_CAN_5_CMD_PENDING}
};

static ssize_t cdev_gen_write(struct file *file, const char __user *buf,
				size_t count, loff_t *pos)
{
	struct qdma_cdev *xcdev = (struct qdma_cdev *)file->private_data;
	unsigned long dev_hndl = xcdev->xcb->xpdev->dev_hndl;
	autox_spec_t *aspec = (autox_spec_t *)xcdev->xcb->xpdev->spec;
	
	ssize_t res;
	uint32_t cmd_status_reg = 0;
 	uint32_t q_min = aspec->queue.event.begin;
	uint32_t q_max = aspec->queue.can.end;
	struct cash3_q_params *cash3_queues = (aspec->type == AUTOX_SPEC_8_CAMERAS)? cash3_queues_C8 : cash3_queues_C16;

	pr_info("cdev_gen_write  video_cnt %d   q_max %d", xcdev->xcb->xpdev->video_cnt, q_max);
	if (xcdev->qidx >= q_min && xcdev->qidx <= q_max
		&& cash3_queues[xcdev->qidx].write_offset 
		&& cash3_queues[xcdev->qidx].cmd_status_register) {
		*pos = cash3_queues[xcdev->qidx].write_offset;
	    cmd_status_reg = cash3_queues[xcdev->qidx].cmd_status_register;
	} else {
		pr_err("file 0x%p, %s, not writable\n", file, xcdev->name);
		return -EINVAL;
	}

	
	{
		int rv;
		uint32_t* shadow_irq_status = &xcdev->xcb->xpdev->cash3_shadow_fpga_irq_status;

		res = cdev_gen_read_write(file, (char *)buf, count, pos, 1);
		rv = write_cash3_user_reg(dev_hndl
			, cmd_status_reg
			, 0xffffffff);
		if (0 != rv) {
			pr_err("failed init cmd status regs rv=%d\n", rv);
			return -EIO;
		}
		__atomic_or_fetch (shadow_irq_status
			, cash3_queues[xcdev->qidx].cmd_pending_mask, __ATOMIC_RELAXED);

		if (*shadow_irq_status) {
			mutex_lock(&xcdev->xcb->xpdev->cash3_cmd_interrupt_lock);
			{
				uint32_t applied = *shadow_irq_status;
				if (applied) {
					rv = write_cash3_user_reg(dev_hndl, CASH3_FPGA_IRQ_STATUS_ADDR
						, applied);
					pr_debug("q %d applied=%x irq staus pending ... %d\n", xcdev->qidx, applied, xcdev->qidx);
					while((rv = read_cash3_user_reg(dev_hndl, CASH3_FPGA_IRQ_STATUS_ADDR))) {
						yield();
					}
					pr_debug("irq staus pending done %d 0x%x\n", xcdev->qidx, rv);

					__atomic_xor_fetch (shadow_irq_status, applied, __ATOMIC_ACQ_REL);
				}
			}
			mutex_unlock(&xcdev->xcb->xpdev->cash3_cmd_interrupt_lock);
		}
		if (rv) {
			pr_err("failed trigger fpga interrupt rv=%d\n", rv);
			return -EIO;
		}
		res = rv;

		pr_debug("cmd status pending ... q %d\n", xcdev->qidx);
		while (0xffffffff == (rv = read_cash3_user_reg(
			dev_hndl, cmd_status_reg))) {
			yield();
		}
		pr_debug("q %d cmd status pending done 0x%x\n", xcdev->qidx, rv);
		if (rv) {
			struct cash3_cmd_status_t* st = (struct cash3_cmd_status_t*)(&rv);
			pr_err("cash3_cmd_status_t=%d, %d\n", st->err_no, st->err_details);
			res = rv;
		}
	}
	pr_debug("q %d write =%lu", xcdev->qidx, res);
	return res;
}

static ssize_t cdev_gen_read(struct file *file, char __user *buf,
				size_t count, loff_t *pos)
{
	uint32_t req_reg_addr = 0;
	struct qdma_cdev *xcdev = (struct qdma_cdev *)file->private_data;
	autox_spec_t *aspec = (autox_spec_t *)xcdev->xcb->xpdev->spec;
	autox_scratch_reg_addr_spec_t *scratch_reg = &aspec->scratch_reg;
	uint32_t q_min = aspec->queue.event.begin;
	uint32_t q_max = aspec->queue.can.end;
	struct cash3_q_params *cash3_queues = (aspec->type == AUTOX_SPEC_8_CAMERAS)? cash3_queues_C8 : cash3_queues_C16;

	pr_info("  cdev_gen_read    q_max %d \n", q_max);
	
	if (xcdev->qidx >= q_min && xcdev->qidx <= q_max
		&& (cash3_queues[xcdev->qidx].read_size == 0 
			|| cash3_queues[xcdev->qidx].read_size == count)) {
	} else {
		pr_err("file 0x%p, %s, read size expected = %ld actual = %ld\n"
			, file, xcdev->name, cash3_queues[xcdev->qidx].read_size, count);
		return -EINVAL;
	}

	if (xcdev->qidx < q_min) {
		req_reg_addr = scratch_reg->video_req_cnt.addr + 4 * xcdev->qidx;
		pr_info("   cdev_gen_read  CASH3_SCRATCH_REG_V_REQ_COUNT_ADDR=%x @%d"
			, req_reg_addr, xcdev->qidx);

		if (write_cash3_user_reg(xcdev->xcb->xpdev->dev_hndl
			, req_reg_addr
			, read_cash3_user_reg(xcdev->xcb->xpdev->dev_hndl, req_reg_addr) + 1)) {
			pr_err("cannot update video request count for qid=%d", xcdev->qidx);
			return -EIO;
		}
	} else { 
		if (xcdev->qidx >= aspec->queue.can.begin && xcdev->qidx <= aspec->queue.can.end) {
			req_reg_addr = scratch_reg->can_req_cnt.addr +
				4 * (xcdev->qidx - aspec->queue.can.begin);
		}	pr_info("CASH3_SCRATCH_REG_CAN_REQ_COUNT_ADDR=%x   queue ID @%d", req_reg_addr, xcdev->qidx);

		if (write_cash3_user_reg(xcdev->xcb->xpdev->dev_hndl, req_reg_addr
			, read_cash3_user_reg(xcdev->xcb->xpdev->dev_hndl, req_reg_addr) + 1)) {
			pr_err("cannot update can request count for qid=%d", xcdev->qidx);
			return -EIO;
		}
	}

	return cdev_gen_read_write(file, (char *)buf, count, pos, 0);
}

static ssize_t cdev_aio_write(struct kiocb *iocb, const struct iovec *io,
				unsigned long count, loff_t pos)
{
	struct qdma_cdev *xcdev =
		(struct qdma_cdev *)iocb->ki_filp->private_data;
	struct cdev_async_io *caio;
	int rv = 0;
	unsigned long i;
	unsigned long qhndl;

	if (!xcdev) {
		pr_err("file 0x%p, xcdev NULL, %llu, pos %llu, W %d.\n",
				iocb->ki_filp, (u64)count, (u64)pos, 1);
		return -EINVAL;
	}

	if (!xcdev->fp_rw) {
		pr_err("No Read write handler assigned\n");
		return -EINVAL;
	}

	caio = kmem_cache_alloc(cdev_cache, GFP_KERNEL);
	if (!caio) {
		pr_err("Failed to allocate caio");
		return -ENOMEM;
	}
	memset(caio, 0, sizeof(struct cdev_async_io));
	caio->qiocb = kzalloc(count * (sizeof(struct qdma_io_cb) +
			sizeof(struct qdma_request *)), GFP_KERNEL);
	if (!caio->qiocb) {
		pr_err("failed to allocate qiocb");
		return -ENOMEM;
	}
	caio->reqv = (struct qdma_request **)(caio->qiocb + count);
	for (i = 0; i < count; i++) {
		caio->qiocb[i].private = caio;
		caio->reqv[i] = &(caio->qiocb[i].req);
		caio->qiocb[i].buf = io[i].iov_base;
		caio->qiocb[i].len = io[i].iov_len;
		rv = map_user_buf_to_sgl(&(caio->qiocb[i]), true);
		if (rv < 0)
			break;

		caio->reqv[i]->write = 1;
		caio->reqv[i]->sgcnt = caio->qiocb[i].pages_nr;
		caio->reqv[i]->sgl = caio->qiocb[i].sgl;
		caio->reqv[i]->dma_mapped = false;
		caio->reqv[i]->udd_len = 0;
		caio->reqv[i]->ep_addr = (u64)pos;
		caio->reqv[i]->no_memcpy = xcdev->no_memcpy ? 1 : 0;
		caio->reqv[i]->count = io->iov_len;
		caio->reqv[i]->timeout_ms = 10 * 1000;	/* 10 seconds */
		caio->reqv[i]->fp_done = qdma_req_completed;

	}
	if (i > 0) {
		iocb->private = caio;
		caio->iocb = iocb;
		caio->req_count = i;
		qhndl = xcdev->h2c_qhndl;
		rv = xcdev->fp_aiorw(xcdev->xcb->xpdev->dev_hndl, qhndl,
				     caio->req_count, caio->reqv);
		if (rv >= 0)
			rv = -EIOCBQUEUED;
	} else {
		pr_err("failed with %d for %lu reqs", rv, caio->req_count);
		kfree(caio->qiocb);
		kmem_cache_free(cdev_cache, caio);
	}

	return rv;
}

static ssize_t cdev_aio_read(struct kiocb *iocb, const struct iovec *io,
						unsigned long count, loff_t pos)
{
	struct qdma_cdev *xcdev =
		(struct qdma_cdev *)iocb->ki_filp->private_data;
	struct cdev_async_io *caio;
	int rv = 0;
	unsigned long i;
	unsigned long qhndl;

	if (!xcdev) {
		pr_err("file 0x%p, xcdev NULL, %llu, pos %llu, W %d.\n",
				iocb->ki_filp, (u64)count, (u64)pos, 1);
		return -EINVAL;
	}

	if (!xcdev->fp_rw) {
		pr_err("No Read write handler assigned\n");
		return -EINVAL;
	}

	caio = kmem_cache_alloc(cdev_cache, GFP_KERNEL);
	if (!caio) {
		pr_err("failed to allocate qiocb");
		return -ENOMEM;
	}
	memset(caio, 0, sizeof(struct cdev_async_io));
	caio->qiocb = kzalloc(count * (sizeof(struct qdma_io_cb) +
			sizeof(struct qdma_request *)), GFP_KERNEL);
	if (!caio->qiocb) {
		pr_err("failed to allocate qiocb");
		return -ENOMEM;
	}
	caio->reqv = (struct qdma_request **)(caio->qiocb + count);
	for (i = 0; i < count; i++) {
		caio->qiocb[i].private = caio;
		caio->reqv[i] = &(caio->qiocb[i].req);
		caio->qiocb[i].buf = io[i].iov_base;
		caio->qiocb[i].len = io[i].iov_len;
		rv = map_user_buf_to_sgl(&(caio->qiocb[i]), false);
		if (rv < 0)
			break;

		caio->reqv[i]->write = 0;
		caio->reqv[i]->sgcnt = caio->qiocb[i].pages_nr;
		caio->reqv[i]->sgl = caio->qiocb[i].sgl;
		caio->reqv[i]->dma_mapped = false;
		caio->reqv[i]->udd_len = 0;
		caio->reqv[i]->ep_addr = (u64)pos;
		caio->reqv[i]->no_memcpy = xcdev->no_memcpy ? 1 : 0;
		caio->reqv[i]->count = io->iov_len;
		caio->reqv[i]->timeout_ms = 10 * 1000;	/* 10 seconds */
		caio->reqv[i]->fp_done = qdma_req_completed;
	}
	if (i > 0) {
		iocb->private = caio;
		caio->iocb = iocb;
		caio->req_count = i;
		qhndl = xcdev->c2h_qhndl;
		rv = xcdev->fp_aiorw(xcdev->xcb->xpdev->dev_hndl, qhndl,
				     caio->req_count, caio->reqv);
		if (rv >= 0)
			rv = -EIOCBQUEUED;
	} else {
		pr_err("failed with %d for %lu reqs", rv, caio->req_count);
		kfree(caio->qiocb);
		kmem_cache_free(cdev_cache, caio);
	}

	return rv;
}

#if KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
static ssize_t cdev_write_iter(struct kiocb *iocb, struct iov_iter *io)
{
	return cdev_aio_write(iocb, io->iov, io->nr_segs, iocb->ki_pos);
}

static ssize_t cdev_read_iter(struct kiocb *iocb, struct iov_iter *io)
{
	return cdev_aio_read(iocb, io->iov, io->nr_segs, iocb->ki_pos);
}
#endif

static const struct file_operations cdev_gen_fops = {
	.owner = THIS_MODULE,
	.open = cdev_gen_open,
	.release = cdev_gen_close,
	.write = cdev_gen_write,
#if KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
	.write_iter = cdev_write_iter,
#else
	.aio_write = cdev_aio_write,
#endif
	.read = cdev_gen_read,
#if KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
	.read_iter = cdev_read_iter,
#else
	.aio_read = cdev_aio_read,
#endif
	.unlocked_ioctl = cdev_gen_ioctl,
	.llseek = cdev_gen_llseek,
};

/*
 * xcb: per pci device character device control info.
 * xcdev: per queue character device
 */
void qdma_cdev_destroy(struct qdma_cdev *xcdev)
{

	if (!xcdev) {
		pr_err("xcdev is NULL.\n");
		return;
	}
	pr_debug("destroying cdev %p", xcdev);

	if (xcdev->sys_device)
		device_destroy(qdma_class, xcdev->cdevno);

	cdev_del(&xcdev->cdev);

	kfree(xcdev);
}

int qdma_cdev_create(struct qdma_cdev_cb *xcb, struct pci_dev *pdev,
			struct qdma_queue_conf *qconf, unsigned int minor,
			unsigned long qhndl, struct qdma_cdev **xcdev_pp,
			char *ebuf, int ebuflen)
{
	struct qdma_cdev *xcdev;
	int rv;
	unsigned long *priv_data;

	xcdev = kzalloc(sizeof(struct qdma_cdev) + strlen(qconf->name) + 1,
			GFP_KERNEL);
	if (!xcdev) {
		pr_err("%s failed to allocate cdev %lu.\n",
		       qconf->name, sizeof(struct qdma_cdev));
		if (ebuf && ebuflen) {
			rv = sprintf(ebuf, "%s failed to allocate cdev %lu.\n",
				qconf->name, sizeof(struct qdma_cdev));
			ebuf[rv] = '\0';

		}
		return -ENOMEM;
	}

	xcdev->cdev.owner = THIS_MODULE;
	xcdev->xcb = xcb;
	priv_data = (qconf->q_type == Q_C2H) ?
			&xcdev->c2h_qhndl : &xcdev->h2c_qhndl;
	*priv_data = qhndl;
	xcdev->dir_init = (1 << qconf->q_type);
	strcpy(xcdev->name, qconf->name);

	xcdev->minor = minor;
	if (xcdev->minor >= xcb->cdev_minor_cnt) {
		pr_err("%s: no char dev. left.\n", qconf->name);
		if (ebuf && ebuflen) {
			rv = sprintf(ebuf, "%s cdev no cdev left.\n",
					qconf->name);
			ebuf[rv] = '\0';
		}
		rv = -ENOSPC;
		goto err_out;
	}
	xcdev->cdevno = MKDEV(xcb->cdev_major, xcdev->minor);

	cdev_init(&xcdev->cdev, &cdev_gen_fops);

	/* bring character device live */
	rv = cdev_add(&xcdev->cdev, xcdev->cdevno, 1);
	if (rv < 0) {
		pr_err("cdev_add failed %d, %s.\n", rv, xcdev->name);
		if (ebuf && ebuflen) {
			int l = sprintf(ebuf, "%s cdev add failed %d.\n",
					qconf->name, rv);
			ebuf[l] = '\0';
		}
		goto err_out;
	}

	/* create device on our class */
	if (qdma_class) {
		xcdev->sys_device = device_create(qdma_class, &(pdev->dev),
				xcdev->cdevno, NULL, "%s", xcdev->name);
		if (IS_ERR(xcdev->sys_device)) {
			rv = PTR_ERR(xcdev->sys_device);
			pr_err("%s device_create failed %d.\n",
				xcdev->name, rv);
			if (ebuf && ebuflen) {
				int l = sprintf(ebuf,
						"%s device_create failed %d.\n",
						qconf->name, rv);
				ebuf[l] = '\0';
			}
			goto del_cdev;
		}
	}

	xcdev->fp_rw = qdma_request_submit;
	xcdev->fp_aiorw = qdma_batch_request_submit;
	xcdev->qidx = qconf->qidx;

	*xcdev_pp = xcdev;
	return 0;

del_cdev:
	cdev_del(&xcdev->cdev);

err_out:
	kfree(xcdev);
	return rv;
}

/*
 * per device initialization & cleanup
 */
void qdma_cdev_device_cleanup(struct qdma_cdev_cb *xcb)
{
	if (!xcb->cdev_major)
		return;

	xcb->cdev_major = 0;
}

int qdma_cdev_device_init(struct qdma_cdev_cb *xcb)
{
	dev_t dev;
	int rv;
	struct xlnx_phy_dev *phy_dev, *tmp, *new_phy_dev;
	struct xlnx_dma_dev *xdev = NULL;

	spin_lock_init(&xcb->lock);

	xcb->cdev_minor_cnt = QDMA_MINOR_MAX;

	if (xcb->cdev_major) {
		pr_warn("major %d already exist.\n", xcb->cdev_major);
		return -EINVAL;
	}

	/* Check if same bus id device is added in global list
	 * If found then assign same major number
	 */
	mutex_lock(&xlnx_phy_dev_mutex);
	xdev = (struct xlnx_dma_dev *)xcb->xpdev->dev_hndl;
	list_for_each_entry_safe(phy_dev, tmp, &xlnx_phy_dev_list, list_head) {
		if (phy_dev->device_bus == xcb->xpdev->pdev->bus->number &&
			phy_dev->dma_device_index == xdev->dma_device_index) {
			xcb->cdev_major = phy_dev->major;
			mutex_unlock(&xlnx_phy_dev_mutex);
			return 0;
		}
	}
	mutex_unlock(&xlnx_phy_dev_mutex);

	/* allocate a dynamically allocated char device node */
	rv = alloc_chrdev_region(&dev, 0, xcb->cdev_minor_cnt,
			QDMA_CDEV_CLASS_NAME);
	if (rv) {
		pr_err("unable to allocate cdev region %d.\n", rv);
		return rv;
	}
	xcb->cdev_major = MAJOR(dev);

	new_phy_dev = kzalloc(sizeof(struct xlnx_phy_dev), GFP_KERNEL);
	if (!new_phy_dev) {
		pr_err("unable to allocate xlnx_dev.\n");
		return -ENOMEM;
	}

	new_phy_dev->major = xcb->cdev_major;
	new_phy_dev->device_bus = xcb->xpdev->pdev->bus->number;
	new_phy_dev->dma_device_index = xdev->dma_device_index;
	xlnx_phy_dev_list_add(new_phy_dev);

	return 0;
}

/*
 * driver-wide Initialization & cleanup
 */

#ifdef AUTOX
static char *autox_devnode(struct device *dev, umode_t *mode)
{
	if (mode) {
		*mode = 0666;
	}

	return NULL;
}
#endif

int qdma_cdev_init(void)
{
	qdma_class = class_create(THIS_MODULE, QDMA_CDEV_CLASS_NAME);
	if (IS_ERR(qdma_class)) {
		pr_err("%s: failed to create class 0x%lx.",
			QDMA_CDEV_CLASS_NAME, (unsigned long)qdma_class);
		qdma_class = NULL;
		return -ENODEV;
	}
#ifdef AUTOX
	qdma_class->devnode = autox_devnode;
#endif
	/* using kmem_cache_create to enable sequential cleanup */
	cdev_cache = kmem_cache_create("cdev_cache",
					sizeof(struct cdev_async_io),
					0,
					SLAB_HWCACHE_ALIGN,
					NULL);
	if (!cdev_cache) {
		pr_err("failed to allocate cdev_cache\n");
		return -ENOMEM;
	}

	return 0;
}

void qdma_cdev_cleanup(void)
{
	struct xlnx_phy_dev *phy_dev, *tmp;

	list_for_each_entry_safe(phy_dev, tmp, &xlnx_phy_dev_list, list_head) {
		unregister_chrdev_region(MKDEV(phy_dev->major, 0),
				QDMA_MINOR_MAX);
		xlnx_phy_dev_list_remove(phy_dev);
		kfree(phy_dev);
	}

	kmem_cache_destroy(cdev_cache);
	if (qdma_class)
		class_destroy(qdma_class);

}
