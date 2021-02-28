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
 * File:        HashMap.c
 * Description: Implements a HashMap to store the MPI_Request objects
 * Contact:     isp-dev@cs.utah.edu
 */

#include "HashMap.h"
#include "mem.h"

static int init_done = 0;

/* Initialize the HashMap */
void HashMap_Init (HashMap *map) {
    int i;

    if (map == NULL || init_done) {
        return;
    }
    for (i = 0; i < HASH_SIZE; i++) {
        (*map)[i] = NULL;
    }
    init_done = 1;
}

/* Create a HashNode */
static HashNode * HashNode_Create (int key, MPI_Request *req, char *env) {

    HashNode *node = (HashNode *)malloc(sizeof(HashNode));
    node->key = key;
    node->req = (MPI_Request *)malloc(sizeof(MPI_Request));
    memcpy(node->req, req, sizeof(MPI_Request));
    node->env = strdup(env);
    node->next = NULL;
    return node;
}

/* Insert into the HashMap an element <key, request, envelope> */
void HashMap_Insert (HashMap *map, int key, MPI_Request *req, char *env) {
    HashNode *head = NULL;
    HashNode *node = NULL;
    HashNode *p = NULL;
    head = (*map)[(key < 0 ? -key : key)%HASH_SIZE];
    node = HashNode_Create (key, req, env);
    if (! head) {
        (*map)[(key < 0 ? -key : key)%HASH_SIZE] = node;
        return;
    }
    for (p = head; ;p = p->next) {
        if (p->key == key) {
            free (p->env);
            p->env = node->env;
            free (p->req);
            p->req = node->req;
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

/* Return the associated MPI_Request */
MPI_Request *HashMap_GetRequest (HashMap *map, int key) {
    HashNode *head = NULL;
    HashNode *p = NULL;
    head = (*map)[(key < 0 ? -key : key)%HASH_SIZE];
    if (! head) {
        return NULL;
    }
    for (p = head; p != NULL; p = p->next) {
        if (p->key == key) {
            return p->req;
        }
    }
    return NULL;
}

/* Return the associated envelope */
char *HashMap_GetEnv (HashMap *map, int key) {
    HashNode *head = NULL;
    HashNode *p = NULL;
    head = (*map)[(key < 0 ? -key : key)%HASH_SIZE];
    if (! head) {
        return NULL;
    }
    for (p = head; p != NULL; p = p->next) {
        if (p->key == key) {
            return p->env;
        }
    }
    return NULL;
}

/*Delete an entry from the HashMap */
void HashMap_Delete (HashMap *map, int key) {
    HashNode *p, *prev;
    prev = (HashNode*) &(*map)[(key < 0 ? -key : key)%HASH_SIZE];
    p = (*map)[(key < 0 ? -key : key)%HASH_SIZE];
    while (p && p->key != key) {
        prev = p;
        p = p->next;
    }
    if (p) {
        prev->next = p->next;
        free(p->env);
        free(p->req);
        free(p);
    }
}

/* Update the envelope being stored in the map */
void HashMap_UpdateEnv (HashMap *map, int key, char *env) {
    HashNode *head = NULL;
    HashNode *p = NULL;
    head = (*map)[(key < 0 ? -key : key)%HASH_SIZE];
    if (! head) {
        return;
    }
    for (p = head; p != NULL; p = p->next) {
        if (p->key == key) {
            /*free (p->env);*/
            p->env = env;
        }
    }

}

void HashMap_Destroy(HashMap *map) {
    int rank;
    HashNode *n, *p;
    int i=0;
//    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//    if(rank==1) return;
//    printf("%d [hashmap destroy begin]\n", rank);
    for(i=0; i<HASH_SIZE; i++) {
//          printf("  %d   i=%x\n", rank, i);
        for (p = (*map)[i]; p !=NULL; p = n) {
//            printf("  %d   p=%x\n", rank, p);
//            printf("  env=%x\n", p->env);
            free (p->env);
//            if(p->env) free (p->env);
//            else printf("  %d  env=%x\n", rank, p->env);
//            printf("  req=%x\n", p->req);
            free (p->req);
//            if(p->req) free (p->req);
//            else printf("  %d  req=%x\n", rank, p->req);
            n = p->next;
//            if(mem.find(p) != mem.end()) free(p);
            free(p);

        }
    }

//    printf("%d [hashmap destroy end]\n", rank);
}
