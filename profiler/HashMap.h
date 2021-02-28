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
 * File:        HashMap.h
 * Description: Implements a HashMap to store the MPI_Request objects
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _HASHMAP_H
#define _HASHMAP_H

#include <mpi.h>
#include <string.h>
#include <stdlib.h>

#define HASH_SIZE    10

/*
 * This structure stores the mapping between an MPI-Request
 * and the corresponding Envelope and Request.
 */

typedef struct HashNode_t {
    struct HashNode_t   *next; /* MUST be first because of HashMap_Delete */
    int                 key;
    MPI_Request         *req;
    char                *env;
} HashNode;

typedef HashNode * HashNodep;

typedef HashNodep HashMap[HASH_SIZE];

/*
 * Initialize the HashMap.
 */
void HashMap_Init (HashMap *);
/*
 * Insert into the HashMap.
 */
void HashMap_Insert (HashMap *, int, MPI_Request *, char *);
/*
 * Search for a request in the HashMap.
 */
MPI_Request * HashMap_GetRequest (HashMap *, int);
/*
 * Search for a environment in the HashMap.
 */
char * HashMap_GetEnv (HashMap *, int);
/*
 * Delete from the HashMap.
 */
void HashMap_Delete (HashMap *, int);
/* Update the envelope of an entry */
void HashMap_UpdateEnv(HashMap*, int, char*);

void HashMap_Destroy(HashMap*);
#endif
