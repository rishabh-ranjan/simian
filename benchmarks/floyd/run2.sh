#!/bin/bash

simian -b -n 8 ./i_floyd 8 p2
simian -b -n 16 ./i_floyd 8 p2
simian -b -n 32 ./i_floyd 8 p2 
simian -b -n 64 ./i_floyd 8 p2 

../tsv.sh runlogs/*p2 > p2.tsv
