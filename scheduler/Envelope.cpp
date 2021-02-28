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
 * File:        Envelope.cpp
 * Description: Implements the various envelopes received from profiled code
 * Contact:     isp-dev@cs.utah.edu
 */

#include "Scheduler.hpp"
#include "Envelope.hpp"
#include <sstream>
#include <iostream>
//static int env_count = 0;

/* == fprs begin == */
Envelope *CreateEnvelope (char *buffer, int id, int order_id, bool to_expl) {
// Envelope *CreateEnvelope (char *buffer, int id, int order_id) {
/* == fprs end == */
/* == fprs begin == */
    int pcontrol_mode;
/* == fprs end == */;

    std::string         str;
    std::istringstream  iss;
    Envelope            *env = new Envelope;
    env->issue_id = -1;

/* == fprs begin == */
env->in_exall = to_expl;
/* == fprs end == */

    if ((std::string (buffer)).empty ()) {
        if (!Scheduler::_quiet) {
            std::cout << "EMPTY BUFFER " << "id is " << id << " !!!!!!!\n";
        }
        delete env;
        return NULL;
    }
    iss.str (std::string (buffer));

    //std::cout << "Scheduler received from process " << id << ": " << buffer << std::endl;

    iss >> env->index;
    
    //read in the function name and parse it to generate the function id
    iss >> str;
    env->func = str;
    env->func_id = name2id::getId (str);
    env->id = id;
    env->order_id = order_id;
    env->comm = -2; // Why -2?
    
    //read in the display name 
    iss >> env->display_name;

		//CGD
/* == fprs start == */
    // if(env->func_id != LEAK){
    if(env->func_id != LEAK && env->func_id != PCONTROL){
/* == fprs end == */
       iss >> env->data_type;
       env->typesMatch = true;
       // --dhriti
       if(env->func_id == SEND || env->func_id == ISEND || env->func_id == RSEND || env->func_id == SSEND)
        {
            int count;
            iss >> count;
            int data;
            iss >> data;
            //if(env->data_type == 6408960 /*&& count == 1*/) // 6408960 stands for MPI_INT; We need the data for making it a data flow aware analysis
            {
                //std::cout << "Scheduler received from process " << id << ": " << data << std::endl;
                env->data = data;
            }
        }
    }
    
    /*
     * Parse format: ... <filename length> <file name> <line number> ...
     */
    int filename_length;
    iss >> filename_length;
    iss.ignore (); // ignore space
    char *filename = new char[filename_length + 1];
    iss.get (filename, filename_length + 1);
    env->filename.insert(0, filename);
    delete [] filename;
    iss >> env->linenumber;

    /*
     * Update the structure based on the function.
     */
    switch (env->func_id) {

    case ASSERT: {
            // Get the assertion text and function name.
            int assertion_length;
            iss >> assertion_length;
            iss.ignore (); // ignore space
            char *assertion = new char[assertion_length + 1];
            iss.get (assertion, assertion_length + 1);
            env->display_name.clear ();
            env->display_name.insert (0, assertion);
            delete assertion;
            iss >> env->func;
            break;
        }

    case ISEND: {
            std::string dest;
            iss >> dest >> env->stag >> env->count >> env->comm;
            if (dest == "MPI_ANY_SOURCE") {
                env->dest = WILDCARD;
                env->dest_wildcard = true;
            } else {
                env->dest = atoi (dest.c_str ());
                env->dest_wildcard = false;
            }
            break;
        }
    case SSEND:
	case RSEND:
    case SEND: {
            std::string dest;
            iss >> dest >> env->stag >> env->comm;
            if (dest == "MPI_ANY_SOURCE") {
                env->dest = WILDCARD;
                env->dest_wildcard = true;
            } else {
                env->dest = atoi (dest.c_str ());
                env->dest_wildcard = false;
            }

            break;
        }

    case IRECV: {
            std::string src;
            iss >> src >> env->rtag >> env->count >>  env->comm;
            if (src == "MPI_ANY_SOURCE") {
                env->src = WILDCARD;
                env->src_wildcard = true;
            } else {
                env->src = atoi (src.c_str ());
                env->src_wildcard = false;
            }
            if (env->rtag < 0) 
                env->rtag = WILDCARD;
            break;
        }
    case IPROBE:
    case PROBE:
    case RECV: {
            std::string src;
            iss >> src >> env->rtag >> env->comm;
            if (src == "MPI_ANY_SOURCE") {
                env->src = WILDCARD;
                env->src_wildcard = true;
            } else {
                env->src = atoi (src.c_str ());
                env->src_wildcard = false;
            }
            if (env->rtag < 0) 
                env->rtag = WILDCARD;
            
            break;
        }

    case SENDRECV:
        iss >> env->dest >> env->stag >> env->src >>
        env->rtag >> env->comm;
        if (env->rtag < 0) env->rtag = WILDCARD;
        break;

    case BCAST:
    case SCATTER:
    case GATHER:
    case SCATTERV:
    case GATHERV:
    case REDUCE: {
            iss >> env->nprocs;
            iss >> env->count;
            if (iss.fail ()) {
                env->count = -1;
            }
            iss >> env->comm;
            break;
        }

    case ALLGATHER:
    case ALLGATHERV:
    case ALLTOALL:
    case ALLTOALLV:
    case SCAN:
/* == fprs begin == */
	case EXSCAN:
/* == fprs end == */
    case BARRIER:
    case ALLREDUCE:
    case REDUCE_SCATTER:
    case CART_CREATE:
    case COMM_CREATE:
    case COMM_DUP:
    case COMM_FREE:
        iss >> env->nprocs >> env->count >> env->comm;
        break;

    case COMM_SPLIT:
        iss >> env->nprocs >> env->count >> env->comm_split_color >> env->comm;
        break;

    case WAIT:
    case TEST:
        iss >> env->count;
        env->req_procs.push_back (env->count);
        break;

    case WAITALL:
    case TESTALL:
    case WAITANY:
    case TESTANY:
        iss >> env->count;
        for (int i = 0; i < env->count ; i++) {
            int req;
            iss >> req;
            env->req_procs.push_back (req) ;
        }
        break;

/* == fprs begin == */
    case PCONTROL:
        iss >> pcontrol_mode;
        env->stag = pcontrol_mode;  // NOTE: we store the mode information in "stag"!!!!
        break;
/* == fprs end == */

    case LEAK:
    case ABORT:
    case FINALIZE:
        break;
    }
    if (Scheduler::_report_progress > 0 && 
        order_id % Scheduler::_report_progress == 0) {
//        std::cout << "Processed for rank " << id <<" MPI call No. " << order_id << "*" <<
//            *env << "*" << std::endl;
        //std::cout << "Processed: " << *env << std::endl;
    } 
    return env;
}

