/*
 * This file contains data structure reported from ARM
 *
 */

#ifndef __CASH3_API_H__
#define __CASH3_API_H__

#define CASH3_API_VER_MAJ 1     /*increase when backward compatibility breaks - which should be avoided*/
#define CASH3_API_VER_MIN 7     /*used for release tracking*/

#ifndef __KERNEL__
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "cash3_host_cmds.h"
#include "cash3_fpga_events.h"
#include "cash3_can_traffic.h"
#else
#include <linux/types.h>
#endif

enum cash3_qdma_cdev_ioctl_cmd_en {
  CASH3_QDMA_CDEV_IOCTL_MANIFEST = 1,
};

struct cash3_qdma_cdev_ioctl_manifest_t {
	uint16_t api_ver_maj;
	uint16_t api_ver_min;
	
	uint32_t pci_devfn;
	uint16_t pci_vendor;
	uint16_t pci_device;
	uint16_t pci_subsystem_vendor;
	uint16_t pci_subsystem_device;
	
	uint16_t fw_ver;             //running firmware version
	uint16_t backup_fw_ver;      //firmware version on file as a backup
	uint16_t hw_major_ver;       //running FPGA (hw) major version
	uint16_t hw_minor_ver;       //running FPGA (hw) minor version
	char reserved[512 - sizeof(uint16_t) * 12];  //512 bytes fixed
} __attribute__((packed));


// #ifndef __KERNEL__
// #include <thread>
// #include <string>
// #include <atomic>
// #include <iostream>
// #include <stdexcept>
// #include <stdlib.h>


// void resync_cash3_event_queue(int fd) {
// 	int rc = 0;
// 	cash3_host_cmd_t cmd;
//     memset(&cmd, 0, sizeof(cmd));
//     cmd.head.cmd = CASH3_HOST_CMD_TRIGGER_DUMMY_EVENT;

//     auto payload
//     	= (struct cash3_host_cmd_trigger_dummy_event_t*)cmd.payload;
//     payload->backReference = rand();

//     std::atomic_bool read_done{false};
//     std::thread read_thrd([&read_done, fd, ref = payload->backReference](){
// 		char buf[CASH3_FPGA_EVENT_SIZE] = {0};
// 		struct cash3_fpga_event_t* event = (cash3_fpga_event_t*)(buf);
// 		while(1) {
// 			auto rc = read(fd, buf, sizeof(buf));
// 			std::cout << rc << std::endl;
// 			if (rc != CASH3_FPGA_EVENT_SIZE) {
// 				read_done = true;
// 				throw (std::runtime_error("read rc=" + std::to_string(rc)));
// 			}
// 			if (event->head.event == CASH3_FPGA_EVENT_DUMMY) {
// 				std::cout << ((cash3_fpga_event_dummy_t*)event->payload)->backReference << std::endl;
// 			}
// 			if (event->head.event == CASH3_FPGA_EVENT_DUMMY
// 				&& ((cash3_fpga_event_dummy_t*)event->payload)->backReference 
// 					== ref) {
// 				break;
// 			}
// 		}
// 		read_done = true;
//     });

//     // do {
// 	    std::cout << "src: " << payload->backReference << std::endl;
//     	usleep(10000);
// 		if ((rc = write(fd, &cmd, sizeof(cmd)))) { //return 0 if success
// 			throw (std::runtime_error("write rc=" + std::to_string(rc)));
// 		}
// 	// } while(!read_done);
// 	read_thrd.join();
// }
// #endif
#endif
