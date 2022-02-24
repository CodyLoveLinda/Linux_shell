#!/bin/bash

flag=0
key=
PRINT=0

str_camer=`echo "  key: \"Camera\""`
str_camer_sched=`echo "process_group: \"camera_sched\""`
str_loc=`echo "  key: \"Localization\""`
str_loc_sched=`echo "process_group: \"high_priority_sched\""`
str_lidar=`echo "  key: \"Lidar\""`
str_lidar_sched=`echo "process_group: \"lidar_sched\""`
str_Qt=`echo "  key: \"HesaiQt\""`
str_Qt_sched=`echo "process_group: \"lidar_sched\""`
str_radar=`echo "  key: \"Front Radar\""`
str_radar_sched=`echo "process_group: \"lidar_sched\""`
str_arry=_sched
#echo $str_camer_sched
IFS=''
function add_sched {
    cat custom_hmi.pb.txt | while read -r line
    do
        
        vat=`echo $line | grep '_sched'`
        echo $vat
        if [ -n "$vat"]; then
            flag=4
        fi
        #for strline in ${str_arry[@]}
	    #do
		   #echo $strline
           #flag=4
	    #done
        vat=`echo $line | grep 'cyber_modules'`
        if [ -n "$vat" ]; then
	    flag=1
        fi
        if [ $flag -eq 1 ]; then 
	        vat=`echo $line | grep 'key'`
	        if [ -n "$vat" ];then
		        flag=2
		        #if [ $vat != $str_camer ]; then
		    	    #echo "t0"
		        #fi
		        case $vat in
			        $str_camer)
				        key=$str_camer_sched
				        ;;
			        $str_loc)
				        key=$str_loc_sched
				        ;;
			        $str_lidar)
				        key=$str_lidar_sched
				        ;;
			        $str_Qt)
				        key=$str_Qt_sched
				        ;;
			        $str_radar)
				        key=$str_radar_sched
				        ;;
			        *)
				        ;;
		        esac
    	    fi
        elif [ $flag -eq 2 ]; then
	        vat=`echo $line | grep '.dag'`
	        if [ -n "$vat" ]; then
		        flag=3
	        fi
        elif [ $flag -eq 3 ]; then
	        vat=`echo $line | grep '}'`
	        if [ -n "$vat" ]; then
		        printf "    $key\n"
		        flag=4
	        fi
        fi
        echo $line
    done
}

add_sched
