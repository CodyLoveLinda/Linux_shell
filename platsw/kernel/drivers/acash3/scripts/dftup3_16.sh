#./dftup.sh 0000:04:00.0 qdma04000

TEST_DEVICE_ADDR=0000:04:00.0
TEST_DEVICE=qdma04000

function main()
{
	cat /sys/bus/pci/devices/$1/qdma/qmax
	echo 28 > /sys/bus/pci/devices/$1/qdma/qmax
	# 16 video q, 1 cmd q, 6 CAN c2h q
	for i in $(seq 0 22); do
		bin/dma-ctl $2 q add idx $i mode mm dir c2h
		bin/dma-ctl $2 q start idx $i dir c2h desc_bypass_en pfetch_bypass_en
	done
	# 1 event q, 6 CAN h2c q
	for i in $(seq 16 22); do
		bin/dma-ctl $2 q add idx $i mode mm dir h2c
		bin/dma-ctl $2 q start idx $i dir h2c
	done

	bin/dma-ctl $2 q add idx 24 mode mm dir bi	# test q
	bin/dma-ctl $2 q start idx 24 dir bi
	# bin/dma-ctl $2 q start idx 16 dir c2h desc_bypass_en pfetch_bypass_en
}

if [[ $# -ne 2 ]]; then
	main $TEST_DEVICE_ADDR $TEST_DEVICE
else
	main $1 $2
fi
