/* 
 * Copyright (C) 2019 AutoX, Inc. 
 */

#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/hrtimer.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/types.h>
#else
#include <linux/sched.h>
#endif

#include "linux/cash_scratch_registers.h"
#include "linux/cash_host_cmds.h"
#include "linux/cash_fpga_events.h"
#include "libqdma_cash.h"
#include "acash_reg.h"
#include "acash_sysm.h"

spinlock_t autox_sysm_lock;

static unsigned char autox_sysm_cnt = 0;

static void autox_sysm_config(autox_sysm_t *asysm)
{
	struct xlnx_pci_dev *xpdev = asysm->xpdev;

	ALOG_DEBUG(xpdev, "done");
}

static long autox_sysm_ioctl(struct file *filp, unsigned int cmd,
    unsigned long arg)
{
	autox_sysm_t *asysm = filp->private_data;
	struct xlnx_pci_dev *xpdev = asysm->xpdev;
	int err = 0;

	ALOG_DEBUG(xpdev, "file 0x%p\n", filp);

	switch (cmd) {
	case AUTOX_IOC_SYSM_VER:
	{
		uint32_t fw_fpga_ver = 
			read_cash_user_reg(xpdev->dev_hndl, CASH_SCRATCH_REG_FPGA_VER_ADDR);
		autox_fw_fpga_ver_t *ver = (void*)&fw_fpga_ver;

		ALOG_INFO(xpdev, "cash_fw_ver=%d file_fw_ver=%d, hw_ver=%d.%d"
			, ver->fw_ver
			, ver->backup_fw_ver
			, ver->hw_major_ver
			, ver->hw_minor_ver);

		if (copy_to_user((void __user *)arg, &fw_fpga_ver, sizeof(fw_fpga_ver))) {
			err = -EFAULT;
		}
		break;
	}

	case AUTOX_IOC_SYSM_UPDATE:
	{
		autox_fw_update_t *ioc_fw_arg;
		int rc = 0;
		struct cash_host_cmd_t *cmd;
		struct cash_host_cmd_backup_fpga_image_flash_t *fpga_image_flash;

		ALOG_DEBUG(xpdev, "AUTOX_IOC_SYSM_UPDATE\n");

		ioc_fw_arg = vmalloc(sizeof(autox_fw_update_t));
		cmd = (void*)&ioc_fw_arg->fw_cmd;
		fpga_image_flash = (void*)cmd->payload;

		if (ioc_fw_arg == NULL) {
			ALOG_ERR(xpdev, "AUTOX_IOC_SYSTEM_UPDATE vmalloc err\n");
			err = -EFAULT;
		}
		
		if (copy_from_user(ioc_fw_arg, (void *)arg,
				sizeof(autox_fw_update_t))) {
			ALOG_ERR(xpdev, "AUTOX_IOC_SYSTEM_UPDATE copy_from_user err\n");
			err = -EFAULT;
		}

		ALOG_DEBUG(xpdev, 
			"AUTOX_IOC_SYSTEM_UPDATE ioc_fw_arg->fw_data_size (%lu)", 
			ioc_fw_arg->fw_data_size);

		fpga_image_flash->flash_image_size = ioc_fw_arg->fw_data_size;

		memcpy(fpga_image_flash->flash_image
			, ioc_fw_arg->fw_data
			, fpga_image_flash->flash_image_size);

		cmd->head.cmd = CASH_HOST_CMD_BACKUP_FPGA_IMAGE_FLASH;
		cmd->head.cmd_count = 1;

		rc = send_host_cmd(xpdev->dev_hndl
			, cmd
			, (200*1024*1024)
			, false);
		if (rc < 0) {
			err = -EFAULT;
		}

		vfree(ioc_fw_arg);
		break;
	}

	case AUTOX_IOC_SYSM_CMDSTS:
	{
		int rc = 0;

		ALOG_DEBUG(xpdev, "AUTOX_IOC_SYSM_CMDSTS\n");

		rc = read_cash_user_reg(xpdev->dev_hndl
			, CASH_SCRATCH_REG_CMD_STATUS_ADDR);

		printk(KERN_ERR "CASH_SCRATCH_REG_CMD_STATUS_ADDR %x\n", rc);
		if (rc != 0) {
			err = -EFAULT;
		}
		break;
	}

	case AUTOX_IOC_SYSM_ENABLE:
	{
		char mem[512] = {0};
		int rc = 0;
		struct cash_host_cmd_t *cmd = (void*)mem;

		ALOG_DEBUG(xpdev, "AUTOX_IOC_SYSM_ENABLE\n");
		
		cmd->head.cmd = CASH_HOST_CMD_ENABLE_BACKUP_FPGA_IMAGE;
		cmd->head.cmd_count = 1;

		rc = send_host_cmd(xpdev->dev_hndl
			, cmd
			, sizeof(mem)
			, true);

		if (rc < 0) {
			err = -EFAULT;
		}

		break;
	}

	default:
		err = -EINVAL;
		break;
	}

	return err;
}

