#!/bin/bash

for n in {1..32}
do
	echo -ne $n'\r'
	simian -n $n ./i_gausselim e1 > /dev/null
done
../tsv.sh runlogs/*e1 > e1.tsv
