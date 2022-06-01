#./dftup.sh 0000:04:00.0 qdma04000

TEST_DEVICE_ADDR=0000:04:00.0
TEST_DEVICE=qdma04000

function main()
{
    cat /sys/bus/pci/devices/$1/qdma/qmax
    echo 20 > /sys/bus/pci/devices/$1/qdma/qmax
    for i in $(seq 0 8); do
        build/dmactl $2 q add idx $i mode mm dir c2h
        build/dmactl $2 q start idx $i dir c2h desc_bypass_en pfetch_bypass_en
    done
    build/dmactl $2 q add idx 8 mode mm dir h2c
    build/dmactl $2 q start idx 8 dir h2c
    build/dmactl $2 q add idx 16 mode mm dir c2h
    build/dmactl $2 q start idx 16 dir c2h
    # build/dmactl $2 q start idx 8 dir c2h desc_bypass_en pfetch_bypass_en
}

if [[ $# -ne 2 ]]; then
	main $TEST_DEVICE_ADDR $TEST_DEVICE
else
	main $1 $2
fi
