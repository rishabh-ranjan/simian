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
 * File:        MpiProc.hpp
 * Description: Defines the basic MPI process structure
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _MPI_PROC_HPP
#define _MPI_PROC_HPP

#include "Envelope.hpp"

/*
 * The following are the various states on which a process can exist
 * while executing an MPI program.
 */
typedef enum {

    RUNNING,
    BLOCKED_ON_SSEND,
    BLOCKED_ON_RECV,
    BLOCKED_ON_PROBE,
    BLOCKED_ON_SENDREVC,
    BLOCKED_ON_BARRIER,
    BLOCKED_ON_WIN_LOCK,
    BLOCKED_ON_WIN_UNLOCK,
    BLOCKED_ON_WIN_FREE,
    DONE
} ProcStatus;

/*
 * This class defines an MPI Process. Every process stores its current state.
 * Every process also stores the envelope corresponding to the function it is
 * currently executing/blocked.
 */

class MpiProc {

public:
    /*
     * Constructor: Takes the process id as its parameter
     */
    MpiProc (int id);

    /*
     * Id : Get the Id of the process.
     */
    int Id ();

    bool        _read_next_env;
    Envelope    *_env;
    int        _count;
    bool        _isselect;
private:
    int        _id;
};

#endif
