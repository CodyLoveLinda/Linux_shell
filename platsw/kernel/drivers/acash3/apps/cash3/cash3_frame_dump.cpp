#include <stdint.h>
#include "../../include/linux/cash3_fpga_events.h"
#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	if (argc < 2 || argc > 4) {
		std::cerr << argv[0] << "<qdma queue device> <frame count> [frame size in MB(6)]" << std::endl;
	}
	char *devname = argv[1];
	size_t frameCount = atoi(argv[2]);
	size_t readSize = 6 * 1024 * 1024;
	if (argc == 4) {
		readSize = atoi(argv[3]) * 1024 * 1024;
	}

	int fpga_fd = open(devname, O_RDWR | O_NONBLOCK);

	if (fpga_fd < 0) {
		std::cerr << "cannot open device " << devname << std::endl;
		exit(fpga_fd);
	}
	void* buf = new char [readSize];
	while (frameCount--) {
		int rc = read(fpga_fd, buf, readSize);
		if (rc > 0) {
			uint32_t* from = (uint32_t*)buf;
			for (int i = 0; i < 16; ++i) {
				printf("%08x ", from[i]);
			}
			std::cout << "---" << std::endl;
		} else {
			std::cerr << "rc = " << rc << std::endl;
		}
	}
	delete [] (char*)buf;
}
