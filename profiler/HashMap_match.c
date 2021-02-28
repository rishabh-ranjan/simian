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
 * File:        HashMap_match.c
 * Description: Implements a HashMap to store the requests that have yet to be waited on
 * Contact:     isp-dev@cs.utah.edu
 */

#include "HashMap_match.h"
#include <stdio.h>
#include "mem.h"

/* Initialize the HashMap */
void HashMapMatch_Init (HashMapMatch *map) {
    int i;

    if (map == NULL) {
        return;
    }
    for (i = 0; i < HASH_SIZE; i++) {
        (*map)[i] = NULL;
    }
}

/* Get the size of each type. */
static size_t GetTypeSize (int type) {

    switch (type) {
        case TYPE_REQUEST:
        case TYPE_PERSIST_REQUEST:
            return sizeof(MPI_Request);
        case TYPE_COMM:
            return sizeof(MPI_Comm);
        case TYPE_TYPE:
            return sizeof(MPI_Datatype);
        default:
            return sizeof(int);
    }
}

/* Create a HashNode */
static HashNodeMatch * HashNodeMatch_Create (void *data, int type, const char *file, int line) {

    HashNodeMatch *node = (HashNodeMatch *)malloc(sizeof(HashNodeMatch));
    node->key = *((int *)data);
    node->data = malloc(GetTypeSize(type));
    node->type = type;
    node->file = strdup(file);
    node->line = line;
    memcpy(node->data, data, GetTypeSize(type));
    node->next = NULL;
    return node;
}

/* Insert into the HashMap an MPI_Request and returns non-zero. If already exists, returns 0. */
int HashMapMatch_Insert (HashMapMatch *map, void *data, int type, const char *file, int line) {
    HashNodeMatch *head = NULL;
    HashNodeMatch *node = NULL;
    HashNodeMatch *p = NULL;
    int key = *((int *)data);
    head = (*map)[(key < 0 ? -key : key)%HASH_SIZE];
    node = HashNodeMatch_Create (data, type, file, line);
    if (! head) {
        (*map)[(key < 0 ? -key : key)%HASH_SIZE] = node;
        return 1;
    }
    for (p = head; ;p = p->next) {
        if (p->type == type && p->key == key && !memcmp(p->data, data, GetTypeSize(type))) {
            free(node->data);
            free(node->file);
            free(node);
            return 0;
        }
        if (p->next == NULL) {
            break;
        }
    }
    p->next = node;
    return 1;
}

/* Returns non-zero and deletes request if exists, otherwise returns 0. */
int HashMapMatch_Delete (HashMapMatch *map, void *data, int type) {
    HashNodeMatch *p, *prev;
    int key = *((int *)data);
    prev = (HashNodeMatch*) &(*map)[(key < 0 ? -key : key)%HASH_SIZE];
    p = (*map)[(key < 0 ? -key : key)%HASH_SIZE];
    while (p && (p->type != type || p->key != key || memcmp(p->data, data, GetTypeSize(type)))) {
        prev = p;
        p = p->next;
    }
    if (p) {
        prev->next = p->next;
        free(p->data);
        free(p->file);
        free(p);
        return 1;
    }
    return 0;
}

/* Returns the number of requests in the HashMap. */
int HashMapMatch_GetCount (HashMapMatch *map) {
    int             i;
    int             count = 0;
    HashNodeMatch   *p;

    for (i = 0; i < HASH_SIZE; i++) {
        for (p = (*map)[i]; p; p = p->next) {
            count++;
        }
    }

    return count;
}

/* Sends the leaks to the scheduler. */
int HashMapMatch_SendLeaks (HashMapMatch *map, int (*snr)(char *, size_t)) {
    int             count = 0;
    int             i;
    char            buf[1024];
    HashNodeMatch   *p;

    char            *idToString[] = {
                        "Request",
                        "PersistentRequest",
                        "Communicator",
                        "Datatype"
    };

    for (i = 0; i < HASH_SIZE; i++) {
        for (p = (*map)[i]; p; p = p->next) {
            sprintf(buf, "%d %s %s %u %s %d", count, "Leak", idToString[p->type],
                        (unsigned int) strlen(p->file), p->file, p->line);
            if (snr (buf, strlen (buf)) < 0) {
                printf ("Client unable to SendnRecv resource leaks\n");
                return -1;
            }
            count++;
        }
    }
    return count;
}

