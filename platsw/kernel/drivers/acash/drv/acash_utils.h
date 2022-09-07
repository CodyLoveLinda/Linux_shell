/* 
 * Copyright (C) 2019 AutoX, Inc. 
 */

#ifndef _AUTOX_UTILS_H_
#define	_AUTOX_UTILS_H_

#include "libqdma/xdev.h"

#define	AUTOX_TRACE_PROBE		(1 << 0)
#define	AUTOX_TRACE_VIDEO		(1 << 1)
#define	AUTOX_TRACE_GPS			(1 << 2)
#define	AUTOX_TRACE_STATS		(1 << 10)
#define	AUTOX_TRACE_REG			(1 << 29)
#define	AUTOX_TRACE_BUF			(1 << 30)
#define	AUTOX_TRACE_ERR			(1 << 31)

#define	autox_trace(flag, msg...) \
	do { \
		if (autox_trace_param & flag) \
			printk(KERN_DEBUG AUTOX_DRV_NAME ": " msg); \
	} while (0)

#define	ASSERT(x)							\
	if (!(x)) {							\
		printk(KERN_WARNING "ASSERT FAILED %s:%d: %s\n",	\
		    __FILE__, __LINE__, #x);				\
	}

#define	MIN(x, y)	(((x) < (y)) ? (x) : (y))

#undef	pr_fmt
#define pr_fmt(fmt)	KBUILD_MODNAME ":< %s >: " fmt, __func__

#define	ALOG(f, xpdev, fmt, ...)	\
	f("%s: " fmt, ((struct xlnx_dma_dev *)xpdev->dev_hndl)->conf.name, \
	## __VA_ARGS__)
#define	ALOG_DEBUG(xpdev, fmt, ...)	\
	ALOG(pr_debug, xpdev, fmt, ## __VA_ARGS__)
#define	ALOG_INFO(xpdev, fmt, ...)	\
	ALOG(pr_info, xpdev, fmt, ## __VA_ARGS__)
#define	ALOG_ERR(xpdev, fmt, ...)	\
	ALOG(pr_err, xpdev, "ERROR, " fmt, ## __VA_ARGS__)

#define	AUTOX_STATS_LOGX(p, id, count, interval)			\
	if (autox_stats_log(&(p)->stats[id], count, interval)) {	\
		pr_err("%s: WARNING! %s, %lu\n",		\
		    (p)->prefix, (p)->stats[id].label,	\
		    (p)->stats[id].cnt);			\
	}
#define	AUTOX_STATS_LOG(p, id)	AUTOX_STATS_LOGX(p, id, 1, -1)
#define	AUTOX_STATS_N(p, id, n)	((p)->stats[id].cnt += (n))
#define	AUTOX_STATS(p, id)	((p)->stats[id].cnt++)

#define	AUTOX_LOG_PREFIX_LEN		32
#define	AUTOX_STATS_LABEL_LEN		24

typedef struct autox_stats {
	unsigned long		cnt;	/* count of errors */
	unsigned long		ts;	/* jiffies of last report */
	const char		*label;
} autox_stats_t;

extern int autox_cdev_create(struct xlnx_pci_dev *xpdev,
		unsigned int minor, struct cdev *cdev, dev_t *dev,
		struct file_operations *fops, char *dev_name, void *drvdata);
extern void autox_cdev_destroy(dev_t dev, struct cdev *cdev);
extern int autox_stats_log(autox_stats_t *stats, int count, int interval);
extern void autox_get_timestamp(struct timespec64 *tv);
extern long tv_diff(struct timespec64 *tv1, struct timespec64 *tv2);
//extern void tv_sub(struct timeval *tv, unsigned int usec);
//extern void tv_add(struct timeval *tv, unsigned int usec);
extern unsigned long div_round(unsigned long val, unsigned long round);

extern unsigned int autox_trace_param;

#endif	/* _AUTOX_UTILS_H_ */
