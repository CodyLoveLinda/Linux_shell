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

#define FW_TAHOE			0
#define FW_CASH1_1			1
#define FW_CASH1_2			2

static int get_cash_fw_ver(char *autox_fw_file) {
    char *file_name = NULL;

    file_name = rindex(autox_fw_file, '/');
    if (file_name == NULL) {
        file_name = autox_fw_file;
    } else {
        file_name++;
    }

    if ((strstr(file_name, "CaSH3") != NULL)) {
        return FW_CASH1_2;
    } else if ((strstr(file_name, "CaSH") != NULL)) {
        return FW_CASH1_1;
    } else if ((strstr(file_name, "Tahoe") != NULL)) {
        return FW_TAHOE;
    } else {
        return -1;
    }
}

static int autox_fw_update_check_integrity(const char *file_name)
{
    int ret;
    char autox_file_check_cmd[256];

    ret = get_cash_fw_ver((char *)file_name);
    if (ret == -1) {
        printf("error: invalid firmware.\n");
        return -1;
    }

    strcpy(autox_file_check_cmd, "gunzip -t ");
    strcat(autox_file_check_cmd, file_name);
    ret = system(autox_file_check_cmd);

    return ret;
}

static void autox_fw_update_get_version(int dev_fd, autox_update_start_args_t *update_start_arg)
{
    int ret;
    autox_fw_fpga_ver_t fpga_ver = {0};
    autox_common_send_cmd_t common_query_cmd_data = {0};  // no need other params
    ret = autox_common_query(dev_fd, &common_query_cmd_data, update_start_arg);
    // after query, to parse the result
    if (ret == 0) {
        memcpy((char *)(&fpga_ver), (char *)(&common_query_cmd_data), sizeof(autox_fw_fpga_ver_t));
        printf("-------------------------------\n");
        printf("CaSH Firmware Version:	%d\n", fpga_ver.fw_ver);
        printf("CaSH FPGA Version:	%d.%d\n", fpga_ver.hw_major_ver, fpga_ver.hw_minor_ver);
        printf("-------------------------------\n");
    } else {
        printf("ERROR:<%s> get version failed, ret %d.\n", __func__, ret);
    }

    return;
}

static int autox_fw_update_upload_file(int dev_fd, FILE *fp) 
{
    int ret;
    autox_update_cmd_t update_cmd = {0};
    autox_update_arg_t *update_arg = NULL;

    update_arg = (autox_update_arg_t *)malloc(sizeof(autox_update_arg_t));
    if (update_arg == NULL) {
        printf("ERROR: <%s> malloc failed.\n", __func__);
        return -EINVAL;
    }

    update_arg->data_size = 0;
    update_cmd.data = (char *)update_arg;
    update_cmd.data_size = sizeof(autox_update_arg_t);
    update_cmd.type = AUTOX_FW_UPDATE;
    ret = autox_common_upload(dev_fd, fp, &update_cmd);
    if (ret != 0) {
        printf("ERORR: <%s> upload file failed.\n", __func__);
    }

    free(update_arg);
    return ret;
}

static int autox_fw_update_active_file(int dev_fd, autox_update_start_args_t *update_start_arg)
{
    autox_common_send_cmd_t common_send_cmd = {0};  // no need other params
    return autox_common_active(dev_fd, &common_send_cmd, update_start_arg);
}

int autox_fw_update_start(int dev_fd, autox_update_start_args_t *update_start_arg)
{
    int ret;
    FILE *fp = NULL;
    if (update_start_arg == NULL) {
        printf("ERROR:autox_fw_update_start:input update_start_arg is NULL\n");
        return -EINVAL;
    }
    if (update_start_arg->print_flag == 1) {
        autox_fw_update_get_version(dev_fd, update_start_arg);
        return 0;
    }

    printf("autox firmware update begin.....\n");
    ret = autox_fw_update_check_integrity(update_start_arg->file_name);
    if (ret != 0) {
        printf("ERROR: corrupted firmware image file%s!\n", update_start_arg->file_name);
        return ret;
    }

    fp = autox_update_open_file(update_start_arg->file_name);
    if (fp == NULL) {
        printf("ERROR: autox firmware update can not open file %s.\n",update_start_arg->file_name);
        return -EIO;
    }

    ret = autox_fw_update_upload_file(dev_fd, fp);
    if (ret != 0) {
        printf("ERROR: autox update upload firmware %s failed.\n", update_start_arg->file_name);
        fclose(fp);
        return ret;
    }
    fclose(fp);

    ret = autox_fw_update_active_file(dev_fd, update_start_arg);
    if (ret != 0) {
        printf("ERROR: <%s> fw update active failed.\n", __func__);
        return ret;
    }
    printf("\n");
    printf("\n");
    printf("*************************************************************\n");
    printf("**********           Done FW UPDATE!!!             **********\n");
    printf("**********                                         **********\n");
    printf("**********         Please DO Power-cycle!!!        **********\n");
    printf("*************************************************************\n");
    printf("\n");
    printf("\n");
    return 0;
}
