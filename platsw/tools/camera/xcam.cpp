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
 *    Copyright (C) 2020 AutoX, Inc.
 *    Version 0.9
 */

#define __STDC_CONSTANT_MACROS
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <float.h>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <chrono>
#include <iomanip>
#include "xcam.h"
//#include <linux/videodev2.h>
#include "turbojpeg.h"

//// #include "v4l2_formats.h"
#include <immintrin.h>
#include <cstdint>

#define VERSION     "0.9.9"

/*
 * Experimental code: use at your own risk
 * TODO
 * - Absolutely no cleanup of any sort
 * - Remove hard coded values esp. for resolution
 * - Image/Video processing should be in a separate thread
 * - option to view meta-data
 */

#define CLEAR(x) memset (&(x), 0, sizeof (x))


void fastMemcpy(void *pvDest, void *pvSrc, size_t nBytes) {
  assert(nBytes % 32 == 0);
  assert((intptr_t(pvDest) & 31) == 0);
  assert((intptr_t(pvSrc) & 31) == 0);
  //printf(" pvDest %p   pvSrc %p   intptr_t(pvDest) 0x%X   intptr_t(pvDest) & 31) 0x%X \n",   pvDest, pvSrc,   (long long) (intptr_t(pvDest)),  (long long) (intptr_t(pvDest) & 31));
  const __m256i *pSrc = reinterpret_cast<const __m256i*>(pvSrc);
  __m256i *pDest = reinterpret_cast<__m256i*>(pvDest);
  int64_t nVects = nBytes / sizeof(*pSrc);
  for (; nVects > 0; nVects--, pSrc++, pDest++) {
    const __m256i loaded = _mm256_stream_load_si256(pSrc);
    _mm256_stream_si256(pDest, loaded);
  }
  _mm_sfence();
}



using namespace std;

