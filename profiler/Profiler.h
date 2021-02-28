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
 * File:        Profiler.h
 * Description: Support for MPI functions to trap into the ISP scheduler
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _PROFILER_H
#define _PROFILER_H

#include "isp.h"

/* MPIAPI is used for MSMPI */
#ifndef MPIAPI
#define MPIAPI
#endif

#ifndef bcopy
#define bcopy(memSrc, memDst, memLen) memcpy((memDst), (memSrc), (memLen))
#endif

#undef MPI_Init
int MPIAPI MPI_Init (int *argc, char ***argv) {
    return ISP_Init (argc, argv, "Unknown", -1);
}

#undef MPI_Send
int MPIAPI MPI_Send (void * buffer, int count, MPI_Datatype datatype, int dest, 
                        int tag, MPI_Comm comm) {
    return ISP_Send (buffer, count, datatype, dest, tag, comm, "Unknown", -1);
}

#undef MPI_Rsend
int MPIAPI MPI_Rsend (void * buffer, int count, MPI_Datatype datatype, int dest, 
                        int tag, MPI_Comm comm) {
    return ISP_Rsend (buffer, count, datatype, dest, tag, comm, "Unknown", -1);
}

#undef MPI_Isend
int MPIAPI MPI_Isend (void *buf, int count, MPI_Datatype datatype, int dest,
        int tag, MPI_Comm comm, MPI_Request *request) {
    return ISP_Isend (buf, count, datatype, dest, tag, comm, request, "Unknown", -1);
}    

#undef MPI_Send_init
int MPIAPI MPI_Send_init (void *buf, int count, MPI_Datatype datatype, int dest,
        int tag, MPI_Comm comm, MPI_Request *request) {
    return ISP_Send_init (buf, count, datatype, dest, tag, comm, request, "Unknown", -1);
}    

#undef MPI_Irecv
int MPIAPI MPI_Irecv (void *buf, int count, MPI_Datatype datatype, int source,
        int tag, MPI_Comm comm, MPI_Request *request) {
    return ISP_Irecv (buf, count, datatype, source, tag, comm, request, "Unknown", -1);
}

#undef MPI_Recv
int MPIAPI MPI_Recv (void *buffer, int count, MPI_Datatype datatype, int source,
                int tag, MPI_Comm comm, MPI_Status *status) {
    return ISP_Recv (buffer, count, datatype, source, tag, comm, status, "Unknown", -1);
}

#undef MPI_Recv_init
int MPIAPI MPI_Recv_init (void *buf, int count, MPI_Datatype datatype, int src,
        int tag, MPI_Comm comm, MPI_Request *request) {
    return ISP_Recv_init (buf, count, datatype, src, tag, comm, request, "Unknown", -1);
}    

#undef MPI_Probe
int MPIAPI MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status) {
    return ISP_Probe (source, tag, comm, status, "Unknown", -1);
}

#undef MPI_Iprobe
int MPIAPI MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status) {
    return ISP_Iprobe (source, tag, comm, flag, status, "Unknown", -1);
}

#undef MPI_Wait
int MPIAPI MPI_Wait (MPI_Request *request, MPI_Status *status) {
    return ISP_Wait (request, status, "Unknown", -1);
}

#undef MPI_Test
int MPIAPI MPI_Test (MPI_Request *request, int *flag, MPI_Status *status) {
    return ISP_Test (request, flag, status, "Unknown", -1);
}

#undef MPI_Request_free
int MPIAPI MPI_Request_free (MPI_Request *request) {
    return ISP_Request_free (request, "Unknown", -1);
}
#if 0
#undef MPI_Bsend
int MPIAPI MPI_Bsend (void * buffer, int count, MPI_Datatype datatype, int dest, 
                        int tag, MPI_Comm comm) {
    return ISP_Bsend (buffer, count, datatype, dest, tag, comm, "Unknown", -1);
}
#endif

#undef MPI_Ssend
int MPIAPI MPI_Ssend (void *buffer, int count, MPI_Datatype datatype, int dest, 
                        int tag, MPI_Comm comm) {
    return ISP_Ssend (buffer, count, datatype, dest, tag, comm, "Unknown", -1);
}

