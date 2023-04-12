/* FEVS: A Functional Equivalence Verification Suite for High-Performance
 * Scientific Computing
 *
 * Copyright (C) 2004-2010, Stephen F. Siegel, Timothy K. Zirkel,
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
 *
 * Simple solution to 2-d Laplace equation by Jacobi Iteration.
 * Each process contains n rows of the grid, plus 2 rows
 * for ghost cells or boundary celss.
 *
 * To compile and execute:
 *    mpicc laplace.c
 *    mpirun -np 8 a.out > out.txt
 */

#include <stdlib.h>
#include <stdio.h>
#include "mpi.h"
#define SQUARE(x) ((x)*(x))

#define nxlg    5    // number of x coordinates (including boundary)
#define nylg    4    // number of rows per process (including 2 extras)
#define EPSILON 5.0  // total error tolerance
int nsteps;

int nprocs, rank, upper, lower;
double u1[nylg][nxlg];
double u2[nylg][nxlg];
double (*old)[nxlg] = u1;
double (*_new)[nxlg] = u2;

void initdata();
void jacobi(double);
void print_frame(int, double (*grid)[nxlg]);

int main(int argc,char *argv[]) {
  MPI_Init(&argc, &argv);
	nsteps = atoi(argv[1]);
  initdata();
  upper = (rank < nprocs - 1 ? rank + 1 : MPI_PROC_NULL);
  lower = (rank > 0 ? rank - 1 : MPI_PROC_NULL);
  print_frame(0, old);
  jacobi(EPSILON);
  MPI_Finalize();
}

void jacobi(double epsilon) {
  double global_error = epsilon;
  double error;
  int row, col, time;
  double (*tmp)[nxlg];
  MPI_Status status;

  time = 0;
  //while (global_error >= epsilon) {
	for (int i = 0; i < nsteps; ++i) {
		MPI_Send(old[nylg-2], nxlg, MPI_DOUBLE, upper, 0, MPI_COMM_WORLD);
		MPI_Recv(old[0], nxlg, MPI_DOUBLE, lower, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		/*
    MPI_Sendrecv(old[nylg-2], nxlg, MPI_DOUBLE, upper, 0, old[0],
		 nxlg, MPI_DOUBLE, lower, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		 */
		MPI_Send(old[1], nxlg, MPI_DOUBLE, lower, 0, MPI_COMM_WORLD);
		MPI_Recv(old[nylg-1], nxlg, MPI_DOUBLE, upper, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		/*
    MPI_Sendrecv(old[1], nxlg, MPI_DOUBLE, lower, 0, old[nylg-1],
		 nxlg, MPI_DOUBLE, upper, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		 */
    error = 0.0;
    for (row = 1; row < nylg-1; row++) {
      for (col = 1; col < nxlg-1; col++) {
	_new[row][col] = (old[row-1][col]+old[row+1][col]+
			 old[row][col-1]+old[row][col+1])*0.25;
	error += SQUARE(old[row][col] - _new[row][col]);
      }
    }
    MPI_Allreduce(&error, &global_error, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
    time++;
    print_frame(time, _new);
    if (rank == 0) {
      //printf("Current error at time %d is %2.5f\n\n", time, global_error);
    }
    tmp = _new; _new = old; old = tmp;
  }
}

void initdata() {
  int row, col;

  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  for (row = 0; row < nylg; row++)
    for (col = 0; col < nxlg; col++)
      old[row][col] = _new[row][col] = 0;
  if (rank == 0)
    for (col = 0; col < nxlg; col++)
      old[0][col] = _new[0][col] = 10;
  if (rank == nprocs-1)
    for (col = 0; col < nxlg; col++)
      old[nylg-1][col] = _new[nylg-1][col] = 10;
  if (rank == 0) {
    //printf("nprocs  = %d\n", nprocs);
    //printf("nxlg    = %d\n", nxlg);
    //printf("nylg    = %d\n", nylg);
    //printf("epsilon = %2.5f\n", EPSILON);
    //printf("\n\n\n");
    fflush(stdout);
  }
}

void print_frame(int time, double (*grid)[nxlg]) {
  int row, col, proc;
  int top = nprocs*(nylg-2)+1;
  double rbufx[nxlg];
  MPI_Status status;

  if (rank != 0) {
    if (rank == nprocs-1)
      MPI_Send(grid[nylg-1], nxlg, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
    for (row = nylg-2; row >= 1; row--)
      MPI_Send(grid[row], nxlg, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
  }
  else {
    //printf("\n\ntime=%d:\n\n", time);
    //printf("\n");
    //printf("   +");
    //for (col = 0; col < nxlg; col++) printf("--------");
    //printf("+\n");
    for (proc = nprocs-1; proc >= 0; proc--) {
      if (proc == nprocs-1) row = nylg-1; else row = nylg-2;
      while (row >= 1 || (proc == 0 && row >= 0)) {
	//printf("%3d|", top);
	if (proc != 0)
	  MPI_Recv(rbufx, nxlg, MPI_DOUBLE, proc, 0, MPI_COMM_WORLD, &status);
	else
	  for (col = 0; col < nxlg; col++) rbufx[col] = grid[row][col];
	//for (col = 0; col < nxlg; col++) printf("%8.2f", rbufx[col]);
	//printf("|%3d", top);
	//printf("\n");
	top--;
	row--;
      }
    }
    //printf("   +");
    //for (col = 0; col < nxlg; col++) printf("--------");
    //printf("+\n");
    //printf("\n");
    fflush(stdout);
  }
}
