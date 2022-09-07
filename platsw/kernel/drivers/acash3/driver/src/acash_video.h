/* 
 * Copyright (C) 2019 AutoX, Inc. 
 */

#ifndef _AUTOX_VIDEO_H_
#define	_AUTOX_VIDEO_H_

#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>

#include "libqdma_cash3.h"
#include "../../include/linux/autox_api.h"

#define VIDEO_FORMAT_NUM		4
#define VIDEO_FORMAT_USE		1
#define VIDEO_BUF_MIN_NUM		4

#define BUFFER_CHUNK_SIZE		(2*1024*1024)

#define	T_ROW(llp, pck)			\
	div_u64((u64)(llp) * USEC_PER_SEC, (pck))
#define	T_FRAME(fll, llp, pck)	\
	mul_u64_u32_div((u64)(llp) * USEC_PER_SEC, (fll), (pck))

#define	VIDEO_STATE_STREAMING		0x01
#define	VIDEO_STATE_LINK_CHANGE		0x10

#define	VIDEO_LINK_CHANGE_DELAY		10	/* msec */

#define META_DATA_SIZE			(4 * 1024)

typedef struct autox_video_format {
	unsigned int		header_lines;
	unsigned int		footer_lines;
	struct v4l2_pix_format	pix;
} autox_video_format_t;

typedef struct autox_video_buffer {
	struct vb2_v4l2_buffer	vbuf;
	struct list_head	list;
	unsigned int		idx;
	unsigned char		*addr;
	unsigned int		size;
} autox_video_buffer_t;

typedef struct autox_video {
	unsigned int		qidx;		/* queue index 0 ~ 7 for 8 Camera per FPGA or 0 ~ 15 for 16 Camera per FPGA */
	unsigned int		index;		/* register index 0 ~ 7 for 8 Camera per FPGA or 0 ~ 15 for 16 Camera per FPGA */
	unsigned int		reg_base;	/* BASE Register from FPGA */
	unsigned long		qhndl;

	struct xlnx_pci_dev	*xpdev;
	struct v4l2_device	v4l2_dev;
	struct v4l2_ctrl_handler ctrl_handler;
	struct vb2_queue	queue;
	struct video_device	vdev;

	struct list_head	free_list;
	struct mutex		mlock;	/* Lock for main serialization */
	struct mutex		slock;	/* Lock for vb queue streaming */
	spinlock_t		qlock;	/* Lock for driver owned queues */

	unsigned int		pixfmts[VIDEO_FORMAT_NUM];
	unsigned int		fmt_mask;
	unsigned int		fmt_idx;
	unsigned int		input;

	unsigned int		state;
	unsigned int		sequence;
	unsigned int		buf_total; /* Total buf num in buf_list */
	unsigned int		buf_avail; /* Available buf num in buf_list */
	autox_video_buffer_t	*cur_buf;
	unsigned int		cur_off;

	unsigned int		link_up;
	unsigned int		fps;
	unsigned int		frame_interval;
	unsigned int		frame_usec_max;
	unsigned int		frame_err_cnt;
	struct timespec64	tv_begin;
	struct timespec64	tv_end;
	unsigned long		ts_link_change;

	struct completion	watchdog_completion;
	struct task_struct	*watchdog_taskp;
	unsigned int		watchdog_interval;

	autox_cam_caps_t	caps;	/* Camera Capabilities */
	struct cash3_qdma_video_request	*vreq;
} autox_video_t;

static inline void autox_video_reg_write(autox_video_t *avideo,
		unsigned int reg, unsigned int val)
{
	qdma_device_write_user_register(avideo->xpdev,
	    avideo->reg_base + reg, val);
}

static inline unsigned int autox_video_reg_read(autox_video_t *avideo,
		unsigned int reg, unsigned int *value)
{
	return qdma_device_read_user_register(avideo->xpdev,
	    avideo->reg_base + reg, value);
}

extern int autox_video_init(autox_video_t *avideo);
extern void autox_video_finish(autox_video_t *avideo);
extern int autox_video_register_device(autox_video_t *avideo);
extern void autox_video_unregister_device(autox_video_t *avideo);
extern int autox_video_alloc(struct xlnx_pci_dev *xpdev);
extern void autox_video_free(struct xlnx_pci_dev *xpdev);
extern int autox_video_init_all(struct xlnx_pci_dev *xpdev);
extern void autox_video_finish_all(struct xlnx_pci_dev *xpdev);

extern int autox_gps_alloc(struct xlnx_pci_dev *xpdev);
extern int autox_gps_init(struct xlnx_pci_dev *xpdev);
extern void autox_gps_finish(struct xlnx_pci_dev *xpdev);
extern void autox_gps_free(struct xlnx_pci_dev *xpdev);

#endif	/* _AUTOX_VIDEO_H_ */