namespace x_cam {
unsigned long long UsbCam::prev_frame_count_ = 0;
unsigned long long UsbCam::frame_count_ = 0;
// for dequeue memcpy test
void *cache;

static void errno_exit(const char *s)
{
    cout << s << " error: " << errno << ": " << strerror(errno) << endl;
    exit(EXIT_FAILURE);
}

static void err_exit(const char *s)
{
    cout << s << " error: " << endl;
    exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void * arg)
{
    int r;

    do
        r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

const unsigned char uchar_clipping_table[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, // -128 - -121
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, // -120 - -113
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, // -112 - -105
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, // -104 -  -97
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //  -96 -  -89
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //  -88 -  -81
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //  -80 -  -73
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //  -72 -  -65
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //  -64 -  -57
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //  -56 -  -49
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //  -48 -  -41
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //  -40 -  -33
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //  -32 -  -25
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //  -24 -  -17
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //  -16 -   -9
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, //   -8 -   -1
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
    60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88,
    89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113,
    114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136,
    137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182,
    183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205,
    206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228,
    229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251,
    252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, // 256-263
    255, 255, 255, 255, 255, 255, 255, 255, // 264-271
    255, 255, 255, 255, 255, 255, 255, 255, // 272-279
    255, 255, 255, 255, 255, 255, 255, 255, // 280-287
    255, 255, 255, 255, 255, 255, 255, 255, // 288-295
    255, 255, 255, 255, 255, 255, 255, 255, // 296-303
    255, 255, 255, 255, 255, 255, 255, 255, // 304-311
    255, 255, 255, 255, 255, 255, 255, 255, // 312-319
    255, 255, 255, 255, 255, 255, 255, 255, // 320-327
    255, 255, 255, 255, 255, 255, 255, 255, // 328-335
    255, 255, 255, 255, 255, 255, 255, 255, // 336-343
    255, 255, 255, 255, 255, 255, 255, 255, // 344-351
    255, 255, 255, 255, 255, 255, 255, 255, // 352-359
    255, 255, 255, 255, 255, 255, 255, 255, // 360-367
    255, 255, 255, 255, 255, 255, 255, 255, // 368-375
    255, 255, 255, 255, 255, 255, 255, 255, // 376-383
    };
const int clipping_table_offset = 128;

/** Clip a value to the range 0<val<255. For speed this is done using an
 * array, so can only cope with numbers in the range -128<val<383.
 */
static unsigned char CLIPVALUE(int val)
{
  // Old method (if)
  /*   val = val < 0 ? 0 : val; */
  /*   return val > 255 ? 255 : val; */

  // New method (array)
  return uchar_clipping_table[val + clipping_table_offset];
}

/**
 * Conversion from YUV to RGB.
 * The normal conversion matrix is due to Julien (surname unknown):
 *
 * [ R ]   [  1.0   0.0     1.403 ] [ Y ]
 * [ G ] = [  1.0  -0.344  -0.714 ] [ U ]
 * [ B ]   [  1.0   1.770   0.0   ] [ V ]
 *
 * and the firewire one is similar:
 *
 * [ R ]   [  1.0   0.0     0.700 ] [ Y ]
 * [ G ] = [  1.0  -0.198  -0.291 ] [ U ]
 * [ B ]   [  1.0   1.015   0.0   ] [ V ]
 *
 * Corrected by BJT (coriander's transforms RGB->YUV and YUV->RGB
 *                   do not get you back to the same RGB!)
 * [ R ]   [  1.0   0.0     1.136 ] [ Y ]
 * [ G ] = [  1.0  -0.396  -0.578 ] [ U ]
 * [ B ]   [  1.0   2.041   0.002 ] [ V ]
 *
 */
static void YUV2RGB(const unsigned char y, const unsigned char u, const unsigned char v, unsigned char* r,
        unsigned char* g, unsigned char* b)
{
    const int y2 = (int)y;
    const int u2 = (int)u - 128;
    const int v2 = (int)v - 128;
    //std::cerr << "YUV=("<<y2<<","<<u2<<","<<v2<<")"<<std::endl;

    // This is the normal YUV conversion, but
    // appears to be incorrect for the firewire cameras
    //   int r2 = y2 + ( (v2*91947) >> 16);
    //   int g2 = y2 - ( ((u2*22544) + (v2*46793)) >> 16 );
    //   int b2 = y2 + ( (u2*115999) >> 16);
    // This is an adjusted version (UV spread out a bit)
    int r2 = y2 + ((v2 * 37221) >> 15);
    int g2 = y2 - (((u2 * 12975) + (v2 * 18949)) >> 15);
    int b2 = y2 + ((u2 * 66883) >> 15);
    //std::cerr << "   RGB=("<<r2<<","<<g2<<","<<b2<<")"<<std::endl;

    // Cap the values.
    *r = CLIPVALUE(r2);
    *g = CLIPVALUE(g2);
    *b = CLIPVALUE(b2);
}

void uyvy2rgb(char *YUV, char *RGB, int NumPixels)
{
    int i, j;
    unsigned char y0, y1, u, v;
    unsigned char r, g, b;
    for (i = 0, j = 0; i < (NumPixels << 1); i += 4, j += 6)
    {
        u = (unsigned char)YUV[i + 0];
        y0 = (unsigned char)YUV[i + 1];
        v = (unsigned char)YUV[i + 2];
        y1 = (unsigned char)YUV[i + 3];
        YUV2RGB(y0, u, v, &r, &g, &b);
        RGB[j + 0] = r;
        RGB[j + 1] = g;
        RGB[j + 2] = b;
        YUV2RGB(y1, u, v, &r, &g, &b);
        RGB[j + 3] = r;
        RGB[j + 4] = g;
        RGB[j + 5] = b;
    }
}

int uyvy2yuvplanar(unsigned char* src, int width, int height, unsigned char* des)
{
    unsigned char*  yp = des;
    unsigned char*  up = des + (width * height);
	unsigned char*  vp = des + (width * height *3 / 2 );
	unsigned char* srcline = src;
	for (int line =0; line <height; line++)
	{
        srcline =  src + (line * width << 1);
		for (int k = 0; k < (width >>1); k++)
		{
            yp[ line * width + (k <<1)] = srcline[ (k << 2) + 1];
            yp[ line * width + (k <<1) + 1] = srcline[ (k << 2) + 3 ];
            up[ (line * width >>1) + k] = srcline[(k << 2)];
            vp[ (line * width >>1) + k] = srcline[(k << 2) + 2];
        }
	}
    return 0;
}


UsbCam::UsbCam()
    : io_(IO_METHOD_MMAP), fd_(-1), buffers_(NULL), n_buffers_(0), save_img(0),
    image_(NULL), is_capturing_(false)
{
}
UsbCam::~UsbCam()
{
    shutdown();
}

#if 0
void UsbCam::process_image(const void * src, int len, camera_image_t *dest)
{
    if (pixelformat_ == V4L2_PIX_FMT_YUYV)
    {
        if (monochrome_)
        { //actually format V4L2_PIX_FMT_Y16, but xioctl gets unhappy if you don't use the advertised type (yuyv)
            mono102mono8((char*)src, dest->image, dest->width * dest->height);
        }
        else
        {
            yuyv2rgb((char*)src, dest->image, dest->width * dest->height);
        }
    }
    else if (pixelformat_ == V4L2_PIX_FMT_UYVY)
        uyvy2rgb((char*)src, dest->image, dest->width * dest->height);
    else if (pixelformat_ == V4L2_PIX_FMT_MJPEG)
        mjpeg2rgb((char*)src, len, dest->image, dest->width * dest->height);
    else if (pixelformat_ == V4L2_PIX_FMT_RGB24)
        rgb242rgb((char*)src, dest->image, dest->width * dest->height);
    else if (pixelformat_ == V4L2_PIX_FMT_GREY)
        memcpy(dest->image, (char*)src, dest->width * dest->height);
}
#endif

char g_img_file[128];
char g_title_str[] = "my_camera";
FILE *fp = NULL;
tjhandle handle = NULL;


#define padding  1
int UsbCam::image_save_jpeg(unsigned char* image, int len, int quality, char* devname, int frame_count)
{
 
    //snprintf(g_img_file, sizeof(g_img_file), "my.jpg", g_img_file);
    snprintf(g_img_file, sizeof(g_img_file), "%s-%03u.jpg", devname, frame_count);
    // printf("   image_save_jpeg   %s  buf size %d   %px \n", g_img_file, len, image);
    int width = 3840;
    int pitch = 3840 * 3;
    int height = 2160;
    int pixelFormat = TJPF_RGB;
    int jpegSubsamp = TJSAMP_420;
    unsigned long jpegSize = 0;
    unsigned char* jpegBuf = NULL;
    int jpegQual = 80;
    int flags = 0;
 	int yuvSize = 0, row, col, i, tilew = width, tileh = height, retval = 0, yuvPad = 1;

    auto startTime = std::chrono::high_resolution_clock::now();

    if ((len == 16588800) || (len == 4147200)) // UYVY 3820*2160*2     1920*1080*2
    {
        if (len == 16588800)
        {
            width = 3840;
            pitch = 3840 * 2;
            height = 2160;
        }
        else
        {
            width =  1920;;
            pitch = 1920 * 2;
            height = 1080;
        }

        //  YUV planar conversion
	    yuvSize = tjBufSizeYUV2(tilew, yuvPad, tileh, TJSAMP_422);
        uyvy2yuvplanar(image, width, height, (unsigned char*)cache);

        auto endTime = std::chrono::high_resolution_clock::now();
	    long long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        if (frame_save_time)
          printf(" *************** tjCompressFromYUV  yuvSize %d  uyvy2yuv422planar time  %u ms\n", yuvSize,  elapsed);

        auto compressTime = std::chrono::high_resolution_clock::now();
        int tj_stat = tjCompressFromYUV( handle, (unsigned char*)cache, width, padding, height,
                                         TJSAMP_422, &(jpegBuf), &jpegSize,  jpegQual, flags);

        endTime = std::chrono::high_resolution_clock::now();
	    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - compressTime).count();
        if (frame_save_time)
          printf(" *************** tjCompressFromYUV  tj_stat %d  encoding time  %u ms\n", tj_stat,  elapsed);

    }
    else
    {
        // RGB format
        if (len == 6220800)
        {
            width = 1920;
            pitch = 1920 * 3;
            height = 1080;
        }

        if (quality > 9 && quality < 101)
        jpegQual = quality;


        int tj_stat = tjCompress2( handle, image, width, pitch, height,
                                pixelFormat, &(jpegBuf), &jpegSize, jpegSubsamp, jpegQual, flags);

        if(tj_stat != 0)
        {
            const char *err = (const char *) tjGetErrorStr();
            cerr << "TurboJPEG Error: " << err << " UNABLE TO COMPRESS JPEG IMAGE\n";
            tjDestroy(handle);
            handle = NULL;
            return -1;
        }
    }
    //auto endTime = std::chrono::high_resolution_clock::now();
    //long long  elapsed  =  std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    //printf(" %s   encode time  %u ms \n", g_img_file, elapsed);

    //write out the compress date to the image file
    //startTime = std::chrono::high_resolution_clock::now();
    FILE *file = fopen(g_img_file, "wb");
    if (!file) {
        cerr << "Could not open JPEG file: " << strerror(errno);
        return -1;
    }
    if (fwrite(jpegBuf, jpegSize, 1, file) < 1) {
        cerr << "Could not write JPEG file: " << strerror(errno);
        return -1;
    }
    tjFree(jpegBuf);  
    jpegBuf = NULL;
    fclose(file);
    auto endTime = std::chrono::high_resolution_clock::now();
    long long elapsed  =  std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    if (frame_save_time)
        printf("  %s :  frame count %05u,   %s frame saving time  %u ms \n", devname, frame_count, g_img_file, elapsed);

    return 0;
}


//IplImage *g_tmp_img=NULL, *g_rgb_img=NULL;

int UsbCam::read_frame(char* devname)
{
    struct v4l2_buffer buf;
    int len;
    static int count = 0;
    //IplImage* frame;
    //CvMat cvmat;
    struct timeval  tv;
    double ts0 = 0;
    double ts1 = 0;
    static float  mimiDelay = FLT_MAX;
    static float  maxDelay = 0;
    float  curDelay = 0;

    switch (io_)
    {
        case IO_METHOD_MMAP:
            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;

            if (-1 == xioctl(fd_, VIDIOC_DQBUF, &buf))
            {
                switch (errno)
                {
                    case EAGAIN:
                        return 0;

                    case EIO:
                        /* Could ignore EIO, see spec. */
                        /* fall through */
                    default:
                        errno_exit("VIDIOC_DQBUF");
                }
            }

            gettimeofday(&tv, NULL);
            ts0 = (buf.timestamp.tv_sec *1000000 + buf.timestamp.tv_usec );
            ts1 = (tv.tv_sec * 1000000 + tv.tv_usec);
            curDelay = (ts1 - ts0)/1000;

            if (frame_count_ >0)
            {
                if ((mimiDelay > curDelay))
                    mimiDelay = curDelay;
                if (maxDelay < curDelay)
                    maxDelay = curDelay;
            }

            if (frame_ts) {
                /*
                 cout << "Image# " << count << ", size " << buf.bytesused << ", TS: "
                 << buf.timestamp.tv_sec << "." << buf.timestamp.tv_usec << " ,  dequeue TS: " 
                 << tv.tv_sec << "." <<  tv.tv_usec << endl;
                */
                printf("%s : Image# %u  size %u  TS(us): %.0F  dq receiving TS(us): %.0F  delay in ms: %.2f ( min %.2f  max %.2f ) \n", 
                       devname, count, buf.bytesused, ts0, ts1, curDelay, mimiDelay, maxDelay);
                ++count;
            }
            frame_count_++;

            assert(buf.index < n_buffers_);
            len = buf.bytesused;

        if(memcopy)
        {
           struct timespec start;
           struct timespec stop;
           auto startTime = std::chrono::high_resolution_clock::now();
           clock_gettime( CLOCK_REALTIME, &start);
           memcpy(cache, buffers_[buf.index].start, len);
           //fastMemcpy(cache, buffers_[buf.index].start, len);
           //memcpy_fast(cache, buffers_[buf.index].start, len);
           auto endTime = std::chrono::high_resolution_clock::now();
           clock_gettime( CLOCK_REALTIME, &stop);
           long long  elapsed  =  std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
           uint64_t startNs = start.tv_sec * 1000 * 1000 * 1000 + start.tv_nsec;
           uint64_t endNs   =  stop.tv_sec * 1000 * 1000 * 1000 + stop.tv_nsec;
           long long  endUs = (endNs - startNs) / 1000;
           printf("  %s :  frame_count %05u,   frame memcpy time  %lld ms    clk %lld us \n", devname, frame_count_, elapsed, endUs);

        }

        if (save_img)
        {
            auto startTime = std::chrono::high_resolution_clock::now();
            snprintf(g_img_file, sizeof(g_img_file), "%s-%03u.raw", devname, frame_count_);

            // write raw_file_frame_count number of frames into one output file
            if ((frame_count_ % raw_file_frame_count == 1) || (raw_file_frame_count == 1))
            {
                fp = fopen(g_img_file,"ab");
                if( fp == NULL)
                {
                    fprintf(stderr,"Error writing to %s\n", g_img_file);
                    return(1);
                }
            }
            if (fp)
            {
                fwrite(buffers_[buf.index].start, sizeof(char), buf.bytesused, fp);
            }
            if (frame_count_ % raw_file_frame_count  == 0)
            {
                if(fp)
                    fclose(fp);
                fp = NULL;
            }
            auto endTime = std::chrono::high_resolution_clock::now();
            long long  elapsed  =  std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
            if (frame_save_time)
                printf("  %s :  frame_count %05u,   %s raw frame saving time  %u ms \n", devname, frame_count_, g_img_file, elapsed);
        }

        if(jpeg_quality > 0)
            image_save_jpeg((unsigned char*)buffers_[buf.index].start, buf.bytesused, jpeg_quality, devname, frame_count_);

/*
            // process_image(buffers_[buf.index].start, len, image_);

            // Needs to be in a separate thread. Needs serious cleanup
#ifdef SAVE_IMG
            if (save_img) {
                cvNamedWindow(g_title_str, CV_WINDOW_NORMAL);
                cvResizeWindow(g_title_str, 600, 600);

                if (!g_rgb_img) {
                    g_rgb_img = cvCreateImage(cvSize(1920, 1080), IPL_DEPTH_8U, 3);
                    g_tmp_img = cvCreateImage(cvSize(1920, 1080), IPL_DEPTH_8U, 2);
                }
                memcpy(g_tmp_img->imageData, buffers_[buf.index].start, len);
                cvCvtColor(g_tmp_img, g_rgb_img, CV_YUV2BGR_YUYV);

                //cvmat = cvMat(1080, 1920, CV_8UC3, (void *)buffers_[buf.index].start);
                //frame = cvDecodeImage(&cvmat, 1);
                //cvNamedWindow("window",CV_WINDOW_AUTOSIZE);
                // cvShowImage("window", frame);
                //cvWaitKey(0);
    
                snprintf(g_img_file, sizeof(g_img_file), "%s-%03u.jpg", "my_image", frame_count_);
                // cvShowImage(g_title_str, g_rgb_img);
                cvSaveImage(g_img_file, g_rgb_img, 0);
                // cvWaitKey(1);

                // cvReleaseImage(&g_tmp_img);
                // cvReleaseImage(&g_rgb_img);
            }
#endif
*/
            if (-1 == xioctl(fd_, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");

            break;
        default:
            errno_exit("Unsupported io method");
    }

    return 1;
}

bool UsbCam::is_capturing() {
    return is_capturing_;
}

void UsbCam::stop_capturing(void)
{
    if(!is_capturing_) return;

    is_capturing_ = false;
    enum v4l2_buf_type type;

    switch (io_)
    {
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (-1 == xioctl(fd_, VIDIOC_STREAMOFF, &type))
                errno_exit("VIDIOC_STREAMOFF");

            break;
        default:
            errno_exit("Unsupported io method");
    }
    printf("#####    xCam::stop_capturing  ######\n");
}

void UsbCam::start_capturing(void)
{

    if(is_capturing_) return;

    unsigned int i;
    enum v4l2_buf_type type;

    switch (io_)
    {
        case IO_METHOD_MMAP:
            for (i = 0; i < n_buffers_; ++i)
            {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                if (-1 == xioctl(fd_, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");
            }

            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

            if (-1 == xioctl(fd_, VIDIOC_STREAMON, &type))
                errno_exit("VIDIOC_STREAMON");

            break;

        default:
            errno_exit("Unsupported io method");
    }
    is_capturing_ = true;
}

void UsbCam::uninit_device(void)
{
    unsigned int i = 0;

    switch (io_)
    {
        case IO_METHOD_MMAP:
            for (i = 0; i < n_buffers_; i++)
            {
                if (-1 == munmap(buffers_[i].start, buffers_[i].length))
                    errno_exit("munmap");
            }
            break;
        default:
            errno_exit("Unsupported io method");
    }

    free(buffers_);
}

void UsbCam::init_read(unsigned int buffer_size)
{
    buffers_ = (buffer*)calloc(1, sizeof(*buffers_));

    if (!buffers_)
    {
        errno_exit("init_read");
    }
    /*
    buffers_[0].length = buffer_size;
    buffers_[0].start = malloc(buffer_size);

    if (!buffers_[0].start)
    {
        errno_exit("init_read");
    }
    */
}

void UsbCam::init_mmap(void)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd_, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
            err_exit("VIDIOC_REQBUFS: no support for memory map");
        else
            errno_exit("VIDIOC_REQBUFS");
    }

    if (req.count < 2)
    {
        err_exit("VIDIOC_REQBUFS: Insufficient buffer memory");
    }

    buffers_ = (buffer*)calloc(req.count, sizeof(*buffers_));

    if (!buffers_)
    {
        errno_exit("init_mmap");
    }

    for (n_buffers_ = 0; n_buffers_ < req.count; ++n_buffers_)
    {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers_;

        if (-1 == xioctl(fd_, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");

        buffers_[n_buffers_].length = buf.length;
        buffers_[n_buffers_].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                MAP_SHARED, fd_, buf.m.offset);

        if (MAP_FAILED == buffers_[n_buffers_].start)
            errno_exit("mmap");
    }
}

void UsbCam::init_userp(unsigned int buffer_size)
{
    struct v4l2_requestbuffers req;
    unsigned int page_size;

    page_size = getpagesize();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (-1 == xioctl(fd_, VIDIOC_REQBUFS, &req))
    {
        if (EINVAL == errno)
            err_exit("Camera does not support user pointer i/o");
        else
            errno_exit("VIDIOC_REQBUFS");
    }

    buffers_ = (buffer*)calloc(4, sizeof(*buffers_));

    if (!buffers_)
    {
        errno_exit("init_userp");
    }

    for (n_buffers_ = 0; n_buffers_ < 4; ++n_buffers_)
    {
        buffers_[n_buffers_].length = buffer_size;
        buffers_[n_buffers_].start = memalign(/* boundary */page_size, buffer_size);

        if (!buffers_[n_buffers_].start)
        {
            errno_exit("memalign");
        }
    }
}

void UsbCam::init_device(int image_width, int image_height, int framerate)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;
    int ret = 0;

    if (-1 == xioctl(fd_, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
            err_exit("Not a V4L2 device");
        }
        else
        {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        err_exit("Not a video capture device");
    }

    switch (io_)
    {
        case IO_METHOD_READ:
            if (!(cap.capabilities & V4L2_CAP_READWRITE))
                err_exit("Device does not support read i/o");

            break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
            if (!(cap.capabilities & V4L2_CAP_STREAMING))
            {
                err_exit("camera does not support streaming i/o");
            }

            break;
        default:
            err_exit("Unsupported io method");
    }

    /* Select video input, video standard and tune here. */

    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd_, VIDIOC_CROPCAP, &cropcap))
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl(fd_, VIDIOC_S_CROP, &crop))
        {
            switch (errno)
            {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                    break;
            }
        }
    }
    else
    {
        /* Errors ignored. */
    }

