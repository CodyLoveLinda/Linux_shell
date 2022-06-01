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

#include "acash_reg.h"
#include "acash_gps.h"

#define	AUTOX_GPS_DRIFT_THRESHOLD	5	/* times */
#define	AUTOX_GPS_SYNC_INTERVAL		60	/* second */

spinlock_t autox_gps_lock;

static unsigned int autox_gps_param_sync_interval = AUTOX_GPS_SYNC_INTERVAL;

static struct xlnx_pci_dev *autox_gps_master_dev = NULL;
static struct task_struct *autox_gps_taskp = NULL;
static struct completion autox_gps_completion;
static int autox_gps_valid_once = 0;
static unsigned char autox_gps_cnt = 0;

static const char gps_stats_label[GPS_STATS_NUM][AUTOX_STATS_LABEL_LEN] = {
	"GPS unlock",
	"System clock drift"
};

static int autox_gps_time_get(autox_gps_t *agps, struct timespec64 *tv)
{
	int rv;
	struct xlnx_pci_dev *xpdev = agps->xpdev;
	unsigned int val;

	spin_lock(&agps->gen_lock);
	rv = autox_gen_reg_read(xpdev, GEN_REG_EPOCH_TIME_0, &val);
	if ((!(val & EPOCH_TIME_VALID)) || (val == (unsigned int)-1)) {
		spin_unlock(&agps->gen_lock);
		ALOG_ERR(xpdev, "GPS time is not valid\n");
		return -EAGAIN;
	} else if (!autox_gps_valid_once) {
		autox_gps_valid_once = 1;
	}

	tv->tv_nsec = BITS_GET(EPOCH_TIME_USEC, val) * NSEC_PER_USEC;

	rv = autox_gen_reg_read(xpdev, GEN_REG_EPOCH_TIME_1, &val);
	tv->tv_sec = val;
	spin_unlock(&agps->gen_lock);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 18, 0)
	ALOG_DEBUG(xpdev, "GPS time %lld.%09ld\n", tv->tv_sec, tv->tv_nsec);
#else
	ALOG_DEBUG(xpdev, "GPS time %ld.%09ld\n", tv->tv_sec, tv->tv_nsec);
#endif
	return 0;
}

static int autox_gps_time_check(autox_gps_t *agps, struct timespec64 *tv_gps)
{
	struct xlnx_pci_dev *xpdev = agps->xpdev;
	struct timespec64 tv_sys[1];
	long diff;
	long delay;
	long thresh;
	long drift_sum;
	long drift_cnt;
	long drift = 0;

	if (agps->sync_cnt & 0x1) {
		autox_get_timestamp(tv_sys);
		if (autox_gps_time_get(agps, tv_gps)) {
			return -1;
		}
	} else {
		if (autox_gps_time_get(agps, tv_gps)) {
			return -1;
		}
		autox_get_timestamp(tv_sys);
	}

	diff = tv_diff(tv_gps, tv_sys);

	/* Bypass the time validation for the first sync */
	if (!agps->tv_last.tv_sec && !agps->tv_last.tv_nsec) {
		goto end;
	}

	delay = tv_diff(&agps->tv_last, tv_gps);
	if (delay <= 0) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 18, 0)
		ALOG_ERR(xpdev, "SYS time: %lld.%09ld, GPS time: %lld.%09ld, "
		    "Last GPS time: %lld.%09ld, abnormal GPS time jump.\n",
		    tv_sys->tv_sec, tv_sys->tv_nsec,
		    tv_gps->tv_sec, tv_gps->tv_nsec,
		    agps->tv_last.tv_sec, agps->tv_last.tv_nsec);
#else
		ALOG_ERR(xpdev, "SYS time: %ld.%09ld, GPS time: %ld.%09ld, "
		    "Last GPS time: %ld.%09ld, abnormal GPS time jump.\n",
		    tv_sys->tv_sec, tv_sys->tv_nsec,
		    tv_gps->tv_sec, tv_gps->tv_nsec,
		    agps->tv_last.tv_sec, agps->tv_last.tv_nsec);
