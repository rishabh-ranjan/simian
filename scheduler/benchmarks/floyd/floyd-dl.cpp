/*
 * (c) copyright 1998,1999 by Vrije Universiteit, Amsterdam, The Netherlands.
 * For full copyright and restrictions on use see the file COPYRIGHT.
 */

/*
 * asp.c:
 * 	All-pairs shortest path implementation based on Floyd's
 * 	algorithms. The distance matrix's rows are block-striped
 * 	across all processors. This allows for pipelining, but leads
 * 	to flow-control problems.
 */

/*
 * changed by xueruini to use in windows. (only bcast)
 * 
 * run: mpiexec -n xx asp.exe [row_number]
 *
 */

#include "stdlib.h"
#include "stdio.h"
#include "mpi.h"

//#include "magpie.h"
/* magpie.h only used for demonstrating statistics */

/******************** ASP stuff *************************/

static int **tab;
static int *spare;

static int size,rank,n;

static void
get_bounds(int *lb, int *ub)
{
    int nlarge, nsmall;
    int size_large, size_small;

    nlarge = n % size;
    nsmall = size - nlarge;
    
    size_small = n / size;
    size_large = size_small + 1;

    if (rank < nlarge) {            /* I'll  have a large chunk */
	*lb = rank * size_large;
	*ub = *lb + size_large;
    } else {
	*lb = nlarge * size_large + (rank - nlarge) * size_small;
	*ub = *lb + size_small;
    }
}

static int owner(int k){
    int nlarge, nsmall;
    int size_large, size_small;

    nlarge = n % size;
    nsmall = size - nlarge;
    size_small = n / size;
    size_large = size_small + 1;

    if ( k < (size_large * nlarge) ){
      return (k / size_large);
    }
    else {
      return (nlarge + (k-(size_large * nlarge))/size_small);
    }
}

static void
init_tab(int n, int lb, int ub, int ***tabptr)
{
    int **tab;
    int i, j;

    tab = (int **)malloc(n * sizeof(int *));
    if (tab == (int **)0) {
        fprintf(stderr,"%d: cannot malloc distance table\n",rank);
        exit (42);
    }

    for (i = 0; i < n; i++) {
	if (lb <= i && i < ub) {   /* my partition */
            tab[i]    = (int *)malloc(n * sizeof(int));
            if (tab[i] == (int *)0) {
              fprintf(stderr,"%d: cannot malloc distance table\n",rank);
              exit (42);
            }
	    for (j = 0; j < n; j++) {
		tab[i][j] = (i == j ? 0 : i + j);
	    }
	}
        else{
            tab[i] = (int*)0;

        }
    }

    spare = (int *)malloc(n * sizeof(int));
    if (spare == (int *)0) {
        fprintf(stderr,"%d: cannot malloc spare vector\n",rank);
        exit (42);
    }

    *tabptr    = tab;
}

static void
free_tab(int **tab, int lb, int ub)
{
    int i;
    
    for (i = lb; i < ub; i++) {
	free(tab[i]);
    }
    free(tab);
    free(spare);
}




static void
do_asp(int **tab, int n, int lb, int ub)
{
    int i, j, k, tmp, nrows;
    MPI_Status status;

    nrows = ub - lb;

    for (k = 0; k < n; k++) {
        if ( rank != owner(k) ){
            tab[k] = spare;
        }
        //MPI_Bcast(tab[k],n,MPI_INTEGER,owner(k),MPI_COMM_WORLD);
                
        if (rank == owner(k)) { // sending
	        for (i = 0; i < size; i++) {
        		if (i == rank) {
        			continue;
	        	}
		        MPI_Send(tab[k], n, MPI_INTEGER, i, 0, MPI_COMM_WORLD);
		}
	} else {	
		MPI_Recv(tab[k], n, MPI_INTEGER, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);		
	}
	
        for (i = lb; i < ub; i++) {
	    if (i != k) {
		for (j = 0; j < n; j++) {
		    tmp = tab[i][k] + tab[k][j];
		    if (tmp < tab[i][j]) {
			tab[i][j] = tmp;
		    }
		}
            }
        }
    }
}

static void 
i_o()
{
  int i;
  int buf;
  MPI_Status status;
    
  if(rank == 0){
    for (i =1; i < size; i++)
      MPI_Recv(&buf, 1, MPI_INTEGER, i, 0, MPI_COMM_WORLD, &status);		
  }
  else {
    MPI_Send(&buf, 1, MPI_INTEGER, 0, 0, MPI_COMM_WORLD);

  }

}

/******************** Main program *************************/

int
main(int argc, char **argv)
{
    int lb, ub;
    double start,end;

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    n = 0;
    if ( argc > 1 )
      n = atoi(argv[1]);
    if (n == 0)
      n = 4000;

    if (rank == 0 ) {
	printf("Running asp on %d nodes with %d rows\n", size, n);
    }

    get_bounds(&lb, &ub);

    init_tab(n, lb, ub, &tab);

    // MPI_Barrier(MPI_COMM_WORLD);
    //MAGPIE_Reset_statistics(1);  /* print/reset statistics before do_asp() */
    /* will only have effect with -magpie-statistics */
 
    //MAGPIE_Reduce_associative(MPI_COMM_WORLD,1);

    //    start = MPI_Wtime();
    do_asp(tab, n, lb, ub);
    //end = MPI_Wtime();
    
    i_o();

    free_tab(tab, lb, ub);

    /* if (rank ==  0) { */
    /* 	printf("asp took %f\n",end - start); */
    /* } */
    MPI_Finalize();

    return 0;
}
