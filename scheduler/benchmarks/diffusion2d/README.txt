github.com/subodhvsharma/benchmarks/isp-pred-tests/regression/diffusion2.c
modified
not the same as Diffusion2D of FEVS (which gives empty buffer during dynamic execution for some reason)
solved 2D diffusion equation using grid topology
isp does not support MPI_Sendrecv so they were replaced by MPI_Send and MPI_Recv
