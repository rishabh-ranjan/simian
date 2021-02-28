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
 * File:        HashMap_match.h
 * Description: Implements a HashMap to store the requests that have yet to be waited on
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _HASHMAP_MATCH_H
#define _HASHMAP_MATCH_H

#include <mpi.h>
#include <string.h>
#include <stdlib.h>

#define HASH_SIZE               10

/* Defines the types of data that can be stored. */
#define TYPE_REQUEST            0
#define TYPE_PERSIST_REQUEST    1
#define TYPE_COMM               2
#define TYPE_TYPE               3

/*
 * This structure stores the mapping between an MPI-Request
 * and the corresponding Envelope and Request.
 */

typedef struct HashNodeMatch_t {
    struct HashNodeMatch_t   *next; /* MUST be first because of HashMap_Delete */
    int                      type;
    int                      key;
    void                     *data;
    char                     *file;
    int                      line;
} HashNodeMatch;

typedef HashNodeMatch * HashNodeMatchp;

typedef HashNodeMatchp HashMapMatch[HASH_SIZE];

/*
 * Initialize the HashMap.
 */
void HashMapMatch_Init (HashMapMatch *);
/*
 * Insert into the HashMap an MPI_Request and returns non-zero. If already
 * exists, returns 0 and exits.
 */
int HashMapMatch_Insert (HashMapMatch *, void *, int, const char *, int);
/*
 * Returns non-zero and deletes request if exists, otherwise returns 0.
 */
int HashMapMatch_Delete (HashMapMatch *, void *, int);
/*
 * Returns the number of requests in the HashMap.
 */
int HashMapMatch_GetCount (HashMapMatch *);
/*
 * Sends the leaks to the scheduler.
 */
int HashMapMatch_SendLeaks (HashMapMatch *map, int (*snr)(char *, size_t));

#endif