#endif
		return -1;
	}

	drift = diff * USEC_PER_SEC / delay;
	drift_sum = agps->drift_sum + drift;
	drift_cnt = agps->drift_cnt + 1;
	drift = drift_sum / drift_cnt;

	/* Check the current drift */
	thresh = AUTOX_GPS_DRIFT_THRESHOLD * drift * delay / USEC_PER_SEC;
	if (((thresh > 0) && (diff > thresh)) ||
	    ((thresh < 0) && (diff < thresh))) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 18, 0)
		ALOG_ERR(xpdev, "SYS time: %lld.%09ld, GPS time: %lld.%09ld, "
		    "diff %ldus, system drift %ldus, diff is too much.\n",
		    tv_sys->tv_sec, tv_sys->tv_nsec,
		    tv_gps->tv_sec, tv_gps->tv_nsec, diff, drift);
#else
		ALOG_ERR(xpdev, "SYS time: %ld.%09ld, GPS time: %ld.%09ld, "
		    "diff %ldus, system drift %ldus, diff is too much.\n",
		    tv_sys->tv_sec, tv_sys->tv_nsec,
		    tv_gps->tv_sec, tv_gps->tv_nsec, diff, drift);
#endif
		return -1;
	}

	agps->drift_sum = drift_sum;
	agps->drift_cnt = drift_cnt;
	agps->stats[GPS_STATS_DRIFT].cnt = drift;
end:

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 18, 0)
	ALOG_INFO(xpdev, "SYS time %lld.%09ld, GPS time %lld.%09ld, "
	    "diff %ldus, system drift %ldus\n",
	    tv_sys->tv_sec, tv_sys->tv_nsec,
	    tv_gps->tv_sec, tv_gps->tv_nsec, diff, drift);
#else
	ALOG_INFO(xpdev, "SYS time %ld.%09ld, GPS time %ld.%09ld, "
	    "diff %ldus, system drift %ldus\n",
	    tv_sys->tv_sec, tv_sys->tv_nsec,
	    tv_gps->tv_sec, tv_gps->tv_nsec, diff, drift);
#endif
	return 0;
}

static int autox_gps_time_sync(autox_gps_t *agps)
{
	struct xlnx_pci_dev *xpdev = agps->xpdev;
	struct timespec64 tv;
	int ret;

	if ((ret = autox_gps_time_check(agps, &tv))) {
		return ret;
	}

	/* update system time */
	if ((ret = do_settimeofday64(&tv))) {
		ALOG_ERR(xpdev, "failed to set system time. "
		    "GPS time sync aborted\n");
		return ret;
	}

	agps->tv_last = tv;
	agps->sync_cnt++;

	return 0;
}

static int autox_gps_sync_thread(void *data)
{
	autox_gps_t *agps = (autox_gps_t *)data;
	struct xlnx_pci_dev *xpdev = agps->xpdev;
	unsigned int delay_time;
	long result;

	WARN_ON(xpdev != autox_gps_master_dev);

	while (!kthread_should_stop()) {

		(void) autox_gps_time_sync(agps);

		delay_time = autox_gps_param_sync_interval;
		if (delay_time == 0) {
			delay_time = AUTOX_GPS_SYNC_INTERVAL;
		}
		result = wait_for_completion_interruptible_timeout(
		    &autox_gps_completion, msecs_to_jiffies(delay_time * 1000));
		if (result > 0) {
			if (autox_gps_master_dev == NULL) {
				break;
			}
			/* Restart the completion wait */
			reinit_completion(&autox_gps_completion);
		} else if (result != 0) {
			/* Interrupted or module is being unloaded */
			ALOG_ERR(xpdev, "gps completion is interrupted!");
			break;
		}
	}

	return 0;
}

/*
 * create the GPS sync thread and schedule it with the second highest priority
 * that is lower than the watchdog (99) and higher than the interrupts (50).
 */
