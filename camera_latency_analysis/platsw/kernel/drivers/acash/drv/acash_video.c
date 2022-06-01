/* 
 * Copyright (C) 2019 AutoX, Inc.
 */
#include <linux/types.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/printk.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include "libqdma/qdma_descq.h"
#include "libqdma/qdma_st_c2h.h"
#include "libqdma/xdev.h"
#include "libqdma/thread.h"
#include "qdma_mod.h"
#include "acash_reg.h"
#include "acash_gps.h"
#include "acash_video.h"
#include "acash_utils.h"
#include "linux/cash_scratch_registers.h"

#define	VLOG(f, v, fmt, ...)	f("q%u video%d %s: " fmt, \
	v->qidx, v->vdev.num, v->caps.name, ## __VA_ARGS__)
#define	VLOG_DEBUG(v, fmt, ...)	VLOG(pr_debug, v, fmt, ## __VA_ARGS__)
#define	VLOG_INFO(v, fmt, ...)	VLOG(pr_info, v, fmt, ## __VA_ARGS__)
#define	VLOG_ERR(v, fmt, ...)	VLOG(pr_err, v, "ERROR, " fmt, ## __VA_ARGS__)

static autox_video_format_t autox_video_formats[VIDEO_FORMAT_NUM] = {
	{
		.header_lines	= 0,
		.footer_lines	= 0,
		.pix		= {
			.width		= 1920,
			.height		= 1080,
			.pixelformat	= V4L2_PIX_FMT_RGB24,
			.field		= V4L2_FIELD_NONE,
			.bytesperline	= 3 * 1920,
			.sizeimage	= 3 * 1920 * 1080,
			.colorspace	= V4L2_COLORSPACE_DEFAULT,
			.priv		= 0
		}
	},
	{
		.header_lines	= 0,
		.footer_lines	= 0,
		.pix		= {
			.width		= 1920,
			.height		= 1080,
			/* 0x56595559 - VYUY */
			.pixelformat	= V4L2_PIX_FMT_YUYV,
			.field		= V4L2_FIELD_NONE,
			.bytesperline	= 2 * 1920,
			.sizeimage	= 2 * 1920 * 1080,
			.colorspace	= V4L2_COLORSPACE_DEFAULT,
			.priv		= 0
		}
	},
	{
		.header_lines	= 0,
		.footer_lines	= 0,
		.pix		= {
			.width		= 1920,
			.height		= 1080,
			/* 0x55595659 - UYVY */
			.pixelformat	= V4L2_PIX_FMT_YVYU,
			.field		= V4L2_FIELD_NONE,
			.bytesperline	= 2 * 1920,
			.sizeimage	= 2 * 1920 * 1080,
			.colorspace	= V4L2_COLORSPACE_DEFAULT,
			.priv		= 0
		}
	},
	{
		.header_lines	= 0,
		.footer_lines	= 0,
		.pix		= {
			.width		= 1920,
			.height		= 1080,
			.pixelformat	= V4L2_PIX_FMT_UYVY,
			/* 0x59565955 - YVYU */
			.field		= V4L2_FIELD_NONE,
			.bytesperline	= 2 * 1920,
			.sizeimage	= 2 * 1920 * 1080,
			.colorspace	= V4L2_COLORSPACE_DEFAULT,
			.priv		= 0
		}
	}
};

static void autox_video_packet_copy(unsigned long user_data,
		unsigned int bytes_done, int err);

static int autox_video_packet_request(autox_video_t *avideo)
{
	struct cash_qdma_video_request *vreq = NULL;
	struct xlnx_pci_dev *xpdev;
	int rv = 0;
	unsigned long dev_hndl, qhndl, video_qhndls[AUTOX_VIDEO_DMA_CNT] = {0};
	unsigned int sizeimage =
		autox_video_formats[avideo->fmt_idx].pix.sizeimage;

	xpdev = avideo->xpdev;

	vreq = kzalloc(sizeof(*vreq), GFP_KERNEL);
	if (!vreq) {
		VLOG_ERR(avideo, "failed to vreq\n");
		return -1;
	}

	vreq->buffer_chunk_count = sizeimage/(BUFFER_CHUNK_SIZE) + 1;
	vreq->buffer_chunk_size = BUFFER_CHUNK_SIZE;

	vreq->user_data = (unsigned long)NULL;
	vreq->aio_done = NULL;

	vreq->pdev = xpdev->pdev;
	vreq->reserved = NULL;

	rv = cash_qdma_get_handles(xpdev->idx, &dev_hndl, video_qhndls, AUTOX_VIDEO_DMA_CNT, NULL, NULL);

	qhndl = video_qhndls[avideo->qidx];
	vreq->dev_hndl = dev_hndl;
	vreq->qhndl = qhndl;
	ASSERT(0 == prepare_cash_qdma_video_request(vreq));

	VLOG_DEBUG(avideo,
		"==sizeimage (%u), \
		vreq->buffer_chunk_count(%lu), \
		vreq->buffer_chunk_size(%lu)==\n",
		sizeimage, vreq->buffer_chunk_count, vreq->buffer_chunk_size);

	avideo->vreq = vreq;

	return 0;
}

static int autox_video_queue_add(autox_video_t *avideo)
{
	struct xlnx_pci_dev *xpdev;
	int rv = 0;

	xpdev = avideo->xpdev;

	return rv;
}

static int autox_video_queue_start(autox_video_t *avideo)
{
	struct xlnx_pci_dev *xpdev;
	int rv = 0;

	xpdev = avideo->xpdev;

	return rv;
}

static int autox_video_queue_stop(autox_video_t *avideo)
{
	struct xlnx_pci_dev *xpdev;
	int rv = 0;

	xpdev = avideo->xpdev;

	VLOG_DEBUG(avideo, "qdma%05x queue stopped, qhndl 0x%lx\n",
	    xpdev->idx, avideo->qhndl);

	return rv;
}

static int autox_video_queue_remove(autox_video_t *avideo)
{
	struct xlnx_pci_dev *xpdev;
	int rv = 0;

	xpdev = avideo->xpdev;

	return rv;
}

