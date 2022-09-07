#!/bin/bash
tempfile=tmp.txt
ps aux | grep wine | awk '{print $2}' | head -n 20 > $tempfile

buf=()
i=0
while read line
do
        buf[$i]=$line
        buf_org_pss[$i]="0"
        ((i++))
done < $tempfile
rm -rf $tempfile

#if [ $# -ne 1 ]; then
#  echo "Usage: `basename $0` process_name"
#  exit 1
#fi

#APPNAME=$1
#PROC="`ps -ef | grep "$APPNAME" | grep -v "grep" | grep -v "awk" | grep -v $0 | awk '{print $2}'`"
#PROC=51917

for pid in ${buf[@]}
do
	kill -9 $pid
done
