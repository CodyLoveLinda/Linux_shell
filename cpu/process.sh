#!/bin/bash

DATE=`date '+%Y%m%d-%H%M%S'`
echo $DATE

LOG="${DATE}process.txt"

pid=2429

LC_ALL=C pidstat -p ALL 1 10 > $LOG

