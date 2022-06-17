#!/bin/bash

if [ -d "Top/" ]; then
	echo "have folder"
else
	echo "no folder"
fi

  camera_irq_dir="$HOME/platsw/kernel/drivers/acash3"
  if [ -d "$camera_irq_dir" ]; then
    sudo autoxmount -m
    cp /autox-sz/mmzhang_cash/acash3_irq_bind_core.sh $camera_irq_dir/
  fi

  camera_irq_file="$HOME/platsw/kernel/drivers/acash3/acash3_irq_bind_core.sh"
  if [ -f "$camera_irq_file" ]; then
      #echo "$camera_irq_file is exit"
      bash $camera_irq_file > camera_irq_log
  else
      info "$camera_irq_file is not exit"
  fi
