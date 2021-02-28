#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
int N, L, M;
#if 0
/* define N, L, M before compiling */
#define N 7
#define L 10
#define M 15
#endif
#define comm MPI_COMM_WORLD

/* 
   To aboid any reply channel based communication
   give number of processes as N+1
 */


void printMatrix(int numRows, int numCols, double *m) {
  int i, j;
  for (i = 0; i < numRows; i++) {
    for (j = 0; j < numCols; j++) {
      printf("%6.1f ", m[i*numCols + j]);
    }
    printf("\n");
  }
  printf("\n");
  fflush(stdout);
}

// computation
#if 0
void vecmat(double vector[L], double matrix[L][M], double result[M]) {
  int j, k;
  for (j = 0; j < M; j++)
    for (k = 0, result[j] = 0.0; k < L; k++)
      result[j] += vector[k]*matrix[k][j];
}
#endif

int main(int argc, char *argv[]) {
  int rank, nprocs, i, j;
  MPI_Status status;
  
  MPI_Init(&argc, &argv);
	N = atoi(argv[1]);
	L = atoi(argv[2]);
	M = atoi(argv[3]);
  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &nprocs);
  if (rank == 0) {
    double a[N][L], b[L][M], c[N][M], tmp[M];
    int count;

    /* read a, b somehow */
    MPI_Bcast(b, L*M, MPI_DOUBLE, 0, comm);
    for (count = 0; count < nprocs-1 && count < N; count++)
      MPI_Send(&a[count][0], L, MPI_DOUBLE, count+1, count+1, comm);
    for (i = 0; i < N; i++) {
      MPI_Recv(tmp, M, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &status);
      // for (j = 0; j < M; j++) c[status.MPI_TAG-1][j] = tmp[j];
      if (count < N) {
	MPI_Send(&a[count][0], L, MPI_DOUBLE, status.MPI_SOURCE, count+1, comm);
	count++;
      }
    }
    for (i = 1; i < nprocs; i++) MPI_Send(NULL, 0, MPI_INT, i, 0, comm);
    // printMatrix(N, M, &c[0][0]);
  } else {
    double b[L][M], in[L], out[M];

    MPI_Bcast(b, L*M, MPI_DOUBLE, 0, comm);
    while (1) {
      MPI_Recv(in, L, MPI_DOUBLE, 0, MPI_ANY_TAG, comm, &status);
      if (status.MPI_TAG == 0) break;
      //vecmat(in, b, out); // some computation 
      MPI_Send(out, M, MPI_DOUBLE, 0, status.MPI_TAG, comm);
    }
  }
  MPI_Finalize();
  return 0;
}
    
