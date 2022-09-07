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

#include "linux/cash3_scratch_registers.h"
#include "linux/cash3_host_cmds.h"
#include "linux/cash3_fpga_events.h"
#include "linux/cash3_conf.h"
#include "libqdma_cash3.h"
#include "acash_reg.h"
#include "acash_sysm.h"
#include "libqdma/libqdma_export.h"
#include "qdma_mod.h"
#include "acash_spec.h"


spinlock_t autox_sysm_lock;

static unsigned char autox_sysm_cnt = 0;

typedef struct autox_sysm_ioc_ops {
	unsigned int cmd;
	int (* ioctl_handle)(autox_sysm_t *asysm, autox_update_type_t type, void *update_arg);
	int is_read_back;
} autox_sysm_ioc_ops_t;

/**
 * @brief query some information,such as isp version or the md5 value of upload file
 *
 * @param asysm
 * @param update_arg:autox_common_send_cmd_t
 * @param update_type: config/isp/custom
 * @return int
 */
static int autox_sysm_common_query_handle(autox_sysm_t *asysm, void *update_arg,
                                          autox_update_type_t update_type)
{
	int ret = 0;
	int i = 0;
	unsigned int reg_val = 0;
	struct xlnx_pci_dev *xpdev = asysm->xpdev;
	struct cash3_host_cmd_t *cmd = NULL;
	autox_common_send_cmd_t *common_query_cmd = (autox_common_send_cmd_t *)update_arg;
	struct cash3_host_cmd_update_t *update = NULL;
	autox_scratch_reg_addr_spec_t *scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;

	cmd = (struct cash3_host_cmd_t *) vmalloc(sizeof(struct cash3_host_cmd_t));
	if (cmd == NULL) {
		ALOG_ERR(xpdev, "AUTOX_IOC_SYSM_VERSION common query handle malloc failed.\n");
		return -1;
	}

	cmd->head.cmd = CASH3_HOST_CMD_UPDATE_FILE_VERSION;
	update = (struct cash3_host_cmd_update_t*)cmd->payload;
	update->type = update_type;
	memcpy(update->data, common_query_cmd, sizeof(autox_common_send_cmd_t));
	update->data_size = sizeof(autox_common_send_cmd_t);

	ret = send_host_cmd(xpdev->dev_hndl, cmd, sizeof(struct cash3_host_cmd_t), true);
	if (ret < 0) {
		ALOG_ERR(xpdev, "send_host_cmd failed,ret = %d.\n", ret);
		vfree(cmd);
		return ret;
	}

	ret = read_cash3_user_reg(xpdev->dev_hndl, scratch_reg->cmd_status.addr);
	if (ret < 0) {
		ALOG_ERR(xpdev, "The returned command execution status is wrong,ret=%d.\n", ret);
		vfree(cmd);
		return ret;
	}
    /**
     * @brief  after query, to parse the result,read from scratch registers,
     *         and then write into common_query_cmd->data
     * A total of 32 bytes are stored in 8 scratch registers:
     * [CASH3_SCRATCH_REG_ACT_MD5_ADDR(4*DWORD) + CASH3_SCRATCH_REG_BACKUP_MD5_ADDR(4*DWORD)]
     *
     * The common_query_cmd->data stored query result content is related to the query type
     * For AUTOX_COMMON_QUERY_MD5:
     *      common_query_cmd.data is md5 value of arm file: "754692ee33dc07c81393eea9707f3c75"
     * For AUTOX_COMMON_QUERY_VERSION:
     *      common_query_cmd.data is IspVersionSendToX86Info
     * For AUTOX_COMMON_QUERY_STATE:
     *      common_query_cmd.data is autox_isp_state_t(only use one byte)
     * For Config_update:
     *      common_query_cmd.data is autox_config_md5_t
     */
	for (i = 0; i < UPDATE_SCRATH_REG_CNT; i++) {
		reg_val = read_cash3_user_reg(xpdev->dev_hndl, CASH3_SCRATCH_REG_ACT_MD5_ADDR + i * 4);
		memcpy(&(common_query_cmd->data[i * 4]), &reg_val, sizeof(reg_val));
	}
	common_query_cmd->data[UPDATE_SCRATH_STR_LEN] = '\0';

	vfree(cmd);
	return 0;
}

