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
 * File:        HashMap_comm.c
 * Description: Implements a HashMap to store the MPI_Comm objects
 * Contact:     isp-dev@cs.utah.edu
 */
#include "HashMap_comm.h"
#include "stdio.h"
#include "mem.h"

void HashMapComm_Init (HashMapComm *map) {
    int i;
    MPI_Comm comm_world = MPI_COMM_WORLD;
    MPI_Comm comm_self = MPI_COMM_SELF;
    MPI_Comm comm_null = MPI_COMM_NULL;

    if (map == NULL) {
        return;
    }
    for (i = 0; i < HASH_SIZE; i++) {
        (*map)[i] = NULL;
    }
    HashMapComm_Insert (map, &comm_world, 0);
    HashMapComm_Insert (map, &comm_self, 1);
    HashMapComm_Insert (map, &comm_null, 2);
}

static HashNodeComm * HashNodeComm_Create (MPI_Comm *comm, int id) {

    HashNodeComm *node = (HashNodeComm *)malloc(sizeof(HashNodeComm));
    node->key = *((int *)comm);
    node->comm = (MPI_Comm *)malloc(sizeof(MPI_Comm));
    memcpy(node->comm, comm, sizeof(MPI_Comm));
    node->id = id;
    node->next = NULL;
    return node;
}

void HashMapComm_Insert (HashMapComm *map, MPI_Comm *comm, int id) {
    HashNodeComm *head = NULL;
    HashNodeComm *node = NULL;
    HashNodeComm *p = NULL;
    int key = *((int *)comm);
    head = (*map)[(key < 0 ? -key : key)%HASH_SIZE];
    node = HashNodeComm_Create (comm, id);
    if (! head) {
        (*map)[(key < 0 ? -key : key)%HASH_SIZE] = node;
        return;
    }
    for (p = head; ;p = p->next) {
        if (p->key == key && *p->comm == *comm) {
            p->id = id;
            free (node->comm);
            free (node);
            return;
        }
        if (p->next == NULL) {
            break;
        }
    }
    p->next = node;
    return;
}

int HashMapComm_GetId (HashMapComm *map, MPI_Comm *comm) {
    HashNodeComm *head = NULL;
    HashNodeComm *p = NULL;
    int key = *((int *)comm);
    head = (*map)[(key < 0 ? -key : key)%HASH_SIZE];
    if (! head) {
        return *((int *)comm);
    }
    for (p = head; p != NULL; p = p->next) {
        if (p->key == key && *p->comm == *comm) {
            return p->id;
        }
    }
    return -1;
}

void HashMapComm_Delete (HashMapComm *map, MPI_Comm *comm) {
    HashNodeComm *p, *prev;
    int key = *((int *)comm);

    prev = (HashNodeComm*) &(*map)[(key < 0 ? -key : key)%HASH_SIZE];
    p = (*map)[(key < 0 ? -key : key)%HASH_SIZE];
    while (p && p->key != key) {
        prev = p;
        p = p->next;
    }
    if (p) {
        prev->next = p->next;
        free (p->comm);
        free (p);
    }
}

void HashMapComm_Destroy(HashMapComm *map) {
    HashNodeComm *n, *p;
    int i;
    for(i=0; i<HASH_SIZE; i++) {
        for (p = (*map)[i]; p !=NULL; p = n) {
            free (p->comm);
            n = p->next;
            //if(mem.find(p) != mem.end()) free(p);
            free(p);

        }
    }
}