    CLEAR(fmt);

    //  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //  fmt.fmt.pix.width = 640;
    //  fmt.fmt.pix.height = 480;
    //  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    //  fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = image_width;
    fmt.fmt.pix.height = image_height;
    fmt.fmt.pix.pixelformat = pixelformat;
    //// cout << "setting pixel format to: " << image_width << image_height << " FMT " << pixelformat << endl;
    // fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    //fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(fd_, VIDIOC_S_FMT, &fmt))
        errno_exit("VIDIOC_S_FMT");

    ret = xioctl(fd_, VIDIOC_S_FMT, &fmt);
    //// printf("    line 800  VIDIOC_S_FMT  W %d  H %d  FMT 0x%x  RET 0x%x \n",  image_width, image_height, pixelformat, ret);
    /* Note VIDIOC_S_FMT may change width and height. */

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    image_width = fmt.fmt.pix.width;
    image_height = fmt.fmt.pix.height;

#if 0
    struct v4l2_streamparm stream_params;
    memset(&stream_params, 0, sizeof(stream_params));
    stream_params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd_, VIDIOC_G_PARM, &stream_params) < 0)
        errno_exit("Couldn't query v4l fps!");

    //cout << "Capability flag: " << stream_params.parm.capture.capability << endl;

    stream_params.parm.capture.timeperframe.numerator = 1;
    stream_params.parm.capture.timeperframe.denominator = framerate;
    if (xioctl(fd_, VIDIOC_S_PARM, &stream_params) < 0)
        err_exit("Couldn't set camera framerate");
    else
        cout << "Set framerate to " << framerate << endl;
