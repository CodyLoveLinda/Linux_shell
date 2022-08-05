#!/usr/bin/env bash
tempfile="pid.memorytop"

function error() {
  >&2 echo -e "\033[091m$*\033[0m"
  LAST_ERROR_STRING="$1"
}

ps aux | sort -rn -k +4 | awk '{print $2}' | head -n 20 > $tempfile

buf=()
buf_org_pss=()
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
j=0
for pid in ${buf[@]}
do
	SMAPS="/proc/$pid/smaps_rollup"
	buf_org_pss[$j]=`cat $SMAPS | grep "Pss:" | awk '{print $2}' | head -n 1`
	((j++))
done 
j=0

#if [ -z $PROC ]; then
#  echo "invalid process_name"
#  exit 1
#fi

#SMAPS="/proc/$PROC/smaps"
#STATUS="/proc/$PROC/status"
#echo "proc ---$PROC----"
#OLDHEAP="0"
while :
do
  DATE=`LC_ALL=\"C\" date`
  echo "-----------$DATE--------------"
  for pid in ${buf[@]}
  do
	SMAPS="/proc/$pid/smaps_rollup"
        pss=`cat $SMAPS | grep "Pss:" | awk '{print $2}' | head -n 1`
	name=`ps aux | grep $pid | awk '{print $11}' | awk -F '/' '{print $NF}' |  head -n 1`
  	if [ $pss -gt ${buf_org_pss[$j]} ]; then
		inc=`expr $pss - ${buf_org_pss[$j]}`
		if [ $inc -gt 500000 ]; then
    			error "$DATE $pid $name pss +`expr $pss - ${buf_org_pss[$j]}` to $pss kb"
		else
			echo "$DATE $pid $name pss +`expr $pss - ${buf_org_pss[$j]}` to $pss kb"
		fi
  	fi
	((j++))	
  done
  j=0
  echo
  echo
  sleep 60
  #HEAP="`cat $STATUS | grep "VmData" | awk '{print $2}'`"
  #HEAP=`cat $SMAPS | grep -A 5 "heap" | grep "Rss" | awk '{print $2}'`
  #echo "HEAP $HEAP  OLDHEAP $OLDHEAP"
  #if [ $HEAP -lt $OLDHEAP ]; then
  #  echo "$DATE HEAP -`expr $OLDHEAP - $HEAP` to $HEAP kb"
  #  OLDHEAP=$HEAP
  #elif [ $HEAP -gt $OLDHEAP ]; then
  #  echo "$DATE HEAP +`expr $HEAP - $OLDHEAP` to $HEAP kb"
  #  OLDHEAP=$HEAP
  #fi
  #sleep 1
done

