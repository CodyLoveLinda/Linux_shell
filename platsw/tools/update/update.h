#ifndef AUTOX_UPDATE_H
#define AUTOX_UPDATE_H
#include "common.h"

int autox_config_update_start(int dev_fd, autox_update_start_args_t *update_start_arg);
int autox_fw_update_start(int dev_fd, autox_update_start_args_t *update_start_arg);
int autox_isp_update_start(int dev_fd, autox_update_start_args_t *update_start_arg);
int autox_custom_file_update_start(int dev_fd, autox_update_start_args_t *update_start_arg);

#endif
