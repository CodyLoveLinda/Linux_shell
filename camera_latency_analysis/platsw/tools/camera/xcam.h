/*********************************************************************
 *
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2014, Robert Bosch LLC.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Robert Bosch nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************/
/*
 *    Copyright (C) 2019 AutoX, Inc.
 */

#ifndef XCAM_H
#define XCAM_H

#include <asm/types.h>          /* for videodev2.h */

extern "C"
{
#include <linux/videodev2.h>
}

#include <string>
#include <sstream>
#include <iostream>

namespace x_cam {

class UsbCam {
public:
    typedef enum
    {
        IO_METHOD_READ, IO_METHOD_MMAP, IO_METHOD_USERPTR, IO_METHOD_UNKNOWN,
    } io_method;

    typedef enum
    {
        PIXEL_FORMAT_YUYV, PIXEL_FORMAT_UYVY, PIXEL_FORMAT_MJPEG, PIXEL_FORMAT_YUVMONO10,
        PIXEL_FORMAT_RGB24, PIXEL_FORMAT_GREY, PIXEL_FORMAT_UNKNOWN
    } pixel_format;

    UsbCam();
    ~UsbCam();

    void start(char *dev_name);
    void shutdown(void);
    void grab_image(char* dev);
    void set_auto_focus(int value);
    void set_v4l_parameter(const std::string& param, int value);
    void set_v4l_parameter(const std::string& param, const std::string& value);
    static io_method io_method_from_string(const std::string& str);
    static pixel_format pixel_format_from_string(const std::string& str);
    void stop_capturing(void);
    void start_capturing(void);
    static void fps_monitor(union sigval);
    bool is_capturing();
    char dev_name[64] = "/dev/video0";
    bool memcopy = false;
    int jpeg_quality = -1;
    int endcount = -1;
    int raw_file_frame_count = 10;
    bool frame_save_time = 0;
    bool frame_ts = 0;
    bool save_img;
    unsigned int pixelformat;
private:
    typedef struct
    {
        int width;
        int height;
        int bytes_per_pixel;
        int image_size;
        char *image;
        int is_new;
    } camera_image_t;

    struct buffer
    {
        void *start;
        size_t length;
    };

    int init_mjpeg_decoder(int image_width, int image_height);
    void mjpeg2rgb(char *MJPEG, int len, char *RGB, int NumPixels);
    void process_image(const void * src, int len, camera_image_t *dest);
    int read_frame(char* devname);
    int image_save_jpeg(unsigned char* image, int len, int quality, char* devname, int frame_count);
    void uninit_device(void);
    void init_read(unsigned int buffer_size);
    void init_mmap(void);
    void init_userp(unsigned int buffer_size);
    void init_device(int image_width, int image_height, int framerate);
    void close_device(void);
    void open_device(void);

    static unsigned long long frame_count_;
    static unsigned long long prev_frame_count_;
    bool is_capturing_;
    int fd_;  
    std::string camera_dev_;
    bool monochrome_;
    io_method io_;
    buffer *buffers_;
    unsigned int n_buffers_;
    camera_image_t *image_;
};

}

#endif