/*
 * to_vb2_v4l2_buffer() - cast struct vb2_buffer * to struct vb2_v4l2_buffer *
 */
static inline autox_video_buffer_t *vb_to_avb(struct vb2_buffer *vb)
{
	return container_of(to_vb2_v4l2_buffer(vb),
	    autox_video_buffer_t, vbuf);
}

static inline unsigned char *autox_vb_vaddr(autox_video_buffer_t *buf)
{
	return buf->addr;
}

static inline unsigned int autox_vb_size(autox_video_buffer_t *buf)
{
	return buf->size;
}

static void autox_video_reset(autox_video_t *avideo)
{
	VLOG_DEBUG(avideo, "reset video channel\n");
}

static void autox_video_enable(autox_video_t *avideo)
{
	struct xlnx_pci_dev *xpdev;
	int rv = 0;

	xpdev = avideo->xpdev;

	rv = clear_cash_video_status(xpdev->dev_hndl, avideo->qidx, 0);
	if (rv != 0) {
		VLOG_ERR(avideo, "clear_cash_video_status ERR (%d)\n", rv);
	}

	VLOG_DEBUG(avideo, "enable video channel %d\n", rv);

	avideo->vreq = NULL;
	rv = autox_video_packet_request(avideo);
	ASSERT(0 == rv);
}

static void autox_video_disable(autox_video_t *avideo)
{
	VLOG_DEBUG(avideo, "disable video channel\n");
}

/*
 * Check the meta data saved in the v4l2 buffer
 */
/* FIXME */
typedef struct meta_data {
	unsigned int mipi_id;
	unsigned int csi_sts;
	unsigned int resp;
	unsigned int req;
} _meta_data;

_meta_data g_meta_data[AUTOX_VIDEO_DMA_CNT] = {0};

int autox_video_check_meta_data(autox_video_t *avideo,
		autox_video_buffer_t *buf, unsigned int *metadata)
{
	struct timespec64 tv;
	unsigned char *mem;
	unsigned int memsz;
	int ret = 0;

/* FIXME */
	unsigned int cur_csi_sts;
	unsigned int cur_resp;
	unsigned int cur_req;
	unsigned int cur_a53_idx;
	unsigned int a53_idx;

	mem = autox_vb_vaddr(buf);
	memsz = autox_vb_size(buf);

	tv = avideo->tv_begin;

	buf->vbuf.vb2_buf.timestamp = timespec64_to_ns(&tv);
	buf->vbuf.sequence = avideo->sequence;

	 /* In case of dropping frames */
	autox_video_reset(avideo);

/* FIXME */
	cur_csi_sts = *(metadata + 1);
	cur_resp = *(metadata + 2);
	cur_req = *(metadata + 3);
	cur_a53_idx = cur_csi_sts >> 17 & 0xFF;
	a53_idx = (g_meta_data[avideo->qidx].csi_sts >> 17) & 0xFF;

/*
	printk(KERN_ERR "cur_csi_sts== (%d) 0x(%x)\n", avideo->qidx, cur_csi_sts);
	printk(KERN_ERR "cur_resp== (%d) 0x(%x)\n", avideo->qidx, cur_resp);
	printk(KERN_ERR "cur_req== (%d) 0x(%x)\n", avideo->qidx, cur_req);
	printk(KERN_ERR "a53_idx== (%d) 0x(%x)\n", avideo->qidx, a53_idx);
	printk(KERN_ERR "cur_a53_idx== (%d) 0x(%x)\n", avideo->qidx, cur_a53_idx);
*/

	if (((a53_idx + 1)%256) != cur_a53_idx)
	{
		VLOG_DEBUG(avideo,
			"A53 Frame Drop== (%d) \
			csi_sts(%x) cur_a53_idx(%x), a53_idx(%x), req(%x) resp(%x)\n", \
			avideo->qidx, \
			g_meta_data[avideo->qidx].csi_sts, \
			cur_a53_idx, a53_idx, \
			cur_req, cur_resp);
	}

	g_meta_data[avideo->qidx].csi_sts = cur_csi_sts;
	g_meta_data[avideo->qidx].resp = cur_resp;
	g_meta_data[avideo->qidx].req = cur_req;

	return ret;
}

/*
 * Called from VIDIOC_REQBUFS() and VIDIOC_CREATE_BUFS() handlers
 * before memory allocation.
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 5, 7)
static int autox_video_queue_setup(struct vb2_queue *vq,
		unsigned int *nbuffers, unsigned int *nplanes,
		unsigned int sizes[], struct device *alloc_devs[])
#else
static int autox_video_queue_setup(struct vb2_queue *vq, const void *parg,
		unsigned int *nbuffers, unsigned int *nplanes,
		unsigned int sizes[], void *alloc_ctxs[])
#endif
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 7)
	const struct v4l2_format *fmt = parg;
#endif
	autox_video_t *avideo = vb2_get_drv_priv(vq);
	unsigned int sizeimage =
	    autox_video_formats[avideo->fmt_idx].pix.sizeimage;

	VLOG_DEBUG(avideo, "nbuffers=%u, nplanes=%u\n", *nbuffers, *nplanes);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 7)
	if (fmt && fmt->fmt.pix.sizeimage != sizeimage) {
		VLOG_ERR(avideo,
		    "invalid requested size %u, supported size %u\n",
		    fmt->fmt.pix.sizeimage, sizeimage);
		return -EINVAL;
	}
#endif

	*nplanes = 1;
	sizes[0] = sizeimage;

	return 0;
}

/*
 * Called once after allocating a buffer in MMAP case.
 */
static int autox_video_buffer_init(struct vb2_buffer *vb)
{
	autox_video_t *avideo = vb2_get_drv_priv(vb->vb2_queue);
	autox_video_buffer_t *buf = vb_to_avb(vb);

	VLOG_DEBUG(avideo, "vb 0x%p, index %u\n", vb, vb->index);

	/*
	 * We save the vaddr and size of the buffer
	 */
	buf->addr = vb2_plane_vaddr(vb, 0);
	buf->size = vb2_plane_size(vb, 0);

	return 0;
}