#undef MPI_Sendrecv
int MPIAPI MPI_Sendrecv (void * sendbuf, int sendcount, MPI_Datatype sendtype, 
    int dest, int sendtag, void * recvbuf, int recvcount, 
    MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, 
                        MPI_Status *status) {
    return ISP_Sendrecv (sendbuf, sendcount, sendtype, dest, sendtag, recvbuf,
               recvcount, recvtype, source, recvtag, comm, status, "Unknown", -1);
}

#undef MPI_Start
int MPIAPI MPI_Start (MPI_Request *request) {
    return ISP_Start (request, "Unknown", -1);
}

#undef MPI_Startall
int MPIAPI MPI_Startall (int count, MPI_Request *array_of_requests) {
    return ISP_Startall (count, array_of_requests, "Unknown", -1);
}

#undef MPI_Barrier
int MPIAPI MPI_Barrier (MPI_Comm comm) {
    return ISP_Barrier (comm, "Unknown", -1);
}

#undef MPI_Comm_create
int MPIAPI MPI_Comm_create ( MPI_Comm comm, MPI_Group group, MPI_Comm *comm_out ) {
    return ISP_Comm_create (comm, group, comm_out, "Unknown", -1);
}
#undef MPI_Cart_create
int MPIAPI MPI_Cart_create (MPI_Comm comm_old, int ndims, int *dims, int *periods, 
                     int reorder, MPI_Comm *comm_cart) {
    return ISP_Cart_create(comm_old, ndims, dims, periods, reorder, comm_cart, "Unknown",-1);
}

#undef MPI_Comm_dup
int MPIAPI MPI_Comm_dup ( MPI_Comm comm, MPI_Comm *comm_out ) {
    return ISP_Comm_dup (comm, comm_out, "Unknown", -1);
}

#undef MPI_Comm_split
int MPIAPI MPI_Comm_split ( MPI_Comm comm, int color, int key, MPI_Comm *comm_out ) {
    return ISP_Comm_split (comm, color, key, comm_out, "Unknown", -1);
}

#undef MPI_Comm_free
int MPIAPI MPI_Comm_free ( MPI_Comm *comm) {
    return ISP_Comm_free (comm, "Unknown", -1);
}

#undef MPI_Type_commit
int MPIAPI MPI_Type_commit ( MPI_Datatype *datatype ) {
    return ISP_Type_commit (datatype, "Unknown", -1);
}

#undef MPI_Type_free
int MPIAPI MPI_Type_free ( MPI_Datatype *datatype ) {
    return ISP_Type_free (datatype, "Unknown", -1);
}

#undef MPI_Bcast
int MPIAPI MPI_Bcast (void *buffer, int count, MPI_Datatype datatype, int root,
        MPI_Comm comm) {
    return ISP_Bcast (buffer, count, datatype, root, comm, "Unknown", -1);
}

#undef MPI_Scatter
int MPIAPI MPI_Scatter (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
                void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                int root, MPI_Comm comm) {
    return ISP_Scatter (sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype,
               root, comm, "Unknown", -1);
}

#undef MPI_Scatterv
int MPIAPI MPI_Scatterv (void *sendbuf, int *sendcnt, int *displs, MPI_Datatype sendtype, 
                void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                int root, MPI_Comm comm) {
    return ISP_Scatterv (sendbuf, sendcnt, displs, sendtype, recvbuf, recvcount,
               recvtype, root, comm, "Unknown", -1);
}

#undef MPI_Gatherv
int MPIAPI MPI_Gatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
                void *recvbuf, int *recvcount, int *displs, MPI_Datatype recvtype, 
                int root, MPI_Comm comm) {
    return ISP_Gatherv (sendbuf, sendcnt, sendtype, recvbuf, recvcount, displs, recvtype,
               root, comm, "Unknown", -1);
}

#undef MPI_Gather
int MPIAPI MPI_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
                void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                int root, MPI_Comm comm) {
    return ISP_Gather (sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype,
               root, comm, "Unknown", -1);
}

#undef MPI_Allgather
int MPIAPI MPI_Allgather (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
                void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                MPI_Comm comm) {
    return ISP_Allgather (sendbuf, sendcnt, sendtype, recvbuf, recvcount,
               recvtype, comm, "Unknown", -1);
}

#undef MPI_Allgatherv
int MPIAPI MPI_Allgatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
                           void *recvbuf, int *recvcount, int *displs,
                           MPI_Datatype recvtype, MPI_Comm comm) {
    return ISP_Allgatherv (sendbuf, sendcnt, sendtype, recvbuf, recvcount, displs,
               recvtype, comm, "Unknown", -1);
}

