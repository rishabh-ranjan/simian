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
 * File:        HashMap_comm.h
 * Description: Implements a HashMap to store the MPI_Comm objects
 * Contact:     isp-dev@cs.utah.edu
 */
 
#ifndef _HASHMAP_COMM_H
#define _HASHMAP_COMM_H

#include <mpi.h>
#include <string.h>
#include <stdlib.h>

#define HASH_SIZE    10

/*
 * This structure stores the mapping between an MPI-Request
 * and the corresponding Envelope.
 */

typedef struct HashNodeComm_t {
    struct HashNodeComm_t   *next; /* MUST be first because of HashMapComm_Delete */
    int                     key;
    MPI_Comm                *comm;
    int                     id;
} HashNodeComm;

typedef HashNodeComm * HashNodeCommp;

typedef HashNodeCommp HashMapComm[HASH_SIZE];

/*
 * Initialize the HashMap.
 */
void HashMapComm_Init (HashMapComm *);
/*
 * Insert into the HashMap.
 */
void HashMapComm_Insert (HashMapComm *, MPI_Comm *, int);
/*
 * Search for a communicator in the HashMap and return its ID.
 */
int HashMapComm_GetId (HashMapComm *, MPI_Comm *);
/*
 * Delete from the HashMap.
 */
void HashMapComm_Delete (HashMapComm *, MPI_Comm *);

void HashMapComm_Destroy(HashMapComm *);

#endif
