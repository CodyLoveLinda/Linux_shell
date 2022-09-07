#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/io.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "write_isp_csv.h"

FILE *fp = NULL;

static void write_header(void);

void open_csv(void)
{
    int flag = CSV_NON_EXIST;
    // check the csv file is exist or not
    fp = fopen(CSV_NAME, "a+");
    if (fp == NULL) {
        printf("open_csv:Fail to open the csv file\n");
    }

    fseek(fp, 0, SEEK_END);
    int fileSize = ftell(fp);
    flag = (fileSize == 0) ? CSV_NON_EXIST : CSV_EXIST;

    // write csv Header
    if (flag == CSV_NON_EXIST) {
        write_header();
    }
}

static void write_header(void)
{
    int i;
    char head_strs[][CSV_ITEM_LARGE_LEN] = {"Board_No", "Mipi_Id", "Version_Before_Upgrade",
    "Rom_Name", "Rom_Md5_X86", "Version_After_Upgrade", "Rom_Md5_Arm", "Cost_Time(s)", "Date_Time",
    "Result"};

    for (i = 0; i < ISP_CSV_ITEMS; i++) {
        (void)fwrite(head_strs[i], 1, strlen(head_strs[i]), fp);
        (void)fwrite(SEPARATOR_ITEM, 1, strlen(SEPARATOR_ITEM), fp);
    }
    (void)fwrite(SEPARATOR_LINE, 1, strlen(SEPARATOR_LINE), fp);
}

void csv_set_base_info(IspCsvData *isp_csv_data, int board_no, int mipi_id, const char *file_name,
                       const char *file_md5_x86)
{
    time_t now;
    struct tm *ptm;
    time(&now);
    ptm = localtime(&now);

    sprintf(isp_csv_data->board_no, "%d", board_no);  // board_no
    sprintf(isp_csv_data->mipi_id, "%d", mipi_id);  // mipi_id
    memcpy(isp_csv_data->rom_name, file_name, strlen(file_name) + 1);// rom name
    memcpy(isp_csv_data->rom_md5_x86, file_md5_x86, strlen(file_md5_x86) + 1);// rom md5 x86
    sprintf((char *)(isp_csv_data->date_time), "%s", asctime(ptm));  // update date time
    // the default result is Fail
    memcpy(isp_csv_data->result, ISP_UPDATE_RESULT_FAIL, strlen(ISP_UPDATE_RESULT_FAIL) + 1);
}

void csv_set_step_upload(IspCsvData *isp_csv_data, const char *file_md5_arm)
{
    memcpy(isp_csv_data->rom_md5_arm, file_md5_arm, strlen(file_md5_arm) + 1);
}

void csv_set_isp_firmware_result(IspCsvData *isp_csv_data, const char *before_ver,
                 const char *after_ver, int cost_time, const char *result)
{
    memcpy(isp_csv_data->before_update_version, before_ver, strlen(before_ver) + 1);
    memcpy(isp_csv_data->after_update_version, after_ver, strlen(after_ver) + 1);
    sprintf((char *)(isp_csv_data->cost_time), "%d", cost_time);
    memcpy(isp_csv_data->result, result, strlen(result) + 1);
}

void csv_set_isp_bootloader_result(IspCsvData *isp_csv_data, int cost_time, const char *result)
{
    memcpy(isp_csv_data->before_update_version, BOOTLOAD_NOT_SUPPORT_VERSION,
            strlen(BOOTLOAD_NOT_SUPPORT_VERSION) + 1);
    memcpy(isp_csv_data->after_update_version, BOOTLOAD_NOT_SUPPORT_VERSION,
            strlen(BOOTLOAD_NOT_SUPPORT_VERSION) + 1);
    sprintf((char *)(isp_csv_data->cost_time), "%d", cost_time);
    memcpy(isp_csv_data->result, result, strlen(result) + 1);
}

void write_isp_csv_data(const IspCsvData *isp_csv_data)
{
    // open csv
    open_csv();
    // board_no
    (void)fwrite(&isp_csv_data->board_no, 1, strlen(isp_csv_data->board_no), fp);
    (void)fwrite(SEPARATOR_ITEM, 1, strlen(SEPARATOR_ITEM), fp);

    // mipi_id
    (void)fwrite(&isp_csv_data->mipi_id, 1, strlen(isp_csv_data->mipi_id), fp);
    (void)fwrite(SEPARATOR_ITEM, 1, strlen(SEPARATOR_ITEM), fp);

    // Version_Before_Upgrade
    (void)fwrite(isp_csv_data->before_update_version, 1,
                strlen(isp_csv_data->before_update_version), fp);
    (void)fwrite(SEPARATOR_ITEM, 1, strlen(SEPARATOR_ITEM), fp);

    // Rom_name
    (void)fwrite(isp_csv_data->rom_name, 1, strlen(isp_csv_data->rom_name), fp);
    (void)fwrite(SEPARATOR_ITEM, 1, strlen(SEPARATOR_ITEM), fp);

    // Rom_Md5_X86
    (void)fwrite(isp_csv_data->rom_md5_x86, 1, strlen(isp_csv_data->rom_md5_x86), fp);
    (void)fwrite(SEPARATOR_ITEM, 1, strlen(SEPARATOR_ITEM), fp);

    // Version_After_Upgrade
    (void)fwrite(isp_csv_data->after_update_version, 1,
                strlen(isp_csv_data->after_update_version), fp);
    (void)fwrite(SEPARATOR_ITEM, 1, strlen(SEPARATOR_ITEM), fp);

    // Rom_Md5_Arm
    (void)fwrite(isp_csv_data->rom_md5_arm, 1, strlen(isp_csv_data->rom_md5_arm), fp);
    (void)fwrite(SEPARATOR_ITEM, 1, strlen(SEPARATOR_ITEM), fp);

    // Cost_Time
    (void)fwrite(isp_csv_data->cost_time, 1, strlen(isp_csv_data->cost_time), fp);
    (void)fwrite(SEPARATOR_ITEM, 1, strlen(SEPARATOR_ITEM), fp);

    // Date_Time
    (void)fwrite(isp_csv_data->date_time, 1,
                strlen(isp_csv_data->date_time) - 1, fp);  // delete the \n
    (void)fwrite(SEPARATOR_ITEM, 1, strlen(SEPARATOR_ITEM), fp);

    // Result
    (void)fwrite(isp_csv_data->result, 1, strlen(isp_csv_data->result), fp);
    (void)fwrite(SEPARATOR_ITEM, 1, strlen(SEPARATOR_ITEM), fp);

    (void)fwrite(SEPARATOR_LINE, 1, strlen(SEPARATOR_LINE), fp);

    fclose(fp);
}

