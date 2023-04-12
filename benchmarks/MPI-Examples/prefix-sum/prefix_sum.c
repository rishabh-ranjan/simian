/* File: main.c
   Author: Jharrod LaFon
   Date: Spring 2011
   Purpose: Compute the prefix sum of an array
   */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<mpi.h>

//#define ARRAY_SIZE 1048576
int array_size;

int main(int argc, char *argv[])
{
int rank;
int size;

int tmp = MPI_Init(&argc, &argv);
if(tmp != MPI_SUCCESS)
{
    fprintf(stderr, "Unable to initialize MPI!\n");
    return -1;
}

if (argc > 1) {
	array_size = atoi(argv[1]);
} else {
	fprintf(stderr, "Error: expected arg array_size!\n");
}

MPI_Comm_rank(MPI_COMM_WORLD, &rank);
MPI_Comm_size(MPI_COMM_WORLD, &size);

if(array_size % size != 0 && rank == 0)
{
    fprintf(stderr,"Array size must be multiple of mpi job size.\n");
    return -1;
}
MPI_Status status;
int * array = (int *) malloc(sizeof(int) * array_size);
int * chunk = (int *) malloc(sizeof(int) * array_size/size);
int i = 0;

int total_sum = 0;
for(i = 0; i < array_size; i++)
{
    array[i] = rand() % 1024;
    total_sum+=array[i];
}

MPI_Barrier(MPI_COMM_WORLD);
MPI_Scatter(array,array_size/size,MPI_INT,chunk,array_size/size,MPI_INT,0,MPI_COMM_WORLD);
int sum = 0;
int temp = 0;
int key = 1;
for(i = 0; i < array_size/size; i++) {
    sum += chunk[i];
}

/* Fancy stuff to keep the indexes correct */
/* Number of processes participating halves each time */
while(key <= size/2)
{
		int tmp = (rank+1) % key;
    if(tmp == 0) {
				int tmp = rank/key % 2;
        if(tmp == 0)
        {
            MPI_Send(&sum, 1, MPI_INT, rank+key, 0, MPI_COMM_WORLD);
        }
        else
        {
            MPI_Recv(&temp, 1, MPI_INT, rank-key, 0, MPI_COMM_WORLD, &status);
            sum += temp;
        }
		}
		key = 2 * key;
		MPI_Barrier(MPI_COMM_WORLD);
}

if(rank == size-1)
{
    //fprintf(stderr,"Total: %d\n",sum);
    //fprintf(stderr,"Correct Sum: %d\n",total_sum);
}
free(array);
free(chunk);
MPI_Finalize();
return 0;

}