static void autox_gps_thread_init(autox_gps_t *agps)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
	sched_set_fifo(autox_gps_taskp);
#else
	struct sched_param param = { .sched_priority = 60 };
	sched_setscheduler(autox_gps_taskp, SCHED_FIFO, &param);
#endif
	init_completion(&autox_gps_completion);
	autox_gps_taskp = kthread_run(autox_gps_sync_thread, agps,
	    "GPS sync thread");
}

/*
 * destroy the GPS sync thread
 */
static void autox_gps_thread_finish(autox_gps_t *agps)
{
	complete(&autox_gps_completion);
	if (autox_gps_taskp) {
		kthread_stop(autox_gps_taskp);
		autox_gps_taskp = NULL;
	}
}

static void autox_gps_config(autox_gps_t *agps)
{
	struct xlnx_pci_dev *xpdev = agps->xpdev;

	ALOG_DEBUG(xpdev, "done");
}

static unsigned int autox_gps_status(autox_gps_t *agps)
{
	int rv;
	struct xlnx_pci_dev *xpdev = agps->xpdev;
	unsigned int val;

	/* Read GPS status from FPGA */
	rv = autox_gen_reg_read(xpdev, GEN_REG_GPS_STATUS, &val);

	ALOG_INFO(xpdev, "PPS %s, GPRMC %s\n",
	    (val & GPS_STATUS_PPS) ? "OK" : "OFF",
	    (val & GPS_STATUS_GPRMC) ? "OK" : "OFF");

	return val;
}

