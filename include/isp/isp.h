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
 * File:        isp.h
 * Description: Replacement mpi.h file, enabling filename and line number input
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _ISP_H
#define _ISP_H

#include <mpi.h>

#define ISP_INTERLEAVING 1
/* == fprs begin == */
#define ISP_START_SAMPLING	2
#define ISP_END_SAMPLING    3
/* == fprs end == */

/*
 * MPIAPI is used for MSMPI
 */
#ifndef MPIAPI
#define MPIAPI
#endif

#if defined (_USRDLL) && defined (WIN32)
#define DLLAPI _declspec(dllexport)
#else
#define DLLAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This code will be used with the C version of the actual MPI code.
 */

int ISP_Assert (const char *assertion, const char *function, const char *file, int line);
void ISP_Assert_fail (const char *assertion, const char *function, const char *file, int line);
void ISP_Assert_perror_fail (int errnum, const char *function, const char *file, int line);

#define MPI_Init(argc, argv) ISP_Init(argc, argv, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Init (int *argc, char ***argv, const char *file, int line);

#define MPI_Send(buffer, count, datatype, dest, tag, comm) ISP_Send(buffer, count, datatype, dest, tag, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Send (void * buffer, int count, MPI_Datatype datatype, int dest, 
                        int tag, MPI_Comm comm, const char *file, int line);

#define MPI_Rsend(buffer, count, datatype, dest, tag, comm) ISP_Rsend(buffer, count, datatype, dest, tag, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Rsend (void * buffer, int count, MPI_Datatype datatype, int dest, 
                        int tag, MPI_Comm comm, const char *file, int line);

#define MPI_Isend(buf, count, datatype, dest, tag, comm, request) ISP_Isend(buf, count, datatype, dest, tag, comm, request, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Isend (void *buf, int count, MPI_Datatype datatype, int dest,
        int tag, MPI_Comm comm, MPI_Request *request, const char *file, int line);

#define MPI_Send_init(buf, count, datatype, dest, tag, comm, request) ISP_Send_init(buf, count, datatype, dest, tag, comm, request, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Send_init (void *buf, int count, MPI_Datatype datatype, int dest,
        int tag, MPI_Comm comm, MPI_Request *request, const char *file, int line);

#define MPI_Irecv(buf, count, datatype, source, tag, comm, request) ISP_Irecv(buf, count, datatype, source, tag, comm, request, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Irecv (void *buf, int count, MPI_Datatype datatype, int source,
        int tag, MPI_Comm comm, MPI_Request *request, const char *file, int line);

#define MPI_Recv(buffer, count, datatype, source, tag, comm, status) ISP_Recv(buffer, count, datatype, source, tag, comm, status, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Recv (void *buffer, int count, MPI_Datatype datatype, int source, 
        int tag, MPI_Comm comm, MPI_Status *status, const char *file, int line);

#define MPI_Recv_init(buf, count, datatype, src, tag, comm, request) ISP_Recv_init(buf, count, datatype, src, tag, comm, request, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Recv_init (void *buf, int count, MPI_Datatype datatype, int src,
        int tag, MPI_Comm comm, MPI_Request *request, const char *file, int line);

#define MPI_Probe(source, tag, comm, status) ISP_Probe (source, tag, comm, status, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status,
        const char* file, int line);

#define MPI_Iprobe(source, tag, comm, flag, status) ISP_Iprobe (source, tag, comm, flag, status, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Iprobe(int source, int tag, MPI_Comm comm, int *flag, 
        MPI_Status *status, const char* file, int line);

#define MPI_Wait(request, status) ISP_Wait(request, status, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Wait (MPI_Request *request, MPI_Status *status, const char *file,
        int line);

#define MPI_Test(request, flag, status) ISP_Test(request, flag, status, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Test (MPI_Request *request, int *flag, MPI_Status *status,
        const char *file, int line);

#if 0

#define MPI_Bsend(buffer, count, datatype, dest, tag, comm) ISP_Bsend(buffer, count, datatype, dest, tag, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Bsend (void * buffer, int count, MPI_Datatype datatype, int dest, 
        int tag, MPI_Comm comm, const char *file, int line);

