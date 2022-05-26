#!/bin/bash

set -m

tmp_file='came_pid_schedtrace.txt'
ps -eTlf | grep "camera_sched" | awk '{print $5}' > $tmp_file

buf=()
i=0
while read line
do
	buf[$i]=$line
	((i++))
done < $tmp_file

echo "get camera sched trace id as following:"
echo ${buf[@]}

for pid in ${buf[@]}
do
p_cmd=$p_cmd" -P "$pid
done
p_cmd=$p_cmd" "
echo $p_cmd

date -d "1970-01-01 UTC `echo "$(date +%s)-$(cat /proc/uptime|cut -f 1 -d' ')+0"|bc ` seconds" > poweruptime.txt
#sudo trace-cmd record -e 'sched_wakeup' -e 'sched_switch' -e 'sched_migrate_task' $p_cmd

for ((tmp_i=1; tmp_i<=20;tmp_i++))
do
echo "start to save in "$tmp_i
sudo trace-cmd record -e 'sched_switch' $p_cmd -o "cam_schedtrace_"$tmp_i".dat" &
sleep 300s
trace_pid=`ps -elf | grep "sudo trace-cmd record" | head -n 1 | awk '{print $4}'`
#echo $trace_pid
sudo kill -2 $trace_pid
done

sleep 4s
