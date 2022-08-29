#!/bin/bash

DATE=`date '+%Y%m%d-%H%M%S'`
echo $DATE

LOG="${DATE}sar.txt"

LC_ALL=C sar -P ALL 1 1000000 > $LOG

#taskset -c 3-5 nohup sar -P ALL 1 100 > $LOG 2>&1 &
