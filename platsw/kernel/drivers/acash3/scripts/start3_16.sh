#!/bin/bash

usbtv=$(lsmod | grep -i usbtv | wc -l)
if [[ $usvtv -eq 0 ]]; then
	modprobe usbtv
fi

acash3=$(lsmod | grep -i acash3_pf | wc -l)
if [[ $acash3 -ne 0 ]]; then
	rmmod acash3-pf
fi


xlnx_device=$(lspci -n | grep 9038 | wc -l)
if [[ $xlnx_device -eq 0 ]]; then
	echo "==================="
	echo " No Xilinx Device"
	echo "==================="
	exit
fi

size=$(ls -l bin/acash3-pf.ko | awk '{print $5}')
if [[ $size -ne '0' ]]; then
	if [[ $# -eq 0 ]]; then
		insmod bin/acash3-pf.ko
#		insmod bin/acash3-pf.ko mode=0x032000
		echo "QDMA"
	else
		insmod bin/acash3-pf.ko videodbg=1
		echo "NO QDMA"
	fi
	sleep 1
else
	echo "==================="
	echo " No bin/acash3-pf.ko "
	echo "==================="
fi

#for ((i=0; i<$xlnx_device; i++))
#do
#	echo $i
#	xlnx_bdf=$(lspci -n | grep 9038 | head -n $((i+1)) | tail -n 1 | awk '{print $1}')
#	echo $xlnx_bdf
#	test_device_addr=0000:$xlnx_bdf
#	test_device=${xlnx_bdf/:/}
#	test_device=qdma${test_device/./}
#	./dftup3_16.sh $test_device_addr $test_device
#	sleep 5
#	chmod 666 /dev/video*
#done