Envelope::Envelope() {
    //env_count++;
    //     printf("env_count : %d\n", env_count);
    dest = 0;
    dest_wildcard = false;
    src = 0;
    src_wildcard = false;
    ref = 1;
    cardinality = -1; 
    isbottom = false;
    istop = false;
    isMultiRecv = false;
    corresponding_top_id = -1;
    corresponding_top_index = -1;
    corresponding_bottom_index = -1;
}

Envelope::Envelope(const Envelope &e) {
    assert(0);
    *this = e;
}

// dhriti
Envelope* Envelope::clone()
{
    Envelope* e = new Envelope();

    e->id = id;
    e->order_id = order_id;
    e->issue_id = issue_id;
    e->func = func;
    e->display_name = display_name;
    e->func_id = func_id;
    e->count = count;
    e->index = index;
    e->dest = dest;
    e->dest_wildcard = dest_wildcard;
    e->src = src;
    e->src_wildcard = src_wildcard;
    e->stag = stag;
    e->comm = comm;
    e->data_type = data_type;
    e->typesMatch = typesMatch;
    e->comm_list = comm_list;
    e->req_procs = req_procs;
    e->rtag = rtag;
    e->nprocs = nprocs;
    e->comm_split_color = comm_split_color;
    e->comm_id = comm_id;
    e->filename = filename;
    e->linenumber = linenumber;
    e->ref = ref;
    e->data = data;
    e->in_exall = in_exall; 
    e->cardinality = cardinality;
    e->corresponding_top_id = corresponding_top_id;
    e->corresponding_top_index = corresponding_top_index;
    e->corresponding_bottom_index = corresponding_bottom_index;
    e->istop = istop; 
    e->isbottom = isbottom;
    e->isMultiRecv = isMultiRecv;

    return e;
}

bool Envelope::operator== (Envelope &e) {
    if (e.func != func) {
        return false;
    }
    switch (e.func_id) {
    case ASSERT:
        return (display_name == e.display_name);

    case BARRIER:
    case BCAST:
    case SCATTER:
    case GATHER:
    case SCATTERV:
    case GATHERV:
    case ALLGATHER:
    case ALLGATHERV:
    case ALLTOALL:
    case ALLTOALLV:
    case SCAN:
/* == fprs begin == */
	case EXSCAN:
/* == fprs end == */
    case ALLREDUCE:
    case REDUCE:
    case REDUCE_SCATTER:
    case CART_CREATE:
    case COMM_CREATE:
    case COMM_DUP:
    case COMM_SPLIT:
    case COMM_FREE:
    case WAIT:
    case TEST:
    case WAITANY:
    case TESTANY:
    case WAITALL:
    case TESTALL:
        return (count == e.count);

    case SEND:
    case SSEND:
    case ISEND:
	case RSEND:
        return (dest==e.dest && e.stag == stag);

    case IRECV:
    case RECV:
    case PROBE:
    case IPROBE:
        return (e.src == src && e.rtag == rtag);

    case ABORT:
    case FINALIZE:
        return true;

    case LEAK:
        return (e.filename == filename && e.linenumber == linenumber &&
                e.count == count);
/* == fprs start == */
    case PCONTROL:
        return (e.stag == stag);
/* == fprs end == */
    }
    return false;
}

