#ifndef __AUTOX_WRITE_ISP_CSV_H__
#define __AUTOX_WRITE_ISP_CSV_H__

#include <stdio.h>

#define CSV_NAME "IspUpdateLog.csv"
#define CSV_EXIST 0
#define CSV_NON_EXIST 1
#define CSV_ITEM_LARGE_LEN 1024
#define CSV_ITEM_MIDDLE_LEN 256
#define CSV_ITEM_SMALL_LEN 8
#define ISP_CSV_ITEMS 10
#define SEPARATOR_ITEM ","
#define SEPARATOR_LINE "\n"
#define ISP_UPDATE_RESULT_FAIL "Fail"
#define ISP_UPDATE_RESULT_PASS "Pass"
#define BOOTLOAD_NOT_SUPPORT_VERSION "bootloader not support get version"

typedef struct {
    char board_no[CSV_ITEM_SMALL_LEN];
    char mipi_id[CSV_ITEM_SMALL_LEN];
    char before_update_version[CSV_ITEM_MIDDLE_LEN];
    char rom_name[CSV_ITEM_LARGE_LEN];
    char rom_md5_x86[CSV_ITEM_MIDDLE_LEN];
    char after_update_version[CSV_ITEM_MIDDLE_LEN];
    char rom_md5_arm[CSV_ITEM_MIDDLE_LEN];
    char cost_time[CSV_ITEM_MIDDLE_LEN];
    char date_time[CSV_ITEM_MIDDLE_LEN];
    char result[CSV_ITEM_MIDDLE_LEN];
} IspCsvData __attribute__ ((aligned (1)));

void csv_set_base_info(IspCsvData *isp_csv_data, int board_no, int mipi_id, const char *file_name,
                       const char *file_md5_x86);
void csv_set_step_upload(IspCsvData *isp_csv_data, const char *file_md5_arm);
void csv_set_isp_firmware_result(IspCsvData *isp_csv_data, const char *before_ver,
                 const char *after_ver, int cost_time, const char *result);
void csv_set_isp_bootloader_result(IspCsvData *isp_csv_data, int cost_time, const char *result);
void write_isp_csv_data(const IspCsvData *isp_csv_data);

#endif
