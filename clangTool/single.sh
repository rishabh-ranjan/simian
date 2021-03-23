#!/bin/bash

cat "$1" | sed '1s/^/#include "GenerateAssumes.h"\nProcessIfs *ifs = new ProcessIfs();\n/' | sed '/MPI_Init.*;/a int my_rank; MPI_Comm_rank (MPI_COMM_WORLD, &my_rank);' | sed '/MPI_Finalize.*;/a ifs->transferInfoToScheduler(my_rank);' > "i_$1"

