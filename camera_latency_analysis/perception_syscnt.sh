#!/bin/bash

tmp_file='perception_pid.txt'
ps -elf | grep "perception_main_sched" | head -n 1 | awk '{print $4}' > $tmp_file

ps -eTlf > ps.txt

buf=()
i=0
while read line
do
	buf[$i]=$line
	((i++))
done < $tmp_file

echo "get perception pid as following:"
echo ${buf[@]}

cycletime=600
for pid in ${buf[@]}
do
ret_file="syscnt_perception_"$pid".txt"
runlat_file="runqlat_perception_"$pid".txt"
sudo python3 /home/autox/luojiaxing/bcc/tools/syscount.py --top=20 --pid=$pid --duration=$cycletime -L -m > $ret_file &
sudo python3 /home/autox/luojiaxing/bcc/tools/runqlat.py -p $pid -mT 1 $cycletime > $runlat_file &
done
