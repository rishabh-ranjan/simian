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
 * File:        Profiler.c
 * Description: Support for MPI functions to trap into the ISP scheduler
 * Contact:     isp-dev@cs.utah.edu
 */

#include "isp.h"
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <errno.h>
#include "Client.h"
#include "HashMap.h"
#include "HashMap_comm.h"
#include "HashMap_match.h"
#include "mpi_func_structs.h"
#include "Profiler.h"
#include "mem.h"
#include <stdlib.h>
#define BUFFER_SIZE 1024
#define REQUEST_CONST 100000
#define MAX_MPI_CALLS 150000
/*#define DEBUG 1*/
extern const char      *goahead = "1";
static const char      *goback = "2";
static const char      *stop = "4";
extern SOCKET          fd = INVALID_SOCKET;
arg_t*           argarray;
arg_t*           persistent_argarray;
static HashMap         rmap;
static HashMapComm     cmap;
static HashMapMatch    mmap;

/* Resource counters */
static int             request_count = 0;
static int             persistent_request_count = 0;
static int             comm_count = 0;
static int             type_count = 0;

static int             func_count = -1;
static int             curr_func_count = 0;
static int             send_block = 0;
static int             requestNo = 0;
static int             my_rank = -1;
static int             is_initialized = 0;
static MPI_Group       MPI_GROUP_WORLD;

static int interleaving;

#ifdef DEBUG
static void DebugPrint(const char *msg) {
    if (is_initialized) {
        printf("Rank %d: %s", my_rank, msg);
    } else {
        printf("Rank ?: %s", msg);
    }
    fflush(stdout);
}
#else
#define DebugPrint(msg)
#endif

static void ISP_printf(int rank, const char *myargs, ...) {
    int size, mpi_rank;
    va_list arg;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    va_start(arg, myargs);
    if (rank < 0 || rank == mpi_rank) {
        printf("rank %d: ", mpi_rank);
        vprintf(myargs, arg);
        printf("\n");
    }
    fflush(stdout);
    va_end(arg);
}

#if defined (_USRDLL) && defined (WIN32)
#include <windows.h>

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
#endif

static void initializeRequestNo(int id) {
    if (requestNo == 0) {
        requestNo = (id + 1) * REQUEST_CONST;
    }
}

static int get_persistent_request_index (int request) {
    if (request < 0) {
        printf("In get_persistent_request_index, request cannot be less than 0\n");
        assert(0);
    }
    return request % REQUEST_CONST;

}

static char *TranslateRanks(int rank, MPI_Comm from_comm, MPI_Comm to_comm) {
    static char     buff[20];
    int             new_rank = 0;
    MPI_Group       from_group;
    MPI_Group       to_group;
    int             from_comm_size;

    if (rank == MPI_ANY_SOURCE) {
        strcpy(buff, "MPI_ANY_SOURCE");
        sprintf (buff, "MPI_ANY_SOURCE");
    } else {
        MPI_Comm_group(from_comm, &from_group);
        MPI_Comm_group(to_comm, &to_group);
        MPI_Comm_size(from_comm, &from_comm_size);
        if (rank < 0 || rank >= from_comm_size) {
            sprintf (buff, "%d", rank);
        } else {
            MPI_Group_translate_ranks(from_group, 1, &rank, to_group, &new_rank);
            sprintf (buff, "%d", new_rank);
        }
    }

    return buff;
}

static void RemoveFromMatchHashMap (void *data, int type, const char *file, int line) {

    /* We ignore the null's. */
    if ((type == TYPE_REQUEST || type == TYPE_PERSIST_REQUEST) &&
            *((MPI_Request *)data) == MPI_REQUEST_NULL) {
        return;
    } else if (type == TYPE_COMM && *((MPI_Comm *)data) == MPI_COMM_NULL) {
        return;
    }

    /* Remove from HashMap. */
    if (HashMapMatch_Delete (&mmap, data, type) == 0) {
        switch (type) {
            case TYPE_REQUEST:
            case TYPE_PERSIST_REQUEST:
                printf("Rank %d: WARNING: Waited on non-existant request in %s:%d\n", my_rank, file, line);
                break;

            case TYPE_COMM:
                printf("Rank %d: WARNING: Tried to free a non-existant communicator in %s:%d\n", my_rank, file, line);
                break;

            case TYPE_TYPE:
                printf("Rank %d: WARNING: Tried to free a non-existant datatype in %s:%d\n", my_rank, file, line);
                break;

            default:
                assert(0); /* should never get here */
                break;
        }
    }
}

static void InsertIntoMatchHashMap (void *data, int type, const char *file, int line) {
    /* We ignore the null's. */
    if ((type == TYPE_REQUEST || type == TYPE_PERSIST_REQUEST) &&
            *((MPI_Request *)data) == MPI_REQUEST_NULL) {
        return;
    } else if (type == TYPE_COMM && *((MPI_Comm *)data) == MPI_COMM_NULL) {
        return;
    }

    /* Add to HashMap. */
    if (HashMapMatch_Insert (&mmap, data, type, file, line) == 0) {
        switch (type) {
            case TYPE_REQUEST:
            case TYPE_PERSIST_REQUEST:
                printf("Rank %d: WARNING: Reusing request before it has been waited on in %s:%d\n", my_rank, file, line);
                break;

            case TYPE_COMM:
                printf("Rank %d: WARNING: Reusing communicator before it has been freed %s:%d\n", my_rank, file, line);
                break;

            case TYPE_TYPE:
                printf("Rank %d: WARNING: Reusing datatype before it has been freed %s:%d\n", my_rank, file, line);
                break;

            default:
                assert(0); /* should never get here */
                break;
        }
    }
}

static void setIrecvReq (int i, MPI_Status *status, int status_index) {
    if (*(argarray[i].request) != MPI_REQUEST_NULL) {
        PMPI_Wait(argarray[i].request, argarray[i].status);
    }
    if (status != MPI_STATUSES_IGNORE && &status[status_index] != MPI_STATUS_IGNORE) {
        status[status_index] = *(argarray[i].status);
        if (argarray[i].status != (&status[status_index]))
            free (argarray[i].status);
    }
}

static void setIsendReq (int i, MPI_Request *request, MPI_Status *status, int status_index) {

    /* if the ISEND has not been completed 
       and we're dealing with non-persistent request in non-blocking mode */
    if (!send_block && !argarray[i].is_complete &&
        argarray[i].func != START) {
        *request = MPI_REQUEST_NULL;
        return;
    }

    /* this means the ISEND has been completed */
    if (*(argarray[i].request) != MPI_REQUEST_NULL) {
        PMPI_Wait(argarray[i].request, argarray[i].status);
    }

    /* for persistent request, do not set to MPI_REQUEST_NULL */
    if (argarray[i].func != START) {
        *request = *argarray[i].request;
    }
    if (argarray[i].buffer_copied) {
        free(argarray[i].buffer);
        argarray[i].buffer = 0;
        argarray[i].buffer_copied = 0;
    }
    if (argarray[i].datatype_copied) {
        PMPI_Type_free(&argarray[i].datatype);
        argarray[i].datatype_copied = 0;
    }
    
    if (status != MPI_STATUSES_IGNORE && &status[status_index] != MPI_STATUS_IGNORE) {
        status[status_index] = *(argarray[i].status);
        if (argarray[i].status != (&status[status_index]))
            free (argarray[i].status);
    }
}

static void PMPI_Execute (int i, int src, int comm_id) {
    int correct_comm_src;
    int source = src;
    MPI_Group group;
    if (argarray[i].is_complete) {
        return;
    }

    switch (argarray[i].func) {

    case SEND: { 
        arg_t _arg = argarray[i];
        
        PMPI_Wait(_arg.request, MPI_STATUS_IGNORE);

        if (_arg.buffer_copied) {
            free (_arg.buffer);
            argarray[i].buffer = 0;
            _arg.buffer_copied = 0;
        }
        
        free(_arg.request);
        if (_arg.datatype_copied) {
            PMPI_Type_free(&_arg.datatype);
            _arg.datatype_copied = 0;
        }
        break;
    }
	case RSEND: { 
        arg_t _arg = (argarray[i]);
        PMPI_Wait(_arg.request, MPI_STATUS_IGNORE);
        if (_arg.buffer_copied) {
            free (_arg.buffer);
            argarray[i].buffer = 0;
            _arg.buffer_copied = 0;
        }
        
        free(_arg.request);
        if (_arg.datatype_copied) {
            PMPI_Type_free(&_arg.datatype);
            _arg.datatype_copied = 0;
        }
        break;
    }
    case SSEND: {
        /*
        arg_t _arg = argarray[i];
        PMPI_Ssend (_arg.buffer, _arg.count, _arg.datatype, 
                _arg.dest, _arg.tag, _arg.comm);
        */
        break;
    }
    case ISEND: {
        arg_t _arg = argarray[i];
        
        int key = *((int *)_arg.request);
        
        /* the send has been issued earlier - we only do the cleaning up now */
        /*
        PMPI_Isend (_arg.buffer, _arg.count, _arg.datatype, 
                _arg.dest, _arg.tag, _arg.comm, _arg.request);
        */
        
        if (my_rank != _arg.dest) {
            PMPI_Wait (_arg.request, argarray[i].status);

            if (!send_block && (HashMap_GetRequest (&rmap, key) == NULL)) {
                /* free (_arg.request);*/
            }
            if (_arg.buffer_copied) {
                free (_arg.buffer);
                argarray[i].buffer = 0;
                argarray[i].buffer_copied = 0;
            }
            if (_arg.datatype_copied) {
                PMPI_Type_free(&_arg.datatype);
                argarray[i].datatype_copied = 0;
            }
        }
        break;
    }
    case IRECV: {
        arg_t _arg = argarray[i];
        /* Because ISP sees everything as if they're in COMM_WORLD, 
         *  we need to retranslate the rank */
        MPI_Comm_group(_arg.comm, &group);
        MPI_Group_translate_ranks(MPI_GROUP_WORLD, 1, &source, group, &correct_comm_src);
        PMPI_Irecv (_arg.buffer, _arg.count, _arg.datatype, 
                correct_comm_src, _arg.tag, _arg.comm, _arg.request);

        if (my_rank != src) {
            PMPI_Wait (_arg.request, argarray[i].status);
        }
        
        /*
        if (my_rank == 1)
            printf("Proc %d Irecv from %d\n", my_rank, src);
        */
        break;
    }
    case START: {
        /* find out the owner of the request */
        int storage_index = argarray[i].persistent_index;
        int request_type = persistent_argarray[storage_index].func;
        /* this has to be either SEND_INIT or RECV_INIT */
        assert (request_type == SEND_INIT ||
                request_type == RECV_INIT);
        if (request_type == SEND_INIT) {
            arg_t _arg = argarray[i];
            /* PMPI_Start(_arg.request); */
            if (my_rank != _arg.dest) {
                PMPI_Wait(_arg.request, argarray[i].status);
            }
            if (_arg.buffer_copied) {
                free (_arg.buffer);
                argarray[i].buffer = 0;
                argarray[i].buffer_copied = 0;
            }
            if (_arg.datatype_copied) {
                PMPI_Type_free(&_arg.datatype);
                argarray[i].datatype_copied = 0;
            }

        } else  {/* if (request_type == RECV_INIT) */
            arg_t _arg = argarray[i];
            PMPI_Start(_arg.request);
            if (my_rank != _arg.source) {
                PMPI_Wait(_arg.request, argarray[i].status);
            }
        }
        break;
    }
    case STARTALL: {
        break;
    }
    case IPROBE: {
        arg_t _arg = argarray[i];
        /* Because ISP sees everything as if they're in COMM_WORLD, 
         *  we need to retranslate the rank */
        MPI_Comm_group(_arg.comm, &group);
        MPI_Group_translate_ranks(MPI_GROUP_WORLD, 1, &source, group, &correct_comm_src);

        /* This means that at this time, no match can be found for Iprobe, return false */
        if (source == -1) {
            *(_arg.flag) = 0;
            break;
        }
        if (_arg.source == MPI_ANY_SOURCE && correct_comm_src != _arg.source) {
            while (*_arg.flag == 0) {
                PMPI_Iprobe(correct_comm_src, _arg.tag, _arg.comm, _arg.flag, _arg.status);
            }
        } else {
            PMPI_Iprobe(correct_comm_src, _arg.tag, _arg.comm, _arg.flag, _arg.status);            
        }
        break;
    }
    case PROBE: {
        arg_t _arg = argarray[i];
        /* Because ISP sees everything as if they're in COMM_WORLD, 
         *  we need to retranslate the rank */
        MPI_Comm_group(_arg.comm, &group);
        MPI_Group_translate_ranks(MPI_GROUP_WORLD, 1, &source, group, &correct_comm_src);
        PMPI_Probe(correct_comm_src, _arg.tag, _arg.comm, _arg.status);
        break;
    }
    case RECV: {
        arg_t _arg = argarray[i];
        /* Because ISP sees everything as if they're in COMM_WORLD, 
         *  we need to retranslate the rank */
        MPI_Comm_group(_arg.comm, &group);
        MPI_Group_translate_ranks(MPI_GROUP_WORLD, 1, &source, group, &correct_comm_src);
        PMPI_Recv (_arg.buffer, _arg.count, _arg.datatype, 
                correct_comm_src, _arg.tag, _arg.comm, _arg.status);
        break;
    }
    case BARRIER: {
        arg_t _arg = argarray[i];
        PMPI_Barrier (_arg.comm);
        break;
    }
    case COMM_CREATE: {
        arg_t _arg = argarray[i];
        PMPI_Comm_create (_arg.comm, _arg.group, _arg.comm_out);
        if (*_arg.comm_out != MPI_COMM_NULL) {
            HashMapComm_Insert (&cmap, _arg.comm_out, comm_id);
        }
        break;
    }
    case CART_CREATE: {
        arg_t _arg = argarray[i];
        PMPI_Cart_create (_arg.comm, _arg.ndims, _arg.dims, _arg.periods, _arg.reorder, _arg.comm_out);
        if (*_arg.comm_out != MPI_COMM_NULL) {
            HashMapComm_Insert (&cmap, _arg.comm_out, comm_id);
        } else {
            fprintf(stderr, "i have some comm_null\n");
        }
        free (_arg.dims);
        free(_arg.periods);
        break;
    }
    case COMM_DUP: {
        arg_t _arg = argarray[i];
        PMPI_Comm_dup (_arg.comm, _arg.comm_out);
        HashMapComm_Insert (&cmap, _arg.comm_out, comm_id);
        break;
    }
    case COMM_SPLIT: {
        arg_t _arg = argarray[i];
        PMPI_Comm_split (_arg.comm, _arg.color, _arg.key, _arg.comm_out);
        HashMapComm_Insert (&cmap, _arg.comm_out, comm_id);
        break;
    }
    case COMM_FREE: {
        arg_t _arg = argarray[i];
        HashMapComm_Delete (&cmap, _arg.comm_out);
        PMPI_Comm_free (_arg.comm_out);
        break;
    }

    case ABORT:
    case ALLREDUCE:
    case BCAST:
    case REDUCE:
    case REDUCE_SCATTER:
    case TEST:
    case WAIT:
    case WAITALL:
    case TESTALL:
    case WAITANY:
    case TESTANY:
    case GATHER:
    case SCATTER:
    case SCATTERV:
    case GATHERV:
    case ALLGATHER:
    case ALLGATHERV:
    case ALLTOALL:
    case ALLTOALLV:
    case SCAN:
    case SENDRECV:
    case SEND_INIT:
    case RECV_INIT:
    case REQUEST_FREE:
    case FINALIZE:
/* == fprs start == */
    case PCONTROL:
/* == fprs end == */
        break;
    }
    argarray[i].is_complete = 1;
}

