#!/bin/bash

#bind chrome to last 4 cpu core
MIN_CPU_FOR_CHROME=$(( $(nproc) -4 ))
MAX_CPU_FOR_CHROME=$(( $(nproc) -1 ))
CPUSETS="$MIN_CPU_FOR_CHROME-$MAX_CPU_FOR_CHROME"

echo $CPUSETS
