#!/bin/sh

for i in $(seq 1 40)  
do   
    #echo $(expr $i \* 3 + 1);
    ps -C test -o pid,pri,cmd,time,psr >>psinfo.log 2>&1
    sleep 2;  
done

#for((i=1;i<=10;i++)) 
#do   
#    echo $(expr $i \* 3 + 1);  
#done 

#for((i = 0; i < 40; i++))
#do
#    ps -C test -o pid,pri,cmd,time,psr >>psinfo.log 2>&1
#    sleep 2;
#done
