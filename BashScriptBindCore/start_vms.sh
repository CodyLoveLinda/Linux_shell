#!/usb/bin/env bash

MIN_CPU_FOR_VMS=$(( $(nproc) -4 ))
MAX_CPU_FOR_VMS=$(( $(nproc) -1 ))
CPUSETS="$MIN_CPU_FOR_VMS-$MAX_CPU_FOR_VMS"

DATE=`date '+%Y%m%d-%H%M%S'`
LOG="${DATE}vms.txt"

taskset -c $CPUSETS nohup python3 /app/vehicle-agent/deploy/systemd/daemon.py > $LOG 2>&1 &
