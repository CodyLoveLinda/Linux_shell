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
#include "linux/autox_api.h"
#include "write_isp_csv.h"

/**
 * @brief A structure defining the response payload of isp version info.
 *
 */
typedef struct {
    char  sensorID[8];
    char  buildDate[6];
    char  romProgramSensorName[7];
    char  romProgramLens[4];
    char  romProgramFrameSync;
    char  romProgramAnc;
    char  romProgramYuv;
    char  romUserRev;
    char  romVersionDate;
    char  romVersionMonth;
    char  romVersionYear;
} IspVersionSendToX86Info __attribute__ ((aligned (1)));

#define ISP_FIRMWARE_NAME_TO_ARM "isp.rom"
#define ISP_BOOTLOAD_NAME_TO_ARM "failsafefw.bin"

typedef enum autox_camera_type {
    AUTOX_CAM_0X8B40 = 0,
    AUTOX_CAM_TYPE_MAX
} autox_camera_type_t;

const char support_update_isp_list[AUTOX_CAM_TYPE_MAX][8] = {
    "OX08B40",
};

typedef enum autox_isp_update_subtype {
    ISP_UPDATE_SUBTYPE_FIRMWARE= 0,
    ISP_UPDATE_SUBTYPE_BOOTLOADER= 1
} autox_isp_update_subtype_t;

// the isp state
typedef enum autox_isp_state{
    ISP_SYSTEM_RESET = 0,
    ISP_SYSTEM_INIT = 1,
    ISP_SYSTEM_ACTIVE = 2,
    ISP_SYSTEM_LOWPOWER = 3,
    ISP_SYSTEM_FAILSAFE = 4,
    ISP_SYSTEM_OTHER_STATE = 5
} autox_isp_state_t;

extern int autox_custom_file_update_upload(int dev_fd, FILE *fp, char *file_name,
                                    autox_update_start_args_t *update_start_arg);

static int autox_isp_update_active_file(int dev_fd, autox_common_send_cmd_t *common_send_cmd,
                                        autox_update_start_args_t *update_start_arg)
{
    memset(common_send_cmd->data, 0, AUTOX_COMMON_SEND_CMD_LEN);
    common_send_cmd->type = update_start_arg->subtype;
    common_send_cmd->data[0] = (char)(update_start_arg->mipi_id);  // add mipi_id
    common_send_cmd->data[1] = (char)(update_start_arg->camera_type);  // add camera_type
    common_send_cmd->data_size = 2;

    return autox_common_active(dev_fd, common_send_cmd, update_start_arg);
}

/**
 * @brief Is support to update
 *
 * @param isp_name
 * @return int: 0:support, -ERROR_CODE:not support
 */
static int autox_is_support_update(const char *isp_name)
{
    int support_list_size = sizeof(support_update_isp_list) / sizeof(support_update_isp_list[0]);
    int i = 0;

    if (isp_name == NULL || isp_name[0] == '\0') {
        printf("autox_is_support_update: input isp_name is invalid\n");
        return -EINVAL;
    }

    for (i = 0; i < support_list_size; i++) {
        if(strcmp(isp_name, support_update_isp_list[i]) == 0) {
            printf("INFO: The camera support to update\n");
            return 0;
        }
    }
    printf("Error: The camera not support to update\n");
    return -EPERM;  // not permitted error
}

/**
 * @brief Get the isp version
 *
 * @param dev_fd
 * @param common_query_cmd_data_ptr
 * @param out_version_buf
 * @param update_start_args
 * @return int
 */
