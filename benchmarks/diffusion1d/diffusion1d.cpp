/* FEVS: A Functional Equivalence Verification Suite for High-Performance
 * Scientific Computing
 *
 * Copyright (C) 2009-2010, Stephen F. Siegel, Timothy K. Zirkel,
 * University of Delaware
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

/* 
 * diffusion1d_par.c: parallel 1d-diffusion simulation.
 */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <mpi.h>
#define OWNER(index) ((nprocs*(index+1)-1)/nx)

/* Global parameters */
int nx;        /* number of discrete points including endpoints */
double k;      /* D*dt/(dx*dx) */
int nsteps;    /* number of _time steps */
int wstep;     /* write frame every this many _time steps */
double lbound; /* left fixed boundary value */
double rbound; /* right fixed boundary value */
double *u;     /* temperature function, local */
double *u_new; /* second copy of temperature function, local */
int nprocs;    /* number of processes */
int rank;      /* the rank of this process */
int left;      /* rank of left neighbor */
int right;     /* rank of right neighbor on torus */
int nxl;       /* horizontal extent of one process */
int first;     /* global index for local index 0 */
double *buf;   /* temp. buffer used on proc 0 only */
int print_pos; /* number of cells printed on current line */
int _time = 0;  /* current _time step */

/* Returns the global index of the first cell owned
 * by the process with given rank */
int firstForProc(int rank) {
  return (rank*(nx))/nprocs;
}

/* Returns the number of cells owned by the process
 * of the given rank (excluding ghosts) */
int countForProc(int rank) {
  int a = firstForProc(rank+1);
  int b = firstForProc(rank);

  return a-b;
}

/* Initializes the global variables.
 * Precondition: the configuration parameters have
 * already been set. */
void init_globals() { 
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // nxl: number actual points (incl. end-points)
  // nxl+2: size of array (incl. ghost cells)
  first = firstForProc(rank);  
  nxl = countForProc(rank);
  left = first==0 || nxl==0 ? MPI_PROC_NULL : OWNER(first-1);
  right = first+nxl >= nx || nxl == 0 ? MPI_PROC_NULL : OWNER(first+nxl);
  if(left == MPI_PROC_NULL) nxl--;
  if(right == MPI_PROC_NULL) nxl--;
  u = (double*)malloc((nxl+2)*sizeof(double));
  assert(u);
  u_new = (double*)malloc((nxl+2)*sizeof(double));
  assert(u_new);
  if (rank == 0)
    buf = (double*)malloc((1+nx/nprocs)*sizeof(double));
}

void initialize() {
	/*
  nx = 10;
  nsteps = 3;
  wstep = 10;
	*/
  k = 0.3;
  lbound = rbound = 0.0;
  init_globals();
  for (int i=1; i<=nxl; i++)
    u[i]=100.0;
  if (nx>=1 && rank == OWNER(0))
    u[0] = u_new[0] = lbound;
  if (nx>=1 && rank == OWNER(nx-1))
    u[nxl+1] = u_new[nxl+1] = rbound;
	/*
  if (rank == 0)
    printf("nx=%d, k=%lf, nsteps=%d, wstep=%d\n",
	   nx, k, nsteps, wstep);
		 */
}

/* Prints header for _time step.  Called by proc 0 only */
void print__time_header() {
  //printf("======= Time %d =======\n", _time);
  print_pos = 0;
}

/* Prints one cell.  Called by proc 0 only. */
void print_cell(double value) {
  //printf("%8.2f", value);
  print_pos++;
}

/* Prints the current values of u. */
void write_plain() {
  if (rank != 0) {
    MPI_Send(u+1, nxl, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  } else {
    print__time_header();
    print_cell(lbound); // left boundary
    for (int source = 0; source < nprocs; source++) {
      int count;

      if (source != 0) {
        MPI_Status status;

        MPI_Recv(buf, 1+nx/nprocs, MPI_DOUBLE, source, 0, MPI_COMM_WORLD,
                 &status);
        MPI_Get_count(&status, MPI_DOUBLE, &count);
      } else {
        for (int i = 1; i <= nxl; i++)
          buf[i-1] = u[i];
        count = nxl;
      }
      for (int i = 0; i < count; i++)
        print_cell(buf[i]);
    }
    print_cell(rbound); // right boundary
    //printf("\n");
  }
}

/* exchange_ghost_cells: updates ghost cells using MPI communication */
void exchange_ghost_cells() {
  MPI_Sendrecv(&u[1], 1, MPI_DOUBLE, left, 0,
               &u[nxl+1], 1, MPI_DOUBLE, right, 0,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Sendrecv(&u[nxl], 1, MPI_DOUBLE, right, 0,
               &u[0], 1, MPI_DOUBLE, left, 0,
               MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

/* Updates u_new using u, then swaps u and u_new.
 * Reads the ghost cells in u.  Purely local operation. */
void update() {
  for (int i = 1; i <= nxl; i++)
    u_new[i] = u[i] + k*(u[i+1] + u[i-1] - 2*u[i]);
  double * tmp = u_new; u_new=u; u=tmp;
}

/* Executes the simulation. */
int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);
	nx = atoi(argv[1]);
	nsteps = atoi(argv[2]);
	wstep = atoi(argv[3]);
  initialize();
  write_plain();
  for (_time=1; _time <= nsteps; _time++) {
    exchange_ghost_cells();
    update();
    if (_time%wstep==0)
      write_plain();
  }
  MPI_Finalize();
  free(u);
  free(u_new);
  if (rank == 0)
    free(buf);
  return 0;
}