/*
 * Called every time the buffer is queued from userspace
 * and from the VIDIOC_PREPARE_BUF() ioctl.
 */
static int autox_video_buffer_prepare(struct vb2_buffer *vb)
{
	autox_video_t *avideo = vb2_get_drv_priv(vb->vb2_queue);
	autox_video_buffer_t *buf = vb_to_avb(vb);
	unsigned int bufsz = autox_vb_size(buf);
	unsigned int size;

	VLOG_DEBUG(avideo, "vb 0x%p, index %u\n", vb, vb->index);

	size = autox_video_formats[avideo->fmt_idx].pix.sizeimage;

	if (bufsz != size) {
		VLOG_ERR(avideo, "invalid buffer size %u, expect %u)\n",
		    bufsz, size);
		return -EINVAL;
	}

	vb2_set_plane_payload(vb, 0, size);

	return 0;
}

/*
 * Finish the buffer for dequeuing
 */
static void autox_video_buffer_finish(struct vb2_buffer *vb)
{
	autox_video_t *avideo = vb2_get_drv_priv(vb->vb2_queue);

	VLOG_DEBUG(avideo, "vb 0x%p, index %u\n", vb, vb->index);
}

/*
 * Cleanup the buffer before it is freed
 */
static void autox_video_buffer_cleanup(struct vb2_buffer *vb)
{
	autox_video_t *avideo = vb2_get_drv_priv(vb->vb2_queue);

	VLOG_DEBUG(avideo, "vb 0x%p, index %u\n", vb, vb->index);
}

/*
 * Passes buffer vb to the driver.
 * It might be called before start_streaming callback
 * if user pre-queued buffers before calling VIDIOC_STREAMON().
 */
static void autox_video_buffer_queue(struct vb2_buffer *vb)
{
	autox_video_t *avideo = vb2_get_drv_priv(vb->vb2_queue);
	autox_video_buffer_t *buf = vb_to_avb(vb);

	VLOG_DEBUG(avideo, "vb 0x%p, index %u, state %d\n",
	    vb, vb->index, vb->state);

	spin_lock_bh(&avideo->qlock);
	list_add_tail(&buf->list, &avideo->free_list);
	avideo->buf_avail++;
	spin_unlock_bh(&avideo->qlock);
}

static void autox_video_init_params(autox_video_t *avideo)
{
	avideo->cur_buf = NULL;
	avideo->cur_off = 0;

	avideo->sequence = 0;
	avideo->frame_err_cnt = 0;
}

/*
 * Called once to enter ‘streaming’ state.
 * If you need a minimum number of buffers before you can start streaming,
 * then set vb2_queue->min_buffers_needed.
 * If that is non-zero then start_streaming won’t be called until at least
 * that many buffers have been queued up by userspace.
 */
static int autox_video_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	autox_video_t *avideo = vb2_get_drv_priv(vq);

	VLOG_DEBUG(avideo, "vq 0x%p, count %u\n", vq, count);

	if (count < VIDEO_BUF_MIN_NUM) {
		VLOG_ERR(avideo, "no enough buffers, count %u\n", count);
		return -ENOBUFS;
	}

	avideo->buf_total = count;
	ASSERT(avideo->buf_total == avideo->buf_avail);

	autox_video_init_params(avideo);

	/*
	 * Enable the video channel.
	 */
	autox_video_enable(avideo);

	/* Add a QDMA queue */
	if (autox_video_queue_add(avideo)) {
		autox_video_disable(avideo);
		return -EINVAL;
	}

	/* Start the QDMA queue */
	if (autox_video_queue_start(avideo)) {
		(void) autox_video_queue_remove(avideo);
		autox_video_disable(avideo);
		return -EINVAL;
	}

	/* Set the streaming flag */
	spin_lock_bh(&avideo->qlock);
	avideo->state |= VIDEO_STATE_STREAMING;
	spin_unlock_bh(&avideo->qlock);

	return 0;
}

static void autox_video_return_all_buffers(autox_video_t *avideo,
		enum vb2_buffer_state state)
{
	autox_video_buffer_t *buf, *node;

	spin_lock_bh(&avideo->qlock);
	list_for_each_entry_safe(buf, node, &avideo->free_list, list) {
		vb2_buffer_done(&buf->vbuf.vb2_buf, state);
		list_del(&buf->list);
		avideo->buf_avail--;
	}
	spin_unlock_bh(&avideo->qlock);
}

/*
 * Called when ‘streaming’ state must be disabled. Driver should stop
 * any DMA transactions or wait until they finish and give back all buffers
 * it got from buf_queue callback by calling vb2_buffer_done() with
 * either VB2_BUF_STATE_DONE or VB2_BUF_STATE_ERROR.
 * May use vb2_wait_for_all_buffers() function
 */
static void autox_video_stop_streaming(struct vb2_queue *vq)
{
	autox_video_t *avideo = vb2_get_drv_priv(vq);

	VLOG_DEBUG(avideo, "vq 0x%p\n", vq);

	spin_lock_bh(&avideo->qlock);
	/* Clear the streaming flag */
	avideo->state &= ~VIDEO_STATE_STREAMING;
	spin_unlock_bh(&avideo->qlock);

	/* Disable the video channel */
	autox_video_disable(avideo);

	/* Stop the QDMA queue */
	(void) autox_video_queue_stop(avideo);

	/* Delete the QDMA queue */
	(void) autox_video_queue_remove(avideo);

	/* Release all active buffers */
	autox_video_return_all_buffers(avideo, VB2_BUF_STATE_ERROR);

	/* Wait until all buffers are returned to vb2 */
	vb2_wait_for_all_buffers(vq);

	avideo->buf_total = 0;
	ASSERT(avideo->buf_total == avideo->buf_avail);
}

/*
 * The vb2 ops.
 * driver-specific callbacks.
 */
