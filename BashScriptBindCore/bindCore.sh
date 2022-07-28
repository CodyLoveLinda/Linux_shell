#!/bin/bash

MIN_CPU_FOR_CHROME=$(( $(nproc) -4 ))
MAX_CPU_FOR_CHROME=$(( $(nproc) -1 ))
CPUSETS="$MIN_CPU_FOR_CHROME-$MAX_CPU_FOR_CHROME"

taskset -p $CPUSETS ./home/mingmingzhang/project/linux_shell/BashScriptBindCore/cycleRun.sh &
