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
 * File:        MpiProc.cpp
 * Description: Defines the basic MPI process structure
 * Contact:     isp-dev@cs.utah.edu
 */

#include "MpiProc.hpp"


MpiProc::MpiProc (int id) : _read_next_env (true), _count (0), _isselect (false), _id (id) {

}

int MpiProc::Id () {
    return _id;
}