#endif

#define MPI_Ssend(buffer, count, datatype, dest, tag, comm) ISP_Ssend(buffer, count, datatype, dest, tag, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Ssend (void *buffer, int count, MPI_Datatype datatype, int dest, 
        int tag, MPI_Comm comm, const char *file, int line);

#define MPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status) ISP_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Sendrecv (void * sendbuf, int sendcount, MPI_Datatype sendtype, 
        int dest, int sendtag, void * recvbuf, int recvcount, 
        MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, 
        MPI_Status *status, const char *file, int line);

#define MPI_Start(request) ISP_Start(request, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Start(MPI_Request *request, const char *file, int line);

#define MPI_Startall(count, request) ISP_Startall(count, request, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Startall(int count, MPI_Request *array_of_requests, const char *file, int line);

#define MPI_Barrier(comm) ISP_Barrier(comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Barrier (MPI_Comm comm, const char *file, int line);

#define MPI_Cart_create(comm_old,ndims,dims,periods,reorder,comm_cart) ISP_Cart_create(comm_old,ndims,dims,periods,reorder,comm_cart,__FILE__,__LINE__)
    DLLAPI int MPIAPI ISP_Cart_create (MPI_Comm comm_old, int ndims, int *dims, int *periods, int reorder, MPI_Comm *comm_cart, const char *file, int line);

#define MPI_Comm_create(comm, group, comm_out) ISP_Comm_create(comm, group, comm_out, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Comm_create (MPI_Comm comm, MPI_Group group, MPI_Comm *comm_out,
        const char *file, int line);

#define MPI_Comm_dup(comm, comm_out) ISP_Comm_dup(comm, comm_out, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Comm_dup (MPI_Comm comm, MPI_Comm *comm_out, const char *file, int line);

#define MPI_Comm_split(comm, color, key, comm_out) ISP_Comm_split(comm, color, key, comm_out, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Comm_split (MPI_Comm comm, int color, int key, MPI_Comm *comm_out,
        const char *file, int line);

#define MPI_Comm_free(comm) ISP_Comm_free(comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Comm_free (MPI_Comm *comm, const char *file, int line);

#define MPI_Type_commit(datatype) ISP_Type_commit(datatype, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Type_commit (MPI_Datatype *datatype, const char *file, int line);

#define MPI_Type_free(datatype) ISP_Type_free(datatype, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Type_free (MPI_Datatype *datatype, const char *file, int line);

#define MPI_Bcast(buffer, count, datatype, root, comm) ISP_Bcast(buffer, count, datatype, root, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Bcast (void *buffer, int count, MPI_Datatype datatype, int root,
        MPI_Comm comm, const char *file, int line);

#define MPI_Scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, root, comm) ISP_Scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, root, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Scatter (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
        void *recvbuf, int recvcount, MPI_Datatype recvtype, 
        int root, MPI_Comm comm, const char *file, int line);

#define MPI_Scatterv(sendbuf, sendcnt, displs, sendtype, recvbuf, recvcount, recvtype, root, comm) ISP_Scatterv(sendbuf, sendcnt, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Scatterv (void *sendbuf, int *sendcnt, int *displs, MPI_Datatype sendtype, 
        void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
        const char *file, int line);

#define MPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcount, displs, recvtype, root, comm) ISP_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcount, displs, recvtype, root, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Gatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
        void *recvbuf, int *recvcount, int *displs, MPI_Datatype recvtype, 
        int root, MPI_Comm comm, const char *file, int line);

#define MPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, root, comm) ISP_Gather(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, root, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
        void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
        MPI_Comm comm, const char *file, int line);

#define MPI_Allgather(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, comm) ISP_Allgather(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Allgather (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
        void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
        const char *file, int line);

#define MPI_Allgatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcount, displs, recvtype, comm) ISP_Allgatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcount, displs, recvtype, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Allgatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
        void *recvbuf, int *recvcount, int *displs, MPI_Datatype recvtype,
        MPI_Comm comm, const char *file, int line);

#define MPI_Alltoall(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, comm) ISP_Alltoall(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Alltoall (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
        void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
        const char *file, int line);

