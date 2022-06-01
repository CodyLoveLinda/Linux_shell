/* 
 * Copyright (C) 2019 AutoX, Inc.
 */

#include <errno.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <pci/pci.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "linux/autox_api.h"

#define VERSION			"1.1"
#define VENDOR_ID_XILINX	0x10ee
#define DEVICE_ID_CASH1_1	0x9034
#define DEVICE_ID_CASH1_2	0x9038
#define FW_TAHOE			0
#define FW_CASH1_1			1
#define FW_CASH1_2			2

static int autox_fw_update(int dev_fd, FILE *img_fp, int print_ver)
{
	autox_fw_update_t *ioc_fw_arg;
	autox_fw_fpga_ver_t fw_fpga_ver;
	int read_bytes = 0;
	int cnt = 0;

	if (print_ver) {
		if (ioctl(dev_fd, AUTOX_IOC_SYSM_VER, &fw_fpga_ver)) {
			printf("failed to do ioctl AUTOX_IOC_FW_GET_VER\n");
			return -1;
		}
		printf("-------------------------------\n");
		printf("CaSH Firmware Version:	%d\n", fw_fpga_ver.fw_ver);
		printf("CaSH FPGA Version:	%d.%d\n", \
			fw_fpga_ver.hw_major_ver, fw_fpga_ver.hw_minor_ver);
		printf("-------------------------------\n");			
		return 0;
	}

	if (img_fp == NULL) {
		printf("no image file or invalid update parameter\n");
        return -1;
	}

	ioc_fw_arg = malloc(sizeof(autox_fw_update_t));
	if (ioc_fw_arg == NULL) {
		printf("failed malloc(autox_fw_update_t)\n");
        return -1;
	}

	printf("uploading the firmware image\n");

	read_bytes = fread(ioc_fw_arg->fw_data, 1, AUTOX_FW_MAX_SIZE, img_fp);
	if (read_bytes > 0 && read_bytes <= AUTOX_FW_MAX_SIZE) {
		ioc_fw_arg->fw_data_size = read_bytes;
		if (ioctl(dev_fd, AUTOX_IOC_SYSM_UPDATE, ioc_fw_arg)) {
			printf("failed to do ioctl AUTOX_IOC_FW_IMAGE_UPLOAD.\n");
			free(ioc_fw_arg);
			return -1;
		}
	}
	printf("End: total_bytes=%d\n", read_bytes);

	printf("\n");
	printf("\n");
	printf("*************************************************************\n");
	printf("**********          START FW UPDATE!!!             **********\n");
	printf("**********         Please Wait for 1 min!!!        **********\n");
	printf("*************************************************************\n");
	printf("\n");
	printf("\n");
	while (cnt < 60)
	{
		printf("cnt (%d)\n", cnt);
		if (ioctl(dev_fd, AUTOX_IOC_SYSM_CMDSTS, ioc_fw_arg)) {
			sleep(1);
			cnt++;
		}
		else {
			break;
		}
	}
	if (cnt >= 60) {
		printf("\n");
		printf("\n");
		printf("*************************************************************\n");
		printf("**********           FAILED FW UPDATE!!!           **********\n");
		printf("*************************************************************\n");
		printf("\n");
		printf("\n");

		free(ioc_fw_arg);
		return -1;
	}

	printf("\n");

	if (ioctl(dev_fd, AUTOX_IOC_SYSM_ENABLE, ioc_fw_arg)) {
		printf("failed to do ioctl AUTOX_IOC_SYSM_ENABLE.\n");
		free(ioc_fw_arg);
		printf("\n");
		printf("\n");
		printf("*************************************************************\n");
		printf("**********           FAILED FW UPDATE!!!           **********\n");
		printf("**********                                         **********\n");
		printf("**********     Please check the upgrade file!!!    **********\n");
		printf("*************************************************************\n");
		printf("\n");
		printf("\n");
		return -1;
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

	free(ioc_fw_arg);
	return 0;
}

int get_cash_fw_ver(char *autox_fw_file) {
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

int match_cash_ver(char *autox_fw_file) {
	struct pci_access *acc = NULL;
	struct pci_dev *dev = NULL;
	int cash_match = 0;
	int dev_found = 0;
	int cash_fw_ver = -1;

	cash_fw_ver = get_cash_fw_ver(autox_fw_file);
	if (cash_fw_ver == -1) {
		printf("error: invalid cash firmware.\n");
		return -1;
	}

	acc = pci_alloc();
	pci_init(acc);
	pci_scan_bus(acc);
	for (dev=acc->devices; dev; dev=dev->next) {
		if (dev->vendor_id == VENDOR_ID_XILINX) {
			dev_found = 1;
			break;
		}
	}

	if (dev_found != 0) {
		/* Tahoe has the same device id as CaSH1.2 */
		if ((dev->device_id == DEVICE_ID_CASH1_2) &&
			((cash_fw_ver == FW_CASH1_2) || (cash_fw_ver == FW_TAHOE))) {
			cash_match = 1;
		} else if ((dev->device_id == DEVICE_ID_CASH1_1) &&
				   (cash_fw_ver == FW_CASH1_1)) {
			cash_match = 1;
		}
	} else {
		printf("error: cash device not found.\n");
		pci_cleanup(acc);
		return -1;
	}

	if (cash_match == 0) {
		printf("error: cash device id: 0x%x, cash firmware:%s.\n",
			   dev->device_id, autox_fw_file);
		pci_cleanup(acc);
		return -1;
	}

	pci_cleanup(acc);

	return 0;
}

int main(int argc, char * const argv[])
{
	int dev_fd, opt, ret;
	FILE *img_fp = NULL;
	int img_file_needed = 1, print_ver = 0;
	char autox_fw_dev[32];
	char autox_fw_file[128];
	char autox_file_tar_gz[256];
	int integrity_status = 0;

	autox_fw_dev[0] = '\0';
	autox_fw_file[0] = '\0';
	autox_file_tar_gz[0] = '\0';
	while ((opt = getopt(argc, argv, "d:f:phv")) != -1) {
		switch (opt) {
		case 'd':
			strncpy(autox_fw_dev, optarg, sizeof(autox_fw_dev));
			break;
		case 'f':
			strncpy(autox_fw_file, optarg, sizeof(autox_fw_file));
			break;;
		case 'p': /* print out the current firmware version */
			img_file_needed = 0;
			print_ver = 1;
			break;
		case 'h':
			printf("Usage:\n");
			printf("  %s -d </dev/autox_sysm0> -f </path/to/fw/file> "
			    "  :update CaSH Board Image\n", argv[0]);
			printf("  %s -d </dev/autox_sysm0> -p                    "				
				"  :print CaSH Firmware Version\n", argv[0]);
			return 0;
		case 'v':
			printf("software version: %s\n", VERSION);
			return 0;
		default:
			printf("%s: bad parameter\n", argv[0]);
			return -1;
		}
	}

	if (autox_fw_dev[0] == '\0') {
		printf("error: need to provide a autox card device with "
		    "'-d' option\n");
		return (-1);
	}
	dev_fd = open(autox_fw_dev, O_RDWR);
	if (dev_fd < 0) {
		printf("error %d opening '%s'!\n", dev_fd, autox_fw_dev);
		return (-1);
	}
	printf("successfully opened AutoX card firmware device %s\n",
	   autox_fw_dev);

	if (img_file_needed) {
		if (autox_fw_file[0] == '\0') {
			close(dev_fd);
			printf("error: need to provide the new firmware image "
			    "file with '-f' option.\n");
			return (-1);
		}
		if (match_cash_ver(autox_fw_file) != 0) {
			close(dev_fd);
			printf("error: firmware version is not match with cash device.\n");
			return (-1);
		}
		/* read-only binary file */
		img_fp = fopen(autox_fw_file, "rb");
		if (img_fp == NULL) {
			close(dev_fd);
			printf("failed to open firmware image file %s!\n",
			    autox_fw_file);
			return (-1);
		}
		printf("successfully opened firmware imge file '%s'\n",
			    autox_fw_file);

		strcpy(autox_file_tar_gz, "gunzip -t ");
		strcat(autox_file_tar_gz, autox_fw_file);
		integrity_status = system(autox_file_tar_gz);
		if (integrity_status != 0) {
			close(dev_fd);
			printf("error: corrupted firmware image file%s!\n",
			    autox_fw_file);
			return (-1);
		}
	}

	/* do firmware update */
	ret = autox_fw_update(dev_fd, img_fp, print_ver);

	if (img_fp) {
		fclose(img_fp);
	}

	close(dev_fd);

	return ret;
}
