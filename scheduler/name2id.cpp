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
 * File:        name2id.cpp
 * Description: Translates from the envelope's string to an enum
 * Contact:     isp-dev@cs.utah.edu
 */

#include "name2id.hpp"


std::map<std::string, int> name2id::name_id;
bool name2id::init_done = false;


void name2id::doInit () {

    if (init_done) {
        return;
    }

    name_id["Assert"] = ASSERT;

    name_id["MPI_Send"] = SEND;
    name_id["Send"] = SEND;

    name_id["MPI_Isend"] = ISEND;
    name_id["Isend"] = ISEND;

    name_id["MPI_Ssend"] = SSEND;
    name_id["Ssend"] = SSEND;

	name_id["MPI_Rsend"] = RSEND;
	name_id["Rsend"] = RSEND;

    name_id["MPI_Send_init"] = SEND_INIT;
    name_id["Send_init"] = SEND_INIT;
    
    name_id["MPI_Recv"] = RECV;
    name_id["Recv"] = RECV;

    name_id["MPI_Irecv"] = IRECV;
    name_id["Irecv"] = IRECV;

    name_id["MPI_Recv_init"] = RECV_INIT;
    name_id["Recv_init"] = RECV_INIT;

    name_id["MPI_Probe"] = PROBE;
    name_id["Probe"] = PROBE;

    name_id["MPI_Iprobe"] = IPROBE;
    name_id["Iprobe"] = IPROBE;

    name_id["MPI_Start"] = START;
    name_id["Start"] = START;

    name_id["MPI_Startall"] = STARTALL;
    name_id["Startall"] = STARTALL;

    name_id["MPI_Request_free"] = REQUEST_FREE;
    name_id["Request_free"] = REQUEST_FREE;

    name_id["MPI_Wait"] = WAIT;
    name_id["Wait"] = WAIT;

    name_id["MPI_Waitall"] = WAITALL;
    name_id["Waitall"] = WAITALL;

    name_id["MPI_Waitany"] = WAITANY;
    name_id["Waitany"] = WAITANY;

    name_id["MPI_Testany"] = TESTANY;
    name_id["Testany"] = TESTANY;

    name_id["MPI_Testall"] = TESTALL;
    name_id["Testall"] = TESTALL;

    name_id["MPI_Test"] = TEST;
    name_id["Test"] = TEST;

    name_id["MPI_Sendrecv"] = SENDRECV;
    name_id["Sendrecv"] = SENDRECV;

    name_id["MPI_Barrier"] = BARRIER;
    name_id["Barrier"] = BARRIER;

    name_id["MPI_Bcast"] = BCAST;
    name_id["Bcast"] = BCAST;

    name_id["MPI_Scatter"] = SCATTER;
    name_id["Scatter"] = SCATTER;

    name_id["MPI_Gather"] = GATHER;
    name_id["Gather"] = GATHER;

    name_id["MPI_Scatterv"] = SCATTERV;
    name_id["Scatterv"] = SCATTERV;

    name_id["MPI_Gatherv"] = GATHERV;
    name_id["Gatherv"] = GATHERV;

    name_id["MPI_Allgather"] = ALLGATHER;
    name_id["Allgather"] = ALLGATHER;

    name_id["MPI_AllGatherv"] = ALLGATHERV;
    name_id["Allgatherv"] = ALLGATHERV;

    name_id["MPI_Alltoall"] = ALLTOALL;
    name_id["Alltoall"] = ALLTOALL;

    name_id["MPI_Alltoallv"] = ALLTOALLV;
    name_id["Alltoallv"] = ALLTOALLV;

    name_id["MPI_Scan"] = SCAN;
    name_id["Scan"] = SCAN;

    name_id["MPI_Reduce"] = REDUCE;
    name_id["Reduce"] = REDUCE;

    name_id["MPI_Reduce_scatter"] = REDUCE_SCATTER;
    name_id["Reduce_scatter"] = REDUCE_SCATTER;

    name_id["MPI_Allreduce"] = ALLREDUCE;
    name_id["Allreduce"] = ALLREDUCE;

    name_id["MPI_Comm_create"] = COMM_CREATE;
    name_id["Comm_create"] = COMM_CREATE;

    name_id["MPI_Cart_create"] = CART_CREATE;
    name_id["Cart_create"] = CART_CREATE;

    name_id["MPI_Comm_dup"] = COMM_DUP;
    name_id["Comm_dup"] = COMM_DUP;

    name_id["MPI_Comm_split"] = COMM_SPLIT;
    name_id["Comm_split"] = COMM_SPLIT;

    name_id["MPI_Comm_free"] = COMM_FREE;
    name_id["Comm_free"] = COMM_FREE;

    name_id["MPI_Abort"] = ABORT;
    name_id["Abort"] = ABORT;

    name_id["MPI_Finalize"] = FINALIZE;
    name_id["Finalize"] = FINALIZE;

    name_id["Leak"] = LEAK;

/* == fprs begin == */
    name_id["MPI_Pcontrol"] = PCONTROL;
    name_id["Pcontrol"] = PCONTROL;
	name_id["MPI_Exscan"] = EXSCAN;
	name_id["Exscan"] = EXSCAN;
/* == fprs end == */

    init_done = true;

}

int name2id::getId (std::string name) {

    if (! init_done) {
        doInit ();
    }

    std::map <std::string, int>::iterator iter;
    if ((iter = name_id.find (name)) == name_id.end ()) {
        return -1;
    }
    return (*iter).second;
}
