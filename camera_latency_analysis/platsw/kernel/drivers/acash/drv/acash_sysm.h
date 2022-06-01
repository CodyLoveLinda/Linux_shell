/* 
 * Copyright (C) 2019 AutoX, Inc. 
 */

#ifndef _AUTOX_SYSM_H_
#define	_AUTOX_SYSM_H_

#include <linux/cdev.h>

#include "linux/autox_api.h"
#include "qdma_mod.h"
#include "acash_utils.h"

typedef struct autox_sysm {
	unsigned char		id;
	struct xlnx_pci_dev	*xpdev;
	struct cdev		cdev;
	dev_t			dev;
} autox_sysm_t;

extern int autox_sysm_alloc(struct xlnx_pci_dev *xpdev);
extern void autox_sysm_free(struct xlnx_pci_dev *xpdev);
extern int autox_sysm_init(struct xlnx_pci_dev *xpdev);
extern void autox_sysm_finish(struct xlnx_pci_dev *xpdev);

#endif	/* _AUTOX_SYSM_H_ */
