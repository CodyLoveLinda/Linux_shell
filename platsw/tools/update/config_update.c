#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include "common.h"
#include "linux/autox_api.h"

static int autox_config_update_upload_file(int dev_fd, FILE *fp, char *file_md5) 
{
    int ret;
    autox_update_cmd_t update_cmd = {0};
    autox_update_arg_t *update_arg = NULL;

    update_arg = (autox_update_arg_t *)malloc(sizeof(autox_update_arg_t));
    if (update_arg == NULL) {
        printf("ERROR: <%s> malloc failed.\n", __func__);
        return -EINVAL;
    }

    memcpy(update_arg->data, file_md5, AUTOX_MD5_STR_LEN);
    update_arg->data_size = AUTOX_MD5_STR_LEN;

    update_cmd.data = (char *)update_arg;
    update_cmd.data_size = sizeof(autox_update_arg_t);
    update_cmd.type = AUTOX_CONFIGURATION_UPDATE;

    ret = autox_common_upload(dev_fd, fp, &update_cmd);
    if (ret != 0) {
        printf("ERORR: <%s> upload file failed.\n", __func__);
    }

    free(update_arg);
    return ret;
}

static int autox_config_update_active_file(int dev_fd, autox_update_start_args_t *update_start_arg)
{
    autox_common_send_cmd_t common_send_cmd = {0};  // no need other params
    return autox_common_active(dev_fd, &common_send_cmd, update_start_arg);
}

static int autox_config_update_get_version(int dev_fd, autox_config_md5_t *config_md5,
                                           autox_update_start_args_t *update_start_arg)
{
    int ret;
    int i;
    unsigned int reg_val;
    autox_common_send_cmd_t common_send_cmd = {0};  // no need other params
    ret = autox_common_query(dev_fd, &common_send_cmd, update_start_arg);
    if (ret != 0) {
        printf("ERROR:<%s> get version failed, ret %d.\n", __func__, ret);
        return ret;
    }

    /**
     * @brief  after query, to parse the result
     * A total of 32 bytes are valid in common_send_cmd.data,and stored data format is unsigned int
     * For example:
     * common_send_cmd.data(every data is uint_32):
     * first 16 bytes(for act_md5) is:{1|10|11|12}  second 16 bytes(for backup_md5) is:{13|14|15|2}
     * then,we can conclude:
     * act_md5_str   ={"000000010000000A0000000B0000000C"}
     * backup_md5_str={"0000000D0000000E0000000F00000002"}
     */
    for (i = 0; i < UPDATE_SCRATH_REG_CNT; i++) {
        // Every data format is uint32_t(4 bytes)
        memcpy(&reg_val, &(common_send_cmd.data[i * 4]), 4);
        if (i < 4) {
            // The first 16 bytes are act_md5_value
            sprintf(&(config_md5->act_md5_value[i * 8]), "%08x", reg_val);
        } else {
            // The second 16 bytes are backup_md5_value
            sprintf(&(config_md5->backup_md5_value[i * 8 - 32]), "%08x", reg_val);
        }
    }

    return 0;
}

int autox_config_update_start(int dev_fd, autox_update_start_args_t *update_start_arg)
{
    int ret;
    char file_md5[AUTOX_MD5_STR_LEN + 1];
    FILE *fp = NULL;
    autox_config_md5_t config_md5 = { '\0' };
    if (update_start_arg == NULL) {
        printf("ERROR:autox_config_update_start:input update_start_arg is NULL\n");
        return -EINVAL;
    }
    ret = autox_config_update_get_version(dev_fd, &config_md5, update_start_arg);
    if (ret != 0) {
        printf("ERROR: <%s> get version failed, ret %d.\n", __func__, ret);
        return ret;
    }

    if (update_start_arg->print_flag == 1) {
        printf("-------------------------------\n");
        printf("CaSH Active Config Md5 Value: %s\n", config_md5.act_md5_value);
        printf("CaSH Backup Config Md5 Value: %s\n", config_md5.backup_md5_value);
        printf("-------------------------------\n");
        return 0;
    }

    printf("autox config update begin.....\n");
    ret = calculateMd5Value(update_start_arg->file_name, file_md5);
    if (ret != 0) {
        printf("autox_config_update_start: Error calculate the file_md5\n");
        return ret;
    }
    if (strcmp(file_md5, config_md5.act_md5_value) == 0) {
        printf("ERROR: <%s> file %s has been already actived.\n", __func__,
                update_start_arg->file_name);
        return 0;
    }

    if (strcmp(file_md5, config_md5.backup_md5_value) == 0) {
        printf("file %s has been backup of config.\n", update_start_arg->file_name);
        ret = autox_config_update_active_file(dev_fd, update_start_arg);
        if (ret != 0) {
            printf("ERROR: <%s> active file failed.\n", __func__);
            return ret;
        }

        printf("\n");
        printf("\n");
        printf("*************************************************************\n");
        printf("**********           Done CONFIG UPDATE!!!             **********\n");
        printf("**********                                         **********\n");
        printf("**********         Please DO Power-cycle!!!        **********\n");
        printf("*************************************************************\n");
        printf("\n");
        printf("\n");
        return 0;
    }

    fp = autox_update_open_file(update_start_arg->file_name);
    if (fp == NULL) {
        printf("ERROR: assh utox config update can not open file %s.\n",
                update_start_arg->file_name);
        return -EIO;
    }

    ret = autox_config_update_upload_file(dev_fd, fp, file_md5);
    if (ret != 0) {
        printf("ERROR: autox update upload config %s failed.\n", update_start_arg->file_name);
        fclose(fp);
        return ret;
    }
    fclose(fp);

    ret = autox_config_update_active_file(dev_fd, update_start_arg);
    if (ret != 0) {
        printf("ERROR: <%s> active file failed.\n", __func__);
        return ret;
    }
    printf("\n");
    printf("\n");
    printf("*************************************************************\n");
    printf("**********           Done Config UPDATE!!!             **********\n");
    printf("**********                                         **********\n");
    printf("**********         Please DO Power-cycle!!!        **********\n");
    printf("*************************************************************\n");
    printf("\n");
    printf("\n");

    return 0;
}