static int autox_sysm_fw_version_handle(autox_sysm_t *asysm, void *update_arg)
{
	struct xlnx_pci_dev *xpdev = asysm->xpdev;
	autox_scratch_reg_addr_spec_t *scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;
	uint32_t fw_fpga_ver = 
			read_cash3_user_reg(xpdev->dev_hndl, scratch_reg->fpga_ver.addr);

	memcpy(update_arg, (char *)&fw_fpga_ver, sizeof(fw_fpga_ver));
	return 0;
}

static int autox_sysm_version_dispatch(autox_sysm_t *asysm,  autox_update_type_t type, void *update_arg)
{
	struct xlnx_pci_dev *xpdev = asysm->xpdev;

	switch(type) {
		case AUTOX_FW_UPDATE:
			return autox_sysm_fw_version_handle(asysm, update_arg);
		case AUTOX_CONFIGURATION_UPDATE:
		case AUTOX_ISP_UPDATE:
		case AUTOX_CUSTOM_FILE_UPDATE:
			return autox_sysm_common_query_handle(asysm, update_arg, type);
		default:
			ALOG_ERR(xpdev, "AUTOX_IOC_SYSM_VERSION type %u invalid.\n", type);
			return -EINVAL;
	}
	
	return 0;
} 

static int autox_sysm_upload_dispatch(autox_sysm_t *asysm, autox_update_type_t type, void *data)
{
	int ret;
	struct xlnx_pci_dev *xpdev = asysm->xpdev;
	autox_update_arg_t *update_arg = (autox_update_arg_t *)data;
	struct cash3_host_cmd_t *cmd = NULL;
	struct cash3_host_cmd_update_t *update = NULL;

	ALOG_DEBUG(xpdev, "AUTOX_IOC_SYSM_UPLOAD\n");
	switch(type) {
		case AUTOX_FW_UPDATE:
		case AUTOX_CONFIGURATION_UPDATE:
		case AUTOX_CUSTOM_FILE_UPDATE:
		case AUTOX_ISP_UPDATE:
		{
			cmd = (void*)&update_arg->cmd;
			update = (void*)cmd->payload;
			update->type = type;

			ALOG_DEBUG(xpdev, "AUTOX_IOC_SYSTEM_UPLOAD data_size (%lu)", update_arg->data_size);
			update->data_size = update_arg->data_size;
			memcpy(update->data, update_arg->data, update->data_size);
			cmd->head.cmd = CASH3_HOST_CMD_UPDATE_UPLOAD_FILE;
			ret = send_host_cmd(xpdev->dev_hndl, cmd, (200*1024*1024), false);
			if (ret < 0) {
				ALOG_ERR(xpdev, "AUTOX_IOC_SYSM_UPLOAD type %u send host cmd failed.\n", type);
				return ret;
			}
			return 0;
		}

		default:
			ALOG_ERR(xpdev, "AUTOX_IOC_SYSM_UPLOAD type %u invalid.\n", type);
			return -EINVAL;
	}
	
	return 0;
}

