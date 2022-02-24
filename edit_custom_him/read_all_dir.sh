#!/bin/bash

dir=$(ls -l $(pwd) |awk '/^d/ {print $NF}')
for i in $dir
do
	cp edit_custom_hmi.sh "$i/"
	cd "$i/"
	echo $(pwd)
	#add_sched
	./edit_custom_hmi.sh > custom_hmi.txt
	rm edit_custom_hmi.sh
        rm custom_hmi.pb.txt
        mv custom_hmi.txt custom_hmi.pb.txt
	#echo $i
	cd ..
done
