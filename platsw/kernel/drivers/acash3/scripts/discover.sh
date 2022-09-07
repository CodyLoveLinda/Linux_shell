#used to discover the rebooted FPGA
#usage example, sudo ../cash/discover.sh 0000\:03\:00.0
#driver needs to be reloaded after this

if [ "$#" -ne 1 ]; then
	echo $0 '<TEST_DEVICE_ADDR>'
	echo "sudo ./discover.sh 0000\:03\:00.0"
else
	# rmmod qdma
	x=$1
	echo "$x"
	sudo echo "1" > /sys/bus/pci/devices/$x/remove
	sleep 1
	sudo echo "1" > /sys/bus/pci/rescan
	sudo setpci -s $1 COMMAND=0x0407
	# insmod $2
fi