static int autox_sysm_active_dispatch(autox_sysm_t *asysm,  autox_update_type_t type, void *update_arg)
{
	int ret;
	struct xlnx_pci_dev *xpdev = asysm->xpdev;
	struct cash3_host_cmd_t *cmd = NULL;
	struct cash3_host_cmd_update_t *update;
	autox_common_send_cmd_t *common_query_cmd = (autox_common_send_cmd_t *)update_arg;

	ALOG_DEBUG(xpdev, "AUTOX_IOC_SYSM_ACTIVE\n");
	cmd = (struct cash3_host_cmd_t *) vmalloc(sizeof(struct cash3_host_cmd_t));
	if (cmd == NULL) {
		ALOG_ERR(xpdev, "AUTOX_IOC_SYSM_ACTIVE type %u malloc failed.\n", type);
		return -1;
	}

	switch(type) {
		case AUTOX_FW_UPDATE:
		case AUTOX_CONFIGURATION_UPDATE:
		case AUTOX_CUSTOM_FILE_UPDATE:
		case AUTOX_ISP_UPDATE: {
			cmd->head.cmd = CASH3_HOST_CMD_UPDATE_ACTIVE_FILE;
			update = (void*)cmd->payload;
			update->type = type;
			update->data_size = sizeof(autox_common_send_cmd_t);
			memcpy(update->data, common_query_cmd, sizeof(autox_common_send_cmd_t));
			// the third param is false: do not check cmd state
			ret = send_host_cmd(xpdev->dev_hndl, cmd, sizeof(struct cash3_host_cmd_t), false);
			break;
		}
		default: {
			ret = -EINVAL;
			ALOG_ERR(xpdev, "AUTOX_IOC_SYSM_UPLOAD type %u invalid.\n", type);
			break;
		}
	}

	vfree(cmd);
	if (ret < 0) {
		ALOG_ERR(xpdev, "AUTOX_IOC_SYSM_ACTIVE type %u send host cmd failed.\n", type);
		return ret;
	}
	return 0;
}

autox_sysm_ioc_ops_t ioc_ops_array[] = {
	{AUTOX_IOC_SYSM_VERSION, autox_sysm_version_dispatch, 1},
	{AUTOX_IOC_SYSM_UPLOAD, autox_sysm_upload_dispatch, 0},
	{AUTOX_IOC_SYSM_ACTIVE, autox_sysm_active_dispatch, 0}
};

