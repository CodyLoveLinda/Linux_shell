#include <stdint.h>
#include "../../include/linux/cash3_fpga_events.h"
#include "../../include/linux/cash3_host_cmds.h"
#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << argv[0] << " <qdma queue device>" << std::endl;
		std::cerr << "for example: " << argv[0] << " /dev/qdma03000-MM-8" << std::endl;
	}
	char *devname = argv[1];


	int fpga_fd = open(devname, O_RDWR | O_NONBLOCK);

	if (fpga_fd < 0) {
		std::cerr << "cannot open device " << devname << std::endl;
		exit(fpga_fd);
	}
	char buf[CASH3_FPGA_EVENT_SIZE] = {0};
	size_t count = 10;

	//prepare a cmd
	cash3_host_cmd_t cmd;
	memset(&cmd, 0, sizeof(cmd));
	cmd.head.cmd = CASH3_HOST_CMD_TRIGGER_DUMMY_EVENT;

	auto payload = (cash3_host_cmd_trigger_dummy_event_t*)cmd.payload;
	payload->backReference = 0xdeadbeefbeefdead;
	while(count--) {
		//write the cmd into FPGA	
		if (write(fpga_fd, &cmd, sizeof(cmd))) { //return 0 if success
			std::cerr << "IO error" << std::endl;
			exit(errno);
		}

		int rc = read(fpga_fd, buf, sizeof(buf));
		if (rc > 0) {
			auto event = reinterpret_cast<cash3_fpga_event_t*>(buf);

			switch (event->head.event) {
				case CASH3_FPGA_EVENT_VIDEO_STATUS_CHANGED: {
					std::cout << "CASH3_FPGA_EVENT_VIDEO_STATUS_CHANGED" << std::endl;
				} break;
				case CASH3_FPGA_EVENT_IMAGE_FLASHED: {
					std::cout << "CASH3_FPGA_EVENT_IMAGE_FLASHED" << std::endl;
				} break;
				case CASH3_FPGA_EVENT_DUMMY: {
					std::cout << "CASH3_FPGA_EVENT_DUMMY" << std::endl;
					auto dummy = (cash3_fpga_event_dummy_t*)event->payload;
					if (dummy->backReference != 0xdeadbeefbeefdead) {
						std::cerr << "CASH3_FPGA_EVENT_DUMMY backReference unexpected" << std::endl;
					}
				} break;
				default: {
					std::cout << "other event =" << event->head.event << std::endl;
				}
			}
		} else {
			std::cerr << "IO Error at " << devname << std::endl;
			exit(rc);
		}
	}
}