static int SendnRecv (char *sbuff, size_t len) {

    char    rbuff[BUFFER_SIZE];
    int     comm_id;

    initializeRequestNo(my_rank);
    if (sbuff == NULL) {
        return -1;
    }
    if (Send (fd, sbuff, (int) len) <= 0) {
        printf ("Client unable to send in SendRecv\n");
        return -1;
    }
    while (1) {
        int ret;
        int num;
        char *result = NULL;
        memset (rbuff, '\0', BUFFER_SIZE);
        Receive (fd, rbuff, BUFFER_SIZE);
        // std::cout << "\nRecieved in Profiler: " << rbuff << "\n";
        result = strtok (rbuff, ":");
        while (result != NULL) {
            if (strncmp (result, goahead, strlen (goahead)) == 0) {
                int instr_to_execute;
                char str[20];
                int src;
                num = sscanf (result, "%s %d %d %d %d", str, 
                              &instr_to_execute, &src, &comm_id, &ret);
                PMPI_Execute (instr_to_execute, src, comm_id);    
                if (ret == 2)  {
                    goto end;
                }
            }  else if(strncmp (result, goback, strlen (goback)) == 0) {
                goto end;
            }  else if(strncmp (result, stop, strlen (stop)) == 0) {
                PMPI_Abort (MPI_COMM_WORLD, 1);
            }
            result = strtok(NULL, ":");
        }
    }
end:
    func_count++;
    return 0;
}


void set_common_args (mpi_isp_func_t func) {
    argarray[func_count].func_count = func_count;
    argarray[func_count].is_complete = 0;
    argarray[func_count].persistent_index = -1;
    argarray[func_count].func = func;
    argarray[func_count].buffer_copied = 0;
    argarray[func_count].datatype_copied = 0;
}

char *Profiler_create_group_ranks (MPI_Comm comm, int *nprocs) {

    int         i;
    MPI_Group   group_world;
    MPI_Group   this_group;
    int         *ranks1;
    int         *ranks2;
    char        *sbuff = NULL;
    int         offset = 0;

    MPI_Comm_group (MPI_COMM_WORLD, &group_world);
    MPI_Comm_group (comm, &this_group);
    MPI_Comm_size (comm, nprocs);

    ranks1 = (int *) malloc ((*nprocs) * sizeof (int)); 
    ranks2 = (int *) malloc ((*nprocs) * sizeof (int)); 
    sbuff = (char *) malloc (BUFFER_SIZE * sizeof (char)); 

    for (i = 0 ; i < (*nprocs) ; i++) {
        ranks1[i] = i;
    }
    MPI_Group_translate_ranks (this_group, (*nprocs), ranks1, group_world, 
                        ranks2);

    memset (sbuff, '\0', BUFFER_SIZE);
    sprintf (sbuff, "%d_", HashMapComm_GetId (&cmap, &comm));
    offset = (int) strlen (sbuff);

    for (i = 0 ; i < (*nprocs); i++) {
        if (ranks2[i] != MPI_UNDEFINED) {
            sprintf (sbuff + offset, "%d:", ranks2[i]);
            offset = (int) strlen (sbuff);
        }
    }
    free (ranks1);
    free (ranks2);
    return sbuff;
}


/*
static char *Profiler_create_group_ranks (MPI_Comm comm, int *nprocs) {
    char        *sbuff = NULL;
    MPI_Comm_size (comm, nprocs);

    sbuff = (char *) malloc (BUFFER_SIZE * sizeof (char)); 
    memset (sbuff, '\0', BUFFER_SIZE);

    sprintf(sbuff, "%d", HashMapComm_GetId (&cmap, &comm));

    return sbuff;
}
*/

void ISP_Assert_fail (const char *assertion, const char *function,
                      const char *file, int line) {
    char        sbuff[BUFFER_SIZE];
    char        errstr[BUFFER_SIZE];

    DebugPrint("Entering ISP_Assert_fail...\n");

    /* Construct the "Assertion `%s' failed." error string. */
    memset (errstr, '\0', BUFFER_SIZE);
    sprintf (errstr, "Assertion `%s' failed.", assertion);

    /* If connected - send, otherwise print to stdout and abort. */
    if (fd == INVALID_SOCKET) {
        printf ("%s:%d: %s: %s\n", file, line, function, errstr);
        abort ();
    } else {
        memset (sbuff, '\0', BUFFER_SIZE);
        //CGD added %d and the corresponding 0
        sprintf (sbuff, "%d %s %s %d %u %s %d %u %s %s", func_count,
                 "Assert", "Assertion", 0,
                 (unsigned int) strlen(file), file, line,
                 (unsigned int) strlen(errstr), errstr, function);

        if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
            printf ("Client unable to SendnRecv in ISP_Assert_fail\n");
            return;
        }
    }
}

void ISP_Assert_perror_fail (int errnum, const char *function,
                             const char *file, int line) {
    char        sbuff[BUFFER_SIZE];
    char        errbuff[BUFFER_SIZE];

    DebugPrint("Entering ISP_Assert_perror_fail...\n");

    /* errnum == 0 means success. */
    if (errnum == 0) {
        return;
    }

    /* Construct the "Unexpected error: %s." error string. */
    memset (sbuff, '\0', BUFFER_SIZE);
#ifndef WIN32
    strerror_r (errnum, sbuff, BUFFER_SIZE);
#else
    strerror_s (sbuff, BUFFER_SIZE, errnum);
#endif
    sprintf (errbuff, "Unexpected error: %s.", sbuff);

    /* If connected - send, otherwise print to stdout and abort. */
    if (fd == INVALID_SOCKET) {
        printf ("%s:%d: %s: %s\n", file, line, function, errbuff);
        abort ();
    } else {
        memset (sbuff, '\0', BUFFER_SIZE);
        //CGD added %d and the corresponding 0
        sprintf (sbuff, "%d %s %s %d %u %s %d %u %s %s", func_count,
                 "Assert", "Assertion", 0,
                 (unsigned int) strlen(file), file, line,
                 (unsigned int) strlen(errbuff), errbuff, function);

        if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
            printf ("Client unable to SendnRecv in ISP_Assert_perror_fail\n");
            return;
        }
    }
}

DLLAPI int MPIAPI ISP_Init (int *argc, char ***argv, const char *file, int line) {

    int     numargs = 6;
    int     i;
    char    *temp = NULL;
    char    *use_env = NULL;

    int     unix_sockets;
    int     port;
    int     numprocs;
    int     result1;
    int     result2;
    char    *host = NULL;
    char    *sockfile = NULL;
    char    buffer[BUFFER_SIZE];
    int     provided;
    result1 = PMPI_Init_thread (argc, argv, MPI_THREAD_MULTIPLE, &provided);
    result2 = MPI_Comm_rank (MPI_COMM_WORLD, &my_rank);
    MPI_Comm_group(MPI_COMM_WORLD, &MPI_GROUP_WORLD);

    DebugPrint("Entering MPI_Init...\n");

    /* Use command-line arguments for settings. */
    use_env = getenv("ISP_USE_ENV");
    if (!use_env || strcmp(use_env, "1")) { //[grz] !strcmp
        if (*argc <= numargs) {
            printf ("ERROR: profiler has too few arguments\n");
            return -1;
        }
        unix_sockets = !strcmp((*argv)[1], "1") ? 1 : 0;
        if (unix_sockets) {
            port = 0;
            host = NULL;
            sockfile = (char*) malloc ((strlen((*argv)[2]) + 1) * sizeof(char));
            strcpy (sockfile, (*argv)[2]);
            interleaving = atoi((*argv)[6]);
        } else {
            port = atoi ((*argv)[2]);
            host = (char*) malloc ((strlen((*argv)[4]) + 1) * sizeof(char));
            strcpy (host, (*argv)[4]);
            sockfile = NULL;
            interleaving = atoi((*argv)[6]);
        }
        numprocs = atoi ((*argv)[3]);
        send_block = !strcmp((*argv)[5], "1") ? 1 : 0;
        for (i = numargs + 1; i < *argc; i++)
        {
            temp = (*argv)[i-numargs];
            (*argv)[i-numargs] = (*argv)[i];
            (*argv)[i] = temp;
        }
        *argc = *argc - numargs;
    }
    
    /* Use environment variables for settings. */
    else {
        temp = getenv("ISP_UNIX_SOCKETS");
        if (temp == NULL) {
            printf ("Error: ISP_UNIX_SOCKETS environment argument not found\n");
            return -1;
        }
        unix_sockets = !strcmp(temp, "1") ? 1 : 0;

        temp = getenv("ISP_HOSTNAME");
        if (temp == NULL) {
            printf ("Error: ISP_HOSTNAME environment argument not found\n");
            return -1;
        }
        if (unix_sockets) {
            port = 0;
            host = NULL;
            sockfile = (char*) malloc ((strlen(temp) + 1) * sizeof(char));
            strcpy (sockfile, temp);
        } else {
            host = (char*) malloc ((strlen(temp) + 1) * sizeof(char));
            strcpy (host, temp);
            sockfile = NULL;
            temp = getenv("ISP_PORT");
            if (temp == NULL) {
                printf ("Error: ISP_PORT environment argument not found\n");
                return -1;
            }
            port = atoi (temp);
        }

        temp = getenv("ISP_NUMPROCS");
        if (temp == NULL) {
            printf("Error: ISP_NUMPROCS environment argument not found\n");
            return -1;
        }
        numprocs = atoi (temp);

        temp = getenv("ISP_SENDBLOCK");
        if (temp == NULL) {
            printf("Error: ISP_SENDBLOCK environment argument not found\n");
            return -1;
        }
        send_block = !strcmp(temp, "1") ? 1 : 0;
    }

    if ((fd = Connect (unix_sockets, host, port, sockfile, my_rank)) == INVALID_SOCKET) {
        printf ("Client %d Unable to connect\n", my_rank);
        return -1;
    }
    
    //argarray = (arg_t*) malloc (MAX_MPI_CALLS * sizeof (arg_t));
    argarray = (arg_t*) calloc (MAX_MPI_CALLS, sizeof (arg_t));
    persistent_argarray = (arg_t*) malloc (MAX_MPI_CALLS * sizeof (arg_t));
    is_initialized = 1;
    if (host != NULL) {
        free(host);
    }
    if (sockfile != NULL) {
        free(sockfile);
    }
    memset (buffer, '\0', BUFFER_SIZE);
    sprintf (buffer, "%d\n", my_rank);
    if (SendnRecv (buffer, strlen (buffer)) < 0) {
        printf ("Client %d unable to send id to server\n", my_rank);
        return -1;
    }
    HashMap_Init (&rmap);
    HashMapComm_Init (&cmap);
    HashMapMatch_Init (&mmap);

    return result1;
}

DLLAPI int MPIAPI mpi_init (int* ierr) {
    return ISP_Init(NULL, NULL, "Unknown", -1);
}

void copyBufferNonBlockingSend (void *inbuffer, void **outbuffer, int count,
                                MPI_Datatype datatype,
                                int *buffer_copied) {
    MPI_Aint     extent, lowerbound;

    MPI_Type_lb(datatype, (MPI_Aint*)&lowerbound);
    if (lowerbound != 0) {
        /* TODO: What is going on here */
        *buffer_copied = 0;
        return;
    }
    MPI_Type_extent(datatype, (MPI_Aint*)&extent);
    if(*outbuffer) {
        printf("[copyBufferNonBlockingSend]\n");
        //exit(100); 
    }
    *outbuffer = malloc(extent * count);
    if (!outbuffer) {
        *buffer_copied = 0;
        return;
    }
    memcpy(*outbuffer, inbuffer, extent * count);
    *buffer_copied = 1;
}


void copyDatatypeNonBlockingSend (MPI_Datatype indatatype, MPI_Datatype* outdatatype,
                                                 int *datatype_copied) {

    MPI_Type_contiguous(1, indatatype, outdatatype);
    PMPI_Type_commit(outdatatype);
    *datatype_copied = 1;
}

