#!/bin/bash
#  ******************************************************************************
#  *
#  * Copyright AutoX Inc. 2022 All Rights Reserved.
#  *
#  * mingmingzhang, mingmingzhang@autox.ai
#  *****************************************************************************
PWD=`echo $(pwd) | sed 's/scripts/configuration/'`

dir=$(ls -l $PWD |awk '/^d/ {print $NF}')

flag=0
key=
process_group_name=

str_camera=`echo "  key: \"Camera\""`
str_camera_top=`echo "  key: \"Top Camera\""`
str_camera_bind=`echo "  key: \"Blindspot Camera\""`
str_camera_sched=`echo "process_group: \"camera_sched\""`
str_loc=`echo "  key: \"Localization\""`
str_loc_sched=`echo "process_group: \"localization_sched\""`
str_lidar=`echo "  key: \"Lidar\""`
str_lidar_cpu2=`echo "  key: \"Lidar CPU2\""`
str_lidar_sched=`echo "process_group: \"lidar_sched\""`
str_Qt=`echo "  key: \"HesaiQt\""`
str_Qt_=`echo "  key: \"Hesai Qt\""`
str_Qt_sched=`echo "process_group: \"lidar_sched\""`
str_front_radar=`echo "  key: \"Front Radar\""`
str_radar=`echo "  key: \"Radar\""`
str_radar_sched=`echo "process_group: \"radar_sched\""`
str_exmonitor=`echo "  key: \"ExMonitor\""`
str_exmonitor_sched=`echo "process_group: \"monitor_sched\""`

ACTION=$1
OPTION=$2
VEHICLE=$3

function exit_usage() {
  echo -e "\
  Usage:  ./custom_hmi_config.sh <check|edit|delete> [options]
  Actions:
    check, check the vehicle sched config and print the vehicle's name which not have sched config
    edit,  add the sched config to the vehicle which not have sched config
    delete, delate the vehicle sched config
  Options:
    -a, for all the vehicles in the dir
    -v, for one vehicle
  Example:
    ./custom_hmi_config.sh check -a
    ./custom_hmi_config.sh check -v pacifica-cn-86
  "
}

function error() {
  >&2 echo -e "\033[091mERROR: $*\033[0m"
  LAST_ERROR_STRING="$1"
}

function read_custom_hmi_config() {
  custom_hmi_filename=$2
  action=$1
  vehicle=$3
  IFS=''
  cat $custom_hmi_filename | while read -r line || [[ -n ${line} ]]
  do
    vat=`echo $line | grep 'process_group'`
    if [ -n "$vat" ]; then
      flag=5
      process_group_name=`echo $vat | sed 's/^[ \t]*//g'`
    fi
    vat=`echo $line | grep 'cyber_modules'`
    if [ -n "$vat" ]; then
      flag=1
    fi
    if [ $flag -eq 1 ]; then
      vat=`echo $line | grep 'key'`
        if [ -n "$vat" ];then
          flag=2
          case $vat in
              $str_camera)
                  key=$str_camera_sched
                  ;;
              $str_camera_top)
                  key=$str_camera_sched
                  ;;
              $str_camera_bind)
                  key=$str_camera_sched
                  ;;
              $str_loc)
                  key=$str_loc_sched
                  ;;
              $str_lidar)
                  key=$str_lidar_sched
                  ;;
              $str_lidar_cpu2)
                  key=$str_lidar_sched
                  ;;
              $str_Qt)
                  key=$str_Qt_sched
                  ;;
              $str_Qt_)
                  key=$str_Qt_sched
                  ;;
              $str_radar)
                  key=$str_radar_sched
                  ;;
              $str_front_radar)
                  key=$str_radar_sched
                  ;;
              $str_exmonitor)
                  key=$str_exmonitor_sched
                  ;;
              *)
                  key=
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
          if [ -n "$key" ]; then
              case $action in
                  edit)
                      printf "    $key\n"
                      key=
                      ;;
                   check)
                      error "$vehicle do not have-<$key>-"
                      ;;
                   delete)
                      flag=4
                      ;;
              esac
          fi
          flag=4
      fi
    fi
    if [ $flag -eq 5 ]; then
      case $action in
        edit)
          echo $line
          flag=0
          ;;
        check)
          if [ "$key" != "$process_group_name" ]; then
            error "$vehicle's <$process_group_name> error, should be <$key>"
          fi
          flag=0
          ;;
        delete)
          flag=0
          ;;
      esac
    else
      if [ "$action" != "check" ]; then
        echo $line
      fi
    fi
  done
  IFS=' '
}

function read_all_dir() {
  custom_hmi_filename="$PWD/$2/custom_hmi.pb.txt"
  if [ -f "$custom_hmi_filename" ]; then
    read_custom_hmi_config $1 $custom_hmi_filename $2
  else
    error "The vehicle $2 do not have file: custom_hmi.pb.txt"
  fi
}

function move_custom_hmi_file() {
  action=$1
  vehicle=$2
  custom_hmi_filename="$PWD/$vehicle/custom_hmi.pb.txt"
  if [ -f "$custom_hmi_filename" ]; then
      mv custom_hmi.pb.txt.tmp ./$vehicle/custom_hmi.pb.txt
      echo "$action $vehicle's sched config file success!"
  else
      echo "$action $vehicle's sched config file failed!"
  fi
}

function check() {
  case $OPTION in
    -a)
      echo "check all vehicles sched config"
      for i in $dir
      do
        if [ "$i" != "byd-cn-4" ] && [ "$i" != "pacifica-cn-44" ]; then
          read_all_dir 'check' $i
        fi
      done
      ;;
    -v)
      echo "check $VEHICLE's sched config "
      read_all_dir 'check' $VEHICLE
      ;;
     *)
      exit_usage
  esac
}

function edit() {
  case $OPTION in
    -a)
      echo "edit all vehicles's sched config"
      for i in $dir
      do
        if [ "$i" != "byd-cn-4" ] && [ "$i" != "pacifica-cn-44" ]; then
            read_all_dir 'edit' $i > custom_hmi.pb.txt.tmp
            move_custom_hmi_file 'edit' $i
        fi
      done
      ;;
    -v)
        echo "edit the $VEHICLE's sched config"
        read_all_dir 'edit' $VEHICLE > custom_hmi.pb.txt.tmp
        move_custom_hmi_file 'edit' $VEHICLE
        ;;
     *)
        exit_usage
    esac
}

function delete() {
  case $OPTION in
    -a)
      echo "delete all vehicle's sched config"
      for i in $dir
      do
        if [ "$i" != "byd-cn-4" ] && [ "$i" != "pacifica-cn-44" ]; then
          read_all_dir 'delete' $i > custom_hmi.pb.txt.tmp
          move_custom_hmi_file 'delete' $i
        fi
      done
      ;;
    -v)
      echo "delete $VEHICLE's sched config"
      read_all_dir 'delete' $VEHICLE > custom_hmi.pb.txt.tmp
      move_custom_hmi_file 'delete' $VEHICLE
      ;;
    *)
      exit_usage
    esac
}

case $ACTION in
  check)
    check
    ;;
  edit)
    edit
    ;;
  delete)
    delete
    ;;
  -h)
    exit_usage
    ;;
  *)
    error "Unknown action $ACTION!"
    exit_usage
    ;;
esac
