#!/bin/bash

for n in {1..32}
do
	echo -ne $n'\r'
	simian -n $n ./i_heat e1 > /dev/null
	#simian -n $n ./i_heat-dl e2 > /dev/null
done
../tsv.sh runlogs/*e1 > e1.tsv
#../tsv.sh runlogs/*e2 > e2.tsv