bool Envelope::operator!= (Envelope &e) {
    return !((*this) == e);
}

std::ostream &operator<< (std::ostream &os, Envelope &e) {

    os << "o=" << e.order_id <<" i="<< e.index <<" rank="<< e.id << " ";
    if (e.func_id != ASSERT) {
        os << e.display_name;
    } else {
        os << "assert";
    }
    if (e.linenumber >= 0) {
        os << " " << e.filename << ":" << e.linenumber;
    }

    /* Note that we do not need a case for START
     * Depends on the request, START is treated as Isend/Irecv
     * Yet the display name will still be displayed as Start
     * Send_init and Recv_init are not processed by the scheduler
     */
    switch (e.func_id) {

    case BARRIER:
    case BCAST:
    case ALLREDUCE:
    case REDUCE:
    case REDUCE_SCATTER:
    case SCATTER:
    case GATHER:
    case SCATTERV:
    case GATHERV:
    case ALLGATHER:
    case ALLGATHERV:
    case ALLTOALL:
    case ALLTOALLV:
    case SCAN:
/* == fprs begin == */
	case EXSCAN:
/* == fprs end == */
    case CART_CREATE:
    case COMM_CREATE:
    case COMM_DUP:
    case COMM_SPLIT:
    case COMM_FREE:
        os << " count=" << e.count;
        break;

    case SEND:
    case SSEND:
	case RSEND:
        os << " dest=" << e.dest << " stag=" << e.stag;
        break;
    case ISEND:
        os << " dest=" << e.dest << " stag=" << e.stag << " count=" <<
        e.count;
        break;

    case IRECV:
        os << " src=" << e.src << " rtag=" << e.rtag << " count=" <<
        e.count;
        break;
    case RECV:
    case PROBE:
    case IPROBE:
        os << " src=" << e.src << " rtag=" << e.rtag;
        break;

    case WAIT:
    case TEST:
      // os <<  " src=" << e.src << " dest=" << e.dest 
      //	 << " stag=" << e.stag << " rtag=" << e.rtag 
      os  << " count=" << e.count;
      //<< "req_proc:";
      // for(std::vector<int>::iterator vit = e.req_procs.begin();
      // 	  vit != e.req_procs.end(); vit++){
      // 	os << *vit;
      // }
      break;
    case ASSERT:
    case ABORT:
    case FINALIZE:
        break;
    }
    return os;
}

void Envelope::PrintLog () {

    Scheduler::_logfile << index << " " << order_id << " " << issue_id << " ";
    if (func_id != ASSERT) {
        Scheduler::_logfile << display_name << " ";
    } else {
        Scheduler::_logfile << "assert ";
    }

    switch (func_id) {

    case BARRIER:
    case BCAST:
    case ALLREDUCE:
    case REDUCE:
    case REDUCE_SCATTER:
    case SCATTER:
    case GATHER:
    case SCATTERV:
    case GATHERV:
    case ALLGATHER:
    case ALLGATHERV:
    case ALLTOALL:
    case ALLTOALLV:
    case SCAN:
/* == fprs begin == */
	case EXSCAN:
/* == fprs end == */
    case CART_CREATE:
    case COMM_CREATE:
    case COMM_DUP:
    case COMM_SPLIT:
    case COMM_FREE:
        Scheduler::_logfile << comm << " ";
        break;

    case SEND:
    case SSEND:
    case ISEND:
	case RSEND:
        Scheduler::_logfile << dest << " " << stag << " " << comm << " ";
        break;

    case IRECV:
    case RECV:
    case PROBE:
    case IPROBE:
        Scheduler::_logfile << src << " " << rtag << " " << comm << " ";
        break;

    case WAIT:
    case TEST:
        break;
    case ASSERT:
    case ABORT:
    case FINALIZE:
        break;
    }
}

void Envelope::ref_inc() {
    ref++;
//[grz]    std::cout << "[Envelope.ref_inc] " << this << " " << ref << "\n";
}

void Envelope::ref_dec() {
    DS( assert(ref>1); )
    ref--;
//[grz]    std::cout << "[Envelope.ref_dec] " << this << " " << ref << "\n";
}

bool Envelope::isshared() {
    DS( assert(ref>0); )
    return ref != 1;
}