static long autox_gps_ioctl(struct file *filp, unsigned int cmd,
    unsigned long arg)
{
	autox_gps_t *agps = filp->private_data;
	struct xlnx_pci_dev *xpdev = agps->xpdev;
	int err = 0;

	ALOG_DEBUG(xpdev, "file 0x%p\n", filp);

	switch (cmd) {
	case AUTOX_IOC_GPS_TIME:
	{
		struct timespec64 tv;

		ALOG_DEBUG(xpdev, "AUTOX_IOC_GPS_TIME\n");

		if ((err = autox_gps_time_get(agps, &tv))) {
			break;
		}

		if (copy_to_user((void __user *)arg, &tv, sizeof(tv))) {
			err = -EFAULT;
		}
		break;
	}
	case AUTOX_IOC_GPS_STATUS:
	{
		unsigned int result = 0;
		unsigned int status;

		ALOG_DEBUG(xpdev, "AUTOX_IOC_GPS_STATUS\n");

		status = autox_gps_status(agps);
		if (status & GPS_STATUS_GPRMC) {
			result |= AUTOX_GPS_GPRMC_OK;
		}
		if (status & GPS_STATUS_PPS) {
			result |= AUTOX_GPS_PPS_OK;
		}

		if (copy_to_user((void __user *)arg, &result, sizeof(result))) {
			err = -EFAULT;
		}
		break;
	}
	case AUTOX_IOC_GPS_STATS:
	{
		autox_gps_stats_t stats;
		int i;

		ALOG_DEBUG(xpdev, "AUTOX_IOC_GPS_STATS\n");

		for (i = 0; i < GPS_STATS_NUM; i++) {
			stats.stats[i] = agps->stats[i].cnt;
		}

		if (copy_to_user((void __user *)arg, &stats,
		    sizeof(autox_gps_stats_t))) {
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

static int autox_gps_open(struct inode *inode, struct file *filp)
{
	autox_gps_t *agps = container_of(inode->i_cdev, autox_gps_t, cdev);
	struct xlnx_pci_dev *xpdev = agps->xpdev;

	filp->private_data = agps;

	ALOG_DEBUG(xpdev, "device opened, file 0x%p\n", filp);

	return 0;
}

static int autox_gps_release(struct inode *inode, struct file *filp)
{
	autox_gps_t *agps = filp->private_data;
	struct xlnx_pci_dev *xpdev = agps->xpdev;

	ALOG_DEBUG(xpdev, "device released, file 0x%p\n", filp);

	return 0;
}

struct file_operations autox_gps_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = autox_gps_ioctl,
	.open		= autox_gps_open,
	.release	= autox_gps_release
};

static int autox_gps_cdev_init(autox_gps_t *agps)
{
	struct xlnx_pci_dev *xpdev = agps->xpdev;
	char dev_name[16];
	int minor;
	int ret;

	/* Create the character device /dev/autox_gps# */
	sprintf(dev_name, AUTOX_DEV_GPS "%u", agps->id);
/* FIXME */
//	minor = xpdev->qmax + agps->id;
	minor = 100 + agps->id;

	if ((ret = autox_cdev_create(xpdev, minor,
	    &agps->cdev, &agps->dev, &autox_gps_fops, dev_name, agps))) {
		ALOG_ERR(xpdev, "failed to create cdev %s, minor %d\n",
		    dev_name, minor);
		return ret;
	}

	return 0;
}

static void autox_gps_cdev_finish(autox_gps_t *agps)
{
	autox_cdev_destroy(agps->dev, &agps->cdev);
}

static void autox_gps_stats_init(autox_gps_t *agps)
{
	int i;

	for (i = 0; i < GPS_STATS_NUM; i++) {
		agps->stats[i].label = gps_stats_label[i];
	}

	snprintf(agps->prefix, AUTOX_LOG_PREFIX_LEN, "%s",
	    ((struct xlnx_dma_dev *)agps->xpdev->dev_hndl)->conf.name);
}

int autox_gps_init(struct xlnx_pci_dev *xpdev)
{
	autox_gps_t *agps = (autox_gps_t *)xpdev->gps;

	if (autox_gps_cdev_init(agps)) {
		return -1;
	}

	spin_lock(&autox_gps_lock);
	if (autox_gps_master_dev != NULL) {
		spin_unlock(&autox_gps_lock);
		return 0;
	}

	autox_gps_master_dev = xpdev;
	spin_unlock(&autox_gps_lock);

	spin_lock_init(&agps->gen_lock);

	autox_gps_config(agps);

	autox_gps_status(agps);

	autox_gps_thread_init(agps);

	autox_gps_stats_init(agps);

	ALOG_DEBUG(xpdev, "done\n");

	return 0;
}

int autox_gps_alloc(struct xlnx_pci_dev *xpdev)
{
	autox_gps_t *agps;

	agps = kzalloc(sizeof(autox_gps_t), GFP_KERNEL);
	if (!agps) {
		pr_err("failed to alloc mem for GPS\n");
		return -1;
	}

	xpdev->gps = agps;

	agps->xpdev = xpdev;
	agps->id = autox_gps_cnt;
	autox_gps_cnt++;

	return 0;
}

void autox_gps_free(struct xlnx_pci_dev *xpdev)
{
	if (xpdev->gps) {
		kfree(xpdev->gps);
		xpdev->gps = NULL;
	}
}

void autox_gps_finish(struct xlnx_pci_dev *xpdev)
{
	autox_gps_t *agps = (autox_gps_t *)xpdev->gps;

	spin_lock(&autox_gps_lock);
	if (xpdev != autox_gps_master_dev) {
		spin_unlock(&autox_gps_lock);
		return;
	}
	autox_gps_master_dev = NULL;
	spin_unlock(&autox_gps_lock);

	autox_gps_thread_finish(agps);

	autox_gps_cdev_finish(agps);

	ALOG_DEBUG(xpdev, "done\n");
}

void autox_gps_changed(autox_gps_t *agps)
{
	unsigned int status;

	status = autox_gps_status(agps);

	if (!(status)) {
		AUTOX_STATS_LOGX(agps, GPS_STATS_UNLOCK, 1, 0);
	}
}

module_param_named(gpssyncintv,
    autox_gps_param_sync_interval, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(gpssyncintv, "GPS time sync interval in seconds");
