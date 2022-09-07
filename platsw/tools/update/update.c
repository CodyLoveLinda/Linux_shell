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
#include "update.h"

#define VERSION         "1.2"

typedef struct autox_update_ops {
    autox_update_type_t type;
    int (* start)(int dev_fd, autox_update_start_args_t *update_start_arg);
} autox_update_ops_t;

autox_update_ops_t update_ops_arrays[] = {
    {AUTOX_FW_UPDATE, autox_fw_update_start},
    {AUTOX_CONFIGURATION_UPDATE, autox_config_update_start},
    {AUTOX_ISP_UPDATE, autox_isp_update_start},
    {AUTOX_CUSTOM_FILE_UPDATE, autox_custom_file_update_start}
};

static autox_update_ops_t *autox_get_update_ops(autox_update_type_t type)
{
    int i;
    int size = sizeof(update_ops_arrays) / sizeof(autox_update_ops_t);

    for (i = 0; i < size; i++) {
        if (type == update_ops_arrays[i].type) {
            return &update_ops_arrays[i];
        }
    }

    printf("ERROR: autox update don't support type %u.\n", type);
    return NULL;
}

void init_update_start_arg(autox_update_start_args_t *update_start_arg)
{
    memset(update_start_arg, 0, sizeof(autox_update_start_args_t));
    update_start_arg->dev_name[0] = '\0';
    update_start_arg->file_name[0] = '\0';
    update_start_arg->update_type = AUTOX_FW_UPDATE; // default firware update
    update_start_arg->subtype = -1;
    update_start_arg->mipi_id = -1;
    update_start_arg->board_no = -1;
    update_start_arg->camera_type = -1;
}

int main(int argc, char * const argv[])
{
    int dev_fd;
    int opt;
    int ret = 0;
    FILE *fp = NULL;
    int file_needed = 1;
    autox_update_start_args_t update_start_arg;
    autox_update_ops_t *update_ops = NULL;

    init_update_start_arg(&update_start_arg);
    while ((opt = getopt(argc, argv, "m:s:d:f:i:t:phv")) != -1) {
        switch (opt) {
        case 'd':
            strncpy(update_start_arg.dev_name, optarg, sizeof(update_start_arg.dev_name));
            break;
        case 'f':
            strncpy(update_start_arg.file_name, optarg, sizeof(update_start_arg.file_name));
            break;;
        case 'p': /* print out the current firmware version */
            file_needed = 0;
            update_start_arg.print_flag = 1;
            break;
        case 'h':
            printf("Usage:\n");
            printf("  %s [-m 0] -d </dev/autox_sysm0> -f </path/to/fw/file> "
                "  :update CaSH Board Image\n", argv[0]);
            printf("  %s -m 1 -d </dev/autox_sysm0> -f </path/config_file> "
                "  :update CaSH Board Configuration\n", argv[0]);
            printf("  %s -m 2 -d </dev/autox_sysm0> -f </path/custom_file.bin>"
                "  :upload custom file\n", argv[0]);
            printf("  %s -m 3 -s <subtype> -d </dev/autox_sysm0> -f <isp_file> -t <camera_type> -i <mipi_id> \n"
                "    subtype=0(isp_firmware)/1(isp_bootloader)...,\n"
                "    mipi_id=0/1/2..,\n    camera_type=(0->0x8B40)/..\n\n"
                "    such as:./x_update -m 3 -s 0 -d /dev/autox_sysm0 -f isp.rom -t 0 -i 0 \n"
                "    update ISP rom for mipi0 ,camera type = 0x8B40\n"
                "    such as:./x_update -m 3 -s 1 -d /dev/autox_sysm0 -f failsafefw.bin -t 0 -i 0 \n"
                "    update ISP bootloader for mipi0 ,camera type = 0x8B40n\n", argv[0]);
            printf("  %s -d </dev/autox_sysm0> -p                    "
                "  :print CaSH Firmware Version\n", argv[0]);
            printf("  %s -m 1 -d </dev/autox_sysm0> -p                    "				
                "  :print Configuration File MD5 Value\n", argv[0]);
            printf("  %s -m 3 -s 0 -d </dev/autox_sysm0> -t 0 -i 0  -p"
                "  :print Isp Version of mipi 0\n", argv[0]);
            return 0;
        case 'v':
            printf("software version: %s\n", VERSION);
            return 0;
        case 'm':
            update_start_arg.update_type = atoi(optarg);
            break;
        case 's':
            update_start_arg.subtype = atoi(optarg);
            break;
        case 'i':
            update_start_arg.mipi_id = atoi(optarg);
            break;
        case 't':
            update_start_arg.camera_type = atoi(optarg);
            break;
        default:
            printf("%s: bad parameter\n", argv[0]);
            return -EINVAL;
        }
    }

    // check parameters
    if (update_start_arg.dev_name[0] == '\0') {
        printf("error: need to provide a autox card device with "
            "'-d' option\n");
        return -EINVAL;
    }

    if (file_needed && update_start_arg.file_name[0] == '\0') {
        printf("error: need to provide the new file with '-f' option.\n");
        return -EINVAL;
    }

    dev_fd = open(update_start_arg.dev_name, O_RDWR);
    if (dev_fd < 0) {
        printf("error %d opening '%s'!\n", dev_fd, update_start_arg.dev_name);
        return -EIO;
    }
    printf("successfully opened AutoX card device %s\n",update_start_arg.dev_name);

    update_ops = autox_get_update_ops(update_start_arg.update_type);

    if (update_ops != NULL && update_ops->start != NULL) {
        ret = update_ops->start(dev_fd, &update_start_arg);
    } else {
        ret = -EFAULT;
    }

    close(dev_fd);
    return ret;
}