static struct vb2_ops autox_vb2_ops = {
	.queue_setup		= autox_video_queue_setup,	/* 1 */
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
	.buf_init		= autox_video_buffer_init,	/* 2 */
	.buf_prepare		= autox_video_buffer_prepare,
	.buf_finish		= autox_video_buffer_finish,
	.buf_cleanup		= autox_video_buffer_cleanup,
	.start_streaming	= autox_video_start_streaming,	/* 4 */
	.stop_streaming		= autox_video_stop_streaming,	/* 5 */
	.buf_queue		= autox_video_buffer_queue	/* 3 */
};

/*
 * Required ioctl querycap.
 */
static int autox_video_querycap(struct file *file, void *priv,
		struct v4l2_capability *cap)
{
	/* gets private data from struct video_device using the struct file */
	autox_video_t *avideo = video_drvdata(file);

	VLOG_DEBUG(avideo, "file 0x%p\n", file);

	strlcpy(cap->driver, KBUILD_MODNAME, sizeof(cap->driver));
	strlcpy(cap->card, avideo->caps.name, sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), "PCI:%s",
	    pci_name(avideo->xpdev->pdev));
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

/*
 * VIDIOC_ENUM_FMT - Enumerate image formats
 */
static int autox_video_enum_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_fmtdesc *fmtdesc)
{
	autox_video_t *avideo = video_drvdata(file);
	unsigned int pixelformat;

	VLOG_DEBUG(avideo, "VIDIOC_ENUM_FMT: file 0x%p,  QUERY FORMAT index %u\n",
			file, fmtdesc->index);

	if ((fmtdesc->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) ||
	    (fmtdesc->index >= VIDEO_FORMAT_USE)) {
		return -EINVAL;
	}

	pixelformat = avideo->pixfmts[avideo->fmt_idx];
	if (pixelformat == 0) {
		return -EINVAL;
	}

	fmtdesc->pixelformat = pixelformat;
	/* The flags and description will be filled by the V4L2 framework */

	VLOG_DEBUG(avideo, "pixelformat %c%c%c%c\n",
	    ((char *)&pixelformat)[0], ((char *)&pixelformat)[1],
	    ((char *)&pixelformat)[2], ((char *)&pixelformat)[3]);

	return 0;
}

static unsigned int autox_video_fill_pix_fmt(autox_video_t *avideo,
		struct v4l2_format *fmt)
{
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	VLOG_INFO(avideo, "pixelformat %c%c%c%c, width %u, height %u\n",
	    ((char *)&pix->pixelformat)[0], ((char *)&pix->pixelformat)[1],
	    ((char *)&pix->pixelformat)[2], ((char *)&pix->pixelformat)[3],
	    pix->width, pix->height);

	*pix = autox_video_formats[avideo->fmt_idx].pix;

	VLOG_DEBUG(avideo, "result index %u, "
	    "pixelformat %c%c%c%c, width %u, height %u\n", avideo->fmt_idx,
	    ((char *)&pix->pixelformat)[0], ((char *)&pix->pixelformat)[1],
	    ((char *)&pix->pixelformat)[2], ((char *)&pix->pixelformat)[3],
	    pix->width, pix->height);

	return avideo->fmt_idx;
}

/*
 * VIDIOC_TRY_FMT - Negotiate the format of data (typically image format)
 * 	exchanged between driver and application
 */
static int autox_video_try_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *fmt)
{
	autox_video_t *avideo = video_drvdata(file);

	VLOG_DEBUG(avideo, "VIDIOC_TRY_FMT: file 0x%p\n", file);

	if (fmt->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		return -EINVAL;
	}

	(void) autox_video_fill_pix_fmt(avideo, fmt);

	return 0;
}

/*
 * VIDIOC_S_FMT - Set the image format
 */
static int autox_video_s_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *fmt)
{
	autox_video_t *avideo = video_drvdata(file);

	VLOG_DEBUG(avideo, "VIDIOC_S_FMT file 0x%p\n", file);

	if (fmt->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		return -EINVAL;
	}

	/*
	 * It is not allowed to change the format while buffers for use with
	 * streaming have already been allocated.
	 */
	if (vb2_is_busy(&avideo->queue)) {
		return -EBUSY;
	}

	avideo->fmt_idx = autox_video_fill_pix_fmt(avideo, fmt);

	return 0;
}

/*
 * VIDIOC_G_FMT - Get the image format
 */
static int autox_video_g_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *fmt)
{
	autox_video_t *avideo = video_drvdata(file);
	struct v4l2_pix_format *pix = &fmt->fmt.pix;

	VLOG_DEBUG(avideo, "VIDIOC_G_FMT: file 0x%p, current index %u\n",
	    file, avideo->fmt_idx);

	if (fmt->type != V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		return -EINVAL;
	}

	*pix = autox_video_formats[avideo->fmt_idx].pix;

	VLOG_DEBUG(avideo, "format index %u, "
	    "pixelformat %c%c%c%c, width %u, height %u\n", avideo->fmt_idx,
	    ((char *)&pix->pixelformat)[0], ((char *)&pix->pixelformat)[1],
	    ((char *)&pix->pixelformat)[2], ((char *)&pix->pixelformat)[3],
	    pix->width, pix->height);

	return 0;
}

/*
 * VIDIOC_ENUMINPUT - Enumerate video inputs
 */
static int autox_video_enum_input(struct file *file, void *priv,
		struct v4l2_input *i)
{
	autox_video_t *avideo = video_drvdata(file);

	VLOG_DEBUG(avideo, "VIDIOC_ENUMINPUT: file 0x%p, index %u\n",
		file, i->index);

	if (i->index > 0) {
		return -EINVAL;
	}

	i->type = V4L2_INPUT_TYPE_CAMERA;
	strlcpy(i->name, avideo->caps.name, sizeof(i->name));

	return 0;
}

/*
 * VIDIOC_S_INPUT - Select the current video input
 */
static int autox_video_s_input(struct file *file, void *priv, unsigned int i)
{
	autox_video_t *avideo = video_drvdata(file);

	VLOG_DEBUG(avideo, "VIDIOC_S_INPUT: file 0x%p, i %u\n", file, i);

	if (i > 0) {
		return -EINVAL;
	}

	/*
	 * Changing the input implies a format change, which is not allowed
	 * while buffers for use with streaming have already been allocated.
	 */
	if (vb2_is_busy(&avideo->queue)) {
		return -EBUSY;
	}

	avideo->input = i;

	return 0;
}

