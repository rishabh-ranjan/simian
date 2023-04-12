#!/bin/bash

for n in {1..32}
do
	echo -ne $n'\r'
	simian -n $n ./i_floyd 8 e1 > /dev/null
	simian -n $n ./i_floyd-dl 8 e3 > /dev/null
done
../tsv.sh runlogs/*e1 > e1.tsv
../tsv.sh runlogs/*e3 > e3.tsv

for x in {1..8}
do
	echo -ne $x'\r'
	simian -n 8 ./i_floyd $x e2 > /dev/null
	simian -n 8 ./i_floyd-dl $x e4 > /dev/null
done
../tsv.sh runlogs/*e2 > e2.tsv
../tsv.sh runlogs/*e4 > e4.tsv
