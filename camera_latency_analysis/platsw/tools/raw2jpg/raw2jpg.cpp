#include<stdio.h>
#include<stdlib.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <signal.h>
#include <float.h>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <chrono>
#include <iostream>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include "turbojpeg.h"

using namespace std;

char g_img_file[128] = { 0 };
tjhandle handle = NULL;

static int uyvy2yuvplanar(unsigned char* src, int width, int height, unsigned char* des)
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

static int image_to_jpeg(char* filename, int pixelFormat, int quality, int loop)
{
	FILE * fp;
	int tj_stat = 0, ret = 0;;
	int frame_count = 0;
	// printf("   image_save_jpeg   %s  buf size %d   %px \n", g_img_file, len, image);
	int width = 3840;
	int pitch = 3840 * 3;
	int height = 2160;
	int tjFormat = TJPF_RGB;
	int jpegSubsamp = TJSAMP_420;
	unsigned long jpegSize = 0;
	unsigned char* jpegBuf = NULL;
	int jpegQual = quality;
	int flags = 0;
	int i = 0;
	int totalJpegSize = 0, tilew, tileh, retval = 0, yuvPad = 1;
	unsigned long yuvSize = 0;
	unsigned char *yuvBuf = NULL;
	size_t bufsz = 3840 * 2160 * 3;
	void *raw_buf = malloc(bufsz);
	void *jpg_buf = malloc(bufsz);
	long long fmin = -1, fmax = -1, cpmin = -1, cpmax = -1;
	long long copytime;
	long long elapsed;
	long long jpegEnc;
	int fps = 0;
	int fps_min = -1, fps_max = -1;

	if (pixelFormat == 1)
		bufsz = width * height * 2;
	else
		bufsz = width * height * 3;

	//chrono::system_clock::to_time_t(chrono::system_clock::now());
	auto begin = std::chrono::high_resolution_clock::now();
	fp = fopen(filename, "rb");
	if (!fp)
	{
		printf("\n input file %s opening failed \n", filename);
		return -1;
	}

	ret = fseek(fp, 0, SEEK_END);
	size_t file_size = ftell(fp);
	auto fopt = std::chrono::high_resolution_clock::now();
	long long foptt = std::chrono::duration_cast<std::chrono::microseconds>(fopt - begin).count();
	printf("\n file open time: %lld us  microseconds  file_size %ld   frames %lu \n", foptt, file_size, file_size / bufsz);

	// FPS  start time
	auto fps_start = std::chrono::high_resolution_clock::now();
    for (int k = 0; k < loop; k++)
	{
		auto startTime = std::chrono::high_resolution_clock::now();
		ret = fseek(fp, 0, SEEK_SET);
		int fcnt = 0;
		size_t blk = 0;
		while (blk = fread(raw_buf, bufsz, 1, fp))
		{
			fcnt++;
			snprintf(g_img_file, sizeof(g_img_file), "%s-%03u.jpg", filename, fcnt);
			auto endTime = std::chrono::high_resolution_clock::now();
			elapsed = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
			printf("\n  loop %d  frame %d  file read time: %lld us microseconds\n", k, fcnt, elapsed);

			for (int i = 0; i < 1; i++)
			{
				auto copystartTime = std::chrono::high_resolution_clock::now();
				memcpy(jpg_buf, raw_buf, bufsz);
				auto copydoneTime = std::chrono::high_resolution_clock::now();

				copytime = std::chrono::duration_cast<std::chrono::microseconds>(copydoneTime - copystartTime).count();
				printf("  raw frame %d  copytime   %lld us microseconds\n", fcnt, copytime);
			}

			if (pixelFormat == 1)  // UYVY
			{
				tilew = width, tileh = height,
					yuvSize = tjBufSizeYUV2(tilew, yuvPad, tileh, TJSAMP_422);
				if (yuvSize == (unsigned long)-1)
					printf("!!!!! error on YUV buffer");
				if ((yuvBuf = (unsigned char *)malloc(yuvSize)) == NULL)
					printf("!!!!! error allocating YUV buffer");
				memset(yuvBuf, 127, yuvSize);

				////  rgb input -> YUV422
				//retval = tjEncodeYUV3(handle, image, width, pitch, height, pixelFormat, yuvBuf,
				//                        yuvPad, TJSAMP_422, flags);

				uyvy2yuvplanar((unsigned char *)raw_buf, width, height, yuvBuf);
				/*
				file = fopen("yuv422.yuv", "wb");
				retval = fwrite(yuvBuf, yuvSize, 1, file);
				fclose(file);
				*/
				//
				auto yuvTime = std::chrono::high_resolution_clock::now();
				long long elapsed = std::chrono::duration_cast<std::chrono::microseconds>(yuvTime - startTime).count();
				printf("  %s :  frame count %05u,   %s  frame uyvy2yuv422planar time  %lld us \n", filename, fcnt, g_img_file, elapsed);

				startTime = std::chrono::high_resolution_clock::now();
				tj_stat = tjCompressFromYUV(handle, yuvBuf, width, yuvPad, height,
					TJSAMP_422, &jpegBuf, &jpegSize, jpegQual, flags);
				auto endTime = std::chrono::high_resolution_clock::now();
				jpegEnc = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
				printf("  %s :  frame count %05u,   %s  frame tjCompressFromYUV time  %lld us \n", filename, fcnt, g_img_file, jpegEnc);
			}
			else
			{
				startTime = std::chrono::high_resolution_clock::now();
				tj_stat = tjCompress2(handle, (unsigned char *)raw_buf, width, pitch, height,
					                  tjFormat, &(jpegBuf), &jpegSize, jpegSubsamp, jpegQual, flags);
				auto endTime = std::chrono::high_resolution_clock::now();
				jpegEnc = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
				printf("  %s :  frame count %05u,   %s  frame tjCompress2 time  %lld us \n", filename, fcnt, g_img_file, jpegEnc);

				if (tj_stat != 0)
				{
					const char *err = (const char *)tjGetErrorStr();
					cerr << "TurboJPEG Error: " << err << " UNABLE TO COMPRESS JPEG IMAGE\n";
					tjDestroy(handle);
					handle = NULL;
					return -1;
				}
			}

			/////////////////////////////////////////////////////////////////////////
			FILE *file = fopen(g_img_file, "wb");
			if (!file) {
				cerr << "Could not open JPEG file: " << strerror(errno);
				return -1;
			}
			if (fwrite(jpegBuf, jpegSize, 1, file) < 1) {
				cerr << "Could not write JPEG file: " << strerror(errno);
				return -1;
			}
			fclose(file);
			/////////////////////////////////////////////////////////////////////////
			tjFree(jpegBuf);
			jpegBuf = NULL;

			endTime = std::chrono::high_resolution_clock::now();
			auto saved = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
			printf("  %s :  frame count %05u,   %s  frame jpeg saving time  %ld us \n", filename, fcnt, g_img_file, saved);

			auto tpast = std::chrono::duration_cast<std::chrono::microseconds>(endTime - fps_start).count();
			// FPS COUNT CHECK POINT > 1000 ms   FPS start = now   FPS_COUNT = 1  or FPS_COUNT++
			if (tpast < 1000000)
				fps++;
			else
			{
				if (fps_min == -1)
					fps_min = fps;
				else
					if (fps < fps_min)
						fps_min = fps;

				if (fps_max == -1)
					fps_max = fps;
				else
					if (fps > fps_max)
						fps_max = fps;

				printf("  ~~~~~~~~  frame jpeg encoding  FPS  %d    ( %d | %d )\n",  fps, fps_min, fps_max);
				fps = 1;
				fps_start = endTime;
			}


			if (fmin == -1)
				fmin = jpegEnc;
			else
				if (jpegEnc < fmin)
					fmin = jpegEnc;

			if (fmax == -1)
				fmax = jpegEnc;
			else
				if (jpegEnc > fmax)
					fmax = jpegEnc;

			if (cpmin == -1)
				cpmin = copytime;
			else
				if (copytime < cpmin)
					cpmin = copytime;

			if (cpmax == -1)
				cpmax = copytime;
			else
				if (copytime > cpmax)
					cpmax = copytime;

			startTime = std::chrono::high_resolution_clock::now();
		}
	}
	fclose(fp);
	printf(" \n\n  performance statistics   jpeg encoding time      min %lld us max %lld us \n", fmin, fmax);
	printf(     "                           frame copy time         min %lld us max %lld us\n", cpmin, cpmax);
	printf(     "                           frame processing FPS    min %d  max %d  fps\n", fps_min, fps_max);

    return tj_stat;
}