static long autox_sysm_ioctl_handle(struct file *filp, unsigned int cmd,
    unsigned long arg)
{
	int i;
	int ret;
	autox_sysm_t *asysm = filp->private_data;
	struct xlnx_pci_dev *xpdev = asysm->xpdev;
	int size = sizeof(ioc_ops_array) / sizeof(autox_sysm_ioc_ops_t);
	autox_sysm_ioc_ops_t *ioc_ops = NULL;
	autox_update_cmd_t update_cmd = {0};
	char *update_arg = NULL;

	for (i = 0; i < size; i++) {
		if (cmd == ioc_ops_array[i].cmd) {
			ioc_ops = &ioc_ops_array[i];
			break;
		}
	}

	if (ioc_ops == NULL) {
		ALOG_ERR(xpdev, "cmd %u not found.\n", cmd);
		return -EFAULT;
	}

	if (copy_from_user(&update_cmd, (void *)arg, sizeof(autox_update_cmd_t))) {
		ALOG_ERR(xpdev, "sysm ioctl update_cmd copy_from_user err\n");
		return -EFAULT;
	}

	if (update_cmd.data == NULL || update_cmd.data_size == 0) {
		return ioc_ops->ioctl_handle(asysm, update_cmd.type, NULL);
	}

	update_arg = (char *)vmalloc(update_cmd.data_size);
	if (update_arg == NULL) {
		ALOG_ERR(xpdev, "sysm ioctl vmalloc failed.\n");
		return -EFAULT;
	}

	if (copy_from_user(update_arg, (void *)update_cmd.data, update_cmd.data_size)) {
		ALOG_ERR(xpdev, "sysm ioctl update_arg copy_from_user err\n");
		vfree(update_arg);
		return -EFAULT;
	}

	ret = ioc_ops->ioctl_handle(asysm, update_cmd.type, update_arg);
	if (ret != 0) {
		vfree(update_arg);
		ALOG_ERR(xpdev, "sysm ioctl handle failed.\n");
		return ret;
	}

	if (ioc_ops->is_read_back == 0) {
		return 0;
	}

	if (copy_to_user(update_cmd.data, update_arg, update_cmd.data_size)) {
		ALOG_ERR(xpdev, "sysm ioctl update_arg copy_to_user err\n");
		return -EFAULT;
	}

	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////
//// qdma control functions
/////////////////////////////////////////////////////////////////////////////////////////
void cash_qconf_init(struct qdma_queue_conf *qconf, unsigned short qidx)
{
	qconf->qidx = qidx;
	qconf->desc_bypass = 0;
	qconf->pfetch_bypass = 0;
	qconf->pfetch_en = 0;
	qconf->wb_status_en = 1;
	qconf->cmpl_status_acc_en = 1;
	qconf->cmpl_status_pend_chk = 1;
	qconf->fetch_credit = 1,
	qconf->cmpl_stat_en = 1;
	qconf->cmpl_en_intr = 0;
	qconf->cmpl_udd_en = 0;
	qconf->cmpl_ovf_chk_dis = 0;

	qconf->desc_rng_sz_idx = 9;
	qconf->cmpl_rng_sz_idx = 9;
	qconf->c2h_buf_sz_idx = 0;
	qconf->cmpl_timer_idx = 0;
	qconf->cmpl_cnt_th_idx = 0;
	qconf->mm_channel = 1;
	qconf->cmpl_desc_sz = 0;
	qconf->sw_desc_sz = 0;
	qconf->ping_pong_en = 0;
	qconf->cmpl_trig_mode = 1;
	qconf->c2h_buf_sz_idx = 0;
}

int cash_qdma_queue_add(struct xlnx_pci_dev *xpdev, struct qdma_queue_conf *qconf)
{
	char ebuf[XNL_RESP_BUFLEN_MIN] = {0};
	int ret = 0;
	ret = xpdev_queue_add(xpdev, qconf, ebuf, XNL_RESP_BUFLEN_MIN);
	ALOG_DEBUG(xpdev, "%s, idx %u, q_type %s,   qdma_queue_add ret %d.\n",
			dev_name(&xpdev->pdev->dev), qconf->qidx, 
			q_type_list[qconf->q_type].name, ret);
	return ret;
}

int cash_qdma_queue_delete(struct xlnx_pci_dev *xpdev, struct qdma_queue_conf *qconf)
{
	char ebuf[XNL_RESP_BUFLEN_MIN] = {0};
	int ret = 0;
	ret = xpdev_queue_delete(xpdev, qconf->qidx, qconf->q_type, ebuf, XNL_RESP_BUFLEN_MIN);
	ALOG_DEBUG(xpdev, "%s, idx %u, q_type %s,   qdma_queue_delete  %d.\n",
			dev_name(&xpdev->pdev->dev), qconf->qidx,
			q_type_list[qconf->q_type].name, ret);
	return ret;
}

int cash_qdma_queue_start(struct xlnx_pci_dev *xpdev, struct qdma_queue_conf *qconf)
{
	char ebuf[XNL_RESP_BUFLEN_MIN] = {0};
	struct xlnx_qdata *qdata;
	int ret = 0;

	qdata = xpdev_queue_get(xpdev, qconf->qidx,
				qconf->q_type, 1, ebuf, XNL_ERR_BUFLEN);
	if (!qdata) {
		ALOG_ERR(xpdev, "%s, idx %u, q_type %s, get failed.\n",
			dev_name(&xpdev->pdev->dev), qconf->qidx,
			q_type_list[qconf->q_type].name);
		snprintf(ebuf, XNL_EBUFLEN,
			"Q idx %u, q_type %s, get failed.\n",
			qconf->qidx, q_type_list[qconf->q_type].name);
		return ENOMEM;
	}

	ret = qdma_queue_config(xpdev->dev_hndl, qdata->qhndl,
				qconf, ebuf, XNL_RESP_BUFLEN_MIN);
	if (ret) {
		ALOG_ERR(xpdev, "qdma_queue_config failed: %d", ret);
	}

	ret = qdma_queue_start(xpdev->dev_hndl, qdata->qhndl, ebuf, XNL_EBUFLEN);

	if (ret) {
		ALOG_ERR(xpdev, "%s, idx %u, q_type %s, start failed %d.\n",
			dev_name(&xpdev->pdev->dev), qconf->qidx,
			q_type_list[qconf->q_type].name, ret);
		snprintf(ebuf, XNL_EBUFLEN,
			"Q idx %u, q_type %s, start failed %d.\n",
			qconf->qidx, q_type_list[qconf->q_type].name, ret);
	}

	ALOG_DEBUG(xpdev, "%s, idx %u, q_type %s,   qdma_queue_start  %d.\n",
			dev_name(&xpdev->pdev->dev), qconf->qidx,
			q_type_list[qconf->q_type].name, ret);
	return ret;
}

int cash_qdma_queue_stop(struct xlnx_pci_dev *xpdev, struct qdma_queue_conf *qconf)
{
	char ebuf[XNL_RESP_BUFLEN_MIN] = {0};
	struct xlnx_qdata *qdata;
	int ret = 0;

	qdata = xpdev_queue_get(xpdev, qconf->qidx,
			qconf->q_type, 1, ebuf, XNL_ERR_BUFLEN);

	ret = qdma_queue_stop(xpdev->dev_hndl, qdata->qhndl, ebuf, XNL_RESP_BUFLEN_MIN);
	ALOG_DEBUG(xpdev, "%s, idx %u, q_type %s,   qdma_queue_stop  %d.\n",
		dev_name(&xpdev->pdev->dev), qconf->qidx,
			q_type_list[qconf->q_type].name, ret);
	return ret;
}

static void cash_qdma_queue_config(struct xlnx_pci_dev *xpdev, struct qdma_queue_conf *qconf, unsigned int queue_begin, unsigned int queue_end)
{
	int ret;
	int queue_idx;

	for (queue_idx = queue_begin; queue_idx <= queue_end; queue_idx++) {
		qconf->qidx = queue_idx;
		ret = cash_qdma_queue_add(xpdev, qconf);
		if (ret) {
			ALOG_INFO(xpdev, "%s, idx %u, q_type %s,  cash_qdma_add failed.\n",
				dev_name(&xpdev->pdev->dev), qconf->qidx,
				q_type_list[qconf->q_type].name);
		}

		ret = cash_qdma_queue_start(xpdev, qconf);
		if (ret) {
			ALOG_INFO(xpdev, "%s, idx %u, q_type %s,  cash_qdma_start failed.\n",
				dev_name(&xpdev->pdev->dev), qconf->qidx,
				q_type_list[qconf->q_type].name);
		}
	}
}

//qdma channels config and start
static int autox_qdma_init(struct xlnx_pci_dev *xpdev)
{
	struct qdma_queue_conf qconf = {0};
	autox_queue_spec_t *queue_spec = &((autox_spec_t *)xpdev->spec)->queue;

	// video queue config
	xpdev->video_cnt = queue_spec->video.count;
	cash_qconf_init(&qconf, 0);
	qconf.q_type = Q_C2H;
	qconf.desc_bypass = 1;
	qconf.pfetch_bypass = 1;
	cash_qdma_queue_config(xpdev, &qconf, queue_spec->video.begin, queue_spec->video.end);
	
	// event queue config card to host
	cash_qconf_init(&qconf, 0);
	qconf.q_type = Q_C2H;
	qconf.desc_bypass = 1;
	qconf.pfetch_bypass = 1;
	cash_qdma_queue_config(xpdev, &qconf, queue_spec->event.begin, queue_spec->event.end);
	// event queue config host to card
	cash_qconf_init(&qconf, 0);
	qconf.q_type = Q_H2C;
	cash_qdma_queue_config(xpdev, &qconf, queue_spec->event.begin, queue_spec->event.end);

	// can queue config card to host
	cash_qconf_init(&qconf, 0);
	qconf.q_type = Q_C2H;
	qconf.desc_bypass = 1;
	qconf.pfetch_bypass = 1;
	cash_qdma_queue_config(xpdev, &qconf, queue_spec->can.begin, queue_spec->can.end);
	// event queue config host to card
	cash_qconf_init(&qconf, 0);
	qconf.q_type = Q_H2C;
	cash_qdma_queue_config(xpdev, &qconf, queue_spec->can.begin, queue_spec->can.end);
	
	// test queue config
	cash_qconf_init(&qconf, 0);
	qconf.q_type = Q_C2H;
	cash_qdma_queue_config(xpdev, &qconf, queue_spec->test.begin, queue_spec->test.end);
	qconf.q_type = Q_H2C;
	cash_qdma_queue_config(xpdev, &qconf, queue_spec->test.begin, queue_spec->test.end);

	ALOG_INFO(xpdev, "%d queues: video(%d) event(%d) can(%d) rem(%d) config finished.\n",
		queue_spec->count, queue_spec->video.count, queue_spec->event.count, 
		queue_spec->can.count, queue_spec->test.count);

	return 0;
}


int	acash_qdma_stop(struct xlnx_pci_dev *xpdev)
{
    int ret = 0;
	struct qdma_queue_conf qconf = {0};
	//char ebuf[256] = {0};
	int k = 0;
	cash_qconf_init(&qconf, 0);
	for (k = 0; k < 15; k++)
	{
		qconf.qidx = k;
		qconf.q_type = Q_C2H;
		ret = cash_qdma_queue_stop(xpdev, &qconf);
		ret = cash_qdma_queue_delete(xpdev, &qconf);
	}
	for (k = 8; k < 15; k++)
	{
		qconf.qidx = k;
		qconf.q_type = Q_H2C;
		ret = cash_qdma_queue_stop(xpdev, &qconf);
		ret = cash_qdma_queue_delete(xpdev, &qconf);
	}
	
	qconf.qidx = 16;
	qconf.q_type = Q_C2H;
	ret = cash_qdma_queue_stop(xpdev, &qconf);
	ALOG_DEBUG(xpdev, " acash_qdma_stop  cash_qdma_queue_stop %d", ret);
	ret = cash_qdma_queue_delete(xpdev, &qconf);
	ALOG_DEBUG(xpdev, " acash_qdma_stop  cash_qdma_queue_delete %d", ret);
	qconf.q_type = Q_H2C;
	ret = cash_qdma_queue_stop(xpdev, &qconf);
	ALOG_DEBUG(xpdev, " acash_qdma_stop  cash_qdma_queue_stop %d", ret);
	ret = cash_qdma_queue_delete(xpdev, &qconf);
	ALOG_DEBUG(xpdev, " acash_qdma_stop  cash_qdma_queue_delete %d", ret);

	return ret;
}

// from Xilinx original code
static int xpdev_qdata_realloc(struct xlnx_pci_dev *xpdev, unsigned int qmax)
{
	if (!xpdev)
		return -EINVAL;

	kfree(xpdev->qdata);
	xpdev->qdata = NULL;

	if (!qmax)
		return 0;
	xpdev->qdata = kzalloc(qmax * 3 * sizeof(struct xlnx_qdata),
			       GFP_KERNEL);
	if (!xpdev->qdata) {
		pr_err("OMM, xpdev->qdata, sz %u.\n", qmax);
		return -ENOMEM;
	}
	xpdev->qmax = qmax;

	return 0;
}

static int autox_sysm_config(autox_sysm_t *asysm)
{
	struct xlnx_pci_dev *xpdev = asysm->xpdev;
	int qmax = 0;
	int ret = 0;

	qmax = qdma_get_qmax(xpdev->dev_hndl);
	ALOG_DEBUG(xpdev, "autox_sysm_config old qmax %d", qmax);
	qmax = MAXQ;
	ret = qdma_set_qmax(xpdev->dev_hndl, -1, qmax);
	ALOG_DEBUG(xpdev, "autox_sysm_config set qmax return %d", ret);
	qmax = qdma_get_qmax(xpdev->dev_hndl);
	ALOG_DEBUG(xpdev, "autox_sysm_config get new qmax  %d", qmax);

	if (!ret)
	    xpdev_qdata_realloc(xpdev, qmax);
	
	ret = autox_qdma_init(xpdev);
	return ret;
}

static long autox_sysm_ioctl(struct file *filp, unsigned int cmd,
    unsigned long arg)
{
	autox_sysm_t *asysm = filp->private_data;
	struct xlnx_pci_dev *xpdev = asysm->xpdev;
	int err = 0;
	autox_scratch_reg_addr_spec_t *scratch_reg = &((autox_spec_t *)xpdev->spec)->scratch_reg;

	ALOG_DEBUG(xpdev, "file 0x%p\n", filp);

	switch (cmd) {
	case AUTOX_IOC_SYSM_VER:
	{
		uint32_t fw_fpga_ver = 
			read_cash3_user_reg(xpdev->dev_hndl, scratch_reg->fpga_ver.addr);
		autox_fw_fpga_ver_t *ver = (void*)&fw_fpga_ver;

		ALOG_INFO(xpdev, "cash_fw_ver=%d file_fw_ver=%d, hw_ver=%d.%d \n"
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
		struct cash3_host_cmd_t *cmd;
		struct cash3_host_cmd_backup_fpga_image_flash_t *fpga_image_flash;

		ALOG_INFO(xpdev, "AUTOX_IOC_SYSM_UPDATE\n");

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

		ALOG_INFO(xpdev, 
			"AUTOX_IOC_SYSTEM_UPDATE ioc_fw_arg->fw_data_size (%lu)   cmd %p", 
			ioc_fw_arg->fw_data_size, cmd);

		fpga_image_flash->flash_image_size = ioc_fw_arg->fw_data_size;

		memcpy(fpga_image_flash->flash_image
			, ioc_fw_arg->fw_data
			, fpga_image_flash->flash_image_size);

		cmd->head.cmd = CASH3_HOST_CMD_BACKUP_FPGA_IMAGE_FLASH;

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
		rc = read_cash3_user_reg(xpdev->dev_hndl, scratch_reg->cmd_status.addr);
				
		printk(KERN_ERR "CASH3_SCRATCH_REG_CMD_STATUS_ADDR %x\n", rc);
		if (rc != 0) {
			err = -EFAULT;
		}
		break;
	}

	case AUTOX_IOC_SYSM_ENABLE:
	{
		char mem[512] = {0};
		int rc = 0;
		struct cash3_host_cmd_t *cmd = (void*)mem;

		ALOG_DEBUG(xpdev, "AUTOX_IOC_SYSM_ENABLE\n");
		
		cmd->head.cmd = CASH3_HOST_CMD_ENABLE_BACKUP_FPGA_IMAGE;

		rc = send_host_cmd(xpdev->dev_hndl
			, cmd
			, sizeof(mem)
			, true);

		if (rc < 0) {
			err = -EFAULT;
			return err;
		}
		// Get command return result from A53
		rc = read_cash3_user_reg(xpdev->dev_hndl, scratch_reg->cmd_status.addr);
		if (rc < 0) {
			ALOG_ERR(xpdev, "The command return result is failed,rc=%d.\n", rc);
			err = -EFAULT;
		}

		break;
	}

	default:
		// TODO: remove old fw update code
		err = autox_sysm_ioctl_handle(filp, cmd, arg);
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
	autox_sysm_config(asysm);
	return 0;
}

int autox_sysm_alloc(struct xlnx_pci_dev *xpdev)
{
	autox_sysm_t *asysm;

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

	autox_sysm_cdev_fini(asysm);

	ALOG_DEBUG(xpdev, "done\n");
}
