#!/bin/bash

#simian -n 4 ./i_diffusion2d 2 2 1 e1
#simian -n 6 ./i_diffusion2d 2 3 1	e1
#simian -n 8 ./i_diffusion2d 2 4 1 e1
#simian -n 9 ./i_diffusion2d 3 3 1 e1
#simian -n 12 ./i_diffusion2d 3 4 1 e1
#simian -n 15 ./i_diffusion2d 3 5 1 e1
#simian -n 16 ./i_diffusion2d 4 4 1 e1
#../tsv.sh runlogs/*e1 > e1.tsv

simian -n 4 ./i_diffusion2d 2 2 1 e2
simian -n 4 ./i_diffusion2d 2 2 2 e2
simian -n 4 ./i_diffusion2d 2 2 3 e2
simian -n 4 ./i_diffusion2d 2 2 4 e2
simian -n 4 ./i_diffusion2d 2 2 5 e2
../tsv.sh runlogs/*e2 > e2.tsv

