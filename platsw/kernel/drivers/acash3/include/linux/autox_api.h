/* 
 * Copyright (C) 2019 AutoX, Inc. 
 */

#ifndef _AUTOX_API_H_
#define	_AUTOX_API_H_

#ifndef __KERNEL__
#include <sys/time.h>
#else
#include <linux/time.h>
#endif

#define	AUTOX_DEV_GPS			"autox_gps"
#define AUTOX_DEV_SYSM			"autox_sysm"

/*
 * Camera Deifinitions
 */
enum autox_cam_ioc_cmd {
	CAM_CMD_CAPS,
	CAM_CMD_TRIGGER_ENABLE,
	CAM_CMD_TRIGGER_DISABLE,
	CAM_CMD_TRIGGER_STATUS,
	CAM_CMD_STATS,
	CAM_CMD_NUM
};

enum autox_video_stats {
	CAM_STATS_RX = 0,
	CAM_STATS_RX_DROP,
	CAM_STATS_RESET,
	CAM_STATS_LINK_CHANGE,
	CAM_STATS_FRAME_GAP_ERR,
	CAM_STATS_FRAME_DROP,
	CAM_STATS_FRAME_DROP_1,
	CAM_STATS_FRAME_DROP_2,
	CAM_STATS_FRAME_DROP_M,
	CAM_STATS_FRAME_ERR,
	CAM_STATS_NO_SOF,
	CAM_STATS_NO_EOF,
	CAM_STATS_NO_VIDEO_BUF,
	CAM_STATS_NUM
};

typedef struct autox_camera_trigger {
	unsigned char	fps;
	unsigned char	enabled;
	unsigned short	delay;
	unsigned int	exposure;
} autox_trigger_t;

#define	AUTOX_VDEV_NAME_LEN		49

typedef struct autox_camera_capabilities {
	char		name[AUTOX_VDEV_NAME_LEN];
	unsigned short	version;
	unsigned char	port;
	unsigned char	timestamp_type;
	unsigned char	trigger_mode;
	unsigned char	embedded_data;
	unsigned short	frame_len_lines;
	unsigned short	line_len_pck;
	unsigned int	pixel_clock;
} autox_cam_caps_t;

typedef struct autox_camera_statistics {
	unsigned long	stats[CAM_STATS_NUM];
} autox_cam_stats_t;

#define	AUTOX_IOC_CAM_TYPE		'v'

#define	AUTOX_IOC_CAM_CAPS			\
	_IOR(AUTOX_IOC_CAM_TYPE, CAM_CMD_CAPS, autox_cam_caps_t)

#define	AUTOX_IOC_CAM_TRIGGER_ENABLE		\
	_IOW(AUTOX_IOC_CAM_TYPE, CAM_CMD_TRIGGER_ENABLE, autox_trigger_t)

#define	AUTOX_IOC_CAM_TRIGGER_DISABLE		\
	_IO(AUTOX_IOC_CAM_TYPE, CAM_CMD_TRIGGER_DISABLE)

#define	AUTOX_IOC_CAM_TRIGGER_STATUS		\
	_IOR(AUTOX_IOC_CAM_TYPE, CAM_CMD_TRIGGER_STATUS, autox_trigger_t)

#define	AUTOX_IOC_CAM_STATS			\
	_IOR(AUTOX_IOC_CAM_TYPE, CAM_CMD_STATS, autox_cam_stats_t)

#define	CAM_CAP_TRIGGER_STANDARD	0
#define	CAM_CAP_TRIGGER_DETERMINISTIC	1
#define	CAM_CAP_TRIGGER_SLAVE_STANDARD	2
#define	CAM_CAP_TRIGGER_SLAVE_SHUTTER_SYNC 3

#define	CAM_CAP_TIMESTAMP_FPGA		0
#define	CAM_CAP_TIMESTAMP_TRIGGER	1
#define	CAM_CAP_TIMESTAMP_FORMATION	2
#define	CAM_CAP_TIMESTAMP_HOST		3

/*
 * GPS Definitions
 */
enum autox_gps_stats {
	GPS_STATS_UNLOCK = 0,
	GPS_STATS_DRIFT,
	GPS_STATS_NUM
};

enum autox_gps_ioc_cmd {
	GPS_CMD_TIME,
	GPS_CMD_STATUS,
	GPS_CMD_STATS,
	GPS_CMD_NUM
};

typedef struct autox_epoch_timestamp {
	unsigned int	valid : 1;
	unsigned int	: 7;
	unsigned int	usec : 24;
	unsigned int	sec;
} autox_epoch_ts_t;

typedef struct autox_gps_timestamp {
	unsigned int	valid : 1;
	unsigned int	ns_cnt : 7;	/* In unit of 8 */
	unsigned int	usec : 12;
	unsigned int	msec : 12;
	unsigned int	sec : 8;
	unsigned int	min : 8;
	unsigned int	hour : 6;
	unsigned int	year : 10;
	unsigned int	day : 8;
	unsigned int	month : 8;
	unsigned int	: 16;
} autox_gps_ts_t;

