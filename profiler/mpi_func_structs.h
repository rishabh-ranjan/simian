/*
 * Copyright (c) 2008-2009
 *
 * School of Computing, University of Utah,
 * Salt Lake City, UT 84112, USA
 *
 * and the Gauss Group
 * http://www.cs.utah.edu/formal_verification
 *
 * See LICENSE for licensing information
 */

/*
 * ISP: MPI Dynamic Verification Tool
 *
 * File:        mpi_func_structs.h
 * Description: Defines the required structs and enums for the profiler code
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _MPI_FUNC_STRUCTS_H
#define _MPI_FUNC_STRUCTS_H
#include <mpi.h>

typedef enum {
    SEND,
    SSEND,
	RSEND,
    SEND_INIT,
    RECV,
    RECV_INIT,
    PROBE,
    IPROBE,
    SENDRECV,
    START,
    STARTALL,
    BARRIER,
    BCAST,
    SCATTER,
    GATHER,
    SCATTERV,
    GATHERV,
    REDUCE,
    REDUCE_SCATTER,
    ISEND,
    IRECV,
    WAIT,
    ALLREDUCE,
    ALLGATHER,
    ALLGATHERV,
    ALLTOALL,
    ALLTOALLV,
    SCAN,
    TEST,
    WAITALL,
    TESTALL,
    WAITANY,
    TESTANY,
    REQUEST_FREE,
    CART_CREATE,
    COMM_CREATE,
    COMM_SPLIT,
    COMM_DUP,    
    COMM_FREE,
    ABORT,
    FINALIZE
/* == fprs begin == */
    , PCONTROL
/* == fprs end == */
} mpi_isp_func_t;

/*
 * Define a structure for arguments for incomplete functions.
 */

typedef struct arg_tag {

    mpi_isp_func_t func;
    int func_count;
    int is_complete;
    int persistent_index;

    /*the rest serves as envelope data */
    void *buffer;    
    int count;
    MPI_Datatype datatype;
    int source;
    int dest;
    int tag;
    int root;
    int* flag;
    int* index;
    MPI_Comm comm;
    MPI_Request* request;
    MPI_Status* status;
    int buffer_copied;
    int datatype_copied;

    /*these serve as communicator data*/
    MPI_Group group;
    int color;
    int key;
    int ndims;
    int *dims;
    int *periods;
    int reorder;
    MPI_Comm *comm_out;
} arg_t;

#endif