#endif

}

void UsbCam::close_device(void)
{
    if (-1 == close(fd_))
        errno_exit("close");

    fd_ = -1;
}

void UsbCam::start(char *dev_name)
{
    unsigned int image_height, image_width, framerate;

    fd_ = open(dev_name, O_RDWR);
    if (fd_ == -1) {
        cout << "device open failed for " << dev_name << endl;
        exit(-1);
    }
    else
        cout << "device open successful for " << dev_name << endl;

    /* hardcode for now */
    image_height = 1920; 
    image_width = 1080;
    framerate = 30;

    init_device(image_height, image_width, framerate);

    if (io_ == IO_METHOD_MMAP) {
        init_mmap();
    } else {
        cout << "Unsupported IO method: " << io_ << endl;
        exit(-1);
    }

    image_ = (camera_image_t *)calloc(1, sizeof(camera_image_t));

    image_->width = image_height;
    image_->height = image_width;
    image_->bytes_per_pixel = 3;      //corrected 11/10/15 (BYTES not BITS per pixel)

    image_->image_size = image_->width * image_->height * image_->bytes_per_pixel;
    image_->is_new = 0;
    image_->image = (char *)calloc(image_->image_size, sizeof(char));
    memset(image_->image, 0, image_->image_size * sizeof(char));

    start_capturing();
}