typedef struct autox_gps_statistics {
	unsigned long	stats[GPS_STATS_NUM];
} autox_gps_stats_t;

#define	AUTOX_GPS_PPS_OK		0x1
#define	AUTOX_GPS_GPRMC_OK		0x2
#define	AUTOX_IOC_GPS_TYPE		'g'

#define	AUTOX_IOC_GPS_TIME		\
	_IOR(AUTOX_IOC_GPS_TYPE, GPS_CMD_TIME, struct timespec64)

#define	AUTOX_IOC_GPS_STATUS		\
	_IOR(AUTOX_IOC_GPS_TYPE, GPS_CMD_STATUS, unsigned int)

#define	AUTOX_IOC_GPS_STATS		\
	_IOR(AUTOX_IOC_GPS_TYPE, GPS_CMD_STATS, autox_gps_stats_t)

/*
 * SYSM Definitions
 */
enum autox_sysm_ioc_cmd {
	SYSM_CMD_VER,
	SYSM_CMD_UPDATE,
	SYSM_CMD_CMDSTS,
	SYSM_CMD_ENABLE,
	SYSM_CMD_VERSION,
	SYSM_CMD_UPLOAD,
	SYSM_CMD_ACTIVE,
	SYSM_CMD_NUM
};

#define AUTOX_FW_MAX_SIZE		(200*1024*1024)
#define AUTOX_UPDATE_DATA_MAX_SIZE		(200*1024*1024)
#define AUTOX_MD5_STR_LEN 32
#define AUTOX_MD5_REG_CNT 4
#define AUTOX_REG_MD5_STR_LEN (AUTOX_MD5_STR_LEN / AUTOX_MD5_REG_CNT)
#define AUTOX_COMMON_SEND_CMD_LEN 1024
// scrath registers const variable of update
#define UPDATE_SCRATH_STR_LEN	32
#define UPDATE_SCRATH_REG_CNT	8

typedef struct autox_fw_update {
	unsigned long	fw_data_size;
	unsigned long	fw_cmd;
	unsigned char	fw_cmd_buf[8];
	char 			fw_data[AUTOX_FW_MAX_SIZE];
} autox_fw_update_t;

typedef struct __attribute__((packed)) {
	unsigned short fw_ver : 10;					//running firmware version
	unsigned short hw_major_ver : 6;				//running FPGA (hw) major version
	unsigned short backup_fw_ver : 10;			//firmware version on file as a backup
	unsigned short hw_minor_ver : 6;				//running FPGA (hw) minor version
} autox_fw_fpga_ver_t ;

typedef enum autox_update_type {
	AUTOX_FW_UPDATE = 0,
	AUTOX_CONFIGURATION_UPDATE = 1,
	AUTOX_CUSTOM_FILE_UPDATE = 2,
	AUTOX_ISP_UPDATE = 3,
	AUTOX_UPDATE_TYPE_MAX
} autox_update_type_t;

typedef struct autox_update_arg {
	unsigned long data_size;
	unsigned long cmd;
	unsigned char cmd_buf[8];
	char data[AUTOX_UPDATE_DATA_MAX_SIZE];
} autox_update_arg_t;

typedef struct autox_update_cmd {
	unsigned int type;
	unsigned long data_size;
	char *data;
} autox_update_cmd_t;

typedef struct autox_config_md5 {
	char act_md5_value[AUTOX_MD5_STR_LEN + 1];
	char backup_md5_value[AUTOX_MD5_STR_LEN + 1];
} autox_config_md5_t;

typedef struct autox_common_send_cmd {
	unsigned int type;
	unsigned int data_size;
	char data[AUTOX_COMMON_SEND_CMD_LEN];
} autox_common_send_cmd_t;

#define	AUTOX_IOC_SYSM_TYPE		's'

#define	AUTOX_IOC_SYSM_VER		\
	_IOR(AUTOX_IOC_SYSM_TYPE, SYSM_CMD_VER, unsigned int)

#define	AUTOX_IOC_SYSM_UPDATE		\
	_IOW(AUTOX_IOC_SYSM_TYPE, SYSM_CMD_UPDATE, unsigned int)

#define	AUTOX_IOC_SYSM_CMDSTS		\
	_IOR(AUTOX_IOC_SYSM_TYPE, SYSM_CMD_CMDSTS, unsigned int)

#define	AUTOX_IOC_SYSM_ENABLE		\
	_IOW(AUTOX_IOC_SYSM_TYPE, SYSM_CMD_ENABLE, unsigned int)

#define AUTOX_IOC_SYSM_VERSION \
	_IOW(AUTOX_IOC_SYSM_TYPE, SYSM_CMD_VERSION, autox_update_cmd_t)

#define AUTOX_IOC_SYSM_UPLOAD \
	_IOW(AUTOX_IOC_SYSM_TYPE, SYSM_CMD_UPLOAD, autox_update_cmd_t)

#define AUTOX_IOC_SYSM_ACTIVE \
	_IOW(AUTOX_IOC_SYSM_TYPE, SYSM_CMD_ACTIVE, autox_update_cmd_t)


#endif /* _AUTOX_API_H_ */