int main(int argc, char *argv[])
{
    int ret = 0;
    int  i = 0,  offset = 0, number = 0, loop = 1;
    int width =3840;
    int height = 2160;
    int format = 0;  //rgb    1 UYVY
    int jpegQuality = 80;

	if (argc == 1)
	{
        cout << "Usage: option switch: "  << endl <<
        " -i input raw filename" << endl <<
        " -w image width  default 3840" << endl <<
        " -h image height  default 2160" << endl <<
        " -r <rgb format>  by default" << endl <<
        " -y UYVY format"<< endl <<
        " -j < jpeg compression quality control number > takes between 1 to 100, default is 80. " << endl <<
        " -l < loop count for performance test> default is 1" << endl;
        exit(0);
	}

	char filename[256] = { 0 };
    while (++i < argc)
        switch (argv[i][0]) {
        case '-':
            while ((i < argc) && (*argv[i]))
            switch (*++argv[i]) {
                case 'i': case 'I':
					if (++i < argc)
					{
						strcpy(filename, argv[i]);
						while (*argv[i]++);
						argv[i]--;
					}
                    printf("-i  filename %s \n", filename);
                    break;

                case 'j': case 'J':
					if (++i < argc)
					{
                        number = 0;
                        while (isdigit(*argv[i]))
                            number = number * 10 + *argv[i]++ - '0';
                        if ((number > 0) && (number < 101))
                            jpegQuality = number;
                        printf("-j  jpeg quality %d \n", jpegQuality);
                    }
                    break;

                case 'l': case 'L':
					if (++i < argc)
					{
						number = 0;
						while (isdigit(*argv[i]))
							number = number * 10 + *argv[i]++ - '0';
                        if ((number > 0) && (number < 10000))
                            loop = number;
						printf("-l  loop test %d \n", loop);
					}
                    break;
                case 'n': case 'N':
                	if (++i < argc)
                    {
                        number = 0;
                        while (isdigit(*argv[i]))
                            number = number * 10 + *argv[i]++ - '0';
                        printf("-n  input number %d \n", number);
                    }
                    break;

                case 'w':  case 'W':
                    if (++i < argc)
                    {
                        number = 0;
                        while (isdigit(*argv[i]))
                            number = number * 10 + *argv[i]++ - '0';
                        if ((number > 0) && (number < 10000))
                            width = number;
                        printf("-w  width %d \n", width);
                    }
                    break;

                case 'h':  case 'H':
                    if (++i < argc)
                    {
                        number = 0;
                        while (isdigit(*argv[i]))
                            number = number * 10 + *argv[i]++ - '0';
                        if ((number > 0) && (number < 10000))
                            height = number;
                        printf("-h  height %d \n", height);
                    }
                    break;


                case 'y':  case 'Y': 
                    format = 1;
                    printf("--  UYUV format\n");
                    argv[i]++; 
                    break;
                case 'r':  case 'R': 
                    format = 0;
                    printf("--  RGB format\n");
                    argv[i]++;
                    break;

            default:
                printf("Bad switch %c, ignored.\n", *argv[i]);
            }
            break;

        default:
            printf("Bad switch %c, ignored.\n", *argv[i]);
                break;

        } /* end switch argc */


    handle = tjInitCompress();
    //cout << "tjhandle  " << handle << endl;

    image_to_jpeg(filename, format, jpegQuality, loop);

    return 0;
}
