#!/bin/bash

usbtv=$(lsmod | grep -i usbtv | wc -l)
if [[ $usvtv -eq 0 ]]; then
	modprobe usbtv
fi

acash=$(lsmod | grep -i acash | wc -l)
if [[ $acash -ne 0 ]]; then
	rmmod acash
fi


xlnx_device=$(lspci -n | grep 9034 | wc -l)
if [[ $xlnx_device -eq 0 ]]; then 
	echo "==================="
	echo " No Xilinx Device"
	echo "==================="
	exit
fi

size=$(ls -l build/acash.ko | awk '{print $5}')
if [[ $size -ne '0' ]]; then
	if [[ $# -eq 0 ]]; then
		insmod build/acash.ko
#		insmod build/acash.ko mode=0x032000
		echo "QDMA"
	else
		insmod build/acash.ko videodbg=1
		echo "NO QDMA"
	fi
	sleep 1
else
	echo "==================="
	echo " No build/acash.ko "
	echo "==================="
fi

for ((i=0; i<$xlnx_device; i++))
do
	echo $i
	xlnx_bdf=$(lspci -n | grep 9034 | head -n $((i+1)) | tail -n 1 | awk '{print $1}')
	echo $xlnx_bdf
	test_device_addr=0000:$xlnx_bdf
	test_device=${xlnx_bdf/:/}
	test_device=qdma${test_device/./}
	./dftup.sh $test_device_addr $test_device
	sleep 5
	chmod 666 /dev/video*
done
