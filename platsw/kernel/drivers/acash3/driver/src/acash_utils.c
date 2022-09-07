/* 
 * Copyright (C) 2019 AutoX, Inc. 
 */

#include <asm/param.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>
#include <linux/cdev.h>
#include <linux/fs.h>

#include "qdma_mod.h"
#include "acash_utils.h"

/* module trace log parameter */
unsigned int autox_trace_param = 0;
/* log interval in second for statistics log throttling */
unsigned int autox_stats_log_interval = 1;

extern struct class *qdma_class;

int autox_cdev_create(struct xlnx_pci_dev *xpdev,
		unsigned int minor, struct cdev *cdev, dev_t *dev,
		struct file_operations *fops, char *dev_name, void *drvdata)
{
	struct qdma_cdev_cb *xcb = &xpdev->cdev_cb;
	struct device *devicep;
	dev_t devno;
	int err;

	pr_debug("xpdev 0x%p, minor %u, cdevp 0x%p, devp 0x%p, "
	    "fops 0x%p, dev_name %s, drvdata 0x%p, qdma_class 0x%p\n",
	    xpdev, minor, cdev, dev, fops, dev_name, drvdata, qdma_class);

	devno = (dev_t)MKDEV(xcb->cdev_major, minor);

	devicep = device_create(qdma_class, NULL, devno, drvdata, dev_name);
	if (IS_ERR(devicep)) {
		pr_err("failed to create device /dev/%s\n",
		    dev_name);
		return -1;
	}

	cdev_init(cdev, fops);

	err = cdev_add(cdev, devno, 1);
	if (err) {
		pr_err("failed to add cdev for /dev/%s\n", dev_name);
		device_destroy(qdma_class, devno);
		return -1;
	}

	*dev = devno;

	pr_info("device /dev/%s created\n", dev_name);

	return 0;
}

void autox_cdev_destroy(dev_t dev, struct cdev *cdev)
{
	cdev_del(cdev);
	device_destroy(qdma_class, dev);
}

int autox_stats_log(autox_stats_t *stats, int count, int interval)
{
	unsigned long log_interval;

	if (stats == NULL) {
		return 0;
	}

	stats->cnt += count;

	if ((autox_trace_param & AUTOX_TRACE_STATS) == 0) {
		log_interval = (interval >= 0) ? interval :
		    (autox_stats_log_interval * HZ);

		if ((unsigned long)(jiffies - stats->ts) < log_interval) {
			return 0;
		}
	}

	stats->ts = jiffies;
	return 1;
}

void autox_get_timestamp(struct timespec64 *tv)
{
	ktime_get_ts64(tv);
}

long tv_diff(struct timespec64 *tv1, struct timespec64 *tv2)
{
	return ((tv2->tv_sec - tv1->tv_sec) * NSEC_PER_SEC +
	    tv2->tv_nsec - tv1->tv_nsec) / NSEC_PER_USEC;
}

//void tv_sub(struct timeval *tv, unsigned int usec)
//{
//       if (tv->tv_usec < usec) {
//               tv->tv_sec--;
//               tv->tv_usec += USEC_PER_SEC;
//       }
//       tv->tv_usec -= usec;
//}
//
//void tv_add(struct timeval *tv, unsigned int usec)
//{
//       tv->tv_usec += usec;
//       if (tv->tv_usec >= USEC_PER_SEC) {
//               tv->tv_sec++;
//               tv->tv_usec -= USEC_PER_SEC;
//       }
//}

unsigned long div_round(unsigned long val, unsigned long round)
{
	unsigned long result;

	result = val / round;
	if ((val - (result * round)) > ((result + 1) * round - val)) {
		result++;
	}

	return result;
}