DLLAPI int MPIAPI ISP_Send (void * buffer, int count, MPI_Datatype datatype, int dest, 
                        int tag, MPI_Comm comm, const char *file, int line) {

    char        sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int        nprocs;
    int        retVal;

    DebugPrint("Entering MPI_Send...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Send(buffer, count, datatype, dest, tag, comm);
    }

    /* 
     * If MPI_PROC_NULL, issue immediately, and don't issue in to scheduler.
     * The scheduler will treat it as MPI_ANY_SOURCE, which is not right.
     */
    if (dest == MPI_PROC_NULL)
    {
        return PMPI_Send (buffer, count, datatype, dest, tag, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);
    /*
     * Send the Send Envelope to the server. The following is the
     * format of the Send Envelope:
     *
     * "Send dest tag comm" 
     * However, since ISP sees everything as in MPI_COMM_WORLD, we 
     * will need to translate the destination into its MPI_COMM_WORLD
     */
    buff = Profiler_create_group_ranks (comm, &nprocs);
    if (send_block) {
        if(buffer != NULL)
        {
    		//CGD added %d and the corresponding datatype
            sprintf (sbuff, "%d %s %s %d %d %d %u %s %d %s %d %s", func_count, "Ssend", 
                 "Send", datatype, count, *((int*)buffer), // count and buffer added by dhriti
                 (unsigned int) strlen(file), file, line,
                 TranslateRanks(dest, comm, MPI_COMM_WORLD), tag, buff);
            set_common_args (SSEND);
        }
        else
        {
            sprintf (sbuff, "%d %s %s %d %d %d %u %s %d %s %d %s", func_count, "Ssend", 
                 "Send", datatype, count, NULL, // count and buffer added by dhriti
                 (unsigned int) strlen(file), file, line,
                 TranslateRanks(dest, comm, MPI_COMM_WORLD), tag, buff);
            set_common_args (SSEND);
        }
    }
    else {
        if(buffer!=NULL)
        {
    		//CGD added %d and the corresponding datatype
            sprintf (sbuff, "%d %s %s %d %d %d %u %s %d %s %d %s", func_count, "Send", 
                    "Send", datatype, count, *((int*)buffer),
                    (unsigned int) strlen(file), file, line,
                    TranslateRanks(dest, comm, MPI_COMM_WORLD), tag, buff);
            set_common_args (SEND);
        }
        else
        {
            sprintf (sbuff, "%d %s %s %d %d %d %u %s %d %s %d %s", func_count, "Send", 
                    "Send", datatype, count, NULL,
                    (unsigned int) strlen(file), file, line,
                    TranslateRanks(dest, comm, MPI_COMM_WORLD), tag, buff);
            set_common_args (SEND);
        }
    }
    free (buff);
    argarray[func_count].count = count;
    argarray[func_count].datatype =datatype;
    argarray[func_count].dest = dest;
    argarray[func_count].tag = tag;
    argarray[func_count].comm = comm;

    curr_func_count = func_count;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Send\n");
        return -1;
    }

    if (!send_block) {
        copyBufferNonBlockingSend (buffer, &(argarray[curr_func_count].buffer), count, datatype,
                                    &(argarray[curr_func_count].buffer_copied));

        copyDatatypeNonBlockingSend (datatype, &(argarray[curr_func_count].datatype),
                                     &(argarray[curr_func_count].datatype_copied));
        

        argarray[curr_func_count].request = calloc (sizeof(MPI_Request), 1);

        retVal = PMPI_Isend(argarray[curr_func_count].buffer, count, datatype, dest, tag, comm, 
                            (argarray[curr_func_count].request));
    }
    else {
        retVal = PMPI_Ssend(buffer, count, datatype, dest, tag, comm);
    }
    return retVal;
}

DLLAPI int MPIAPI ISP_Rsend (void * buffer, int count, MPI_Datatype datatype, int dest, 
                        int tag, MPI_Comm comm, const char *file, int line) {

    char        sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int        nprocs;
    int        retVal;

    DebugPrint("Entering MPI_Rsend...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Rsend(buffer, count, datatype, dest, tag, comm);
    }

    /* 
     * If MPI_PROC_NULL, issue immediately, and don't issue in to scheduler.
     * The scheduler will treat it as MPI_ANY_SOURCE, which is not right.
     */
    if (dest == MPI_PROC_NULL)
    {
        return PMPI_Rsend (buffer, count, datatype, dest, tag, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);
    /*
     * Send the Send Envelope to the server. The following is the
     * format of the Send Envelope:
     *
     * "Send dest tag comm"
     * However, since ISP sees everything as in MPI_COMM_WORLD, we 
     * will need to translate the destination into its MPI_COMM_WORLD
     */
    buff = Profiler_create_group_ranks (comm, &nprocs);
    //CGD added %d and the corresponding datatype
    sprintf (sbuff, "%d %s %s %d %d %d %u %s %d %s %d %s", func_count, "Rsend",
        "Rsend", datatype, count, *((int*)buffer),
        (unsigned int) strlen(file), file, line,
        TranslateRanks(dest, comm, MPI_COMM_WORLD), tag, buff);
    set_common_args (RSEND);

    free (buff);

#ifdef DEBUG_PROF_MEM
    if(mem.find(argarray[func_count].buffer) != mem.end()) {
        free(argarray[func_count].buffer);
    }
#endif

    argarray[func_count].buffer = buffer;
    argarray[func_count].count = count;
    argarray[func_count].datatype =datatype;
    argarray[func_count].dest = dest;
    argarray[func_count].tag = tag;
    argarray[func_count].comm = comm;

    curr_func_count = func_count;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Send\n");
        return -1;
    }

    if (!send_block) {
        copyBufferNonBlockingSend (buffer, &argarray[curr_func_count].buffer, count, datatype,
                                                       &argarray[curr_func_count].buffer_copied);

        copyDatatypeNonBlockingSend (datatype,&argarray[curr_func_count].datatype,
                                     &argarray[curr_func_count].datatype_copied);
        
        argarray[curr_func_count].request = (MPI_Request*) malloc (sizeof(MPI_Request));
        retVal = PMPI_Isend(argarray[curr_func_count].buffer, count, datatype, dest, tag, comm, 
                            (argarray[curr_func_count].request));
    }
    else {
        retVal = PMPI_Ssend(buffer, count, datatype, dest, tag, comm);
    }
    /* clean up */
    if (argarray[func_count].buffer_copied) {
        free(argarray[func_count].buffer);
        argarray[func_count].buffer = 0;
        argarray[func_count].buffer_copied = 0;
    }
    argarray[func_count].buffer = 0;
    if (argarray[func_count].datatype_copied) {
        PMPI_Type_free(&(argarray[func_count].datatype));
        argarray[func_count].datatype_copied = 0;
    }
    return retVal;
}

DLLAPI int MPIAPI ISP_Isend (void *buf, int count, MPI_Datatype datatype, int dest,
        int tag, MPI_Comm comm, MPI_Request *request, const char *file, int line) {

    char            sbuff[BUFFER_SIZE];
    char            pbuff[BUFFER_SIZE];
    char            *buff = NULL;
    int             nprocs;
    int             isprequest = requestNo++;
    static int      i;
    int             retval;

    DebugPrint("Entering MPI_Isend...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Isend(buf, count, datatype, dest, tag, comm, request);
    }

    /* This makes sure there is a wait for each request. */
    *((int *)request) = isprequest;
    InsertIntoMatchHashMap (request, TYPE_REQUEST, file, line);

    /* 
     * If MPI_PROC_NULL, issue immediately, and don't issue in to scheduler.
     * The scheduler will treat it as MPI_ANY_SOURCE, which is not right.
     */
    if (dest == MPI_PROC_NULL) {
        retval = PMPI_Isend (buf, count, datatype, dest, tag, comm, request);
        HashMap_Insert (&rmap, isprequest, request, "MPI_PROC_NULL");
        *((int *)request) = isprequest;
        request_count++;
        return retval;
    }

    if (!send_block) {
        copyBufferNonBlockingSend (buf, &argarray[func_count].buffer, count, datatype,
            &argarray[func_count].buffer_copied);
        copyDatatypeNonBlockingSend (datatype, &argarray[func_count].datatype,
            &argarray[func_count].datatype_copied);
    }

    memset (sbuff, '\0', BUFFER_SIZE);
    memset (pbuff, '\0', BUFFER_SIZE);
    /*
     * Send the Isend Envelope to the server. The following is the
     * format of the Isend Envelope:
     *
     * "Isend dest tag comm"
     * However, because ISP sees everything as in MPI_COMM_WORLD, we will
     * need to translate the rank of dest into its MPI_COMM_WORLD rank
     */
    buff = Profiler_create_group_ranks (comm, &nprocs);
    i = func_count;
    //CGD added %d and the corresponding datatype
    sprintf (sbuff, "%d %s %s %d %d %d %u %s %d %s %d %d %s", func_count, "Isend", "Isend",
             datatype, count, *((int*)buf),
             (unsigned int) strlen(file), file, line,
             TranslateRanks(dest, comm, MPI_COMM_WORLD), tag, i, buff);
    free (buff);

    set_common_args (ISEND);

#ifdef DEBUG_PROF_MEM
    if(mem.find(argarray[func_count].buffer) != mem.end()) {
        free(argarray[func_count].buffer);
    }
#endif

    argarray[func_count].buffer = buf;
    argarray[func_count].count = count;
    argarray[func_count].datatype =datatype;
    argarray[func_count].dest = dest;
    argarray[func_count].tag = tag;
    argarray[func_count].comm = comm;
    argarray[func_count].status = (MPI_Status*)malloc (sizeof(MPI_Status));

    sprintf (pbuff, "%d", func_count);
    HashMap_Insert(&rmap, isprequest, request, pbuff);

    argarray[func_count].request = HashMap_GetRequest(&rmap, isprequest);

    curr_func_count = func_count;
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Isend\n");
        return -1;
    }

    /* issue the send as we see it */    
    request_count++;
    retval = PMPI_Isend(argarray[curr_func_count].buffer, count, argarray[curr_func_count].datatype, dest, tag, comm,
            argarray[curr_func_count].request);
    argarray[func_count].buffer = 0;
    return retval;
}    

/* What is happening in Send_init:
 * we need to a way to store the structure associated with this request
 * add_to_persistent_set(this request); --> add this request to some database
 * and we also need to offer a way to extract the information out of this request 
 * find_request_structure (this request); --> return the index to its information */
