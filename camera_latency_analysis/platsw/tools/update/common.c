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
#include <fcntl.h>
#include "common.h"

FILE *autox_update_open_file(const char *file_name)
{
    FILE *fp = NULL;

    if (file_name == NULL || file_name[0] == '\0') {
        printf("ERROR: need to provide the new file with '-f' option.\n");
        return NULL;
    }

    fp =  fopen(file_name, "rb");
    return fp;
}

int autox_update_get_version(int dev_fd, autox_update_cmd_t *update_cmd)
{
    if (update_cmd == NULL) {
        printf("ERROR:autox_update_get_version:param is invalid\n");
        return -EINVAL;
    }

    if (ioctl(dev_fd, AUTOX_IOC_SYSM_VERSION, update_cmd)) {
        printf("failed to do ioctl AUTOX_IOC_SYSM_VERSION\n");
        return -EIO;
    }

    return 0;
}

int autox_update_active_file(int dev_fd, autox_update_cmd_t *update_cmd)
{
    if (update_cmd == NULL) {
        printf("ERROR:autox_update_active_file:param is invalid\n");
        return -EINVAL;
    }

    if (ioctl(dev_fd, AUTOX_IOC_SYSM_ACTIVE, update_cmd)) {
        printf("failed to do ioctl AUTOX_IOC_SYSM_ACTIVE\n");
        return -EIO;
    }

    return 0;
}

int calculateMd5Value(const char *filepath, char *value)
{
    FILE *file = NULL;
    char cmd[] =  "md5sum ";
    char cmd_buf[1024];

    if (filepath == NULL || filepath[0] == '\0' || value == NULL) {
        printf("calculateMd5Value:params are invalid\n");
        return -EINVAL;
    }

    strcpy(cmd_buf, cmd);
    strcat(cmd_buf, filepath);

    file = popen(cmd_buf, "r");
    if (file == NULL) {
	    printf("%s: calculate md5 value failed!\n", __func__);
	    return -EIO;
    }

    fscanf(file, "%s", value);
    pclose(file);
    return 0;
}

/**
 * @brief common query data,support isp/custom/config to query...
 *
 * @param dev_fd
 * @param common_send_cmd
 * @param update_start_arg
 * @return int
 */
int autox_common_query(int dev_fd, autox_common_send_cmd_t *common_send_cmd,
                            autox_update_start_args_t *update_start_arg)
{
    int ret;
    int len;
    autox_update_cmd_t update_cmd = {0};
    if (common_send_cmd == NULL || update_start_arg == NULL) {
        printf("autox_common_query:params are invalid\n");
        return -EINVAL;
    }

    update_cmd.data = (char *)common_send_cmd;
    update_cmd.data_size = sizeof(autox_common_send_cmd_t);
    update_cmd.type = update_start_arg->update_type;

    ret = autox_update_get_version(dev_fd, &update_cmd);
    if (ret != 0) {
        printf("ERROR:<%s> get version failed, ret %d.\n", __func__, ret);
        return ret;
    }

    return 0;
}

/**
 * @brief Get the md5 from arm file object,and compare with md5 of x86
 *
 * @param dev_fd
 * @param common_query_cmd_data_ptr
 * @param update_start_arg
 * @param md5_x86_file
 * @param md5_arm_file
 * @return int
 */
int get_md5_from_arm_file(int dev_fd, autox_common_send_cmd_t *common_query_cmd_data_ptr,
                          autox_update_start_args_t *update_start_arg ,char *md5_x86_file,
                          char *md5_arm_file)
{
    int ret = 0;
    if (common_query_cmd_data_ptr == NULL || update_start_arg == NULL || md5_x86_file == NULL
        || md5_arm_file == NULL) {
         printf("ERROR: <%s> params are invalid\n", __func__);
        return -EINVAL;
    }
    memset(common_query_cmd_data_ptr->data, 0, AUTOX_COMMON_SEND_CMD_LEN);
    common_query_cmd_data_ptr->data[0] = (char)AUTOX_COMMON_QUERY_MD5;  // add query action type
    common_query_cmd_data_ptr->data_size = 1;
    ret = autox_common_query(dev_fd, common_query_cmd_data_ptr, update_start_arg);
    if (ret != 0) {
        printf("ERROR: <%s> get md5 failed, ret=%d.\n", __func__, ret);
        return ret;
    }
    memcpy(md5_arm_file, common_query_cmd_data_ptr->data, AUTOX_MD5_STR_LEN);
    printf("To compare md5,md5_x86 = %s,md5_arm = %s\n", md5_x86_file, md5_arm_file);
    if (strcmp(md5_x86_file, md5_arm_file) != 0) {
        printf("ERROR: <%s> file transfer failed!md5 is not equal\n", __func__);
        return -EFAULT;
    }
    printf("\n\n******************************\n");
    printf("INFO: md5_x86 is equal to md5_arm, begin to active......\n");
    printf("\n******************************\n");
    return 0;
}