static int get_isp_version(int dev_fd, autox_common_send_cmd_t *common_query_cmd_data_ptr,
                           char *out_version_buf, autox_update_start_args_t *update_start_args)
{
    int ret = 0;
    IspVersionSendToX86Info isp_version_info = {0};
    char legacyBuildDate[8] = {'\0'};  // only for old isp version
    memset(common_query_cmd_data_ptr->data, 0, AUTOX_COMMON_SEND_CMD_LEN);
    common_query_cmd_data_ptr->type = update_start_args->subtype;
    common_query_cmd_data_ptr->data[0] = (char)AUTOX_COMMON_QUERY_VERSION;  // add query action
    common_query_cmd_data_ptr->data[1] = (char)(update_start_args->mipi_id);  // add mipi_id
    common_query_cmd_data_ptr->data[2] = (char)(update_start_args->camera_type); // add camera_type
    common_query_cmd_data_ptr->data_size = 3;
    ret = autox_common_query(dev_fd, common_query_cmd_data_ptr, update_start_args);
    if (ret != 0) {
        printf("ERROR: <%s> get isp version failed, ret=%d.\n", __func__, ret);
        return ret;
    }
    // after query, to parse the result
    memcpy(&isp_version_info, common_query_cmd_data_ptr->data, sizeof(isp_version_info));
    isp_version_info.sensorID[7] = '\0';  // the sensorId[7] is not use
    isp_version_info.romUserRev = '\0';
    memcpy(legacyBuildDate, (char *)(&isp_version_info.buildDate),
           sizeof(isp_version_info.buildDate));
    sprintf(out_version_buf,
            "Camera Name: %s || ISP Date: %d/%d/%d || ISP Info: %s|| BuildDate(legacy):%s",
            isp_version_info.sensorID, isp_version_info.romVersionYear,
            isp_version_info.romVersionMonth, isp_version_info.romVersionDate,
            isp_version_info.romProgramSensorName, legacyBuildDate);
    printf("\n\n******************************\n");
    printf("---------*ISP Version*---------\n");
    printf("%s\n", out_version_buf);
    printf("-------------------------------\n");
    printf("\n******************************\n");
    if (update_start_args->print_flag == 1) {
        return 0;  // only print version
    }

    printf("get_isp_version:INFO judge the sensor name whether support to update\n");
    return autox_is_support_update(isp_version_info.sensorID);
}

/**
 * @brief Get the isp state
 *
 * @param dev_fd
 * @param common_query_cmd_data_ptr
 * @param update_start_args
 * @return int
 */
static int get_isp_state(int dev_fd, autox_common_send_cmd_t *common_query_cmd_data_ptr,
                         autox_update_start_args_t *update_start_args)
{
    int ret = 0;
    autox_isp_state_t isp_state = 0;
    if (common_query_cmd_data_ptr == NULL || update_start_args == NULL) {
         printf("ERROR: <%s> params are invalid\n", __func__);
        return -EINVAL;
    }
    memset(common_query_cmd_data_ptr->data, 0, AUTOX_COMMON_SEND_CMD_LEN);
    common_query_cmd_data_ptr->type = update_start_args->subtype;
    common_query_cmd_data_ptr->data[0] = (char)AUTOX_COMMON_QUERY_STATE;  // add query action
    common_query_cmd_data_ptr->data[1] = (char)(update_start_args->mipi_id);  // add mipi_id
    common_query_cmd_data_ptr->data[2] = (char)(update_start_args->camera_type);  // add cameratype
    common_query_cmd_data_ptr->data_size = 3;
    ret = autox_common_query(dev_fd, common_query_cmd_data_ptr, update_start_args);
    if (ret != 0) {
        printf("ERROR: <%s> get isp state failed, ret=%d.\n", __func__, ret);
        return ret;
    }
    // after query, to parse the result
    isp_state = (int)(common_query_cmd_data_ptr->data[0]);  // isp_state only use one byte
    printf("get_isp_state=%d\n", (int)isp_state);

    return isp_state;
}

/**
 * @brief update isp bootloader
 *        step 0: query the isp state,Judge whether it can be upgraded
 *        step 1: transfer the update file from x86 to arm
 *        step 2: check the file md5,the md5 of x86 is need equal to arm
 *        step 3: active the update
 * @param dev_fd
 * @param update_start_arg
 */
