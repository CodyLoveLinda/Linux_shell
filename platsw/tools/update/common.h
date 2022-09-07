#ifndef __AUTOX_COMMON_H__
#define __AUTOX_COMMON_H__
#include <stdio.h>
#include "linux/autox_api.h"

typedef struct autox_update_start_args {
    char dev_name[32];  // device name: such as /dev/autox_sysm0
    char file_name[1024];  // the name of update file
    int print_flag;  // whether only print version
    autox_update_type_t update_type;  // fw/config/isp/custom
    int subtype;  // the sub type of update mode: isp_firmware/isp_bootloader
    int mipi_id;  // the mipi_id
    int board_no;  // board_no: 0 or 1
    int camera_type;  // camera_type:0-0x8b40
} autox_update_start_args_t;

typedef enum autox_common_query_type {
	AUTOX_COMMON_QUERY_VERSION = 0,
	AUTOX_COMMON_QUERY_MD5 = 1,
	AUTOX_COMMON_QUERY_STATE = 2,
	AUTOX_COMMON_QUERY_TYPE_MAX
} autox_common_query_type_t;

#define COMMON_ACTIVE_WAIT_CNT_MAX 60
#define COMMON_UPLOAD_WAIT_CNT_MAX 300

FILE *autox_update_open_file(const char *file_name);

int autox_update_get_version(int dev_fd, autox_update_cmd_t *update_cmd);
int autox_update_active_file(int dev_fd, autox_update_cmd_t *update_cmd);

int calculateMd5Value(const char *filepath, char *value);
int get_md5_from_arm_file(int dev_fd, autox_common_send_cmd_t *common_query_cmd_data_ptr,
                          autox_update_start_args_t *update_start_arg, char *md5_x86_file,
                          char *md5_arm_file);

int autox_common_query(int dev_fd, autox_common_send_cmd_t *common_send_cmd,
                            autox_update_start_args_t *update_start_arg);
int autox_common_active(int dev_fd, autox_common_send_cmd_t *common_send_cmd,
                                        autox_update_start_args_t *update_start_arg);
int autox_common_upload(int dev_fd, FILE *fp, autox_update_cmd_t *update_cmd);

#endif