#define MPI_Alltoallv(sendbuf, sendcnt, sdispls, sendtype, recvbuf, recvcount, rdispls, recvtype, comm) ISP_Alltoallv(sendbuf, sendcnt, sdispls, sendtype, recvbuf, recvcount, rdispls, recvtype, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Alltoallv (void *sendbuf, int *sendcnt, int *sdispls, MPI_Datatype sendtype,
        void *recvbuf, int *recvcount, int *rdispls, MPI_Datatype recvtype,
        MPI_Comm comm, const char *file, int line);

#define MPI_Scan(sendbuff, recvbuff, count, datatype, op, comm) ISP_Scan(sendbuff, recvbuff, count, datatype, op, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Scan (void *sendbuff, void *recvbuff, int count, MPI_Datatype datatype,
        MPI_Op op, MPI_Comm comm, const char *file, int line);

/* == fprs begin == */
#define MPI_Exscan(sendbuff, recvbuff, count, datatype, op, comm) ISP_Scan(sendbuff, recvbuff, count, datatype, op, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Exscan (void *sendbuff, void *recvbuff, int count, MPI_Datatype datatype,
        MPI_Op op, MPI_Comm comm, const char *file, int line);
/* == fprs end == */

#define MPI_Reduce(sendbuff, recvbuff, count, datatype, op, root, comm) ISP_Reduce(sendbuff, recvbuff, count, datatype, op, root, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Reduce (void *sendbuff, void *recvbuff, int count, 
        MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
        const char *file, int line);

#define MPI_Reduce_scatter(sendbuff, recvbuff, recvcnt, datatype, op, comm) ISP_Reduce_scatter(sendbuff, recvbuff, recvcnt, datatype, op, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Reduce_scatter (void *sendbuff, void *recvbuff, int *recvcnt, 
        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, const char *file, int line);

#define MPI_Allreduce(sendbuff, recvbuff, count, datatype, op, comm) ISP_Allreduce(sendbuff, recvbuff, count, datatype, op, comm, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Allreduce (void *sendbuff, void *recvbuff, int count, 
        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, const char *file, int line);

#define MPI_Waitall(count, array_of_requests, array_of_statuses) ISP_Waitall(count, array_of_requests, array_of_statuses, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Waitall (int count, MPI_Request *array_of_requests, 
        MPI_Status *array_of_statuses, const char *file, int line);

#define MPI_Waitany(count, array_of_requests, index, status) ISP_Waitany(count, array_of_requests, index, status, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Waitany (int count, MPI_Request *array_of_requests, 
        int *index, MPI_Status *status, const char *file, int line);

#define MPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses) ISP_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Waitsome (int incount, MPI_Request *array_of_requests, 
        int *outcount, int *array_of_indices, MPI_Status *array_of_statuses,
        const char *file, int line);

#define MPI_Testany(count, array_of_requests, index, flag, array_of_statuses) ISP_Testany(count, array_of_requests, index, flag, array_of_statuses, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Testany (int count, MPI_Request *array_of_requests, int *index,
        int *flag, MPI_Status *array_of_statuses, const char *file, int line);

#define MPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses) ISP_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Testsome (int incount, MPI_Request *array_of_requests, 
        int *outcount, int *array_of_indices,MPI_Status *array_of_statuses,
        const char *file, int line);

#define MPI_Testall(count, array_of_requests, flag, array_of_statuses) ISP_Testall(count, array_of_requests, flag, array_of_statuses, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Testall (int count, MPI_Request *array_of_requests, int *flag,
        MPI_Status *array_of_statuses, const char *file, int line);

#define MPI_Request_free(request) ISP_Request_free(request, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Request_free (MPI_Request *request, const char *file, int line);

#define MPI_Abort(comm, errorcode) ISP_Abort(comm, errorcode, __FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Abort (MPI_Comm comm, int errorcode, const char *file, int line);

#define MPI_Finalize() ISP_Finalize(__FILE__, __LINE__)
DLLAPI int MPIAPI ISP_Finalize (const char *file, int line);

#ifdef __cplusplus
}
#endif

#endif