static int autox_isp_update_bootloader(int dev_fd, autox_update_start_args_t *update_start_arg)
{
    int ret = 0;
    char x86_file_md5[AUTOX_MD5_STR_LEN + 1] = {'\0'};
    FILE *fp = NULL;
    char arm_file_md5[AUTOX_MD5_STR_LEN + 1] = {'\0'};
    autox_common_send_cmd_t common_query_cmd_data = {'\0'};
    IspCsvData isp_csv_data = {0};
    struct timeval tvpre;
    struct timeval tvafter;
    int cost_time = 0;
    int camera_state = -1;
    if (update_start_arg->subtype != ISP_UPDATE_SUBTYPE_BOOTLOADER) {
        printf("error:autox_isp_bootloader_update_start:subtype=%d is invalid with '-s' option\n",
                update_start_arg->subtype);
        return -EINVAL;
    }
    gettimeofday(&tvpre,NULL);

    if (update_start_arg->print_flag == 1) {
        printf("\n\n-------------------------------\n");
        printf("INFO: isp bootloader not support to query version\n");
        printf("-------------------------------\n");
        return 0;
    }

    // step 0: query the isp state,Judge whether it can be upgraded
    camera_state = get_isp_state(dev_fd, &common_query_cmd_data, update_start_arg);
    if (camera_state < 0 || camera_state <= ISP_SYSTEM_FAILSAFE) {
        printf("\n\n-------------------------------\n");
        printf("ERROR: camera_state = %d, not support update bootloader!\n"
                "      please try to update isp rom!\n", camera_state);
        printf("-------------------------------\n");
        return -EINVAL;
    }

    ret = calculateMd5Value(update_start_arg->file_name, x86_file_md5);
    if (ret != 0) {
        printf("ERROR:autox_isp_bootloader_update_start:fail to calculate the x86_file_md5\n");
        return -EFAULT;
    }
    csv_set_base_info(&isp_csv_data, update_start_arg->board_no, update_start_arg->mipi_id,
                      update_start_arg->file_name, x86_file_md5);

    // step 1: transfer the bootloader file from x86 to arm
    fp = autox_update_open_file(update_start_arg->file_name);
    if (fp == NULL) {
        printf("ERROR: autox isp bootloader update can not open file %s.\n",
                update_start_arg->file_name);
        return -EIO;
    }
    ret = autox_custom_file_update_upload(dev_fd, fp, ISP_BOOTLOAD_NAME_TO_ARM, update_start_arg);
    if (ret != 0) {
        printf("ERROR: autox update upload isp bootloader %s failed.\n",
                update_start_arg->file_name);
        fclose(fp);
        return ret;
    }
    fclose(fp);

    // step 2: check the file md5,the md5 of x86 is need equal to arm
    ret = get_md5_from_arm_file(dev_fd, &common_query_cmd_data, update_start_arg,
                                x86_file_md5, arm_file_md5);
    if (ret != 0) {
        printf("ERROR: check md5 failed.\n");
        return ret;
    }
    csv_set_step_upload(&isp_csv_data, arm_file_md5);

    // step 3: active the update
    ret = autox_isp_update_active_file(dev_fd, &common_query_cmd_data,
                                       update_start_arg);
    if (ret != 0) {
        printf("ERROR: <%s> active file failed.\n", __func__);
        goto isp_bootloader_update_end;
    }

    // cost time
    gettimeofday(&tvafter,NULL);
    cost_time = (tvafter.tv_sec - tvpre.tv_sec) + (tvafter.tv_usec - tvpre.tv_usec) / 1000000;
    printf("isp bootloader cost time = %ds\n", cost_time);
    csv_set_isp_bootloader_result(&isp_csv_data, cost_time, ISP_UPDATE_RESULT_PASS);

    printf("\n");
    printf("\n");
    printf("*************************************************************\n");
    printf("**********     *PASS*  ISP BootLoader UPDATE!!!    **********\n");
    printf("**********                                         **********\n");
    printf("**********         Please DO Power-cycle!!!        **********\n");
    printf("*************************************************************\n");
    printf("\n");
    printf("\n");

isp_bootloader_update_end:
    write_isp_csv_data(&isp_csv_data);

    return ret;
}

