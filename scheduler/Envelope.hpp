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
 * File:        Envelope.hpp
 * Description: Implements the various envelopes received from profiled code
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _ENVELOPE_HPP
#define _ENVELOPE_HPP

#include "name2id.hpp"
#include <iostream>
#include <vector>
#include <cassert>
#ifdef WIN32
#include <winsock2.h>
#endif
#define WILDCARD (-1)

class Envelope {
public:
    /*
     * The envelope contains all the possible parameters for a
     * function. Only those valid for a particular function must
     * be used. Use with care!
     */
    int                 id;
    int                 order_id;
    int                 issue_id;
    std::string         func;
    std::string         display_name;
    int                 func_id;
    int                 count;
    int                 index;
    int                 dest;
    bool                dest_wildcard;
    int                 src;
    bool                src_wildcard;
    int                 stag;
    std::string         comm;
    int					data_type; //CGD
    bool				typesMatch;//CGD
    std::vector<int>    comm_list;
    std::vector<int>    req_procs;
 
    int                 rtag;
    int                 nprocs;
    int                 comm_split_color;
    int                 comm_id;
    std::string         filename;
    int                 linenumber;
    unsigned int        ref;

    int                 data; // data sent by the Sending process -- dhriti
/* == fprs begin == */
    bool in_exall;
/* == fprs end == */

/*  modifications of the multireceive FM encoding   */   
    int cardinality;
    int corresponding_top_id;
    int corresponding_top_index;
    int corresponding_bottom_index;
    bool istop; 
    bool isbottom;
    bool isMultiRecv;

    void ref_inc();
    void ref_dec();
    bool isshared();
    inline void Issued () {
        static int issue_number = 0;
        issue_id = ++issue_number;
    }
    void PrintLog();

    inline bool isSendType () {
        return (func_id == SSEND || func_id == SEND ||
    			func_id == RSEND || func_id == ISEND);
    }

    inline bool isRecvType () {
        return (func_id == IRECV || func_id == RECV ||
                func_id == PROBE || func_id == IPROBE);
    }

    inline bool isRecvTypeOnly () {
      //return (func_id == IRECV || func_id == RECV);
        return (func_id == IRECV || func_id == RECV ||
                func_id == PROBE || func_id == IPROBE);
    }

    /*inline bool isBarrier () {
        return func_id == BARRIER;
    }

    inline bool isCollectiveTypeSansBarrier () {
        return (func_id == BCAST || func_id == CART_CREATE
                || func_id == COMM_CREATE || func_id == COMM_DUP
                || func_id == COMM_SPLIT || func_id == COMM_FREE
                || func_id == ALLREDUCE || func_id == REDUCE
                || func_id == GATHER || func_id == SCATTER
                || func_id == GATHERV || func_id == SCATTERV
                || func_id == ALLGATHER || func_id == ALLGATHERV
                || func_id == ALLTOALL || func_id == ALLTOALLV
                || func_id == SCAN || func_id == REDUCE_SCATTER);
    }*/

    inline bool isCollectiveType () {
        return (func_id == BARRIER || func_id == BCAST || func_id == CART_CREATE
                || func_id == COMM_CREATE || func_id == COMM_DUP
                || func_id == COMM_SPLIT || func_id == COMM_FREE
                || func_id == ALLREDUCE || func_id == REDUCE
                || func_id == GATHER || func_id == SCATTER
                || func_id == GATHERV || func_id == SCATTERV
                || func_id == ALLGATHER || func_id == ALLGATHERV
                || func_id == ALLTOALL || func_id == ALLTOALLV
                || func_id == SCAN || func_id == REDUCE_SCATTER);
    }    

    /*inline bool isBlockingTypeButNotCollectiveType () {
    return (func_id == RECV || func_id == SSEND
        || func_id == WAIT || func_id == PROBE || func_id == IPROBE
        || func_id == FINALIZE || func_id == TEST
        || func_id == WAITANY || func_id == TESTANY
        || func_id == WAITALL || func_id == TESTALL); 
    }*/

    inline bool isBlockingType () {
    return (func_id == RECV || func_id == SSEND
        || func_id == WAIT || func_id == PROBE || func_id == IPROBE
        || func_id == FINALIZE || func_id == TEST
        || func_id == WAITANY || func_id == TESTANY
        || func_id == WAITALL || func_id == TESTALL || isCollectiveType()); 
        // We have to remove isCollectiveType from here: Dhriti
        // Because collective calls are not blocking types now. 
    }

    inline bool isWaitorTestType () {
        return (func_id == WAIT || func_id == TEST
                || func_id == WAITANY || func_id == TESTANY
                || func_id == WAITALL || func_id == TESTALL
                );
    }


    bool        operator==(Envelope &);
    bool        operator!=(Envelope &);
    friend std::ostream &operator<< (std::ostream &os, Envelope &e);
    Envelope();
    Envelope(const Envelope &e);
    Envelope* clone(); // dhriti
};

/* == fprs begin == */
Envelope *CreateEnvelope (char *buffer, int id, int order_id, bool to_expl);
// Envelope *CreateEnvelope (char *buffer, int id, int order_id);
/* == fprs end == */
std::ostream &operator<< (std::ostream *os, Envelope &e);

#endif

