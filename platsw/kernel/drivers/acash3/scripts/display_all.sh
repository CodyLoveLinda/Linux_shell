sudo insmod ./build/qdma.ko
sleep 1
sudo ../cash/dftup.sh 0000:03:00.0 qdma03000
sleep 1
echo 0 | sudo tee /sys/bus/pci/devices/0000\:03\:00.0/qdma/test_case #do this once to initialize cash
sleep 3
sudo xterm -hold -e apps/cash/display_cash /dev/qdma03000-MM-0 $1 &
sudo xterm -hold -e apps/cash/display_cash /dev/qdma03000-MM-1 $1 &
sudo xterm -hold -e apps/cash/display_cash /dev/qdma03000-MM-2 $1 &
sudo xterm -hold -e apps/cash/display_cash /dev/qdma03000-MM-3 $1 &
sudo xterm -hold -e apps/cash/display_cash /dev/qdma03000-MM-4 $1 &
sudo xterm -hold -e apps/cash/display_cash /dev/qdma03000-MM-5 $1 &
sudo xterm -hold -e apps/cash/display_cash /dev/qdma03000-MM-6 $1 &
sudo xterm -hold -e apps/cash/display_cash /dev/qdma03000-MM-7 $1 &