static int autox_sysm_open(struct inode *inode, struct file *filp)
{
	autox_sysm_t *asysm = container_of(inode->i_cdev, autox_sysm_t, cdev);
	struct xlnx_pci_dev *xpdev = asysm->xpdev;

	filp->private_data = asysm;

	ALOG_DEBUG(xpdev, "device opened, file 0x%p\n", filp);

	return 0;
}

static int autox_sysm_release(struct inode *inode, struct file *filp)
{
	autox_sysm_t *asysm = filp->private_data;
	struct xlnx_pci_dev *xpdev = asysm->xpdev;

	ALOG_DEBUG(xpdev, "device released, file 0x%p\n", filp);

	return 0;
}

struct file_operations autox_sysm_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = autox_sysm_ioctl,
	.open		= autox_sysm_open,
	.release	= autox_sysm_release
};

static int autox_sysm_cdev_init(autox_sysm_t *asysm)
{
	struct xlnx_pci_dev *xpdev = asysm->xpdev;
	char dev_name[16];
	int minor;
	int ret;

	/* Create the character device /dev/autox_sysm# */
	sprintf(dev_name, AUTOX_DEV_SYSM "%u", asysm->id);
/* FIXME */
//	minor = xpdev->qmax + asysm->id;
	minor = 101 + asysm->id;

	if ((ret = autox_cdev_create(xpdev, minor,
	    &asysm->cdev, &asysm->dev, &autox_sysm_fops, dev_name, asysm))) {
		ALOG_ERR(xpdev, "failed to create cdev %s, minor %d\n",
		    dev_name, minor);
		return ret;
	}

	return 0;
}

static void autox_sysm_cdev_fini(autox_sysm_t *asysm)
{
	autox_cdev_destroy(asysm->dev, &asysm->cdev);
}

int autox_sysm_init(struct xlnx_pci_dev *xpdev)
{
	autox_sysm_t *asysm = (autox_sysm_t *)xpdev->sysm;

	if (autox_sysm_cdev_init(asysm)) {
		return -1;
	}

	spin_lock(&autox_sysm_lock);
	spin_unlock(&autox_sysm_lock);

	autox_sysm_config(asysm);

	return 0;
}

int autox_sysm_alloc(struct xlnx_pci_dev *xpdev)
{
	autox_sysm_t *asysm;

{
	char *buf = vmalloc(200*1024*1024);
	if (buf == 0) {
		pr_err("failed to allocation vmalloc\n");
		return -1;
	}
	vfree(buf);
}
	asysm = kzalloc(sizeof(autox_sysm_t), GFP_KERNEL);
	if (!asysm) {
		pr_err("failed to alloc mem for GPS\n");
		return -1;
	}

	xpdev->sysm = asysm;

	asysm->xpdev = xpdev;
	asysm->id = autox_sysm_cnt;
	autox_sysm_cnt++;

	return 0;
}

void autox_sysm_free(struct xlnx_pci_dev *xpdev)
{
	if (xpdev->sysm) {
		kfree(xpdev->sysm);
		xpdev->sysm = NULL;
	}
}

void autox_sysm_finish(struct xlnx_pci_dev *xpdev)
{
	autox_sysm_t *asysm = (autox_sysm_t *)xpdev->sysm;

	spin_lock(&autox_sysm_lock);
	spin_unlock(&autox_sysm_lock);

	autox_sysm_cdev_fini(asysm);

	ALOG_DEBUG(xpdev, "done\n");
}
