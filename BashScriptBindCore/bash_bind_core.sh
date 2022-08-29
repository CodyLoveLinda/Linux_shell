#!/bin/bash

#DATE=`date '+%Y%m%d-%H%M%S'`
#echo $DATE

#LOG="${DATE}sar.txt"

#LC_ALL=C sar -P ALL 1 100 > $LOG
CPUSET=$2
CMD=$1
taskset -c $CPUSET nohup bash $CMD 2>&1 &