int autox_common_active(int dev_fd, autox_common_send_cmd_t *common_send_cmd,
                        autox_update_start_args_t *update_start_arg)
{
    int ret;
    autox_update_cmd_t update_cmd = {0};
    int active_wait_cnt = 0;

    if (common_send_cmd == NULL || update_start_arg == NULL) {
        printf("ERROR:autox_common_active:params are invalid\n");
        return -EINVAL;
    }

    update_cmd.data = (char *)common_send_cmd;
    update_cmd.data_size = sizeof(autox_common_send_cmd_t);
    update_cmd.type = update_start_arg->update_type;

    ret = autox_update_active_file(dev_fd, &update_cmd);
    if (ret != 0) {
        printf("ERROR:<%s> active file failed, ret %d.\n", __func__, ret);
        return ret;
    }

    printf("\n");
    printf("*************************************************************\n");
    printf("**********             Upload *Pass*!!!            **********\n");
    printf("**********               Now Active!!!             **********\n");
    printf("**********         Please Wait for 10 min!!!       **********\n");
    printf("*************************************************************\n");
    printf("\n");
    while (active_wait_cnt < COMMON_ACTIVE_WAIT_CNT_MAX) {
        printf("active_wait_cnt (%d) s\n", active_wait_cnt*10);
        if (ioctl(dev_fd, AUTOX_IOC_SYSM_CMDSTS, 1)) {
            sleep(10);
            active_wait_cnt++;
        } else {
            break;
        }
    }
    if (active_wait_cnt >= COMMON_ACTIVE_WAIT_CNT_MAX) {
        printf("\n");
        printf("\n");
        printf("*************************************************************\n");
        printf("**********           *FAILED* ACTIVE!!!            **********\n");
        printf("*************************************************************\n");
        printf("\n");
        printf("\n");
        return -EFAULT;
    }

    return 0;
}

int autox_common_upload(int dev_fd, FILE *fp, autox_update_cmd_t *update_cmd)
{
    int ret;
    int read_bytes = 0;
    int cnt = 0;
    autox_update_arg_t *update_arg = (autox_update_arg_t *)update_cmd->data;

    if (fp == NULL || update_cmd == NULL || update_arg == NULL) {
        printf("ERROR:autox_common_upload:params are invalid\n");
        return -EINVAL;
    }

    read_bytes = fread(update_arg->data + update_arg->data_size, 1,
                       AUTOX_UPDATE_DATA_MAX_SIZE,fp);
    printf("upload file  size: %d.\n", read_bytes);
    if (read_bytes > 0 && read_bytes <= AUTOX_UPDATE_DATA_MAX_SIZE) {
        update_arg->data_size += read_bytes;
        ret = ioctl(dev_fd, AUTOX_IOC_SYSM_UPLOAD, update_cmd);
        if (ret != 0) {
            printf("ERORR: <%s> upload file failed.\n", __func__);
            return -EIO;
        }
    }

    printf("\n");
    printf("\n");
    printf("*************************************************************\n");
    printf("**********                UPLOADING!!!             **********\n");
    printf("**********         Please Wait for 5 min!!!       **********\n");
    printf("*************************************************************\n");
    printf("\n");
    printf("\n");
    while (cnt < COMMON_UPLOAD_WAIT_CNT_MAX)
    {
        printf("upload_wait_cnt (%d)s\n", cnt);
        if (ioctl(dev_fd, AUTOX_IOC_SYSM_CMDSTS, 1)) {
            sleep(1);
            cnt++;
        } else {
            break;
        }
    }
    if (cnt >= COMMON_UPLOAD_WAIT_CNT_MAX) {
        printf("\n");
        printf("\n");
        printf("*************************************************************\n");
        printf("**********           *FAILED* UPDATE!!!            **********\n");
        printf("*************************************************************\n");
        printf("\n");
        printf("\n");
        return -EFAULT;
    }

    printf("\n");
    return 0;
}