#undef MPI_Alltoall
int MPIAPI MPI_Alltoall (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
                         void *recvbuf, int recvcount, MPI_Datatype recvtype,
                         MPI_Comm comm) {
    return ISP_Alltoall (sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype,
               comm, "Unknown", -1);
}

#undef MPI_Alltoallv
int MPIAPI MPI_Alltoallv (void *sendbuf, int *sendcnt, int *sdispls, MPI_Datatype sendtype,
                         void *recvbuf, int *recvcount, int *rdispls, MPI_Datatype recvtype,
                         MPI_Comm comm) {
    return ISP_Alltoallv (sendbuf, sendcnt, sdispls, sendtype, recvbuf, recvcount, rdispls,
               recvtype, comm, "Unknown", -1);
}

#undef MPI_Scan
int MPIAPI MPI_Scan (void *sendbuff, void *recvbuff, int count, 
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
    return ISP_Scan (sendbuff, recvbuff, count, datatype, op, comm, "Unknown", -1);
}

/* == fprs begin == */
#undef MPI_Exscan
int MPIAPI MPI_Exscan (void *sendbuff, void *recvbuff, int count, 
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
    return ISP_Exscan (sendbuff, recvbuff, count, datatype, op, comm, "Unknown", -1);
}
/* == fprs end == */

#undef MPI_Reduce
int MPIAPI MPI_Reduce (void *sendbuff, void *recvbuff, int count, 
    MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) {
    return ISP_Reduce (sendbuff, recvbuff, count, datatype, op, root, comm, "Unknown", -1);
}

#undef MPI_Reduce_scatter
int MPIAPI MPI_Reduce_scatter (void *sendbuff, void *recvbuff, int *recvcnt, 
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
    return ISP_Reduce_scatter (sendbuff, recvbuff, recvcnt, datatype, op, comm, "Unknown", -1);
}

#undef MPI_Allreduce
int MPIAPI MPI_Allreduce (void *sendbuff, void *recvbuff, int count, 
    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) {
    return ISP_Allreduce (sendbuff, recvbuff, count, datatype, op, comm, "Unknown", -1);
}

#undef MPI_Waitall
int MPIAPI MPI_Waitall (int count, MPI_Request *array_of_requests, 
                            MPI_Status *array_of_statuses) {
    return ISP_Waitall (count, array_of_requests, array_of_statuses, "Unknown", -1);
}

#undef MPI_Waitany
int MPIAPI MPI_Waitany (int count, MPI_Request *array_of_requests, 
                int *index, MPI_Status *status) {
    return ISP_Waitany (count, array_of_requests, index, status, "Unknown", -1);
}

#undef MPI_Waitsome
int MPIAPI MPI_Waitsome (int incount, MPI_Request *array_of_requests, 
                int *outcount, int *array_of_indices, MPI_Status *array_of_statuses) {
    return ISP_Waitsome (incount, array_of_requests, outcount, array_of_indices,
               array_of_statuses, "Unknown", -1);
}

#undef MPI_Testany
int MPIAPI MPI_Testany (int count, MPI_Request *array_of_requests, 
        int *index, int *flag, MPI_Status *array_of_statuses) {
    return ISP_Testany (count, array_of_requests, index, flag,
               array_of_statuses, "Unknown", -1);
}

#undef MPI_Testsome
int MPIAPI MPI_Testsome (int incount, MPI_Request *array_of_requests, 
        int *outcount, int *array_of_indices, MPI_Status *array_of_statuses) {
    return ISP_Testsome (incount, array_of_requests, outcount, array_of_indices,
               array_of_statuses, "Unknown", -1);
}

#undef MPI_Testall
int MPIAPI MPI_Testall (int count, MPI_Request *array_of_requests, int *flag,
                            MPI_Status *array_of_statuses) {
    return ISP_Testall (count, array_of_requests, flag, array_of_statuses, "Unknown", -1);
}

#undef MPI_Abort
int MPIAPI MPI_Abort (MPI_Comm comm, int errorcode) {
    return ISP_Abort (comm, errorcode, "Unknown", -1);
}

#undef MPI_Finalize
int MPIAPI MPI_Finalize () {
    return ISP_Finalize ("Unknown", -1);
}

#endif