DLLAPI int MPIAPI ISP_Send_init (void *buf, int count, MPI_Datatype datatype, int dest,
        int tag, MPI_Comm comm, MPI_Request *request, const char *file, int line) {

    int     isprequest;
    char    pbuff[BUFFER_SIZE];
    int     retval;
    int     storage_index;

    /* Using Isend structure for Send_init */
    arg_t isp_struct;

    
    DebugPrint("Entering MPI_Send_init...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
    }

    memset(pbuff, '\0', BUFFER_SIZE);

    if (dest != MPI_PROC_NULL) {
        sprintf(pbuff, "%d", func_count);
    } else {
        sprintf(pbuff, "MPI_PROC_NULL_PERSIST");
    }

    /* issue a call to PMPI_Send_init to get the request; */
    retval = PMPI_Send_init(buf, count, datatype, dest, tag, comm, request);
    if(retval != MPI_SUCCESS) {
        return retval;
    }
    
    /* generate a dummy request to give it back to the client */
    isprequest = requestNo++;
    persistent_request_count++;

    isp_struct.buffer = buf;
    isp_struct.count = count;
    isp_struct.datatype =datatype;
    isp_struct.dest = dest;
    isp_struct.comm = comm;
    isp_struct.tag = tag;
    
    /* map this to the real request */
    HashMap_Insert(&rmap, isprequest, request, pbuff);

    /* MPI_PROC_NULL does not store any of the normal state */
    if (dest != MPI_PROC_NULL) {
        /* Add this request to our persistent request database 
         * Also get back a position where we can store the data */
        storage_index = get_persistent_request_index(isprequest);

        isp_struct.request = HashMap_GetRequest(&rmap, isprequest);

        /* store the information of the request to the given storage place */
        persistent_argarray[storage_index].func = SEND_INIT;
        persistent_argarray[storage_index] = isp_struct;
    }

    /* send the dummy request back */
    *((int *) request) = isprequest;
    InsertIntoMatchHashMap (request, TYPE_PERSIST_REQUEST, file, line);

    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Recv (void *buffer, int count, MPI_Datatype datatype, int source, 
        int tag, MPI_Comm comm, MPI_Status *status, const char *file, int line) {

    char        sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int         nprocs;

    DebugPrint("Entering MPI_Recv...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Recv(buffer, count, datatype, source, tag, comm, status);
    }
    /* 
     * If MPI_PROC_NULL, issue immediately, and don't issue in to scheduler.
     * The scheduler will treat it as MPI_ANY_SOURCE, which is not right.
     */
    if (source == MPI_PROC_NULL)
    {
        return PMPI_Recv (buffer, count, datatype, source, tag, comm, status);
    }

    memset (sbuff, '\0', BUFFER_SIZE);
    //buff = Profiler_create_group_ranks (comm, &nprocs);

    /*
     * Recv Envelope format: "Recv source tag comm"
     */
    buff = Profiler_create_group_ranks (comm, &nprocs);
    //CGD added %d and the corresponding datatype
    sprintf (sbuff, "%d %s %s %d %u %s %d %s %d %s", func_count, "Recv", "Recv",
             datatype, 
             (unsigned int) strlen(file), file, line,
             TranslateRanks(source, comm, MPI_COMM_WORLD), tag, buff);
    free (buff);
    set_common_args (RECV);

#ifdef DEBUG_PROF_MEM
    if(mem.find(argarray[func_count].buffer) != mem.end()) {
        free(argarray[func_count].buffer);
    }
#endif

    argarray[func_count].buffer = buffer;
    argarray[func_count].count = count;
    argarray[func_count].datatype =datatype;
    argarray[func_count].source = source;
    argarray[func_count].tag = tag;
    argarray[func_count].comm = comm;
    argarray[func_count].status = status;
    
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Recv\n");
        return -1;
    }

    argarray[func_count].buffer = 0;
    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Probe (int source, int tag, MPI_Comm comm, MPI_Status *status, 
                      const char *file, int line) {

    char        sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int         nprocs;

    DebugPrint("Entering MPI_Probe...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Probe(source, tag, comm, status);
    }

    memset (sbuff, '\0', BUFFER_SIZE);

    /*
     * Recv Envelope format: "Probe source tag comm "
     */
    buff = Profiler_create_group_ranks (comm, &nprocs);
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %s %d %s", func_count, "Probe", "Probe",
                0, 
                (unsigned int) strlen(file), file, line,
                TranslateRanks(source, comm, MPI_COMM_WORLD), tag, buff);
    free (buff);
    set_common_args(PROBE);
    argarray[func_count].source = source;
    argarray[func_count].tag = tag;
    argarray[func_count].comm = comm;
    argarray[func_count].status = status;
    
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Probe\n");
        return -1;
    }

    return MPI_SUCCESS;
}
DLLAPI int MPIAPI ISP_Iprobe (int source, int tag, MPI_Comm comm, int *flag, 
                       MPI_Status *status, const char *file, int line) {

    char        sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int         nprocs;
    arg_t isp_struct_temp;
    *flag = 0;
    DebugPrint("Entering MPI_Iprobe...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Iprobe(source, tag, comm, flag, status);
    }

    memset (sbuff, '\0', BUFFER_SIZE);

    if  (argarray[func_count-1].func == IPROBE) {
        isp_struct_temp = argarray[func_count-1];
        /* If this is a repeat Iprobe - we don't have to trap it */
        if (isp_struct_temp.source == source && isp_struct_temp.tag == tag &&
            isp_struct_temp.comm == comm && isp_struct_temp.flag == flag &&
            isp_struct_temp.status == status) 
            return PMPI_Iprobe(source, tag, comm, flag, status);
            
    }
    /*
     * Iprobe Envelope format: "Probe source tag comm "
     */
    buff = Profiler_create_group_ranks (comm, &nprocs);
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %s %d %s", func_count, "Iprobe", "Iprobe",
                0, 
                (unsigned int) strlen(file), file, line,
                TranslateRanks(source, comm, MPI_COMM_WORLD), tag, buff);
    free (buff);
    set_common_args(IPROBE);
    argarray[func_count].source = source;
    argarray[func_count].tag = tag;
    argarray[func_count].comm = comm;
    argarray[func_count].flag = flag;
    argarray[func_count].status = status;
    
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Iprobe\n");
        return -1;
    }


    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Irecv (void *buf, int count, MPI_Datatype datatype, int source,
        int tag, MPI_Comm comm, MPI_Request *request, const char *file, int line) {

    char           sbuff[BUFFER_SIZE];
    char           pbuff[BUFFER_SIZE];
    char           *buff = NULL;
    int            nprocs;
    int            isprequest = requestNo++;
    static int     i;
    int            retval;

    DebugPrint("Entering MPI_Irecv...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
    }

    /* This makes sure there is a wait for each request. */
    *((int *)request) = isprequest;
    InsertIntoMatchHashMap (request, TYPE_REQUEST, file, line);

    /* 
     * If MPI_PROC_NULL, issue immediately, and don't issue in to scheduler.
     * The scheduler will treat it as MPI_ANY_SOURCE, which is not right.
     */
    if (source == MPI_PROC_NULL) {
        retval = PMPI_Irecv (buf, count, datatype, source, tag, comm, request);
        HashMap_Insert (&rmap, isprequest, request, "MPI_PROC_NULL");
        *((int *)request) = isprequest;
        request_count++;
        return retval;
    }

    memset (sbuff, '\0', BUFFER_SIZE);
    memset (pbuff, '\0', BUFFER_SIZE);
    buff = Profiler_create_group_ranks (comm, &nprocs);

    /*
     * Send the Isend Envelope to the server. The following is the
     * format of the Isend Envelope:
     *
     * "Irecv dest tag i MPI_COMM_WORLD"
     */ 
    i = func_count;
    //CGD added %d and the corresponding datatype
    sprintf (sbuff, "%d %s %s %d %u %s %d %s %d %d %s", func_count, "Irecv","Irecv",
    		 		datatype,
             (unsigned int) strlen(file), file, line,
             TranslateRanks(source, comm, MPI_COMM_WORLD), tag, i, buff);
    free (buff);
    sprintf (pbuff, "%d", func_count);

    set_common_args (IRECV);

#ifdef DEBUG_PROF_MEM
    if(mem.find(argarray[func_count].buffer) != mem.end()) {
        free(argarray[func_count].buffer);
    }
#endif

    argarray[func_count].buffer = buf;
    argarray[func_count].count = count;
    argarray[func_count].datatype =datatype;
    argarray[func_count].source = source;
    argarray[func_count].tag = tag;
    argarray[func_count].comm = comm;
    /* need to allocate the status here - because we might force a completion
     * of Irecv without having seen the Wait yet */
    argarray[func_count].status = (MPI_Status*) malloc (sizeof (MPI_Status));

    HashMap_Insert(&rmap, isprequest, request, pbuff);
    argarray[func_count].request = HashMap_GetRequest(&rmap, isprequest);
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Irecv\n");
        return -1;
    }
    request_count++;

    argarray[func_count].buffer = 0;
    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Recv_init (void *buf, int count, MPI_Datatype datatype, int src,
        int tag, MPI_Comm comm, MPI_Request *request, const char *file, int line) {

    int     isprequest;
    char    pbuff[BUFFER_SIZE];
    int     retval;
    int     storage_index;

    /* Using Irecv structure for Recv_init */
    arg_t isp_struct;    
    DebugPrint("Entering MPI_Recv_init...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Recv_init(buf, count, datatype, src, tag, comm, request);
    }

    memset(pbuff, '\0', BUFFER_SIZE);

    if (src != MPI_PROC_NULL) {
        sprintf(pbuff, "%d", func_count);
    } else {
        sprintf(pbuff, "MPI_PROC_NULL_PERSIST");
    }

    /* issue a call to PMPI_Send_init to get the request; */
    retval = PMPI_Recv_init(buf, count, datatype, src, tag, comm, request);
    if(retval != MPI_SUCCESS) {
        return retval;
    }
    
    /* generate a dummy request to give it back to the client */
    isprequest = requestNo++;
    persistent_request_count++;
    
    isp_struct.buffer = buf;
    isp_struct.count = count;
    isp_struct.datatype =datatype;
    isp_struct.source = src;
    isp_struct.comm = comm;
    isp_struct.tag = tag;

    /* map this to the real request */
    HashMap_Insert(&rmap, isprequest, request, pbuff);

    /* MPI_PROC_NULL does not store any of the normal state */
    if (src != MPI_PROC_NULL) {
        /* Add this request to our persistent request database 
         * Also get back a position where we can store the data */
        storage_index = get_persistent_request_index(isprequest);

        isp_struct.request = HashMap_GetRequest(&rmap, isprequest);

        /* store the information of the request to the given storage place */
        persistent_argarray[storage_index].func = RECV_INIT;
        persistent_argarray[storage_index] = isp_struct;
    }

    /* send the dummy request back */
    *((int *) request) = isprequest;
    InsertIntoMatchHashMap (request, TYPE_PERSIST_REQUEST, file, line);

    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Wait (MPI_Request *request, MPI_Status *status, const char *file,
        int line) {
    char           sbuff[BUFFER_SIZE];
    char           *env = NULL;
    int            isprequest;
    int            request_type = -1;

    DebugPrint("Entering MPI_Wait...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Wait(request, status);
    }

    RemoveFromMatchHashMap (request, TYPE_REQUEST, file, line);

    memset (sbuff, '\0', BUFFER_SIZE);
    /*
     * Send the Wait Envelope to the server. The following is the
     * format of the Wait Envelope:
     *
     * "Wait Irecv dest tag comm"
     * "Wait Isend src tag comm"
     */ 
    env = HashMap_GetEnv (&rmap, *((int *)request));
    if (!env) {
        return 0;
    }

    /*
     * If it is an MPI_PROC_NULL, return success immediately. MPI_PROC_NULLs
     * are not issued in to the ISP scheduler.
     */
    isprequest = *((int *)request);
    if (!strcmp(env, "MPI_PROC_NULL"))
    {
        request_count--;
        *request = *HashMap_GetRequest (&rmap, isprequest);
        HashMap_Delete (&rmap, isprequest);
        return PMPI_Wait (request, status);
    }
    else if (!strcmp(env, "MPI_PROC_NULL_PERSIST")) {
        request_count--;
        return PMPI_Wait (HashMap_GetRequest (&rmap, isprequest), status);
    }

    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %s", func_count, "Wait",
                "Wait", 0,
                (unsigned int) strlen(file), file, line, env);
    set_common_args (WAIT);
    request_count--;

    argarray[func_count].request = request;
    argarray[func_count].status = status;
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Wait\n");
        return -1;
    }

    request_type = 
        persistent_argarray[get_persistent_request_index(isprequest)].func;

    if (request_type != IRECV && request_type != ISEND) {
        *request = *HashMap_GetRequest(&rmap, isprequest);
    }

    if (argarray[atoi(env)].func == IRECV || request_type == RECV_INIT) {
        setIrecvReq (atoi(env), status, 0);
        if (argarray[atoi(env)].func == IRECV) {
            HashMap_Delete (&rmap, isprequest);
        }
        return MPI_SUCCESS;
    }
    if (argarray[atoi(env)].func == ISEND || request_type == SEND_INIT) {
        setIsendReq (atoi(env), request, status, 0);
        if (argarray[atoi(env)].func == ISEND) {

        }
        return MPI_SUCCESS;
    }

    /* requests that ISP cannot handle right now */
    assert(0);
    HashMap_Delete (&rmap, isprequest);

    return PMPI_Wait (request, status);
}    