/**
 * @brief update isp firmware
 *        step 0: query the isp state,Judge whether it can be upgraded
 *        step 1: transfer the rom file from x86 to arm
 *        step 2: check the rom md5,the md5 value of x86 is need equal to the md5 of arm
 *        step 3: active the update
 * @param dev_fd
 * @param update_start_arg
 */
static int autox_isp_update_firmware(int dev_fd, autox_update_start_args_t *update_start_arg)
{
    int ret = 0;
    char x86_file_md5[AUTOX_MD5_STR_LEN + 1] = {'\0'};
    FILE *fp = NULL;
    char arm_file_md5[AUTOX_MD5_STR_LEN + 1] = {'\0'};
    autox_common_send_cmd_t common_query_cmd_data = {'\0'};
    IspCsvData isp_csv_data = {'\0'};
    struct timeval tvpre;
    struct timeval tvafter;
    int cost_time = 0;
    char isp_before_version_buf[CSV_ITEM_LARGE_LEN] = {'\0'};
    char isp_after_version_buf[CSV_ITEM_LARGE_LEN] = {'\0'};
    int camera_state = -1;

    if (update_start_arg->subtype != ISP_UPDATE_SUBTYPE_FIRMWARE) {
        printf("error:autox_isp_firmware_update_start:subtype=%d is invalid with '-s' option\n",
                update_start_arg->subtype);
        return -EINVAL;
    }
    gettimeofday(&tvpre, NULL);

    // get isp version before upgrade and judge the sensor name
    ret = get_isp_version(dev_fd, &common_query_cmd_data, isp_before_version_buf,
                          update_start_arg);
    if (update_start_arg->print_flag == 1) {
        return ret;
    }
    if (ret == -EPERM) {
        printf("\n\n-------------------------------\n");
        printf("ERROR:Operation not permitted: the sensor name not support to update\n");
        printf("-------------------------------\n");
        return ret;
    } else {
        /**
         * @brief query failed no need to return, unless ret == -EPERM
         * because after updated bootloader then update isp rom, in this case,
         * not support to query version
         */
        printf("\n\n-------------------------------\n");
        printf("WARNNING: <%s> get version failed before update.The error can be ignore!ret=%d\n",
                 __func__, ret);
        printf("-------------------------------\n");
    }
    // step 0: query the isp state,Judge whether it can be upgraded
    camera_state = get_isp_state(dev_fd, &common_query_cmd_data, update_start_arg);
    if (camera_state < 0 || camera_state > ISP_SYSTEM_FAILSAFE) {
        printf("\n\n-------------------------------\n");
        printf("ERROR: camera_state = %d, not support update isp rom!\n"
                "      please try to update bootloader!\n", camera_state);
        printf("-------------------------------\n");
        return -EINVAL;
    }
    ret = calculateMd5Value(update_start_arg->file_name, x86_file_md5);
    if (ret != 0) {
        printf("ERROR:autox_isp_firmware_update_start:fail to calculate the x86_file_md5\n");
        return ret;
    }
    csv_set_base_info(&isp_csv_data, update_start_arg->board_no, update_start_arg->mipi_id,
                      update_start_arg->file_name, x86_file_md5);

    // step 1: transfer the rom file from x86 to arm
    fp = autox_update_open_file(update_start_arg->file_name);
    if (fp == NULL) {
        printf("ERROR: autox isp update can not open file %s.\n", update_start_arg->file_name);
        return -EIO;
    }
    ret = autox_custom_file_update_upload(dev_fd, fp, ISP_FIRMWARE_NAME_TO_ARM, update_start_arg);
    if (ret != 0) {
        printf("ERROR: autox update upload isp %s failed.\n", update_start_arg->file_name);
        fclose(fp);
        return ret;
    }
    fclose(fp);

    // step 2: check the rom md5,the md5 value of x86 is need equal to the md5 of arm
    ret = get_md5_from_arm_file(dev_fd, &common_query_cmd_data, update_start_arg,
                                x86_file_md5, arm_file_md5);
    if (ret != 0) {
        printf("ERROR: check md5 failed.\n");
        return ret;
    }
    csv_set_step_upload(&isp_csv_data, arm_file_md5);

    // step 3: active the update
    ret = autox_isp_update_active_file(dev_fd, &common_query_cmd_data, update_start_arg);
    if (ret != 0) {
        printf("ERROR: <%s> active file failed.\n", __func__);
        goto isp_firmware_update_end;
    }

    // get isp version after upgrade
    ret = get_isp_version(dev_fd, &common_query_cmd_data, isp_after_version_buf, update_start_arg);
    if (ret != 0) {
        printf("ERROR: <%s> get version failed, ret %d.\n", __func__, ret);
        goto isp_firmware_update_end;
    }

    // cost time
    gettimeofday(&tvafter, NULL);
    cost_time = (tvafter.tv_sec - tvpre.tv_sec) + (tvafter.tv_usec - tvpre.tv_usec) / 1000000;
    printf("isp rom update cost time = %ds\n", cost_time);
    csv_set_isp_firmware_result(&isp_csv_data, isp_before_version_buf, isp_after_version_buf,
                                cost_time, ISP_UPDATE_RESULT_PASS);

    printf("\n");
    printf("\n");
    printf("*************************************************************\n");
    printf("**********      *PASS* ISP Firmware UPDATE!!!      **********\n");
    printf("**********                                         **********\n");
    printf("***After update all cameras! Then please DO Power-cycle!!!***\n");
    printf("*************************************************************\n");
    printf("\n");
    printf("\n");

isp_firmware_update_end:
    write_isp_csv_data(&isp_csv_data);

    return ret;
}

