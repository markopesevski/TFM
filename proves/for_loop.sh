#!/bin/bash
for i in `seq 20 10 1000`;
do
	echo $i
	# sudo ping 192.168.1.200 -c 100 -s $i -q -i 0.01 -w 0.01 >> lwip/lwip.txt
	# sudo ping 192.168.1.200 -c 100 -s $i -q -i 0.01 -w 0.01 >> no-lwip/no-lwip.txt
	sleep 0.5
done
