#!/bin/bash

simian -n 4 ./i_diffusion2d 2 2 2 m1
simian -n 4 ./i_diffusion2d 2 2 4 m1
simian -n 4 ./i_diffusion2d 2 2 8 m1
simian -n 4 ./i_diffusion2d 2 2 16 m1

../tsv.sh runlogs/*m1 > m1.tsv
