#!/bin/bash

tmp_file='came_pid.txt'
ps -elf | grep "camera_sched" | head -n 2 | awk '{print $4}' > $tmp_file

ps -eTlf > ps.txt

buf=()
i=0
while read line
do
	buf[$i]=$line
	((i++))
done < $tmp_file

tmp_video_file="video_wd_pid.txt"
ps -elf | grep "video watchdog" | head -n 32 | awk '{print $4}' > $tmp_video_file

video_buf=()
i=0
while read line
do
	video_buf[$i]=$line
	((i++))
done < $tmp_video_file

echo "get camera pid as following:"
echo ${buf[@]}

cycletime=4000
for pid in ${buf[@]}
do
ret_file="syscnt_cam_"$pid".txt"
runlat_file="runqlat_cam_"$pid".txt"
funclat_file="select_funclat"$pid".txt"
sudo python3 /home/autox/luojiaxing/bcc/tools/funclatency.py -m -i 1 -d $cycletime -T -p $pid kern_select  > $funclat_file &
sudo python3 /home/autox/luojiaxing/bcc/tools/syscount.py --top=20 --pid=$pid --duration=$cycletime -L -m > $ret_file &
sudo python3 /home/autox/luojiaxing/bcc/tools/runqlat.py -p $pid -mT -L 1 $cycletime > $runlat_file &
done

for vpid in ${video_buf[@]}
do
wd_rqlat_file="video_wd_rqunlat_"$vpid".txt"
sudo python3 /home/autox/luojiaxing/bcc/tools/runqlat.py -p $vpid -mT -L 1 $cycletime > $wd_rqlat_file &
done