DLLAPI int MPIAPI ISP_Test (MPI_Request *request, int *flag, MPI_Status *status,
        const char *file, int line) {
    char           sbuff[BUFFER_SIZE];
    char           *env = NULL;
    int            isprequest;
    int            request_type;
    MPI_Request    request_copy;

    DebugPrint("Entering MPI_Test...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Test(request, flag, status);
    }

    memset (sbuff, '\0', BUFFER_SIZE);
    /*
     * Send the Test Envelope to the server. The following is the
     * format of the Test Envelope:
     *
     * "Test Irecv_index comm"
     * "Test Isend_index comm"
     */ 

    env = HashMap_GetEnv (&rmap, *((int *)request));
    if (!env) {
        RemoveFromMatchHashMap (request, TYPE_REQUEST, file, line);
        *flag = 1;
        return 0;
    }
    
    /*
     * If it is an MPI_PROC_NULL, wait for the request and return success
     * immediately. MPI_PROC_NULLs are not issued in to the ISP scheduler.
     * A wait is used here instead of a Test, since it will always complete.
     */
    isprequest = *((int *)request);
    if (!strcmp(env, "MPI_PROC_NULL")) {
        RemoveFromMatchHashMap (request, TYPE_REQUEST, file, line);
        *flag = 1;
        request_count--;
        *request = *HashMap_GetRequest (&rmap, isprequest);
        HashMap_Delete (&rmap, isprequest);
        return PMPI_Wait (request, status);

    } else if (!strcmp(env, "MPI_PROC_NULL_PERSIST")) {
        RemoveFromMatchHashMap (request, TYPE_REQUEST, file, line);
        *flag = 1;
        request_count--;
        return PMPI_Wait (HashMap_GetRequest (&rmap, isprequest), status);
    }

    //CGD added %d and the corresponding datatype
    sprintf (sbuff, "%d %s %s %d %u %s %d %s", func_count, "Test", "Test",
             0,
             (unsigned int) strlen(file), file, line, env);

    set_common_args (TEST);
    argarray[func_count].request = request;
    argarray[func_count].status = status;
    argarray[func_count].flag = flag;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Test\n");
        return -1;
    }
    isprequest = *((int *)request);
    request_type = 
        persistent_argarray[get_persistent_request_index(isprequest)].func;
    
    request_copy = *request;
    if (request_type != SEND_INIT && request_type != RECV_INIT) {
        *request = *HashMap_GetRequest (&rmap, isprequest);
    }
    

    /* If this happens, this means the test call was completed but no 
     * match has been found - return false to flag.
     * However, there's a subtle issue here. This only works with the Irecv
     * With the Isend and buffering, we need to return true, since the send 
     * will be buffered!!!*/
    if (isprequest == * ((int *) HashMap_GetRequest(&rmap, isprequest))) {
        if (argarray[atoi(env)].func == IRECV ||
            argarray[atoi(env)].func == RECV_INIT || send_block) {
            *flag = 0;
            return MPI_SUCCESS;
        }
    }

    request_count--;
    if (argarray[atoi(env)].func == IRECV ||request_type == RECV_INIT) {
        RemoveFromMatchHashMap (&request_copy, TYPE_REQUEST, file, line);
        setIrecvReq (atoi(env), status, 0);
        *flag = 1;
        if (argarray[atoi(env)].func == IRECV) {
            HashMap_Delete (&rmap, isprequest);
        }
        return 0;
    }
    if (argarray[atoi(env)].func == ISEND || request_type == SEND_INIT) {
        RemoveFromMatchHashMap (&request_copy, TYPE_REQUEST, file, line);
        setIsendReq (atoi(env), request, status, 0);
        *flag = 1;
        if (argarray[atoi(env)].func == ISEND) {
            /*
            HashMap_Delete (&rmap, isprequest);
            */
        }
        return 0;
    }
    while (1) {
        PMPI_Test (request, flag, status);
        if (*flag) {
            break;
        }
    }
    RemoveFromMatchHashMap (&request_copy, TYPE_REQUEST, file, line);
    HashMap_Delete (&rmap, isprequest);
    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Start (MPI_Request* request, const char *file, int line) {

    char            sbuff[BUFFER_SIZE];
    char            *pbuff = NULL;
    char            *env = NULL;
    char            *buff = NULL;
    int             nprocs;
    int             request_type;
    int             storage_index;
    int             isprequest;

    DebugPrint ("Entering MPI_Start...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Start(request);
    }

    /* If MPI_PROC_NULL, issue immediately. */
    isprequest = *((int*)request);
    env = HashMap_GetEnv (&rmap, isprequest);
    if (!strcmp(env, "MPI_PROC_NULL") || !strcmp(env, "MPI_PROC_NULL_PERSIST")) {
        request_count++;
        InsertIntoMatchHashMap(request, TYPE_REQUEST, file, line);
        return PMPI_Start(HashMap_GetRequest (&rmap, isprequest));
    }

    storage_index = get_persistent_request_index((*(int*)request));

    request_type = persistent_argarray[storage_index].func;
    /* We are in trouble if this request is not created before */
    assert (request_type == SEND_INIT ||
            request_type == RECV_INIT);

    /* Now we need to update the envelope with the correct func_count */
    memset(sbuff, '\0', BUFFER_SIZE);
    pbuff = (char*) malloc(BUFFER_SIZE);
    sprintf(pbuff, "%d", func_count);
    
    HashMap_UpdateEnv(&rmap, *((int *) request), pbuff);

    if (!env) {
        /* this is bad, it means we havent seen this request before */
        assert (0);
        return 0;
    }

    set_common_args(START);
    /* store the index of the 'owner' of the request 
       We'll need it later to find out whether this is from a send or recv */
    argarray[func_count].persistent_index = storage_index;

    
    /* if this is a request from SEND_INIT */
    if (request_type == SEND_INIT) {
        /* copy the information from the arguments of the send_init call 
         * earlier */
        if (!send_block) {
            copyBufferNonBlockingSend 
                (persistent_argarray[storage_index].buffer, &argarray[func_count].buffer,
                 persistent_argarray[storage_index].count, 
                 persistent_argarray[storage_index].datatype, 
                 &argarray[func_count].buffer_copied);
            copyDatatypeNonBlockingSend (persistent_argarray[storage_index].datatype, 
                                         &argarray[func_count].datatype,
                                         &argarray[func_count].datatype_copied);
        }
        argarray[func_count].comm = persistent_argarray[storage_index].comm;
        argarray[func_count].dest = persistent_argarray[storage_index].dest;
        argarray[func_count].tag = persistent_argarray[storage_index].tag;
        argarray[func_count].count = persistent_argarray[storage_index].count;
        

        buff = Profiler_create_group_ranks(argarray[func_count].comm, &nprocs);
        
        /* Have the scheduler treat this call as an Isend */
        //CGD added %d and the corresponding 0
        sprintf(sbuff, "%d %s %s %d %u %s %d %d %d %d %s", func_count, "Isend", 
                    "Start", 0,
                    (unsigned int) strlen(file), file, line, argarray[func_count].dest, 
                    argarray[func_count].tag, func_count, buff);
        
        if (SendnRecv(sbuff, strlen(sbuff)) < 0) {
            printf("Client unable to SendnRecv in MPI_Start\n");
            return -1;
        }

        /* This makes sure there is a wait for each request. */
        InsertIntoMatchHashMap(request, TYPE_REQUEST, file, line);

        request_count++;
        free(buff);
        return PMPI_Start(argarray[func_count-1].request);
        
    /* Handling requests coming from RECV_INIT */
    } else {
        /* copy the arguments information from the recv_init earlier */
        argarray[func_count] =  persistent_argarray[storage_index];
        /* also need to allocate a status now */
        argarray[func_count].status = (MPI_Status*) malloc (sizeof(MPI_Status));

        buff = Profiler_create_group_ranks(argarray[func_count].comm, &nprocs);
        
        /* Have the scheduler treat this as an Irecv */
        //CGD added %d and the corresponding 0
        sprintf(sbuff, "%d %s %s %d %u %s %d %d %d %d %s", func_count, "Irecv", 
                "Start", 0,
                (unsigned int) strlen(file), file, line, argarray[func_count].source, 
                argarray[func_count].tag, func_count, buff);
        
        if (SendnRecv(sbuff, strlen(sbuff)) < 0) {
            printf("Client unable to SendnRecv in MPI_Start\n");
            return -1;
        }

        /* This makes sure there is a wait for each request. */
        InsertIntoMatchHashMap(request, TYPE_REQUEST, file, line);

        request_count++;
        free(buff);
        return MPI_SUCCESS;
    }
}
DLLAPI int MPIAPI ISP_Startall(int count, MPI_Request *array_of_requests, const char *file, int line) {

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Startall(count, array_of_requests);
    }

    return MPI_SUCCESS;
}
DLLAPI int MPIAPI ISP_Request_free(MPI_Request *request, const char *file, int line) {
    char            *env = NULL;
    int             isprequest;
    int             storage_index;
    int             request_type;

    DebugPrint ("Entering MPI_Request_free...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Request_free(request);
    }

    /* If MPI_PROC_NULL, free the request. */
    isprequest = *((int*)request);
    env = HashMap_GetEnv (&rmap, isprequest);
    if (!env) {
        return MPI_ERR_REQUEST;
    } else if (!strcmp(env, "MPI_PROC_NULL_PERSIST")) {
        persistent_request_count--;
        RemoveFromMatchHashMap (request, TYPE_PERSIST_REQUEST, file, line);
        *request = *HashMap_GetRequest (&rmap, isprequest);
        HashMap_Delete (&rmap, isprequest);
        return PMPI_Request_free(request);
    }

    /* If persistant request, mark that resource free. Rest isn't implemented
     * yet. */
    storage_index = get_persistent_request_index(isprequest);
    request_type = persistent_argarray[storage_index].func;
    if (request_type == SEND_INIT || request_type == RECV_INIT) {
        persistent_request_count--;
        RemoveFromMatchHashMap (request, TYPE_PERSIST_REQUEST, file, line);
    }

    /* TODO: Not implemented for non MPI_PROC_NULL. */
    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Ssend (void *buffer, int count, MPI_Datatype datatype, int dest, 
        int tag, MPI_Comm comm, const char *file, int line) {

    char        sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int         nprocs;
    int         retVal;

    DebugPrint("Entering MPI_Ssend...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Ssend(buffer, count, datatype, dest, tag, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);
    /*
     * Ssend Envelope format : "Ssend dest tag comm"
     * However, since ISP sees everything as in MPI_COMM_WORLD, we 
     * will need to translate the destination into its MPI_COMM_WORLD
     */
     
    buff = Profiler_create_group_ranks (comm, &nprocs);
    //CGD added %d and the corresponding datatype
    sprintf (sbuff, "%d %s %s %d %u %s %d %s %d %s",func_count, "Ssend",
            "Ssend", datatype,
            (unsigned int) strlen(file), file, line,
            TranslateRanks(dest, comm, MPI_COMM_WORLD), tag, buff);
    free (buff);
    set_common_args (SSEND);

#ifdef DEBUG_PROF_MEM
    if(mem.find(argarray[func_count].buffer) != mem.end()) {
        free(argarray[func_count].buffer);
    }
#endif

    argarray[func_count].buffer = buffer;
    argarray[func_count].count = count;
    argarray[func_count].datatype =datatype;
    argarray[func_count].dest = dest;
    argarray[func_count].tag = tag;
    argarray[func_count].comm = comm;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Ssend\n");
        return -1;
    }
    
    retVal = PMPI_Ssend(buffer, count, datatype, dest, tag, comm);
    argarray[func_count].buffer = 0;
    return retVal;
    
}

DLLAPI int MPIAPI ISP_Sendrecv (void * sendbuf, int sendcount, MPI_Datatype sendtype, 
        int dest, int sendtag, void * recvbuf, int recvcount, 
        MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, 
        MPI_Status *status, const char *file, int line) {

    DebugPrint("Entering MPI_Sendrecv...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf,
                recvcount, recvtype, source, recvtag, comm, status);
    }

    ISP_Send (sendbuf, sendcount, sendtype, dest, sendtag, comm, file, line);
    ISP_Recv (recvbuf, recvcount, recvtype, source, recvtag, comm, status, file, line);
    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Barrier (MPI_Comm comm, const char *file, int line) {

    char          sbuff[BUFFER_SIZE];
    char          *buff = NULL;
    int           nprocs;
    static int    count = 1;

    DebugPrint("Entering MPI_Barrier...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Barrier(comm);
    }

    /*
     * Barrier Envelope consists of: 
     * "Barrier 3 2 1:2:3" 
     */

    memset (sbuff, '\0', BUFFER_SIZE);
    buff = Profiler_create_group_ranks (comm, &nprocs);  
    //CGD added %d and the corresponding 0  
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Barrier", 
            "Barrier", 0,
            (unsigned int) strlen(file), file, line, nprocs, count, buff);
    free (buff);
    set_common_args (BARRIER);
    argarray[func_count].comm = comm;
    count++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Barrier\n");
        return -1;
    }

    return MPI_SUCCESS;
}
DLLAPI int MPIAPI ISP_Cart_create ( MPI_Comm comm_old, int ndims, int *dims, int *periods, 
                             int reorder, MPI_Comm *comm_cart, const char* file, int line){
    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int        nprocs;
    static int count = 1;

    DebugPrint("Entering MPI_Cart_create...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Cart_create(comm_old, ndims, dims, periods, reorder, comm_cart);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm_old, &nprocs);
    /*
     * Cart_create envelope : "Cart_create 3 1:2:3"
     */
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Cart_create",
            "Cart_create", 0,
            (unsigned int) strlen(file), file, line, nprocs, count, buff);
    free (buff);
    set_common_args (CART_CREATE);

    argarray[func_count].comm = comm_old;
    argarray[func_count].group = MPI_GROUP_NULL;
    argarray[func_count].color = 0;
    argarray[func_count].key = 0;
    argarray[func_count].ndims = ndims;
    argarray[func_count].dims = (int*) malloc (ndims * sizeof(int));
    argarray[func_count].periods = (int*) malloc (ndims * sizeof(int));
    bcopy(dims, argarray[func_count].dims, ndims*sizeof(int));
    bcopy(periods, argarray[func_count].periods, ndims*sizeof(int));
    argarray[func_count].reorder = reorder;
    argarray[func_count].comm_out = comm_cart;
    count++;
    
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Cart_create\n");
        return -1;
    }

    comm_count++;
    InsertIntoMatchHashMap (comm_cart, TYPE_COMM, file, line);

    /*PMPI_Comm_create (comm, group, comm_out);*/

    return MPI_SUCCESS;

}

DLLAPI int MPIAPI ISP_Comm_create (MPI_Comm comm, MPI_Group group, MPI_Comm *comm_out,
        const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int        nprocs;
    static int count = 1;

    DebugPrint("Entering MPI_Comm_create...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Comm_create(comm, group, comm_out);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Comm_create envelope : "Comm_create 3 1:2:3"
     */
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Comm_create",
            "Comm_create", 0,
            (unsigned int) strlen(file), file, line, nprocs, count, buff);
    free (buff);
    set_common_args (COMM_CREATE);
    argarray[func_count].comm = comm;
    argarray[func_count].group = group;
    argarray[func_count].color = 0;
    argarray[func_count].key = 0;
    argarray[func_count].comm_out = comm_out;
    count++;
    
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Comm_create\n");
        return -1;
    }

    comm_count++;
    InsertIntoMatchHashMap (comm_out, TYPE_COMM, file, line);

    /*PMPI_Comm_create (comm, group, comm_out);*/

    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Comm_dup (MPI_Comm comm, MPI_Comm *comm_out, const char *file, 
                                int line) {

    char        sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int         nprocs;
    static int  count = 1;

    DebugPrint("Entering MPI_Comm_dup...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Comm_dup(comm, comm_out);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Bcast envelope : "Bcast 3 1:2:3"
     */
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Comm_dup",
                "Comm_dup", 0,
                (unsigned int) strlen(file), file, line, nprocs, count, buff);
    free (buff);
    set_common_args (COMM_DUP);
    argarray[func_count].comm = comm;
    argarray[func_count].group = MPI_GROUP_NULL;
    argarray[func_count].color = 0;
    argarray[func_count].key = 0;
    argarray[func_count].comm_out = comm_out;
    count++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Comm_dup\n");
        return -1;
    }
    comm_count++;
    InsertIntoMatchHashMap (comm_out, TYPE_COMM, file, line);
    /*PMPI_Comm_dup (comm, comm_out);*/

    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Comm_split (MPI_Comm comm, int color, int key, MPI_Comm *comm_out,
        const char *file, int line) {

    char        sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int         nprocs;
    static int  count = 1;

    DebugPrint("Entering MPI_Comm_split...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Comm_split(comm, color, key, comm_out);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Bcast envelope : "Bcast 3 1:2:3"
     */
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %d %s", func_count, "Comm_split",
                "Comm_split", 0,
                (unsigned int) strlen(file), file, line, nprocs, count, color, buff);
    free (buff);
    set_common_args (COMM_SPLIT);
    argarray[func_count].comm = comm;
    argarray[func_count].group = MPI_GROUP_NULL;
    argarray[func_count].color = color;
    argarray[func_count].key = key;
    argarray[func_count].comm_out = comm_out;
    count++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Comm_split\n");
        return -1;
    }
    comm_count++;
    InsertIntoMatchHashMap (comm_out, TYPE_COMM, file, line);
    /*PMPI_Comm_split (comm, color, key, comm_out);*/

    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Comm_free (MPI_Comm *comm, const char *file, int line) {

    char        sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int         nprocs;
    static int  count = 1;

    DebugPrint("Entering MPI_Comm_free...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Comm_free(comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (*comm, &nprocs);
    /*
     * Bcast envelope : "Bcast 3 1:2:3"
     */
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Comm_free",
            "Comm_free", 0,
            (unsigned int) strlen(file), file, line, nprocs, count, buff);
    free (buff);
    set_common_args (COMM_FREE);
    argarray[func_count].comm = MPI_COMM_NULL;
    argarray[func_count].group = MPI_GROUP_NULL;
    argarray[func_count].color = 0;
    argarray[func_count].key = 0;
    argarray[func_count].comm_out = comm;
    count++;

    RemoveFromMatchHashMap (comm, TYPE_COMM, file, line);

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Comm_free\n");
        return -1;
    }
    comm_count--;
    /*PMPI_Comm_free (comm);*/
    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Type_commit (MPI_Datatype *datatype, const char *file, int line) {

    DebugPrint("Entering MPI_Type_commit...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Type_commit(datatype);
    }

    type_count++;
    InsertIntoMatchHashMap (datatype, TYPE_TYPE, file, line);
    return PMPI_Type_commit(datatype);
}

DLLAPI int MPIAPI ISP_Type_free (MPI_Datatype *datatype, const char *file, int line) {

    DebugPrint("Entering MPI_Type_free...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Type_free(datatype);
    }

    type_count--;
    RemoveFromMatchHashMap (datatype, TYPE_TYPE, file, line);
    return PMPI_Type_free(datatype);
}

DLLAPI int MPIAPI ISP_Bcast (void *buffer, int count, MPI_Datatype datatype, int root,
        MPI_Comm comm, const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;

    DebugPrint("Entering MPI_Bcast...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Bcast(buffer, count, datatype, root, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Bcast envelope : "Bcast 3 1:2:3"
     */
    //CGD added %d and the corresponding datatype
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %s %s", func_count, "Bcast",
                "Bcast", datatype,
                (unsigned int) strlen(file), file, line, nprocs,
                TranslateRanks(root, comm, MPI_COMM_WORLD), buff);
    free (buff);
    set_common_args (BCAST);

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Bcast\n");
        return -1;
    }

    return PMPI_Bcast(buffer, count, datatype, root, comm);
}

