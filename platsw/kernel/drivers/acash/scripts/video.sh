#!/bin/bash

rm /dev/cam*

xlnx_device=$(lspci -n | grep 9034 | wc -l)
echo $xlnx_device

if [[ $xlnx_device -eq 0 ]]; then
echo "============================="
echo "No Xlinix Device"
echo "============================="
exit
fi

max_video_device=$((8*xlnx_device))
echo $max_video_device

for ((dev=0; dev<$xlnx_device; dev++))
do
	xlnx_bdf=$(lspci -n | grep 9034 | head -n $((dev+1)) | tail -n 1 | awk '{print $1}')
	echo $xlnx_bdf
	test_device_addr=0000:$xlnx_bdf
	test_device=${xlnx_bdf/:/}
	test_device=qdma${test_device/./}
	echo $test_device
	
	for ((i=0; i<$max_video_device; i++))
	do
		dev_is_true=$(ls -l /sys/class/video4linux/video$i/device/acash | grep -i $test_device)
		if [[ -n $dev_is_true ]]; then
			cam_num=$(cat /sys/class/video4linux/video$i/name | cut -b 4)
			echo "$i:$cam_num"
			if [ -n $cam_num ]; then
				ln -s /dev/video$i /dev/cam$((cam_num+$((dev*8))))
			fi
		fi
	done
done


ls -l /dev/vi*
ls -l /dev/cam*
