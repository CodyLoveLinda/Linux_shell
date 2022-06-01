#!/bin/bash

sudo rm /dev/cam*

xlnx_device=$(lspci -n | grep 9038 | wc -l)
echo "Xilinx Device Count: $xlnx_device"

if [[ $xlnx_device -eq 0 ]]; then
echo "============================="
echo "No Xlinix Device"
echo "============================="
exit
fi

max_video_device=$((16*xlnx_device))
echo "Max Video Supported: $max_video_device"

for ((dev=0; dev<$xlnx_device; dev++))
do
	xlnx_bdf=$(lspci -n | grep 9038 | head -n $((dev+1)) | tail -n 1 | awk '{print $1}')
	echo $xlnx_bdf
	test_device_addr=0000:$xlnx_bdf
	test_device=${xlnx_bdf/:/}
	test_device=qdma${test_device/./}
	echo $test_device

	for ((i=0; i<$max_video_device; i++))
	do
		dev_is_true=$(ls -l /sys/class/video4linux/video$i/device/acash3 | grep -i $test_device)
		if [[ -n $dev_is_true ]]; then
			cam_num=$(cat /sys/class/video4linux/video$i/name | tr -d '[A-Za-z]')
			echo "$i:$cam_num"
			if [ -n $cam_num ]; then
				ln -s /dev/video$i /dev/cam$((cam_num+$((dev*16))))
			fi
		fi
	done
done


ls -l /dev/vi*
ls -l /dev/cam*