void UsbCam::shutdown(void)
{
    stop_capturing();
    uninit_device();
    close_device();
}

void UsbCam::grab_image(char* dev)
{
    fd_set fds;
    struct timeval tv;
    int r;
    char* devname = dev + 5;
    //printf(" !!!! %s    =>   %s : ", dev, devname);

    FD_ZERO(&fds);
    FD_SET(fd_, &fds);

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    r = select(fd_ + 1, &fds, NULL, NULL, &tv);

    if (-1 == r)
    {
        if (EINTR == errno)
            return;

        errno_exit("select");
    }

    if (0 == r)
        err_exit("select timeout");


    read_frame(devname);
    image_->is_new = 1;
}

void UsbCam::set_auto_focus(int value)
{
    struct v4l2_queryctrl queryctrl;
    struct v4l2_ext_control control;

    memset(&queryctrl, 0, sizeof(queryctrl));
    queryctrl.id = V4L2_CID_FOCUS_AUTO;

    if (-1 == xioctl(fd_, VIDIOC_QUERYCTRL, &queryctrl))
    {
        if (errno != EINVAL)
        {
            perror("VIDIOC_QUERYCTRL");
            return;
        }
        else
        {
            cout << "V4L2_CID_FOCUS_AUTO is not supported" << endl;
            return;
        }
    }
    else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
    {
        cout << "V4L2_CID_FOCUS_AUTO is not supported" << endl;
        return;
    }
    else
    {
        memset(&control, 0, sizeof(control));
        control.id = V4L2_CID_FOCUS_AUTO;
        control.value = value;

        if (-1 == xioctl(fd_, VIDIOC_S_CTRL, &control))
        {
            perror("VIDIOC_S_CTRL");
            return;
        }
    }
}

