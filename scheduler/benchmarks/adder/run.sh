#!/bin/bash

for n in {4..32}
do
	echo -ne $n'\r'
	simian -n $n ./i_adder > /dev/null
done
