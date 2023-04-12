#!/bin/bash

for n in {1..32}
do
	echo -ne $n'\r'
	simian -n $n ./i_diffusion1d $n 1 1 e1 > /dev/null
done
../tsv.sh runlogs/*e1 > e1.tsv

for t in {1..16}:
do
	echo -ne $t'\r'
	simian -n 4 ./i_diffusion1d 4 $t $t e2 > /dev/null
done
../tsv.sh runlogs/*e2 > e2.tsv