/**
 * Set video device parameter via call to v4l-utils.
 *
 * @param param The name of the parameter to set
 * @param param The value to assign
 */
void UsbCam::set_v4l_parameter(const std::string& param, int value)
{
    //  set_v4l_parameter(param, boost::lexical_cast<std::string>(value));
}
/**
 * Set video device parameter via call to v4l-utils.
 *
 * @param param The name of the parameter to set
 * @param param The value to assign
 */
void UsbCam::set_v4l_parameter(const std::string& param, const std::string& value)
{
    // build the command
    std::stringstream ss;
    ss << "v4l2-ctl --device=" << camera_dev_ << " -c " << param << "=" << value << " 2>&1";
    std::string cmd = ss.str();

    // capture the output
    std::string output;
    int buffer_size = 256;
    char buffer[buffer_size];
    FILE *stream = popen(cmd.c_str(), "r");
    if (stream)
    {
        while (!feof(stream))
            if (fgets(buffer, buffer_size, stream) != NULL)
                output.append(buffer);
        pclose(stream);
        // any output should be an error
        if (output.length() > 0)
            err_exit(output.c_str());
    }
    else
       err_exit("usb_cam_node could not be run");
}

UsbCam::io_method UsbCam::io_method_from_string(const std::string& str)
{
    if (str == "mmap")
        return IO_METHOD_MMAP;
    else if (str == "read")
        return IO_METHOD_READ;
    else if (str == "userptr")
        return IO_METHOD_USERPTR;
    else
        return IO_METHOD_UNKNOWN;
}

