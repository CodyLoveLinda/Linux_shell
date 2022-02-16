#!/bin/bash

#sed -i '/echo_red "Error, please check logs."/a \ \ \ exit 1' camer.pb.txt
#num=1
#sed -i '1i drink tea' camer.pb.txt
NUM=1
vsr1=1
cat camer.pb.txt | while read line
do
    #if [ echo $line | grep 'image' ]; then
	#echo $line
    #fi
    vat=`echo $line | grep '/image/'` #找到包含的字符串
    if [ -n "$vat" ]; then
	echo $line
	line=`echo $line | sed '/^$/d'` #去掉空格
    	line=`echo ${vat%/*} | sed  "s/image/image_header/g"` #替换字符串
	echo $line
	NUM=$[$NUM + $vsr1]
	#sed '$1i drink tea'
    else
	    echo $line
    fi
	#echo $line
    #echo $line | sed -e "/image"
    #if [ echo $line | sed '/image/p' ]; then
    	#line=`echo ${line%/*} | sed  "s/image/image_header/g"`
    	#echo $line
    #fi
done
echo $NUM