/*
 * VIDIOC_G_INPUT - Query the current video input
 */
static int autox_video_g_input(struct file *file, void *priv, unsigned int *i)
{
	autox_video_t *avideo = video_drvdata(file);

	*i = avideo->input;

	VLOG_DEBUG(avideo, "VIDIOC_G_INPUT: file 0x%p, *i %u\n", file, *i);

	return 0;
}

/*
 * VIDIOC_G_PARM - Get streaming parameters
 */
static int autox_video_g_parm(struct file *file, void *priv,
		struct v4l2_streamparm *parm)
{
	autox_video_t *avideo = video_drvdata(file);

	/* FIXEME */
	struct v4l2_fract timeperframe = {
		.numerator = 1001 * 1,
		.denominator = 10,
	};

	VLOG_DEBUG(avideo, "VIDIOC_G_PARM: file 0x%p\n", file);

	if (parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	parm->parm.capture.readbuffers = 4;
	parm->parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	parm->parm.capture.timeperframe = timeperframe;

	return 0;
}

/*
 * VIDIOC_S_PARM - Set streaming parameters
 */
static int autox_video_s_parm(struct file *file, void *priv,
		struct v4l2_streamparm *parm)
{
	autox_video_t *avideo = video_drvdata(file);
	unsigned int n, d;

	VLOG_DEBUG(avideo, "VIDIOC_S_PARM: file 0x%p\n", file);

	if (parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	n = 10 * parm->parm.capture.timeperframe.numerator;
	d = 1001 * parm->parm.capture.timeperframe.denominator;

	return autox_video_g_parm(file, priv, parm);
}

/*
 * VIDIOC_ENUM_FRAMESIZE - Enumerate frame sizes
 */
static int autox_video_enum_framesizes(struct file *file, void *fh,
		struct v4l2_frmsizeenum *fsize)
{
	autox_video_t *avideo = video_drvdata(file);
	struct v4l2_pix_format *pix_fmt = NULL;

	VLOG_INFO(avideo,
		"VIDIOC_ENUM_FRAMESIZE: pixelformat %c%c%c%c, index %u  \n",
		((char *)&fsize->pixel_format)[0],
		((char *)&fsize->pixel_format)[1],
		((char *)&fsize->pixel_format)[2],
		((char *)&fsize->pixel_format)[3],
		fsize->index);

	pix_fmt = &autox_video_formats[avideo->fmt_idx].pix;
	if (fsize->pixel_format != pix_fmt->pixelformat)
		return -EINVAL;

	if (fsize->index >= VIDEO_FORMAT_USE)
		return -EINVAL;

	VLOG_INFO(avideo, "VIDIOC_ENUM_FRAMESIZE frame size support  width %u, height %u\n",
		pix_fmt->width, pix_fmt->height);

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = pix_fmt->width;
	fsize->discrete.height = pix_fmt->height;

