#!/bin/bash

tmp_file='irq_id.txt'
cat /proc/interrupts | grep "qdma" | awk '{print $1}' > $tmp_file

buf=()
i=0
while read line
do
	buf[$i]=${line%?}
	((i++))
done < $tmp_file

echo "get irq id as following"
echo ${buf[@]}

IRQBALANCE_BANNED_CPUS="000001c3"
for irq_id in ${buf[@]}
do
	echo $irq_id
	affinity=`cat /proc/irq/$irq_id/smp_affinity`
	echo "previous affinity: $affinity"
	while [ "$affinity" != "$IRQBALANCE_BANNED_CPUS" ]
	do
		sudo chmod +666 /proc/irq/$irq_id/smp_affinity
		sudo echo "$IRQBALANCE_BANNED_CPUS" > /proc/irq/$irq_id/smp_affinity
		affinity=`cat /proc/irq/$irq_id/smp_affinity`
		echo "after modification affinity: $affinity"
	done
done
echo "exit"