int autox_isp_update_start(int dev_fd, autox_update_start_args_t *update_start_arg)
{
    int ret = 0;
    char *dev_name = update_start_arg->dev_name;
    // check parameters
    if (update_start_arg == NULL || update_start_arg->update_type != AUTOX_ISP_UPDATE) {
        printf("\n\n-------------------------------\n");
        printf("ERROR:autox_isp_update_start:input update_start_arg is invalid\n");
        printf("-------------------------------\n");
        return -EINVAL;
    }
    if (update_start_arg->camera_type == -1 || update_start_arg->camera_type >= AUTOX_CAM_TYPE_MAX)
    {
        printf("error:autox_isp_update_start:camera_type = %d is invalid with '-t' option.\n",
                update_start_arg->camera_type);
        return -EINVAL;
    }
    if (update_start_arg->mipi_id == -1) {
        printf("error:autox_isp_update_startmipi_id = %d is invalid with '-i' option.\n",
                update_start_arg->mipi_id);
        return -EINVAL;
    }
    update_start_arg->board_no = dev_name[strlen(dev_name) - 1] - '0';

    // begin to update
    switch(update_start_arg->subtype) {
        case ISP_UPDATE_SUBTYPE_FIRMWARE : {
            ret = autox_isp_update_firmware(dev_fd, update_start_arg);
            break;
        }
        case ISP_UPDATE_SUBTYPE_BOOTLOADER : {
            ret = autox_isp_update_bootloader(dev_fd, update_start_arg);
            break;
        }
        default: {
            printf("\n\n-------------------------------\n");
            printf("ERROR:autox_isp_update_start:update_start_arg->subtype=%d is invalid\n",
                   update_start_arg->subtype);
            printf("-------------------------------\n");
            ret  = -EINVAL;
            break;
        }
    }

    return ret;
}
