#!/usr/bin/env python3
# -*- coding: utf-8 -*-
'''
******************************************************************************
 * Copyright (C) 2021 AutoX, Inc. All Rights Reserved.
*****************************************************************************
'''

import numpy as np
import pandas as pd
from cyber_py import cyber
import matplotlib.pyplot as plt
from modules.drivers.proto.sensor_image_pb2 import Image, CompressedImage

latencys = dict()

def latency_process(data : CompressedImage):
    global latencys
    camera = data.header.frame_id
    if (not camera in latencys):
        print('Find camera: ' + camera)
        latencys[camera] = [[],[]]
    latencys[camera][0].append(data.header.timestamp_sec)
    latencys[camera][1].append(data.header.timestamp_sec - data.measurement_time)

def plot_latency():
    global latencys
    print('#############Plotting################')
    plt.title("Camera Latency analysis") 
    plt.xlabel("Timestamp") 
    plt.ylabel("latency (s)") 
    for camera in latencys:
        plt.plot(latencys[camera][0], latencys[camera][1], label = camera)
        print('\n' + camera)
        print('  Max: ' + str(max(latencys[camera][1])))
        print('  Average: ' + str(np.average(latencys[camera][1])))
        print('  Min: ' + str(min(latencys[camera][1])))
    plt.legend(loc='best')
    plt.show()

if __name__ == '__main__':
    cyber.init()
    node = cyber.Node('camera_latency_analysis')
    # gen5 14 cameras
    node.create_reader("/xurban/sensor/camera/left_0_n_6mm/image/compressed",
                       CompressedImage, latency_process)
    node.create_reader("/xurban/sensor/camera/left_0_n_12mm/image/compressed",
                       CompressedImage, latency_process)
    node.create_reader("/xurban/sensor/camera/left_315_n_6mm/image/compressed",
                       CompressedImage, latency_process)
    node.create_reader("/xurban/sensor/camera/left_270_n_6mm/image/compressed",
                       CompressedImage, latency_process)
    node.create_reader("/xurban/sensor/camera/left_225_n_6mm/image/compressed",
                       CompressedImage, latency_process)

    node.create_reader("/xurban/sensor/camera/right_0_n_6mm/image/compressed",
                       CompressedImage, latency_process)
    node.create_reader("/xurban/sensor/camera/right_0_n_12mm/image/compressed",
                       CompressedImage, latency_process)
    node.create_reader("/xurban/sensor/camera/right_45_n_6mm/image/compressed",
                       CompressedImage, latency_process)
    node.create_reader("/xurban/sensor/camera/right_90_n_6mm/image/compressed",
                       CompressedImage, latency_process)
    node.create_reader("/xurban/sensor/camera/right_135_n_6mm/image/compressed",
                       CompressedImage, latency_process)

    node.create_reader("/xurban/sensor/camera/blindspot_0_fisheye/image/compressed",
                       CompressedImage, latency_process)
    node.create_reader("/xurban/sensor/camera/blindspot_90_fisheye/image/compressed",
                       CompressedImage, latency_process)
    node.create_reader("/xurban/sensor/camera/blindspot_180_fisheye/image/compressed",
                       CompressedImage, latency_process)
    node.create_reader("/xurban/sensor/camera/blindspot_270_fisheye/image/compressed",
                       CompressedImage, latency_process)
    print('####################################')
    node.spin()
    cyber.shutdown()
    plot_latency()