	return 0;
}

static long autox_video_ioctl_default(struct file *file, void *fh,
		bool valid_prio, unsigned int cmd, void *arg)
{
	autox_video_t *avideo = video_drvdata(file);
	int err = 0;

	switch (cmd) {
	case AUTOX_IOC_CAM_CAPS:
		VLOG_DEBUG(avideo, "AUTOX_IOC_CAM_CAPS\n");
		break;
	case AUTOX_IOC_CAM_TRIGGER_ENABLE:
		VLOG_DEBUG(avideo, "AUTOX_IOC_CAM_TRIGGER_ENABLE\n");
		break;
	case AUTOX_IOC_CAM_TRIGGER_DISABLE:
		VLOG_DEBUG(avideo, "AUTOX_IOC_CAM_TRIGGER_DISABLE\n");
		break;
	case AUTOX_IOC_CAM_TRIGGER_STATUS:
		VLOG_DEBUG(avideo, "AUTOX_IOC_CAM_TRIGGER_STATUS\n");
		break;
	case AUTOX_IOC_CAM_STATS:
	{
		VLOG_DEBUG(avideo, "AUTOX_IOC_CAM_STATS\n");
		break;
	}
	default:
		VLOG_ERR(avideo, "unknown ioctl command\n");
		err = -EINVAL;
		break;
	}

	return err;
}

#ifdef AUTOX_VIDEO_CTRL_OPS
static int autox_video_s_ctrl(struct v4l2_ctrl *ctrl)
{
	autox_video_t *avideo =
	    container_of(ctrl->handler, autox_video_t, ctrl_handler);

	VLOG_DEBUG(avideo, "id=%u\n", ctrl->id - V4L2_CID_BASE);

	return 0;
}
#endif

/*
 * The set of file operations
 */
static const struct v4l2_file_operations autox_video_fops = {
	.owner		= THIS_MODULE,
	.open		= v4l2_fh_open,
	.release	= vb2_fop_release,
	.unlocked_ioctl	= video_ioctl2,
	.read		= vb2_fop_read,
	.mmap		= vb2_fop_mmap,
	.poll		= vb2_fop_poll,
};

/*
 * The set of all supported ioctls
 */
static const struct v4l2_ioctl_ops autox_video_ioctl_ops = {
	.vidioc_querycap		= autox_video_querycap,

	/* Video capture format callbacks */
	.vidioc_enum_fmt_vid_cap	= autox_video_enum_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap		= autox_video_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap		= autox_video_s_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap		= autox_video_g_fmt_vid_cap,

	/* Input callbacks */
	.vidioc_enum_input		= autox_video_enum_input,
	.vidioc_s_input			= autox_video_s_input,
	.vidioc_g_input			= autox_video_g_input,
	.vidioc_s_parm			= autox_video_s_parm,
	.vidioc_g_parm			= autox_video_g_parm,

	/* Streaming I/O */
	.vidioc_reqbufs			= vb2_ioctl_reqbufs,
	.vidioc_querybuf		= vb2_ioctl_querybuf,
	.vidioc_qbuf			= vb2_ioctl_qbuf,
	.vidioc_dqbuf			= vb2_ioctl_dqbuf,
	.vidioc_expbuf			= vb2_ioctl_expbuf,
	.vidioc_create_bufs		= vb2_ioctl_create_bufs,
	.vidioc_streamon		= vb2_ioctl_streamon,
	.vidioc_streamoff		= vb2_ioctl_streamoff,

	/* Logging and events */
	.vidioc_log_status		= v4l2_ctrl_log_status,
	.vidioc_subscribe_event		= v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event	= v4l2_event_unsubscribe,

	.vidioc_enum_framesizes		= autox_video_enum_framesizes,
	.vidioc_default			= autox_video_ioctl_default
};

#ifdef AUTOX_VIDEO_CTRL_OPS
static const struct v4l2_ctrl_ops autox_video_ctrl_ops = {
	.s_ctrl = autox_video_s_ctrl,
};
#endif

static void autox_video_format_init(autox_video_t *avideo)
{
	struct v4l2_pix_format *pix_fmt;

	avideo->fmt_mask = 0x3;
	avideo->fmt_idx = 0;
	//Cash1.1 only support one video format ,  index 0 V4L2_PIX_FMT_RGB24
	pix_fmt = &autox_video_formats[avideo->fmt_idx].pix;
	avideo->pixfmts[avideo->fmt_idx] = pix_fmt->pixelformat;

	VLOG_INFO(avideo, "pixelformat %c%c%c%c, width %u, height %u  avideo->fmt_idx %d\n",
	    ((char *)&pix_fmt->pixelformat)[0], ((char *)&pix_fmt->pixelformat)[1],
	    ((char *)&pix_fmt->pixelformat)[2], ((char *)&pix_fmt->pixelformat)[3],
	    pix_fmt->width, pix_fmt->height, avideo->fmt_idx);
}

static void autox_video_get_camera_info(autox_video_t *avideo)
{
	autox_cam_caps_t *caps = &avideo->caps;

	snprintf(caps->name, sizeof(caps->name), "cam%d", avideo->index);
	caps->port = avideo->index;

	autox_video_format_init(avideo);

	VLOG_DEBUG(avideo, "port %u, timestamp_type %u, trigger_mode %u\n",
	    caps->port, caps->timestamp_type, caps->trigger_mode);
}

static int autox_video_check_link(autox_video_t *avideo)
{
	uint32_t video_state_shift = 4 * avideo->qidx;
	uint32_t video_state;
	unsigned char prev;
	int changed;

	prev = avideo->link_up;

	/* Check camera link status */
	video_state =
		(read_cash_user_reg(avideo->xpdev->dev_hndl,
			CASH_SCRATCH_REG_VIDEO_STATE_ADDR)
				>> video_state_shift) & 0xf;

	VLOG_DEBUG(avideo, "video_state=0x%x", video_state);

	if (video_state == CASH_VIDEO_STATE_NOT_READY)
		avideo->link_up = 0;
	else
		avideo->link_up = 1;

	changed = (avideo->link_up != prev);
	if (changed) {
		VLOG_INFO(avideo, "camera link %s\n",
		    avideo->link_up ? "Up" : "Down");
	}

	return changed;
}

static void autox_video_link_change(autox_video_t *avideo)
{
	int link_changed;
	int rv = 0;

	if (avideo->state & VIDEO_STATE_STREAMING) {
		rv = send_cash_qdma_video_request(avideo->vreq);
		autox_video_packet_copy((unsigned long)avideo, rv, 0);
	}
	else {
		if (avideo->vreq) {
			unprepare_cash_qdma_video_request(avideo->vreq);

			VLOG_ERR(avideo, "==free2=== (%d)\n", avideo->qidx);
			kfree(avideo->vreq);

			avideo->vreq = NULL;
		}
	}

	spin_lock(&avideo->qlock);
	link_changed = autox_video_check_link(avideo);
	if (link_changed) {
		if (avideo->state & VIDEO_STATE_STREAMING) {
			VLOG_ERR(avideo, "device busy\n");
		}
		if (avideo->state & VIDEO_STATE_LINK_CHANGE) {
			avideo->state &= ~VIDEO_STATE_LINK_CHANGE;
		} else {
			avideo->state |= VIDEO_STATE_LINK_CHANGE;
		}
		link_changed = 0;
	} else {
		link_changed = (avideo->state & VIDEO_STATE_LINK_CHANGE);
	}
	spin_unlock(&avideo->qlock);

	if (!link_changed) {
		return;
	}
	avideo->state &= ~VIDEO_STATE_LINK_CHANGE;

	if (avideo->link_up) {
/* FIXME */
#ifdef AUTOX_LINK_CHECK
		autox_video_get_camera_info(avideo);
		(void) autox_video_register_device(avideo);
#endif

	} else {
		autox_video_unregister_device(avideo);
	}

}

static int autox_video_watchdog(void *data)
{
	autox_video_t *avideo = (autox_video_t *)data;
	long result;

	while (!kthread_should_stop()) {
		result = wait_for_completion_interruptible_timeout(
		    &avideo->watchdog_completion,
		    msecs_to_jiffies(avideo->watchdog_interval));
		if (result < 0) {
			/* Interrupted or module is being unloaded */
			VLOG_ERR(avideo, "watchdog completion interrupted!\n");
			break;
		}

		autox_video_link_change(avideo);

		if (result > 0) {
			reinit_completion(&avideo->watchdog_completion);
		}
	}

	return 0;
}

static void autox_video_watchdog_init(autox_video_t *avideo)
{
	init_completion(&avideo->watchdog_completion);
	avideo->watchdog_interval = VIDEO_LINK_CHANGE_DELAY;
	avideo->watchdog_taskp = kthread_run(autox_video_watchdog,
		avideo, "video watchdog");
}

static void autox_video_watchdog_finish(autox_video_t *avideo)
{
	complete(&avideo->watchdog_completion);
	if (avideo->watchdog_taskp) {
		kthread_stop(avideo->watchdog_taskp);
		avideo->watchdog_taskp = NULL;
	}
}

int autox_video_register_device(autox_video_t *avideo)
{
	struct video_device *vdev;
	int ret;

	/* Initialize the video_device structure */
	vdev = &avideo->vdev;
	if (video_is_registered(vdev)) {
		VLOG_ERR(avideo, "video device already registerd\n");
		return -EEXIST;
	}

	strlcpy(vdev->name, avideo->caps.name, sizeof(vdev->name));
	/*
	 * There is nothing to clean up, so release is set to an empty release
	 * function. The release callback must be non-NULL.
	 */
	vdev->release = video_device_release_empty;
	vdev->fops = &autox_video_fops,
	vdev->ioctl_ops = &autox_video_ioctl_ops,
	/*
	 * The main serialization lock. All ioctls are serialized by this
	 * lock. Exception: if q->lock is set, then the streaming ioctls
	 * are serialized by that separate lock.
	 */
	vdev->lock = &avideo->mlock;
	vdev->queue = &avideo->queue;
	vdev->v4l2_dev = &avideo->v4l2_dev;
	/* VFL_DIR_RX : device is a receiver */
	vdev->vfl_dir = VFL_DIR_RX;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
#endif

	/* Set avideo information to vdev */
	video_set_drvdata(vdev, avideo);

	/* VFL_TYPE_GRABBER/VFL_TYPE_VIDEO : for video input/output devices */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
	ret = video_register_device(vdev, VFL_TYPE_VIDEO, -1);
#else
	ret = video_register_device(vdev, VFL_TYPE_GRABBER, -1);
#endif
	if (ret) {
		VLOG_ERR(avideo,
			"vid%d: failed to register video device\n",
			avideo->qidx);
		return ret;
	}

	VLOG_DEBUG(avideo, "video device registered\n");

	return 0;
}

void autox_video_unregister_device(autox_video_t *avideo)
{
	video_unregister_device(&avideo->vdev);

	VLOG_DEBUG(avideo, "video device unregistered\n");
}

static void autox_video_stats_init(autox_video_t *avideo)
{

}

/*
 * There are AUTOX_VIDEO_DMA_CNT cameras at Max.
 * There are AUTOX_VIDEO_DMA_CNT V4L2 device drivers
 * Each V4L2 driver will have an instance of Xilinx PCIe driver
 *
 */
int autox_video_alloc(struct xlnx_pci_dev *xpdev)
{
	autox_video_t *avideo;
	int i;

	xpdev->video_cnt = AUTOX_VIDEO_DMA_CNT;

	avideo = kzalloc(xpdev->video_cnt * sizeof(autox_video_t), GFP_KERNEL);
	if (!avideo) {
		VLOG_ERR(avideo, "failed to alloc video channel array\n");
		return -1;
	}

	xpdev->videos = avideo;

	for (i = 0; i < xpdev->video_cnt; i++, avideo++) {
		avideo->xpdev = xpdev;
		avideo->qidx = i;
	}

	return 0;
}

void autox_video_free(struct xlnx_pci_dev *xpdev)
{
	if (xpdev->videos) {
		kfree(xpdev->videos);
		xpdev->videos = NULL;
	}
}

/*
 * The initial setup of this device instance. Note that the initial state of
 * the driver should be complete. So the initial format, standard, timings
 * and video input should all be initialized to some reasonable value.
 */
int autox_video_init(autox_video_t *avideo)
{
	struct v4l2_ctrl_handler *hdl;
	struct vb2_queue *q;
	int ret;

	INIT_LIST_HEAD(&avideo->free_list);
	spin_lock_init(&avideo->qlock);
	mutex_init(&avideo->mlock);
	mutex_init(&avideo->slock);

	/* Initialize the top-level structure */
	ret = v4l2_device_register(&avideo->xpdev->pdev->dev,
				  &avideo->v4l2_dev);
	if (ret) {
		VLOG_ERR(avideo, "q%d, failed to regiser v4l2\n", avideo->qidx);
		return ret;
	}

	/* Add the controls */
	hdl = &avideo->ctrl_handler;
	ret = v4l2_ctrl_handler_init(hdl, 0);
	if (ret) {
		VLOG_ERR(avideo, "q%d, failed to init v4l2 ctrl handler\n",
			avideo->qidx);
		v4l2_device_unregister(&avideo->v4l2_dev);
		return ret;
	}
	avideo->v4l2_dev.ctrl_handler = hdl;

	/* Initialize the vb2 queue */
	q = &avideo->queue;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP;
	q->lock = &avideo->slock;
	q->ops = &autox_vb2_ops;
	q->mem_ops = &vb2_vmalloc_memops;
	q->drv_priv = avideo;
	q->buf_struct_size = sizeof (autox_video_buffer_t);
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_COPY;
	q->min_buffers_needed = VIDEO_BUF_MIN_NUM;
	q->gfp_flags = GFP_DMA;

	ret = vb2_queue_init(q);
	if (ret) {
		VLOG_ERR(avideo, "q%d, failed to init vb2 queue\n",
			avideo->qidx);
		goto err;
	}

	avideo->input = V4L2_INPUT_TYPE_CAMERA;
/* FIXME */
	/* Check the camera link status and register video device */
	(void) autox_video_check_link(avideo);
	if (avideo->link_up) {
		autox_video_get_camera_info(avideo);
		ret = autox_video_register_device(avideo);
		if (ret) {
			goto err;
		}
	}

	autox_video_watchdog_init(avideo);
	autox_video_stats_init(avideo);

	VLOG_DEBUG(avideo, "video initialization done. avideo=%p\n", avideo);

	return 0;

err:
	v4l2_ctrl_handler_free(&avideo->ctrl_handler);
	v4l2_device_unregister(&avideo->v4l2_dev);

	return ret;
}

void autox_video_finish(autox_video_t *avideo)
{
	VLOG_DEBUG(avideo, "avideo=0x%p\n", avideo);

	autox_video_watchdog_finish(avideo);
	autox_video_unregister_device(avideo);
	v4l2_ctrl_handler_free(&avideo->ctrl_handler);
	v4l2_device_unregister(&avideo->v4l2_dev);
}

int autox_video_init_all(struct xlnx_pci_dev *xpdev)
{
	autox_video_t *avideo;
	int i;

	avideo = xpdev->videos;
	for (i = 0; i < xpdev->video_cnt; i++, avideo++) {
		avideo->index = i;
		avideo->reg_base = VIDEO_REG_BASE + (i * VIDEO_REG_STEP);

		if (autox_video_init(avideo)) {
			goto err;
		}
	}

	return 0;
err:
	for (i--, avideo--; i >= 0; i--, avideo--) {
		autox_video_finish(avideo);
	}
	return -1;
}

void autox_video_finish_all(struct xlnx_pci_dev *xpdev)
{
	autox_video_t *avideo;
	int i;

	avideo = xpdev->videos;
	for (i = 0; i < xpdev->video_cnt; i++, avideo++) {
		autox_video_finish(avideo);
	}
}

static void autox_video_packet_copy(unsigned long user_data,
		unsigned int bytes_done, int err)
{
	autox_video_t *avideo = (autox_video_t *)user_data;
	struct cash_qdma_video_request *vreq = avideo->vreq;
	unsigned int sizeimage =
		autox_video_formats[avideo->fmt_idx].pix.sizeimage;
	autox_video_buffer_t *buf = NULL;
	struct vb2_buffer *vb;
	unsigned char *mem;
	unsigned int memsz;
	int i = 0;

#ifdef NO_META_DATA
#else
	char *ptr;
	char *meta_data;

	meta_data = kzalloc(META_DATA_SIZE, GFP_KERNEL);
	meta_data[i] = 0xff;
#endif

	spin_lock(&avideo->qlock);
	if (!(avideo->state & VIDEO_STATE_STREAMING)) {
		spin_unlock(&avideo->qlock);
		VLOG_ERR(avideo, "not at streaming state\n");
		goto err;
	}

	if (avideo->cur_buf) {
		buf = avideo->cur_buf;
	} else {
		if (!list_empty(&avideo->free_list)) {
			buf = list_first_entry(&avideo->free_list,
			    autox_video_buffer_t, list);
			list_del(&buf->list);
			avideo->buf_avail--;
			avideo->cur_buf = buf;
			avideo->cur_off = 0;
			autox_get_timestamp(&avideo->tv_begin);
		}
	}

	spin_unlock(&avideo->qlock);

	if (buf == NULL) {
		VLOG_ERR(avideo, "CAM_STATS_NO_VIDEO_BUF\n");
		goto err;
	}

	vb = &buf->vbuf.vb2_buf;

	/* Get the virtual address and size of the video buffer */
	mem = autox_vb_vaddr(buf);
	memsz = autox_vb_size(buf);

#ifdef NO_META_DATA
	for (i = 0; i < (vreq->buffer_chunk_count - 1); i++) {
		memcpy(mem, vreq->buffer_chunks[i], vreq->buffer_chunk_size);
		mem = (mem + vreq->buffer_chunk_size);
		VLOG_DEBUG(avideo,
			"i(%d) buffer_chunk_count(%d) buffer_chunk_size(%d)\n",
			i, (int)vreq->buffer_chunk_count,
			(int)vreq->buffer_chunk_size);
	}
	memcpy(mem,
		vreq->buffer_chunks[i], (sizeimage%vreq->buffer_chunk_size));
	VLOG_DEBUG(avideo,
		"i(%d) buffer_chunk_count(%d) buffer_chunk_size(%d)\n",
		i, (int)vreq->buffer_chunk_count,
		(int)(sizeimage%vreq->buffer_chunk_size));
#else
	ptr = (char *)vreq->buffer_chunks[0];
	memcpy(meta_data, ptr, META_DATA_SIZE);
	memcpy(mem, (ptr+META_DATA_SIZE),
		(vreq->buffer_chunk_size - META_DATA_SIZE));
	mem = (mem + (vreq->buffer_chunk_size - META_DATA_SIZE));

	VLOG_DEBUG(avideo,
		"i(%d) buffer_chunk_count(%d) buffer_chunk_size(%d)\n",
		i, (int)vreq->buffer_chunk_count, (int)vreq->buffer_chunk_size);

	for (i = 1; i < (vreq->buffer_chunk_count - 1); i++) {
		memcpy(mem, vreq->buffer_chunks[i], vreq->buffer_chunk_size);
		mem = (mem + vreq->buffer_chunk_size);
		VLOG_DEBUG(avideo,
			"i(%d) buffer_chunk_count(%d) buffer_chunk_size(%d)\n",
			i, (int)vreq->buffer_chunk_count,
			(int)vreq->buffer_chunk_size);
	}

	memcpy(mem, vreq->buffer_chunks[i],
		((sizeimage%vreq->buffer_chunk_size) + META_DATA_SIZE));
	VLOG_DEBUG(avideo,
		"i(%d) buffer_chunk_count(%d) buffer_chunk_size(%d)\n",
		i, (int)vreq->buffer_chunk_count,
		(int)(sizeimage%vreq->buffer_chunk_size));
#endif

	avideo->cur_off = sizeimage;

	autox_get_timestamp(&avideo->tv_end);

	VLOG_DEBUG(avideo, "buffer done, "
	    "vb=0x%p, index=%u, state=%d, size=%u\n",
	    vb, vb->index, vb->state, avideo->cur_off);

	if (autox_video_check_meta_data(avideo, buf, (unsigned int *)meta_data))
	{
		VLOG_ERR(avideo, "CAM_STATS_RX_DROP\n");
	}
	else
	{
		VLOG_DEBUG(avideo, "buffer done, "
			"vb=0x%p, index=%u, state=%d, size=%u\n",
			vb, vb->index, vb->state, avideo->cur_off);
		vb2_set_plane_payload(vb, 0, avideo->cur_off);
		vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
	}

	VLOG_DEBUG(avideo, "CAM_STATS_RX\n");

	avideo->sequence++;
	avideo->cur_buf = NULL;
	avideo->cur_off = 0;

err:
#ifdef NO_META_DATA
#else
	kfree(meta_data);
#endif
}
