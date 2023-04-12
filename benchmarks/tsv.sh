#!/bin/bash

cut -d: -f1 $1/stats.txt | tr '\n' '\t'
echo
for d in $@
do
	cut -d: -f2 $d/stats.txt | tr '\n' '\t'
	echo
done
