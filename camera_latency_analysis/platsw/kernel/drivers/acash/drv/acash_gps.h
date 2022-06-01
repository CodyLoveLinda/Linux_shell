/* 
 * Copyright (C) 2019 AutoX, Inc. 
 */

#ifndef _AUTOX_GPS_H_
#define	_AUTOX_GPS_H_

#include <linux/cdev.h>

#include "linux/autox_api.h"
#include "qdma_mod.h"
#include "acash_utils.h"

typedef struct autox_gps {
	unsigned char		id;
	struct xlnx_pci_dev	*xpdev;
	struct cdev		cdev;
	dev_t			dev;
	spinlock_t		gen_lock;
	long			drift_sum;	/* usec */
	long			drift_cnt;
	unsigned long		sync_cnt;
	struct timespec64       tv_last;
	autox_stats_t		stats[GPS_STATS_NUM];
	char			prefix[AUTOX_LOG_PREFIX_LEN];
} autox_gps_t;

extern int autox_gps_alloc(struct xlnx_pci_dev *xpdev);
extern void autox_gps_free(struct xlnx_pci_dev *xpdev);
extern int autox_gps_init(struct xlnx_pci_dev *xpdev);
extern void autox_gps_finish(struct xlnx_pci_dev *xpdev);

extern spinlock_t autox_gps_lock;

#endif	/* _AUTOX_GPS_H_ */