DLLAPI int MPIAPI ISP_Scatter (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
        void *recvbuf, int recvcount, MPI_Datatype recvtype, 
        int root, MPI_Comm comm, const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;
    static int   bcount = 1;
    int type = 0;

    DebugPrint("Entering MPI_Scatter...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, root, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Scatter envelope : "Scatter 3 1:2:3"
     */
    //CGD added %d and the corresponding type which is 0 if the types match and -1 otherwise
    if(sendtype != recvtype){
    	type = -1;
    }
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %s %s", func_count, "Scatter",
                "Scatter", type,
                (unsigned int) strlen(file), file, line, nprocs,
                TranslateRanks(root, comm, MPI_COMM_WORLD), buff);
    free (buff);
    set_common_args (SCATTER);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Scatter\n");
        return -1;
    }
    return PMPI_Scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcount, 
                recvtype, root, comm);
}

DLLAPI int MPIAPI ISP_Scatterv (void *sendbuf, int *sendcnt, int *displs, MPI_Datatype sendtype, 
        void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm,
        const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;
    static int   bcount = 1;
    int type = 0;

    DebugPrint("Entering MPI_Scatterv...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Scatterv(sendbuf, sendcnt, displs, sendtype, recvbuf, recvcount, recvtype, root, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    //CGD added %d and the corresponding type which is 0 if the types match and -1 otherwise
    if(sendtype != recvtype){
     	type = -1;
    }
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %s %s", func_count, "Scatterv",
                "Scatterv", type,
                (unsigned int) strlen(file), file, line, nprocs,
                TranslateRanks(root, comm, MPI_COMM_WORLD), buff);
    free (buff);
    set_common_args (SCATTERV);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Scatterv\n");
        return -1;
    }
    return PMPI_Scatterv(sendbuf, sendcnt, displs, sendtype, recvbuf, 
                recvcount, recvtype, root, comm);
}

DLLAPI int MPIAPI ISP_Gatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
        void *recvbuf, int *recvcount, int *displs, MPI_Datatype recvtype, 
        int root, MPI_Comm comm, const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;
    static int   bcount = 1;
    int type = 0;

    DebugPrint("Entering MPI_Gatherv...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcount, displs, recvtype, root, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Gatherv envelope : "Gatherv 3 1:2:3"
     */
    //CGD added %d and the corresponding type which is 0 if the types match and -1 otherwise
    if(sendtype != recvtype){
       	type = -1;
    }
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %s %s", func_count, "Gatherv",
                "Gatherv", type,
                (unsigned int) strlen(file), file, line, nprocs,
                TranslateRanks(root, comm, MPI_COMM_WORLD), buff);
    free (buff);
    set_common_args (GATHERV);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Gatherv\n");
        return -1;
    }
    return PMPI_Gatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcount, 
                displs, recvtype, root, comm);
}

DLLAPI int MPIAPI ISP_Gather (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
        void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
        MPI_Comm comm, const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;
    static int   bcount = 1;
    int type = 0;

    DebugPrint("Entering MPI_Gather...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, root, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Gather envelope : "Gather 3 1:2:3"
     */
    //CGD added %d and the corresponding type which is 0 if the types match and -1 otherwise
    if(sendtype != recvtype){
       	type = -1;
    }
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %s %s", func_count, "Gather", "Gather",
    						type,
                (unsigned int) strlen(file), file, line, nprocs,
                TranslateRanks(root, comm, MPI_COMM_WORLD), buff);
    free (buff);
    set_common_args (GATHER);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Gather\n");
        return -1;
    }
    return PMPI_Gather(sendbuf, sendcnt, sendtype, recvbuf, 
                recvcount, recvtype, root, comm);
}

DLLAPI int MPIAPI ISP_Allgather (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
        void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
        const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;
    static int   bcount = 1;
    int type = 0;

    DebugPrint("Entering MPI_Allgather...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Allgather(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Allgather envelope : "Allgather 3 1:2:3"
     */
    //CGD added %d and the corresponding type which is 0 if the types match and -1 otherwise
    if(sendtype != recvtype){
       	type = -1;
    }
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Allgather",
            "Allgather", type,
            (unsigned int) strlen(file), file, line, nprocs, bcount, buff);
    free (buff);
    set_common_args (ALLGATHER);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Allgather\n");
        return -1;
    }
    return PMPI_Allgather(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, comm);
}

DLLAPI int MPIAPI ISP_Allgatherv (void *sendbuf, int sendcnt, MPI_Datatype sendtype, 
        void *recvbuf, int *recvcount, int *displs, MPI_Datatype recvtype,
        MPI_Comm comm, const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;
    static int   bcount = 1;
    int type = 0;

    DebugPrint("Entering MPI_Allgatherv...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Allgatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcount, displs, recvtype, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Allgather envelope : "Allgatherv 3 1:2:3"
     */
    //CGD added %d and the corresponding type which is 0 if the types match and -1 otherwise
    if(sendtype != recvtype){
       	type = -1;
    }
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Allgatherv",
            "Allgatherv", type,
            (unsigned int) strlen(file), file, line, nprocs, bcount, buff);
    free (buff);
    set_common_args (ALLGATHERV);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Allgatherv\n");
        return -1;
    }
    return PMPI_Allgatherv(sendbuf, sendcnt, sendtype, recvbuf, recvcount, 
            displs, recvtype, comm);
}

DLLAPI int MPIAPI ISP_Alltoall (void *sendbuf, int sendcnt, MPI_Datatype sendtype,
        void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm,
        const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;
    static int   bcount = 1;
    int type = 0;

    DebugPrint("Entering MPI_Alltoall...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Alltoall(sendbuf, sendcnt, sendtype, recvbuf, recvcount, recvtype, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Alltoall envelope : "Alltoall 3 1:2:3"
     */
    //CGD added %d and the corresponding type which is 0 if the types match and -1 otherwise
    if(sendtype != recvtype){
       	type = -1;
    }
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Alltoall",
                "Alltoall", type,
                (unsigned int) strlen(file), file, line, nprocs, bcount, buff);
    free (buff);
    set_common_args (ALLTOALL);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Alltoall\n");
        return -1;
    }
    return PMPI_Alltoall(sendbuf, sendcnt, sendtype, recvbuf, recvcount, 
                recvtype, comm);
}

DLLAPI int MPIAPI ISP_Alltoallv (void *sendbuf, int *sendcnt, int *sdispls, 
            MPI_Datatype sendtype, void *recvbuf, int *recvcount, int *rdispls, 
            MPI_Datatype recvtype, MPI_Comm comm, const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;
    static int   bcount = 1;
    int type = 0;

    DebugPrint("Entering MPI_Alltoallv...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Alltoallv(sendbuf, sendcnt, sdispls, sendtype, recvbuf, recvcount, rdispls,
                recvtype, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Alltoall envelope : "Alltoallv 3 1:2:3"
     */
    //CGD added %d and the corresponding type which is 0 if the types match and -1 otherwise
    if(sendtype != recvtype){
       	type = -1;
    }
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Alltoallv",
                "Alltoallv", type,
                (unsigned int) strlen(file), file, line, nprocs, bcount, buff);
    free (buff);
    set_common_args (ALLTOALLV);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Alltoallv\n");
        return -1;
    }
    return PMPI_Alltoallv(sendbuf, sendcnt, sdispls, sendtype, recvbuf, 
                recvcount, rdispls, recvtype, comm);
}

DLLAPI int MPIAPI ISP_Scan (void *sendbuff, void *recvbuff, int count, MPI_Datatype datatype,
        MPI_Op op, MPI_Comm comm, const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;
    static int   bcount = 1;

    DebugPrint("Entering MPI_Scan...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Scan(sendbuff, recvbuff, count, datatype, op, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Scan envelope : "Scan 3 1:2:3"
     */
    //CGD added %d and the corresponding datatype
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Scan",
                "Scan", datatype,
                (unsigned int) strlen(file), file, line, nprocs, bcount, buff);
    free (buff);
    set_common_args (REDUCE);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Scan\n");
        return -1;
    }
    return PMPI_Scan (sendbuff, recvbuff, count, datatype, op, comm);
}

DLLAPI int MPIAPI ISP_Reduce (void *sendbuff, void *recvbuff, int count, 
        MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm,
        const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char         *buff = NULL;
    int          nprocs;

    DebugPrint("Entering MPI_Reduce...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Reduce(sendbuff, recvbuff, count, datatype, op, root, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Reduce envelope : "Reduce 3 1:2:3"
     */
    //CGD added %d and the corresponding datatype
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %s %s", func_count, "Reduce",
                "Reduce", datatype,
                (unsigned int) strlen(file), file, line, nprocs,
                TranslateRanks(root, comm, MPI_COMM_WORLD), buff);
    free (buff);
    set_common_args (REDUCE);

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Reduce\n");
        return -1;
    }

    return PMPI_Reduce (sendbuff, recvbuff, count, datatype, op, root, comm);
}

/* == fprs begin == */
DLLAPI int MPIAPI ISP_Exscan (void *sendbuff, void *recvbuff, int count, MPI_Datatype datatype,
        MPI_Op op, MPI_Comm comm, const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;
    static int   bcount = 1;

    DebugPrint("Entering MPI_Exscan...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Exscan(sendbuff, recvbuff, count, datatype, op, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Scan envelope : "Scan 3 1:2:3"
     */
    sprintf (sbuff, "%d %s %s %u %s %d %d %d %s", func_count, "Exscan", 
                "Exscan", (unsigned int) strlen(file), file, line, nprocs, bcount, buff);
    free (buff);
    set_common_args (REDUCE);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Exscan\n");
        return -1;
    }
    return PMPI_Exscan (sendbuff, recvbuff, count, datatype, op, comm);
}
/* == fprs end == */

DLLAPI int MPIAPI ISP_Reduce_scatter (void *sendbuff, void *recvbuff, int *recvcnt, 
        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int          nprocs;
    static int   bcount = 1;

    DebugPrint("Entering MPI_Reduce_scatter...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Reduce_scatter(sendbuff, recvbuff, recvcnt, datatype, op, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Reduce_scatter envelope : "Reduce 3 1:2:3"
     */
    //CGD added %d and the corresponding datatype
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Reduce_scatter",
                "Reduce_scatter", datatype,
                (unsigned int) strlen(file), file, line, nprocs, bcount, buff);
    free (buff);
    set_common_args (REDUCE_SCATTER);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Reduce_scatter\n");
        return -1;
    }
    return PMPI_Reduce_scatter (sendbuff, recvbuff, recvcnt, datatype, op, comm);
}

DLLAPI int MPIAPI ISP_Allreduce (void *sendbuff, void *recvbuff, int count, 
        MPI_Datatype datatype, MPI_Op op, MPI_Comm comm, const char *file, int line) {

    char         sbuff[BUFFER_SIZE];
    char        *buff = NULL;
    int            nprocs;
    static int    bcount = 1;

    DebugPrint("Entering MPI_Allreduce...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Allreduce(sendbuff, recvbuff, count, datatype, op, comm);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    buff = Profiler_create_group_ranks (comm, &nprocs);
    /*
     * Allreduce envelope : "AllReduce 3 1:2:3"
     */
    //CGD added %d and the corresponding datatype
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %d %s", func_count, "Allreduce",
            "Allreduce", datatype,
            (unsigned int) strlen(file), file, line, nprocs, bcount, buff);
    free (buff);
    set_common_args (ALLREDUCE);
    bcount++;

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Allreduce\n");
        return -1;
    }
    return PMPI_Allreduce(sendbuff, recvbuff, count, datatype, op, comm);
}
        
