#!/usr/bin/env bash

    cash_exist=$(ls /dev | grep qdma | grep MM-9 | wc -l)
    if [[ $cash_exist -eq 0 ]]; then
        echo "No CAN device found"
        return 0
    fi  

    # get CAN start index from config file
    CASH3_CAN_QUEUE_START=$(cat ../../../kernel/drivers/acash3/include/linux/cash3_conf.h | grep CASH3_CAN_QUEUE_START | sed -e's/\t/ /g' | cut -d ' ' -f 3- | sed -e's/ //g')

    # flag to rebuild library
    REBUILD=0

    # get current ID
    PRV_QDMA_ID1=$(cat ../include/axcan_util.h | grep  AXCAN_DEV_1_NAME | cut -d '"' -f 2 | sed s/qdma//g | sed s/-MM-//g)
    PRV_QDMA_ID2=$(cat ../include/axcan_util.h | grep  AXCAN_DEV_2_NAME | cut -d '"' -f 2 | sed s/qdma//g | sed s/-MM-//g)

    # get new ID
    QDMA_ID1=$(lspci | grep 9038 | sed -n 1p | cut -d ' ' -f 1 | tr -d : | tr -d . )
    QDMA_ID2=$(lspci | grep 9038 | sed -n 2p | cut -d ' ' -f 1 | tr -d : | tr -d . )

    # update device id 1
    if [ $QDMA_ID1 -z ]; then
        sed -i 's/AXCAN_DEV_1_NAME "qdma.....-MM-"/AXCAN_DEV_1_NAME "qdmaffff1-MM-"/g' ../include/axcan_util.h 
        echo "no dev1"
    elif [ "$PRV_QDMA_ID1" == "$QDMA_ID1" ]; then
        echo "dev1 not change"
    else
        sed -i 's/AXCAN_DEV_1_NAME "qdma.....-MM-"/AXCAN_DEV_1_NAME "qdma'${QDMA_ID1}'-MM-"/g' ../include/axcan_util.h 
        echo "update dev1: $(cat ../include/axcan_util.h | grep  AXCAN_DEV_1_NAME )"
        REBUILD=1
    fi

    # update device id 2
    if [ $QDMA_ID2 -z ]; then
        sed -i 's/AXCAN_DEV_2_NAME "qdma.....-MM-"/AXCAN_DEV_2_NAME "qdmaffff2-MM-"/g' ../include/axcan_util.h 
        echo "no dev2"
    elif [ "$PRV_QDMA_ID2" == "$QDMA_ID2" ]; then
        echo "dev2 not change"
    else
        sed -i 's/AXCAN_DEV_2_NAME "qdma.....-MM-"/AXCAN_DEV_2_NAME "qdma'${QDMA_ID2}'-MM-"/g' ../include/axcan_util.h 
        echo "update dev2: $(cat ../include/axcan_util.h | grep  AXCAN_DEV_2_NAME )"
        REBUILD=1
    fi    

    # check if axcan library alread exist    
    if [ -f ../build/bin/libaxcan.so ]; then
        echo "axcan library already exist"
    else
        echo "axcan library not exist"
        REBUILD=1
    fi

    # build library and export if qdma id is not match or library is not exis
    if [ $REBUILD -eq 1 ]; then
        cd ..
        make clean
        make
        echo "export LD_LIBRARY_PATH=`pwd`/build/bin" > ../../tools/canapp/setenv.sh
        cd scripts
    fi

    # mapping qdma device to axcan device
    d=0
    count=0;
    for i in $(seq ${CASH3_CAN_QUEUE_START} $(($CASH3_CAN_QUEUE_START+5))); 
    do
        for q in $(ls /dev | grep qdma | grep MM-${i}); 
        do
            qdma_device=/dev/${q}
            x=$((count+d*6))
            can_device=/dev/axcan${x}
            $(sudo rm -rf "$can_device")
            $(sudo ln -s "$qdma_device" "$can_device")
            echo $(ls -l "$can_device")
            d=$((d+1))
        done
        count=$((count+1))
        d=0
    done

