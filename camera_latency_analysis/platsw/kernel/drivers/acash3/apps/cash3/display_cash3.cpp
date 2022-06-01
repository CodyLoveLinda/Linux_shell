// usage:
// echo 0 | sudo tee /sys/bus/pci/devices/0000\:03\:00.0/qdma/test_case #do this once to initialize cash3
// sudo display_cash3 /dev/qdma03000-MM-1
// to build:
// g++ tools/display_cash3.cpp -o tools/display_cash3 `pkg-config --cflags --libs opencv` 2>&1|less
#include <stdint.h>
#include "../../include/linux/cash3_meta.h"
// #include <opencv2/core/core.hpp>
// #include <opencv2/highgui/highgui.hpp>
// #include <opencv2/imgproc/imgproc.hpp>
#include <vector>
#include <iostream>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include <zlib.h>

int main() {
	auto fpga_fd = open("/dev/qdma01000-MM-1", O_RDWR | O_NONBLOCK);
	auto readSize = 6*1024*1024;
	char* buf = (char*)aligned_alloc(4096, readSize);
	auto rc = read(fpga_fd, buf, readSize);
	auto last_word = (uint32_t*)(buf + 6 * 1024 * 1024) - 1;
	auto last_v_word = (uint32_t*)(buf + 4096 + 1920 * 1080 * 3) - 1;
	auto meta = (cash3_video_meta_data_t*)buf;
	if (rc == readSize) {
	  	// auto my_crc = crc32(0x80000000,  (unsigned char*)(buf + 4 * 1024), 32); //1024 * 1024);

		auto index = meta->mipi_frame_idx;		
		std::cout << "rc=" << rc << '\t' << " : index=" << index 
			<< " resp=" << meta->resp
			<< " req=" << meta->req
			<< " curr_ts_sec=" << meta->curr_ts_sec
			<< " curr_ts_us=" <<  meta->curr_ts_us
			<< " prev_ts_sec=" << meta->prev_ts_sec
			<< " prev_ts_us=" <<  meta->prev_ts_us
			<< std::hex
			<< " last_v_word="  << *last_v_word
			<< " last_word="  << *last_word
			<< std::dec
			<< std::endl;
	}
	free(buf);
}

// int main( int argc, char** argv ) {
// 	using namespace std;
// 	// using namespace cv;

// 	// uint32_t crc_buf[] = {0x1234, 0x5678, 0x9abc, 0xdef0, 0x1234, 0x5678, 0xbeef, 0xdead};
// 	// // std::cout << "my_crc=" << std::hex << crc32(0x80000000,  (unsigned char*)(crc_buf), 32) << std::endl;
// 	// std::cout << "my_crc=" << std::hex << (crc32(0x80000000 ^ 0xffffffff,  (unsigned char*)(crc_buf), 32) ^ 0xffffffff) << std::endl;
// 	// exit(0);
// 	if (argc != 3) {
// 		std::cerr << argv[0] << "<qdma queue device> <frame_count>" << std::endl;
// 		std::cerr << "echo 0 | sudo tee /sys/bus/pci/devices/0000\\:03\\:00.0/qdma/test_case #do this once to initialize cash3" << std::endl;
//         std::cerr << "sudo tools/display_cash3 /dev/qdma03000-MM-1" << std::endl;
//         std::cerr << "ctrl-c to exit early" << std::endl;
//         exit(-1);
// 	}
// 	auto devname = argv[1];
// 	size_t readSize = 6 * 1024 * 1024;
// 	size_t frame_count = atoi(argv[2]);

// 	auto fpga_fd = open(devname, O_RDWR | O_NONBLOCK);

// 	if (fpga_fd < 0) {
// 		std::cerr << "cannot open device(s) " << devname << std::endl;
// 		exit(-1);
// 	}
// 	std::vector<char*> bufs;
// 	for (auto i = 0; i < 20; i++) {
// 		bufs.push_back(new char [readSize]);
// 	}
//     // cv::namedWindow(devname, cv::WINDOW_AUTOSIZE);
//     int acc_frame_count = 0;
//     double display_fps = 0;