DLLAPI int MPIAPI ISP_Waitall (int count, MPI_Request *array_of_requests, 
        MPI_Status *array_of_statuses, const char *file, int line) {
        
    char       sbuff[BUFFER_SIZE];
    char       sbuff1[BUFFER_SIZE];
    int        i;
    int        offset = 0;
    char *     env = NULL;
    int        _count = 0;
    int        isprequest;
    int        request_type;

    DebugPrint("Entering MPI_Waitall...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Waitall(count, array_of_requests, array_of_statuses);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    memset (sbuff1, '\0', BUFFER_SIZE);    
    set_common_args (WAITALL);
    argarray[func_count].request = array_of_requests;
    argarray[func_count].status = array_of_statuses;
    
    for (i = 0 ; i < count ; i++) {
        env = HashMap_GetEnv (&rmap, *((int *)&array_of_requests[i]));
        if (env && strcmp(env, "MPI_PROC_NULL") && strcmp(env, "MPI_PROC_NULL_PERSIST")) {
            sprintf (sbuff1+offset, "%s ", env);
            offset = (int) strlen(sbuff1);
            _count++;
        }
    }
    /*
     * Waitall Envelope: WaitAll 
     */      
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %s", func_count, "Waitall",
                "Waitall", 0,
                (unsigned int) strlen(file), file, line, _count, sbuff1);
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Waitall\n");
        return -1;
    }

    for (i = 0 ; i < count ; i++) {
        RemoveFromMatchHashMap (&array_of_requests[i], TYPE_REQUEST, file, line);
        isprequest = *((int *)&array_of_requests[i]);
        env = HashMap_GetEnv (&rmap, isprequest);
        
        /* MPI_PROC_NULL requests aren't sent to scheduler, so complete them here. */
        if (env && !strcmp(env, "MPI_PROC_NULL")) {
            _count++;
            array_of_requests[i] = *HashMap_GetRequest (&rmap, isprequest);
            HashMap_Delete (&rmap, isprequest);
            if (array_of_statuses == MPI_STATUSES_IGNORE) {
                PMPI_Wait (&array_of_requests[i], MPI_STATUS_IGNORE);
            } else {
                PMPI_Wait (&array_of_requests[i], &array_of_statuses[i]);
            }
        } else if (env && !strcmp(env, "MPI_PROC_NULL_PERSIST")) {
            _count++;
            if (array_of_statuses == MPI_STATUSES_IGNORE) {
                PMPI_Wait (HashMap_GetRequest (&rmap, isprequest), MPI_STATUS_IGNORE);
            } else {
                PMPI_Wait (HashMap_GetRequest (&rmap, isprequest), &array_of_statuses[i]);
            }
        } else if (env) {
            request_type =
                persistent_argarray[get_persistent_request_index(isprequest)].func;

            if (argarray[atoi(env)].func == IRECV ||
                argarray[atoi(env)].func == ISEND) {
                array_of_requests[i] = *HashMap_GetRequest(&rmap, isprequest);
            }
            if (argarray[atoi(env)].func == IRECV || 
                request_type == RECV_INIT) {
                setIrecvReq (atoi(env), array_of_statuses, i);
            }
            if (argarray[atoi(env)].func == ISEND || 
                request_type == SEND_INIT) {
                setIsendReq (atoi(env), &array_of_requests[i], 
                             array_of_statuses, i);
            }
            if (request_type != SEND_INIT && request_type != RECV_INIT){
                
                /*HashMap_Delete (&rmap, isprequest);*/
            }
                        
        } else {
            array_of_requests[i] = MPI_REQUEST_NULL;
        }
    }

    request_count -= _count;
    /*PMPI_Waitall (count, array_of_requests, array_of_statuses);*/
    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Waitany (int count, MPI_Request *array_of_requests, 
        int *index, MPI_Status *status, const char *file, int line) {
        
    char        sbuff[BUFFER_SIZE];
    char        sbuff1[BUFFER_SIZE];
    int         i;
    int         offset = 0;
    char        *env = NULL;
    int         *ilist = NULL;
    int         _count = 0;
    int         isprequest;
    int         request_to_complete;
    int         earliest_send;
    MPI_Request request_to_complete_copy; 


    DebugPrint("Entering MPI_Waitany...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Waitany(count, array_of_requests, index, status);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    memset (sbuff1, '\0', BUFFER_SIZE);    
    ilist = (int *)malloc (sizeof (int)*count);
    set_common_args (WAITANY);
    argarray[func_count].request= array_of_requests;
    argarray[func_count].status = status;
    for (i = 0 ; i < count ; i++) {
        env = HashMap_GetEnv (&rmap, *((int *)&array_of_requests[i]));
        if (env == NULL) {
            ilist[i] = -1;
            continue;
        } else if (!strcmp(env, "MPI_PROC_NULL") || !strcmp(env, "MPI_PROC_NULL_PERSIST")) {
            /* This completes an MPI_PROC_NULL if present. In the future, there probably
             * needs to be smarter handling. */
            RemoveFromMatchHashMap (&array_of_requests[i], TYPE_REQUEST, file, line);
            isprequest = *((int *)&array_of_requests[i]);
            request_count--;
            *index = i;
            if (!strcmp(env, "MPI_PROC_NULL")) {
                array_of_requests[i] = *HashMap_GetRequest (&rmap, isprequest);
                return PMPI_Wait (&array_of_requests[i], status);
            } else {
                return PMPI_Wait (HashMap_GetRequest (&rmap, isprequest), status);
            }
        } else if (argarray[atoi(env)].is_complete) {
            request_count--;
            if (argarray[atoi(env)].func == IRECV) {
                setIrecvReq (atoi(env), status, 0);
            } else if (argarray[atoi(env)].func == ISEND) {
                setIsendReq (atoi(env), &array_of_requests[i], status, 0);
            }
        }
        _count++;
        sprintf (sbuff1+offset, "%s ", env);
        ilist[i] = atoi(env);
        offset = (int) strlen(sbuff1);
    }    
    if (_count == 0) {
        free(ilist);
        *index = MPI_UNDEFINED;
        return MPI_SUCCESS;
    }
    /*
     * Waitany Envelope: WaitAny 
     */      
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %s", func_count, "Waitany",
            "Waitany", 0,
            (unsigned int) strlen(file), file, line, _count, sbuff1);
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Waitany\n");
        return -1;
    }
    
    earliest_send = -1;
    request_to_complete = -1;
    /* Find the request to complete, perferably something that is already complete.
     * If not, then the Isend with the lowest function ID. */
    for (i = 0 ; i < count ; i++) {
        if (ilist[i] == -1) {
            continue;
        }
        if (argarray[ilist[i]].func == ISEND &&
            (earliest_send == -1 || ilist[i] < earliest_send)) {
            earliest_send = ilist[i];
            request_to_complete = i;
        }
        if (argarray[ilist[i]].is_complete) {
            request_to_complete = i;
            break;
        }
    }

    /* There are no requests to complete. */
    if (request_to_complete == -1) {
        free (ilist);
        *index = MPI_UNDEFINED;
        return MPI_SUCCESS;
    }

    isprequest = *((int *)&array_of_requests[request_to_complete]);
    request_to_complete_copy = array_of_requests[request_to_complete];
    request_count--;
    if (argarray[ilist[request_to_complete]].func == IRECV ||
        argarray[ilist[request_to_complete]].func == ISEND) {
        *index = request_to_complete;
        array_of_requests[request_to_complete] = MPI_REQUEST_NULL;
        if (argarray[ilist[request_to_complete]].func == IRECV) {
            setIrecvReq (ilist[request_to_complete], status, 0);

        } else if (argarray[ilist[request_to_complete]].func == ISEND) {
            setIsendReq (ilist[request_to_complete],
                &array_of_requests[request_to_complete], status, 0);
        }
    } else {
        array_of_requests[request_to_complete] = *HashMap_GetRequest (&rmap, isprequest);
    }
    RemoveFromMatchHashMap (&request_to_complete_copy, TYPE_REQUEST, file, line);
    /*HashMap_Delete (&rmap, isprequest);*/
    free (ilist);
    return MPI_SUCCESS;
}

DLLAPI int MPIAPI ISP_Waitsome (int incount, MPI_Request *array_of_requests, 
        int *outcount, int *array_of_indices, MPI_Status *array_of_statuses,
        const char *file, int line) {
        
    int        result;
    char       sbuff[BUFFER_SIZE];
    char       sbuff1[BUFFER_SIZE];
    int        i;
    int        offset = 0;
    char       *env = NULL;
    int        *ilist = NULL;
    int        _count = 0;
    int        _procnull_count = 0;
    int        rcount = 0;
    int        isprequest;

    DebugPrint("Entering MPI_Waitsome...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Waitsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    memset (sbuff1, '\0', BUFFER_SIZE);    
    ilist = (int *)malloc (sizeof (int)*incount);
    set_common_args (WAITANY);
    argarray[func_count].request = array_of_requests;
    argarray[func_count].status = array_of_statuses;
    
    for (i = 0 ; i < incount ; i++) {
        env = HashMap_GetEnv (&rmap, *((int *)&array_of_requests[i]));
        if (env == NULL) {
            ilist[i] = -1;
            continue;
        }
        else if (!strcmp(env, "MPI_PROC_NULL")) {
            ilist[i] = -2;
            _procnull_count++;
            continue;
        }
        else if (!strcmp(env, "MPI_PROC_NULL_PERSIST")) {
            ilist[i] = -3;
            _procnull_count++;
            continue;
        }
        else if (argarray[atoi(env)].is_complete) {
            if (argarray[atoi(env)].func == IRECV) {
                setIrecvReq (atoi(env), array_of_statuses, i);
            } else if (argarray[atoi(env)].func == ISEND) {
                setIsendReq (atoi(env), &array_of_requests[i], array_of_statuses, i);
            }
        }
        _count++;
        sprintf (sbuff1+offset, "%s ", env);
        ilist[i] = atoi(env);
        offset = (int) strlen(sbuff1);
    }
    if (_count == 0 && _procnull_count == 0) {
        return 0;
    }    
    /*
     * Waitany Envelope: Waitany
     */ 
    if (_count != 0) {
        //CGD added %d and the corresponding 0
        sprintf (sbuff, "%d %s %s %d %u %s %d %d %s", func_count, "Waitany",
                    "Waitsome", 0,
                    (unsigned int) strlen(file), file, line, _count, sbuff1);
        if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
            printf ("Client unable to SendnRecv in MPI_Waitsome\n");
            return -1;
        }
    }
    _count += _procnull_count;
    for (i = 0 ; i < incount ; i++) {
        array_of_indices[i] = 0;
        if (ilist[i] == -1) {
            continue;
        }
        isprequest = *((int *)&array_of_requests[i]);
        if (ilist[i] == -2) { /* MPI_PROC_NULL */
            RemoveFromMatchHashMap (&array_of_requests[i], TYPE_REQUEST, file, line);
            request_count--;
            array_of_indices[i] = 1;
            array_of_requests[i] = *HashMap_GetRequest (&rmap, isprequest);
            HashMap_Delete (&rmap, isprequest);
            rcount++;
            if (array_of_statuses == MPI_STATUSES_IGNORE) {
                PMPI_Wait (&array_of_requests[i], MPI_STATUS_IGNORE);
            } else {
                PMPI_Wait (&array_of_requests[i], &array_of_statuses[i]);
            }
        } else if (ilist[i] == -3) { /* MPI_PROC_NULL persistant request */
            RemoveFromMatchHashMap (&array_of_requests[i], TYPE_REQUEST, file, line);
            request_count--;
            array_of_indices[i] = 1;
            rcount++;
            if (array_of_statuses == MPI_STATUSES_IGNORE) {
                PMPI_Wait (HashMap_GetRequest (&rmap, isprequest), MPI_STATUS_IGNORE);
            } else {
                PMPI_Wait (HashMap_GetRequest (&rmap, isprequest), &array_of_statuses[i]);
            }
        } else if (!argarray[ilist[i]].is_complete) {
            /* TODO: Fix this, it is completely wrong. I think the array_of_requests should
             * be set to MPI_REQUEST_NULL, and then the isprequest restored in to it after
             * the PMPI_Waitsome below. Any cases where it gets here but PMPI_Waitsome not
             * called? (I don't think so.)
             */
            *HashMap_GetRequest (&rmap, isprequest) = MPI_REQUEST_NULL;
        } else {
            request_count--;
            if (argarray[ilist[i]].func == IRECV || argarray[ilist[i]].func == ISEND) {
                RemoveFromMatchHashMap (&array_of_requests[i], TYPE_REQUEST, file, line);
                array_of_indices[i] = 1;
                array_of_requests[i] = MPI_REQUEST_NULL;
                rcount++;
            } else {
                array_of_requests[i] = *HashMap_GetRequest (&rmap, isprequest);
            }
            /*
            HashMap_Delete (&rmap, isprequest);
            */
        }
    }
    if (rcount == _count) {
        *outcount = _count;
        return 0;
    }
    result = PMPI_Waitsome (incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    *outcount+= _count;

    return result;
}

DLLAPI int MPIAPI ISP_Testany (int count, MPI_Request *array_of_requests, int *index,
        int *flag, MPI_Status *status, const char *file, int line) {
        
    int        result;
    char       sbuff[BUFFER_SIZE];
    char       sbuff1[BUFFER_SIZE];
    int        i;
    int        offset = 0;
    char       *env = NULL;
    int        *ilist = NULL;
    int        _count = 0;
    int        done = 0;
    int        no_completion;
    int        isprequest;


    DebugPrint("Entering MPI_Testany...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Testany(count, array_of_requests, index, flag, status);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    memset (sbuff1, '\0', BUFFER_SIZE);    
    ilist = (int *)malloc (sizeof (int)*count);

    set_common_args (TESTANY);
    argarray[func_count].request = array_of_requests;
    argarray[func_count].status = status;
    argarray[func_count].index = index;
    argarray[func_count].flag = flag;

    for (i = 0 ; i < count ; i++) {
        env = HashMap_GetEnv (&rmap, *((int *)&array_of_requests[i]));
        if (env == NULL) {
            ilist[i] = -1;
            continue;
        } else if (!strcmp(env, "MPI_PROC_NULL") || !strcmp(env, "MPI_PROC_NULL_PERSIST")) {
            /* This completes an MPI_PROC_NULL if present. In the future, there probably
             * needs to be smarter handling. */
            RemoveFromMatchHashMap (&array_of_requests[i], TYPE_REQUEST, file, line);
            request_count--;
            isprequest = *((int *)&array_of_requests[i]);
            *index = i;
            *flag = 1;
            if (!strcmp(env, "MPI_PROC_NULL")) {
                array_of_requests[i] = *HashMap_GetRequest (&rmap, isprequest);
                return PMPI_Wait (&array_of_requests[i], status);
            } else {
                return PMPI_Wait (HashMap_GetRequest (&rmap, isprequest), status);
            }
        } else if (argarray[atoi(env)].is_complete) {
            if (argarray[atoi(env)].func == IRECV) {
                setIrecvReq (atoi(env), status, i);
            } else if (argarray[atoi(env)].func == ISEND) {
                setIsendReq (atoi(env), &array_of_requests[i], status, i);
            }
        }
        _count++;
        sprintf (sbuff1+offset, "%s ", env);
        ilist[i] = atoi(env);
        offset = (int) strlen(sbuff1);
    }    
    /*
     * Waitall Envelope: WaitAll 
     */      
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %s", func_count, "Testany", "Testany",
    						0,
                (unsigned int) strlen(file), file, line, _count, sbuff1);
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Testany\n");
        return -1;
    }

    /* need to initialized no_completion to true, 
     * otherwise bypassing Test won't work*/
    no_completion = 1;

    for (i = 0 ; i < count ; i++) {
        if (ilist[i] == -1) {
            continue;
        }
        isprequest = *((int *)&array_of_requests[i]);

        /* If this happens, this means the test call was completed but no 
         * match has been found - if this happens to all of the requests
         * we'll return false here */
        env = HashMap_GetEnv(&rmap, isprequest);
        if (no_completion && isprequest == * ((int *) HashMap_GetRequest(&rmap, isprequest)) 
            && (argarray[atoi(env)].func == IRECV || 
                argarray[atoi(env)].func == RECV_INIT || send_block)) {
            no_completion = 1;
            continue;
        } 
        else {
            no_completion = 0;
        }

        if (!argarray[ilist[i]].is_complete) {
            *HashMap_GetRequest (&rmap, isprequest) = MPI_REQUEST_NULL;
        } else {
            request_count--;
            if (argarray[ilist[i]].func == IRECV || argarray[ilist[i]].func == ISEND) {
                RemoveFromMatchHashMap (&array_of_requests[i], TYPE_REQUEST, file, line);
                *flag = 1;
                *index = i;
                array_of_requests[i] = MPI_REQUEST_NULL;
                done = 1;
            } else {
                array_of_requests[i] = *HashMap_GetRequest (&rmap, isprequest);
            }
            /*
            HashMap_Delete (&rmap, isprequest);
            */
        }
    }

    /* if none of the request could be matched, we'll return false here */
    if (no_completion) {
        *flag = 0;
        *index = MPI_UNDEFINED;
        return MPI_SUCCESS;
    }


    if (done) {
        return MPI_SUCCESS;
    }
    while (1) {
        result = PMPI_Testany (count, array_of_requests, index, flag, status);
        if (*flag) {
            break;
        }
    }    
    return result;
}

