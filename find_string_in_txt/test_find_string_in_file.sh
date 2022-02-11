#!/bin/bash
VEHICLE_NAME=$(hostname)
SCHED_CONFIG_DIR=$(pwd)/conf
echo $SCHED_CONFIG_DIR

if [ `grep -c "$(hostname)" $SCHED_CONFIG_DIR/IPC_mutli_camer.conf` -ne '0' ]
then
	echo "!"
else
	echo "$(hostname)"
fi
