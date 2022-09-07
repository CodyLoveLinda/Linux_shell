#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/ioctl.h>

#include "common.h"

/**
 * @brief transfer file from x86 to arm
 *        update_arg->data:
 *                  0~1 bytes:              filename_lens < 1024
 *                  2~2+filename_lens:      filename
 *                  2+filename_lens+1~end:  filedata
 * @param dev_fd
 * @param fp
 * @param file_name
 * @param update_start_arg
 * @return int
 */
int autox_custom_file_update_upload(int dev_fd, FILE *fp, char *file_name,
                                    autox_update_start_args_t *update_start_arg)
{
    int ret;
    autox_update_cmd_t update_cmd = {0};
    autox_update_arg_t *update_arg = NULL;
    unsigned short int filename_len = 0;
    char *name_ptr = NULL;
    char *path_buff = file_name;

    if (fp == NULL || file_name == NULL) {
        printf("ERROR: <%s> fp or filename is NULL.\n", __func__);
        return -EINVAL;
    }

    update_arg = (autox_update_arg_t *)malloc(sizeof(autox_update_arg_t));
    if (update_arg == NULL) {
        printf("ERROR: <%s> malloc failed.\n", __func__);
        return -ENOMEM;
    }
    // get filename without path
    name_ptr = strsep(&path_buff, "/");
    while (path_buff != NULL) {
        name_ptr = strsep(&path_buff, "/");
        printf("name_ptr=%s,path_buff=%s\n", name_ptr, path_buff);
    }
    filename_len = strlen(name_ptr) + 1;  // two bytes
    printf("file_name=%s,name_len=%d\n", name_ptr, filename_len);

    // copy filename_lens to update_arg->data, filename_lens < 1024
    memcpy(update_arg->data, (char *)(&filename_len), sizeof(filename_len));
    // copy filename to update_arg->data + 2
    memcpy(update_arg->data + sizeof(filename_len), name_ptr, filename_len);

    update_arg->data_size = filename_len + sizeof(filename_len);
    update_cmd.data = (char *)update_arg;
    update_cmd.data_size = sizeof(autox_update_arg_t);
    update_cmd.type = update_start_arg->update_type;

    // copy file datas to update_arg->data
    ret = autox_common_upload(dev_fd, fp, &update_cmd);
    if (ret != 0) {
        printf("ERORR: <%s> upload file failed.\n", __func__);
    }

    free(update_arg);
    return ret;
}

/**
 * @brief update custom file
 *        step 1: transfer the file from x86 to arm
 *        step 2: check the file md5,the md5 value of x86 is need equal to the md5 of arm
 * @param dev_fd
 * @param update_start_arg
 */
int autox_custom_file_update_start(int dev_fd, autox_update_start_args_t *update_start_arg)
{
    int ret = 0;
    char x86_file_md5[AUTOX_MD5_STR_LEN + 1] = {'\0'};
    FILE *fp = NULL;
    char arm_file_md5[AUTOX_MD5_STR_LEN + 1] = {'\0'};
    autox_common_send_cmd_t custom_send_cmd_data = {'\0'};

    struct timeval tvpre;
    struct timeval tvafter;
    int cost_time = 0;

    // check parameter
    if (update_start_arg->update_type != AUTOX_CUSTOM_FILE_UPDATE) {
        printf("-------------------------------\n");
        printf("ERROR: input update_type =%d is incorrect\n", update_start_arg->update_type);
        printf("-------------------------------\n");
        return -EINVAL;
    }
    gettimeofday(&tvpre, NULL);

    if (update_start_arg->print_flag == 1) {
        printf("INFO: custom file update not support to query version\n");
        return 0;
    }
    ret = calculateMd5Value(update_start_arg->file_name, x86_file_md5);
    if (ret != 0) {
        printf("autox_custom_update_start: Error calculate the x86_file_md5\n");
        return ret;
    }

    // step 1: transfer the file from x86 to arm
    fp = autox_update_open_file(update_start_arg->file_name);
    if (fp == NULL) {
        printf("ERROR:custom file update can not open file %s.\n", update_start_arg->file_name);
        return -EIO;
    }
    ret = autox_custom_file_update_upload(dev_fd, fp, update_start_arg->file_name,
                                          update_start_arg);
    if (ret != 0) {
        printf("ERROR:custom file fail to upload the file %s.\n", update_start_arg->file_name);
        fclose(fp);
        return ret;
    }
    fclose(fp);

    // step 2: check the rom md5,the md5 value of x86 is need equal to the md5 of arm
    ret = get_md5_from_arm_file(dev_fd, &custom_send_cmd_data, update_start_arg,
                                x86_file_md5, arm_file_md5);
    if (ret != 0) {
        printf("ERROR: check md5 failed.\n");
        return ret;
    }

    // cost time
    gettimeofday(&tvafter, NULL);
    cost_time = (tvafter.tv_sec - tvpre.tv_sec) + (tvafter.tv_usec - tvpre.tv_usec) / 1000000;
    printf("Custom file upload cost time = %ds\n", cost_time);

    printf("\n");
    printf("\n");
    printf("*************************************************************\n");
    printf("**********    *PASS* Custom File Upload Done!!!    **********\n");
    printf("*************************************************************\n");
    printf("\n");
    printf("\n");

    return 0;
}