UsbCam::pixel_format UsbCam::pixel_format_from_string(const std::string& str)
{
    if (str == "yuyv")
        return PIXEL_FORMAT_YUYV;
    else if (str == "uyvy")
        return PIXEL_FORMAT_UYVY;
    else if (str == "mjpeg")
        return PIXEL_FORMAT_MJPEG;
    else if (str == "yuvmono10")
        return PIXEL_FORMAT_YUVMONO10;
    else if (str == "rgb24")
        return PIXEL_FORMAT_RGB24;
    else if (str == "grey")
        return PIXEL_FORMAT_GREY;
    else
        return PIXEL_FORMAT_UNKNOWN;
}

using namespace chrono;
void UsbCam::fps_monitor(union sigval si)
{
    time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    if (prev_frame_count_)
        cout << (char *)si.sival_ptr << ": " << put_time(localtime(&now), "%T ") << "fps: " << (frame_count_ - prev_frame_count_) << endl;
    prev_frame_count_ = frame_count_;
}

}


using namespace x_cam;
int main(int argc, char *argv[])
{
    int opt;
    UsbCam usbcam;

    struct sigevent sev;
    struct itimerspec its;
    timer_t timerid;

    //usbcam.pixelformat = V4L2_PIX_FMT_YUYV;
    usbcam.memcopy = 0;
    while ((opt = getopt(argc, argv, "cd:e:n:j:p:sfhvt")) != -1) {
        switch (opt) {
            case 'h':
            default:
                cout << "Usage: All args are optional " << endl <<  argv[0] <<
                " -d </dev/video#> -f -s -t <> -n <> -j <> -e <>" << endl <<
                " -f print frame rate with timestamp" << endl <<
                " -s save raw video stream" << endl <<
                " -t show each frame saving time" << endl <<
                " -n <number of raw frame in each file>" << endl <<
                " -j < jpeg compression quality control number > takes between 20 to 100, default is 80. " << endl <<
                " -e < end frame count number > default is infinity" << endl <<
                " -c raw frame memory copy to local buffer test with timing info" <<endl;
                exit(0);
                break;

            case 'c':
                // local memory copy for the raw frame
                 usbcam.memcopy = 1;
                 cout << "---------test memory copy  ---------" << endl;
                 break;
            case 'd':
                if (optarg == NULL) {
                    cout << "Camera device path required. e.g. -d /dev/video0" << endl;
                    exit(0);
                }
                strncpy(usbcam.dev_name, optarg, sizeof(usbcam.dev_name));
                break;
            case 'e':
                if (optarg == NULL) {   
                   usbcam.endcount = 1000;
                }
                else
                   // end frame count >= 1
                   if (atoi(optarg) > 0)
                       usbcam.endcount = atoi(optarg);
                cout << "Camera frame capture end count:  " << usbcam.endcount << endl;
                break; 
            case 'f':
                usbcam.frame_ts = 1;
                break;

            case 'n':
                if ((atoi(optarg) > 0) && (atoi(optarg) < 1001))
                   usbcam.raw_file_frame_count = atoi(optarg);
                usbcam.save_img = 1;   
                cout << "Camera frame count in single dump file:  " << usbcam.raw_file_frame_count << endl;
                break;

            case 'j':
                if ((atoi(optarg) >= 20) && (atoi(optarg) <= 100))
                   usbcam.jpeg_quality = atoi(optarg);
                else
                   usbcam.jpeg_quality = 80;
                cout << "Camera frame jpeg compression quality control:  " << usbcam.jpeg_quality << endl;
                break;

            case 'p':
                if (!strncmp(optarg, "yuyv", 4)) {
                    usbcam.pixelformat = V4L2_PIX_FMT_YUYV;
                    cout << "format: YUYV " << endl;
                } else if (!strncmp(optarg, "rgb888", 6)) {
                    usbcam.pixelformat = V4L2_PIX_FMT_RGB24;
                    cout << "format: RGB24 " << endl;
                } else if (!strncmp(optarg, "bgr888", 6)) {
                    usbcam.pixelformat = V4L2_PIX_FMT_BGR24;
                    cout << "format: BGR24 " << endl;
                } else if (!strncmp(optarg, "mjpeg", 5)) {
                    usbcam.pixelformat = V4L2_PIX_FMT_MJPEG;
                    cout << "format: MJPEG " << endl;
                } else {
                    cout << "Unsupported pixel format: " << optarg << 
                        ". See help for valid options. " << endl;
                    exit(0);
                }
                break;
                    
            case 's':
                usbcam.save_img = 1;
                break;

            case 't':
                usbcam.frame_save_time = 1;
                break;

            case 'v':
                cout << "Version: " << VERSION << endl;
                break;

        }
    }

    memset(&sev, 0, sizeof(sev));
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = usbcam.fps_monitor;
    sev.sigev_value.sival_ptr = &usbcam.dev_name;
    if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) == -2)
        errno_exit("timer_create");

    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    if (timer_settime(timerid, 0, &its, NULL) == -1)
        errno_exit("timer_settime");

    cout << "Camera path: " << usbcam.dev_name << endl;

    handle = tjInitCompress();
    cout<< "tjhandle  " << handle << endl;

    //cache = malloc(256 * 1048576);
    cache = memalign(4096, 128 * 1048576);
    usbcam.start(usbcam.dev_name);

    for (;;) {
//        printf(" usbcam.endcount  %d   memcopy ? %d \n",   usbcam.endcount,  usbcam.memcopy);
        usbcam.grab_image(usbcam.dev_name);
        if (usbcam.endcount  > 0)
            usbcam.endcount --;

        if (usbcam.endcount == 0)  
           break;  

    }
    if (handle)
        tjDestroy(handle);

    return 0;
}