DLLAPI int MPIAPI ISP_Testsome (int incount, MPI_Request *array_of_requests, 
        int *outcount, int *array_of_indices,MPI_Status *array_of_statuses,
        const char *file, int line) {
        
    int        result;
    char        sbuff[BUFFER_SIZE];
    char        sbuff1[BUFFER_SIZE];
    int        i;
    int        offset = 0;
    char       *env = NULL;
    int        *ilist = NULL;
    int        _count = 0;
    int        _procnull_count = 0;
    int        rcount = 0;
    int        isprequest;

    DebugPrint("Entering MPI_Testsome...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Testsome(incount, array_of_requests, outcount, array_of_indices, array_of_statuses);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    memset (sbuff1, '\0', BUFFER_SIZE);    
    ilist = (int *)malloc (sizeof (int)*incount);

    *outcount = 0;
    set_common_args (TESTANY);
    argarray[func_count].request = array_of_requests;
    argarray[func_count].status = array_of_statuses;
    argarray[func_count].index = array_of_indices;
    
    for (i = 0 ; i < incount ; i++) {
        env = HashMap_GetEnv (&rmap, *((int *)&array_of_requests[i]));
        if (env == NULL) {
            ilist[i] = -1;
            array_of_requests[i] = MPI_REQUEST_NULL;
            continue;
        }
        else if (!strcmp(env, "MPI_PROC_NULL")) {
            ilist[i] = -2;
            _procnull_count++;
            continue;
        }
        else if (!strcmp(env, "MPI_PROC_NULL_PERSIST")) {
            ilist[i] = -3;
            _procnull_count++;
            continue;
        }
        else if (argarray[atoi(env)].is_complete) {
            if (argarray[atoi(env)].func == IRECV) {
                setIrecvReq (atoi(env), array_of_statuses, i);
            } else if (argarray[atoi(env)].func == ISEND) {
                setIsendReq (atoi(env), &array_of_requests[i], array_of_statuses, i);
            }
        }
        _count++;
        sprintf (sbuff1+offset, "%s ", env);
        ilist[i] = atoi(env);
        offset = (int) strlen(sbuff1);
    }
    if (_count == 0 && _procnull_count == 0) {
        return 0;    
    }        
    /*
     * Testany Envelope: Testany
     */
    if (_count != 0) {
        //CGD added %d and the corresponding 0
        sprintf (sbuff, "%d %s %s %d %u %s %d %d %s", func_count, "Testany", "Testsome",
                    0,
                    (unsigned int) strlen(file), file, line, _count, sbuff1);
        if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
            printf ("Client unable to SendnRecv in MPI_Testsome\n");
            return -1;
        }
    }

    _count += _procnull_count;
    for (i = 0 ; i < incount ; i++) {
        if (ilist[i] == -1) {
            continue;
        }
        isprequest = *((int *)&array_of_requests[i]);

        if (ilist[i] == -2) { /* MPI_PROC_NULL */
            RemoveFromMatchHashMap (&array_of_requests[i], TYPE_REQUEST, file, line);
            request_count--;
            array_of_indices[i] = 1;
            array_of_requests[i] = *HashMap_GetRequest (&rmap, isprequest);
            HashMap_Delete (&rmap, isprequest);
            rcount++;
            if (array_of_statuses == MPI_STATUSES_IGNORE) {
                PMPI_Wait (&array_of_requests[i], MPI_STATUS_IGNORE);
            } else {
                PMPI_Wait (&array_of_requests[i], &array_of_statuses[i]);
            }
        } else if (ilist[i] == -3) { /* MPI_PROC_NULL persistant request */
            RemoveFromMatchHashMap (&array_of_requests[i], TYPE_REQUEST, file, line);
            array_of_indices[i] = 1;
            rcount++;
            if (array_of_statuses == MPI_STATUSES_IGNORE) {
                PMPI_Wait (HashMap_GetRequest (&rmap, isprequest), MPI_STATUS_IGNORE);
            } else {
                PMPI_Wait (HashMap_GetRequest (&rmap, isprequest), &array_of_statuses[i]);
            }
        } else if (!argarray[ilist[i]].is_complete) {
            /* TODO: See the comment in MPI_Waitsome, this is completely wrong. */
            *HashMap_GetRequest (&rmap, isprequest) = MPI_REQUEST_NULL;
        } else {
            request_count--;
            if (argarray[ilist[i]].func == IRECV || argarray[ilist[i]].func == ISEND) {
                RemoveFromMatchHashMap (&array_of_requests[i], TYPE_REQUEST, file, line);
                rcount++;
                array_of_indices[i] = 1;
                array_of_requests[i] = MPI_REQUEST_NULL;
            } else {
                array_of_requests[i] = *HashMap_GetRequest (&rmap, isprequest);
            }
            /*
            HashMap_Delete (&rmap, isprequest);
            */
        }
    }

    if (rcount == _count) {
        *outcount = _count;
        return 0;
    }
    while (1) {
        result = PMPI_Testsome (incount, array_of_requests, outcount,
            array_of_indices, array_of_statuses);
        if (*outcount) {
            break;
        }
    }
    *outcount += rcount;    
    return result;
}

DLLAPI int MPIAPI ISP_Testall (int count, MPI_Request *array_of_requests, int *flag,
        MPI_Status *array_of_statuses, const char *file, int line) {
        
    int        result;
    char       sbuff[BUFFER_SIZE];
    char       sbuff1[BUFFER_SIZE];
    int        i;
    int        offset = 0;
    char       *env = NULL;
    int        _count = 0;
    int        no_completion;
    int        isprequest;
    int        request_type;
    MPI_Request *array_of_requests_copy;

    DebugPrint("Entering MPI_Testall...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Testall(count, array_of_requests, flag, array_of_statuses);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    memset (sbuff1, '\0', BUFFER_SIZE);    
    set_common_args (TESTALL);
    argarray[func_count].request = array_of_requests;
    argarray[func_count].status = array_of_statuses;
    argarray[func_count].flag = flag;

    for (i = 0 ; i < count ; i++) {
        env = HashMap_GetEnv (&rmap, *((int *)&array_of_requests[i]));
        if (env && strcmp(env, "MPI_PROC_NULL") && strcmp(env, "MPI_PROC_NULL_PERSIST")) {
            sprintf (sbuff1+offset, "%s ", env);
            offset = (int) strlen(sbuff1);
            _count++;
        }
    }

    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d %d %s", func_count, "Testall",
            "Testall", 0,
            (unsigned int) strlen(file), file, line, _count, sbuff1);
    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Testall\n");
        return -1;
    }
    
    no_completion = 1;
    
    _count = 0;
    array_of_requests_copy = malloc(count * sizeof(*array_of_requests));
    memcpy(array_of_requests_copy, array_of_requests, count * sizeof(*array_of_requests));
    for (i = 0 ; i < count ; i++) {
        isprequest = *((int *)&array_of_requests[i]);
        env = HashMap_GetEnv (&rmap, isprequest);
        if (!env) {
            RemoveFromMatchHashMap (&array_of_requests[i], TYPE_REQUEST, file, line);
            *flag = 1;
            free(array_of_requests_copy);
            return 0;
        }
        
        /* If MPI_PROC_NULL, put the correct request in the array. */
        if (!strcmp(env, "MPI_PROC_NULL")) {
            _count++;
            no_completion = 0;
            array_of_requests[i] = *HashMap_GetRequest (&rmap, isprequest);
            HashMap_Delete (&rmap, isprequest);
            continue;
        }

        /* MPI_PROC_NULL persistant request */
        if (!strcmp(env, "MPI_PROC_NULL_PERSIST")) {
            _count++;
            no_completion = 0;
            continue;
        }

        /* If this happens, this means the test call was completed but no 
         * match has been found - if this happens to all of the requests
         * we'll return false here */

        if (no_completion && isprequest == * ((int *) HashMap_GetRequest(&rmap, isprequest)) 
            && (argarray[atoi(env)].func == IRECV || 
                argarray[atoi(env)].func == RECV_INIT || send_block)) {
            no_completion = 1;
            continue;
        } 
        else {
            no_completion = 0;
        }

        request_type = 
            persistent_argarray[get_persistent_request_index(isprequest)].func;
        if (argarray[atoi(env)].func == IRECV ||
            argarray[atoi(env)].func == ISEND) {
            array_of_requests[i] = *HashMap_GetRequest(&rmap, isprequest);
        }
        
        if (argarray[atoi(env)].func == IRECV || 
            request_type == RECV_INIT) {
            _count++;
            setIrecvReq (atoi(env), array_of_statuses, i);
        }
        if (argarray[atoi(env)].func == ISEND || 
            request_type == SEND_INIT) {
            _count++;
            setIsendReq (atoi(env), &array_of_requests[i], 
                         array_of_statuses, i);
        }
        if (request_type != SEND_INIT && request_type != RECV_INIT) {
            /*
            HashMap_Delete (&rmap, isprequest);
            */
        }
    }    

    /* if none of the request could be matched, we'll return flag to false */
    if (no_completion) {
        *flag = 0;
        free(array_of_requests_copy);
        return MPI_SUCCESS;
    }
    request_count -= _count;
    while (1) {
        result = PMPI_Testall (count, array_of_requests, flag, array_of_statuses);
        if (*flag) {
            break;
        }
    }
    for (i = 0; i < count; i++) {
        RemoveFromMatchHashMap (&array_of_requests_copy[i], TYPE_REQUEST, file, line);
    }
    free(array_of_requests_copy);
    return result;
}


DLLAPI int MPIAPI ISP_Abort (MPI_Comm comm, int errorcode, const char *file, int line) {
    char       sbuff[BUFFER_SIZE];

    DebugPrint("Entering MPI_Abort...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Abort(comm, errorcode);
    }

    memset (sbuff, '\0', BUFFER_SIZE);    
    /*
     * Abort envelope : "Abort"
     */
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d", func_count, "Abort", "Abort",
    						0,
                (unsigned int) strlen(file), file, line);
    set_common_args (ABORT);

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Abort\n");
        return -1;
    }
    #ifdef DEBUG_PROF_MEM
        memcheck();
    #endif
    return PMPI_Abort (comm, errorcode);
}

DLLAPI int MPIAPI ISP_Finalize (const char *file, int line) {
    char        sbuff[BUFFER_SIZE];
    int         retval;
    //int i;

    DebugPrint("Entering MPI_Finalize...\n");

    /* Make sure MPI_Init has been called first. */
    if (! is_initialized) {
        return PMPI_Finalize();
    }

    /* This sends the leak information to the scheduler with SendnRecv(...) */
#if 1
    retval = HashMapMatch_SendLeaks (&mmap, &SendnRecv);
//    printf(" leaks=%d\n", retval);
    if (retval < 0) {
        return retval;
    } else {
        func_count -= retval;
    }
#endif
    memset (sbuff, '\0', BUFFER_SIZE);    
    /*
     * Finalize envelope : "Finalize resource_count"
     */
    //CGD added %d and the corresponding 0
    sprintf (sbuff, "%d %s %s %d %u %s %d", func_count, "Finalize", "Finalize",
    						0, 
                (unsigned int) strlen(file), file, line);
    set_common_args (FINALIZE);

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Finalize\n");
        return -1;
    }
    //Disconnect (fd); // dhriti -- I want to send the assert statements after Finalize statements. Hence, I am not disconnecting
    //fd = INVALID_SOCKET; 

    HashMap_Destroy(&rmap);
    HashMapComm_Destroy(&cmap);

#ifdef DEBUG_PROF_MEM
    for(i=0; i<MAX_MPI_CALLS; i++) {
        if(argarray[i].buffer) {
            if(mem.find(argarray[i].buffer) != mem.end()) {
                free(argarray[i].buffer);
                //mem.erase(argarray[i].buffer);
            }
        }
    }
#endif

    free(argarray);
    free(persistent_argarray);
  
#ifdef DEBUG_PROF_MEM
    memcheck();
#endif
    return PMPI_Finalize ();
}

/* == fprs start == */
/*
//DLLAPI 
int MPIAPI MPI_Pcontrol(const int level, ...) {
  va_list va;
  va_start(va, level);
  switch(level) {
  case ISP_INTERLEAVING:
    int *interleaving_ = va_arg(va, int*);
    *interleaving_ = interleaving;
  }
  va_end(va);
  return MPI_SUCCESS;
}
*/

int MPIAPI MPI_Pcontrol(const int level, ...) {
// variables used for cases "start concern" and "end concern"
  char      sbuff[BUFFER_SIZE];
  int *interleaving_;
  va_list va;
  va_start(va, level);
  switch(level) {
  case ISP_INTERLEAVING:
    interleaving_ = va_arg(va, int*);
    *interleaving_ = interleaving;
    break;
  case ISP_START_SAMPLING:
  case ISP_END_SAMPLING:
//    sprintf (sbuff, "%d %s %s %u %s %d %d", func_count, "Pcontrol", "Pcontrol", 
//                (unsigned int) strlen(file), file, line, level);
    sprintf (sbuff, "%d %s %s %u %s %d %d", func_count, "Pcontrol", "Pcontrol", 
                (unsigned int) strlen("file"), "file", 101, level);
    set_common_args (PCONTROL);

    if (SendnRecv (sbuff, strlen (sbuff)) < 0) {
        printf ("Client unable to SendnRecv in MPI_Pcontrol\n");
        return -1;
    }
    break;
  }
  va_end(va);
  return MPI_SUCCESS;
}
/* == fprs end == */
