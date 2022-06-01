/*
 * Copyright (C) 2019 AUTOX
 */

#ifndef _AUTOX_REGS_H_
#define	_AUTOX_REGS_H_

#include "qdma_mod.h"

#define	GEN_REG_BASE			0x0

#define	VIDEO_REG_BASE			0x100
#define	VIDEO_REG_STEP			0x100

#define	BITS_GET(bits, val)		\
	(((val) >> (bits ## _SHIFT)) & (bits ## _MASK))
#define	BITS_SET(bits, val)		\
	(((val) & (bits ## _MASK)) << (bits ## _SHIFT))

/*
 * General Registers
 */
#define	GEN_REG_VERSION			0x0000

#define	GEN_REG_DMA_RESET		0x0004
#define	DMA_MASTER_RESET		0x00000001

#define	GEN_REG_CAM_MASTER_CTRL		0x0008
#define	CAM_ENABLE_SHIFT		0
#define	CAM_ENABLE_MASK			0xFF
#define	CAM_RESET_SHIFT			8
#define	CAM_RESET_MASK			0xFF
#define	CAM_CFG_DONE_SHIFT		16
#define	CAM_CFG_DONE_MASK		0xFF

#define	GEN_REG_BOARD_TEMP		0x0010

#define	GEN_REG_GPS_STATUS		0x3010
#define	GPS_STATUS_PPS			0x00000001
#define	GPS_STATUS_GPRMC		0x00000002

#define	GEN_REG_PPS_THRESH		0x14

#define	GEN_REG_GPS_TIME_0		0x3020
#define	GPS_TIME_VALID			0x00000001
#define	GPS_TIME_NS_CNT_SHIFT		1
#define	GPS_TIME_NS_CNT_MASK		0x7F
#define	GPS_TIME_US_SHIFT		8
#define	GPS_TIME_US_MASK		0xFFF
#define	GPS_TIME_MS_SHIFT		20
#define	GPS_TIME_MS_MASK		0xFFF

#define	GEN_REG_GPS_TIME_1		0x3024
#define	GPS_TIME_SEC_SHIFT		0
#define	GPS_TIME_SEC_MASK		0xFF
#define	GPS_TIME_MIN_SHIFT		8
#define	GPS_TIME_MIN_MASK		0xFF
#define	GPS_TIME_HOUR_SHIFT		16
#define	GPS_TIME_HOUR_MASK		0xFF
#define	GPS_TIME_YEAR_SHIFT		24
#define	GPS_TIME_YEAR_MASK		0xFF

#define	GEN_REG_GPS_TIME_2		0x3028
#define	GPS_TIME_DAY_SHIFT		0
#define	GPS_TIME_DAY_MASK		0xFF
#define	GPS_TIME_MON_SHIFT		8
#define	GPS_TIME_MON_MASK		0xFF

#define	GEN_REG_EPOCH_TIME_0		0x3030
#define	EPOCH_TIME_VALID		0x00000001
#define	EPOCH_TIME_USEC_SHIFT		8
#define	EPOCH_TIME_USEC_MASK		0xFFFFFF

#define	GEN_REG_EPOCH_TIME_1		0x3034

/*
 * Video Channel Registers
 */
#define	VIDEO_REG_CONTROL		0x04
#define	VIDEO_CTRL_EN			0x00000001
#define	VIDEO_CTRL_TPG_SHIFT		1
#define	VIDEO_CTRL_TPG_MASK		0x3
#define	VIDEO_CTRL_TPG_BAR		2
#define	VIDEO_CTRL_MODE_SHIFT		4
#define	VIDEO_CTRL_MODE_MASK		0xF
#define	VIDEO_CTRL_MODE_C2H		5
#define	VIDEO_CTRL_FORMAT_SHIFT		8
#define	VIDEO_CTRL_FORMAT_MASK		0x3
#define	VIDEO_CTRL_FORMAT_UYVY		0
#define	VIDEO_CTRL_FORMAT_YUYV		1
#define	VIDEO_CTRL_FORMAT_YVYU		2

#define	VIDEO_REG_CONFIG		0x08
#define	VIDEO_QID_SHIFT			0
#define	VIDEO_QID_MASK			0xFFFF
#define	VIDEO_BUF_SZ_SHIFT		16
#define	VIDEO_BUF_SZ_MASK		0xFFFF

#define	VIDEO_REG_RESOLUTION		0x0C
#define	VIDEO_RES_WIDTH_SHIFT		0
#define	VIDEO_RES_WIDTH_MASK		0xFFFF
#define	VIDEO_RES_HEIGHT_SHIFT		16
#define	VIDEO_RES_HEIGHT_MASK		0xFFFF

#define	VIDEO_REG_TRIGGER_CTRL		0x10
#define	VIDEO_TRIGGER_ENABLE		0x00000001
#define	VIDEO_TRIGGER_POLARITY		0x00000002
#define	VIDEO_TRIGGER_FPS_SHIFT		8
#define	VIDEO_TRIGGER_FPS_MASK		0xFF
#define	VIDEO_TRIGGER_DELAY_SHIFT	16
#define	VIDEO_TRIGGER_DELAY_MASK	0xFFFF
#define	VIDEO_TRIGGER_FPS_DEFAULT	20
#define	VIDEO_TRIGGER_FPS_MAX		25

static inline void autox_gen_reg_write(struct xlnx_pci_dev *xpdev,
		unsigned int reg, unsigned int val)
{
	qdma_device_write_user_register(xpdev, GEN_REG_BASE + reg, val);
}

static inline unsigned int autox_gen_reg_read(struct xlnx_pci_dev *xpdev,
		unsigned int reg, unsigned int *value)
{
	return qdma_device_read_user_register(xpdev, GEN_REG_BASE + reg, value);
}

#endif	/* _AUTOX_REGS_H_ */
