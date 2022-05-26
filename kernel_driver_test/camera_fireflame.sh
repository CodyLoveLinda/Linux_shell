#!/bin/bash

sudo perf script -i perf.data > out.perf
../FlameGraph/stackcollapse-perf.pl out.perf > out.floded
../FlameGraph/flamegraph.pl out.floded > cp.svg

exit
tmp_file='came_pid.txt'
ps -elf | grep "camera_sched" | head -n 2 | awk '{print $4}' > $tmp_file

ps -elf > ps.txt

buf=()
i=0
while read line
do
	buf[$i]=$line
	((i++))
done < $tmp_file

echo "get camera pid as following:"
echo ${buf[@]}

for pid in ${buf[@]}
do
ret_file="syscnt_cam_"$pid".txt"
sudo python3 /home/autox/luojiaxing/bcc/tools/syscount.py --top=20 --pid=$pid --duration=360 -L -m > $ret_file &
done