//     timeval start_tv;
//     gettimeofday(&start_tv, NULL);
//     uint32_t pre_index = 0;
// 	int      waitKeyMs = 1;

//     std::cout << "starting " << frame_count << " frames on " << devname << " bufs.size()=" << bufs.size() << std::endl;
//     bool paused = false;
// 	while (frame_count--) {
// 		auto buf = bufs[frame_count % bufs.size()];
// 		auto meta = (cash3_video_meta_data_t*)buf;
// 		int rc = paused?readSize:read(fpga_fd, buf, readSize);
// 		if (rc == readSize) {
// 		  	// auto my_crc = crc32(0x80000000,  (unsigned char*)(buf + 4 * 1024), 32); //1024 * 1024);

// 			auto index = meta->mipi_frame_idx;
// 			struct timeval tv;
// 			gettimeofday(&tv, NULL);
// 			auto last_word = (uint32_t*)(buf + 6 * 1024 * 1024) - 1;
// 			auto last_v_word = (uint32_t*)(buf + 4096 + 1920 * 1080 * 3) - 1;
// 			std::cout << devname << '\t' << tv.tv_sec << ':' << tv.tv_usec << " : index=" << index 
// 				<< " resp=" << meta->resp
// 				<< " req=" << meta->req
// 				<< " curr_ts_sec=" << meta->curr_ts_sec
// 				<< " curr_ts_us=" <<  meta->curr_ts_us
// 				<< " prev_ts_sec=" << meta->prev_ts_sec
// 				<< " prev_ts_us=" <<  meta->prev_ts_us
// 				<< std::hex
// 				<< " last_v_word="  << *last_v_word
// 				<< " last_word="  << *last_word
// 				<< std::dec
// 				<< std::endl;

// 		    // auto my_crc = crc32(0x80000000 ^ 0xffffffff, (unsigned char*)(buf + 4 * 1024), 1024 * 1024) ^ 0xffffffff;
// 			++acc_frame_count;
// 		    timeval now_tv;
// 		    gettimeofday(&now_tv, NULL);
// 		    auto usec = (now_tv.tv_sec - start_tv.tv_sec) * 1000000 + now_tv.tv_usec - start_tv.tv_usec;
// 		    if (usec >= 1000000) {
// 		    	display_fps = (double)acc_frame_count * 1000000.0 / usec;
// 		    	acc_frame_count = 0;
// 		    	start_tv = now_tv;
// 		    }

// 	  //   	cv::Mat frame(1080, 1920, CV_8UC3, buf + 4 * 1024);
// 			// if (!paused) cv::cvtColor(frame, frame, CV_RGB2BGR);
// 			// cv::putText(frame, std::to_string(display_fps),
//    //          	cv::Point(50,50), // Coordinates
//    //          	cv::FONT_HERSHEY_COMPLEX_SMALL, // Font
//    //          	1.0, // Scale. 2.0 = 2x bigger
//    //          	cv::Scalar(255,255,255), // BGR Color
//    //          	1 // Line Thickness (Optional)
//    //          	); // Anti-alias (Optional)

// 			// imshow(devname, frame);
// 			// char c=(char)waitKey(paused?1000 * waitKeyMs:waitKeyMs);
// 		    // if(c==27)
// 		    //   break;
// 		    // else if (c=='+') paused=true;
// 		    // else if (c=='-') paused=false;
// 		  // auto crc = meta->crc_video;
// 		  // if (!paused && crc != my_crc) {
// 		  // 	std::cerr << "crc error detected crc!=my_crc:" << std::hex << crc << "!=" << my_crc << std::dec << std::endl;
// 		  // 	std::string l;
// 		  // 	// std::getline(std::cin, l);
// 		  // }
// 		  if (!paused && (index == pre_index || (256 + index - pre_index) % 256 > 128)) {
// 		  	std::cerr << "out of seq detected! " << std::endl;
// 		  	std::string l;
// 		  	std::getline(std::cin, l);
// 		  }
// 		  pre_index = index;
// 		} else {
// 			std::cerr << "rc = " << rc << std::endl;
// 		}
// 	}
// 	// delete [] buf;
//   	return 0;
// }

