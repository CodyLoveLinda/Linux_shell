#!/bin/bash

MIN_CPU_FOR_CHROME=$(( $(nproc) -4 ))
MAX_CPU_FOR_CHROME=$(( $(nproc) -1 ))
CPUSETS="$MIN_CPU_FOR_CHROME-$MAX_CPU_FOR_CHROME"

#taskset -c 3 bash cycleRun.sh &

#taskset -c 3 nohup bash cycleRun.sh &

taskset -c 3-5 nohup python3 pythonWhileTrue.py > mmzhang.log 2>&1 &
