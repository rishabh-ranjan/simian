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
 * File:        TransitionList.cpp
 * Description: Implements lists of transitions from an interleaving
 * Contact:     isp-dev@cs.utah.edu
 */

#include "TransitionList.hpp"
#include "Scheduler.hpp"

#include <iostream>
#include <sstream>

TransitionList::TransitionList () {
    last_matched = -1;
	_leaks_count = 0;
    _leaks_string.clear ();
}

TransitionList::TransitionList (int id) : _id (id) {
    last_matched = -1;
	_leaks_count = 0;
    _leaks_string.clear ();
}

TransitionList::TransitionList (TransitionList &tl) {
    *this = tl;
}

// dhriti
TransitionList* TransitionList::clone() {
    TransitionList* t = new TransitionList();
    std::transform(_tlist.begin(), _tlist.end(), back_inserter(t->_tlist), cloneFunctor()); // --> Deep copy
    t->_ulist = this->_ulist;
    return t;
}

/* avo 06/03/08 need to free memory */
TransitionList::~TransitionList() {
    std::vector <Transition>::iterator ti = _tlist.begin();
    std::vector <Transition>::iterator tie = _tlist.end();
    for(; ti != tie; ti++) {
        ti->unref();
    }
    std::list<int>().swap(_ulist);
}

TransitionList& TransitionList::operator= (TransitionList &t) {
    std::vector <Transition>::iterator  iter;
    std::vector <Transition>::iterator  iter_end = t._tlist.end();

    last_matched = t.last_matched;
    _leaks_string << t._leaks_string.str();
    _leaks_count = t._leaks_count;
    _tlist.clear ();
    _id = t.GetId ();
    //std::cout << _id << " TransitionList = begin]\n";
    for (iter = t._tlist.begin(); iter != iter_end;    iter++) {
        _tlist.push_back(Transition(*iter, true));
    }
    //std::cout << _id << " TransitionList = end]\n";
    std::list<int>::iterator ui = t._ulist.begin();
    std::list<int>::iterator uie = t._ulist.end();
    for(; ui != uie; ui++) {
      _ulist.push_back(*ui);
    }
    return (*this);
}

int TransitionList::GetId () {
    return _id;
}

// copied from mopper
bool TransitionList::AddTransition (Transition *t) {
    int index = t->GetEnvelope ()->index;
    int msize = (int)_tlist.size();

    if (index <=  msize-1) {
        if (*(t->GetEnvelope()) == *_tlist[index].GetEnvelope()) {
            _tlist[index].GetEnvelope()->issue_id = -1;
            t->unref();
            t->t = _tlist[index].t;
            //t->ref();
            return true;
        }
/*
        DR(
            Envelope *e = t->GetEnvelope();
            std::cout << "\nrank=" << e->id << "\n";
            std::cout << "[\n";
            for(int i=0; i<=index; i++) {
                std::cout << *_tlist[i]->GetEnvelope() << "\n";
            }
            std::cout << "]\n";
            std::cout << "( " << *e << " )\n";
        )
*/
        return false;
    }

    _tlist.push_back (*t);
    t = &_tlist.back();   // important! not a no-op
    int size = (int)_tlist.size ();
    CB c(_id, size-1);

    Envelope *env_t = t->GetEnvelope();
    //    std::cout << (*env_t) << std::endl; // added by svs
    Envelope *env_f;
    bool blocking_flag = false;

    if (env_t->func_id == ISEND || env_t->func_id == IRECV) {
        _ulist.push_back (index);
    } else if (env_t->func_id == WAIT || env_t->func_id == WAITALL ||
               env_t->func_id == TEST || env_t->func_id == TESTALL) {
        std::vector<int>::iterator it = env_t->req_procs.begin ();
        std::vector<int>::iterator it_end = env_t->req_procs.end ();
        for (; it != it_end; it++) {
	  if (_tlist[*it].AddIntraCB(c)) {
	    /* [svs] -- under !blocking _flag 
	       1) do not add the MB edge when (*it) is a Send
	    */
	    if(!Scheduler::_send_block && (_tlist[*it].GetEnvelope()->func_id == ISEND))
	      continue;
	    t->mod_ancestors().push_back(*it);
	  }
	  _ulist.remove (*it);
        }
    }

    std::vector<Transition>::reverse_iterator iter = _tlist.rbegin() + 1;
    std::vector<Transition>::reverse_iterator iter_end = _tlist.rend();

    int i = size - 2;
    bool flag = false;
    for (; iter != iter_end; iter++) {
        flag = intraCB(*iter, *t);
        if (flag) {
            if (!blocking_flag) {
                if (iter->AddIntraCB(c)) {
                    t->mod_ancestors().push_back(i);
                }
                
                /* avo 06/11/08 - trying not to add redundant edges */
                env_f = iter->GetEnvelope();
                
                //a blocking call occured earlier
                //to avoid unnecessary CB edges
                if (env_f->isBlockingType())
              
                    blocking_flag = true;
            } else if (blocking_flag) {
                if (env_t->func_id != SEND && (_ulist.size () == 0 || index < _ulist.front ())) {
                    return true;
                }
                //if a blocking call occured in the past
                //the only calls that can slip through it are
                //Isend and Irecv
                env_f = iter->GetEnvelope();
                if (env_f->func_id == IRECV) {
                    if (iter->AddIntraCB(c)) {
                        t->mod_ancestors().push_back(i);
                    }

                    //terminate if this satisfies the Irecv intraCB rule
                    if (env_t->isRecvType() &&
                        env_f->src == env_t->src &&
                        env_f->comm == env_t->comm &&
                        env_f->rtag == env_t->rtag)
                        return true;
                } else if (env_f->func_id == ISEND) {
                    if (iter->AddIntraCB(c)) {
                        t->mod_ancestors().push_back(i);
                    }
                    
                    //terminate if this satisfies the Isend intraCB rule
                    if (env_t->isSendType() &&
                        env_f->dest == env_t->dest &&
                        env_f->comm == env_t->comm &&
                        env_f->stag == env_t->stag )
		      return true;
                }
		else if (env_f->func_id == SEND){
		  if (iter->AddIntraCB(c)) {
		    t->mod_ancestors().push_back(i);
		  }
		  if (env_t->isSendType() &&
		      env_f->dest == env_t->dest &&
		      env_f->comm == env_t->comm &&
		      env_f->stag == env_t->stag )
		    return true;
		}
            }
        }
        i--;
    }
    return true;
}

// original from Hermes -- buggy
#if 0
bool TransitionList::AddTransition (Transition *t) {
    int index = t->GetEnvelope ()->index;
    int msize = (int)_tlist.size();

    if (index <= msize-1) 
    {
        //if (*(t->GetEnvelope()) == *_tlist[index].GetEnvelope()) 
        {
            _tlist[index].GetEnvelope()->issue_id = -1;
            t->unref();
            t->t = _tlist[index].t;
            //t->ref();
            return true;
        }
        /*
        DR(
        Envelope *e = t->GetEnvelope();
        std::cout << "\nrank=" << e->id << "\n";
        std::cout << "[\n";
        for(int i=0; i<=index; i++) {
        std::cout << *_tlist[i]->GetEnvelope() << "\n";
        }
        std::cout << "]\n";
        std::cout << "( " << *e << " )\n";
        )
        */
        return false;
    }

    _tlist.push_back (*t);
    t = &_tlist.back();   // important! not a no-op
    int size = (int)_tlist.size ();
    CB c(_id, size-1);

    Envelope *env_t = t->GetEnvelope();
    // std::cout << (*env_t) << std::endl; // added by svs
    Envelope *env_f;
    bool blocking_flag = false;

    if (env_t->func_id == ISEND || env_t->func_id == IRECV) 
    {
        _ulist.push_back (index);
    } 
    else if (env_t->func_id == WAIT || env_t->func_id == WAITALL || env_t->func_id == TEST || env_t->func_id == TESTALL) 
    {
        std::vector<int>::iterator it = env_t->req_procs.begin ();
        std::vector<int>::iterator it_end = env_t->req_procs.end ();
        for (; it != it_end; it++) 
        {
            if (_tlist[*it].AddIntraCB(c)) 
            {
                /* [svs] -- under !blocking _flag 
                1) do not add the MB edge when (*it) is a Send
                */
                //if(!Scheduler::_send_block && (_tlist[*it].GetEnvelope()->func_id == ISEND))
                //continue;
                t->mod_ancestors().push_back(*it);
            }
            _ulist.remove (*it);
        }
    }

    std::vector<Transition>::reverse_iterator iter = _tlist.rbegin() + 1;
    std::vector<Transition>::reverse_iterator iter_end = _tlist.rend();

    int i = size - 2;
    bool flag = false;
    for (; iter != iter_end; iter++) 
    {
        flag = intraCB(*iter, *t);
        if (flag) 
        {
            if (!blocking_flag) 
            {
                if (iter->AddIntraCB(c)) 
                {
                    t->mod_ancestors().push_back(i);
                }

                /* avo 06/11/08 - trying not to add redundant edges */
                env_f = iter->GetEnvelope();

                //a blocking call occured earlier
                //to avoid unnecessary CB edges
                if (env_f->isBlockingType()) // dhriti - removed collectives from isBlockingType
                    blocking_flag = true;
            } 
            else if (blocking_flag) 
            {
                if (env_t->func_id != SEND && (_ulist.size () == 0 || index < _ulist.front ())) 
                {
                    return true;
                }
                //if a blocking call occured in the past
                //the only calls that can slip through it are
                //Isend and Irecv
                env_f = iter->GetEnvelope();
                if (env_f->func_id == IRECV) 
                {
                    if (iter->AddIntraCB(c)) 
                    {
                        t->mod_ancestors().push_back(i);
                    }

                    //terminate if this satisfies the Irecv intraCB rule
                    if (env_t->isRecvType() &&
                    env_f->src == env_t->src &&
                    env_f->comm == env_t->comm &&
                    env_f->rtag == env_t->rtag)
                        return true;
                } 
                else if (env_f->func_id == ISEND) 
                {
                    if (iter->AddIntraCB(c)) 
                    {
                        t->mod_ancestors().push_back(i);
                    }

                    //terminate if this satisfies the Isend intraCB rule
                    if (env_t->isSendType() &&
                    env_f->dest == env_t->dest &&
                    env_f->comm == env_t->comm &&
                    env_f->stag == env_t->stag )
                    return true;
                }
                else if (env_f->func_id == SEND)
                {
                    if (iter->AddIntraCB(c)) 
                    {
                        t->mod_ancestors().push_back(i);
                    }
                    if (env_t->isSendType() &&
                    env_f->dest == env_t->dest &&
                    env_f->comm == env_t->comm &&
                    env_f->stag == env_t->stag )
                        return true;
                }
            }
        }
        i--;
    }
    return true;
}
#endif

std::ostream &operator<< (std::ostream &os, TransitionList &tl) {
    std::vector<Transition>::iterator iter;
    std::vector<Transition>::iterator iter_end = tl._tlist.end();
    int i = 0;
    bool finalize_called = false;
    for (iter = tl._tlist.begin (); iter != iter_end; iter++) {
        os << i << " " << *iter << std::endl;
        if (iter->GetEnvelope()->func_id == FINALIZE) {
            finalize_called = true;
        }
        i++;
    }
    if (finalize_called) {
        if (tl._leaks_count == 0) {
            os << "No resource leaks detected, " << i << " MPI calls trapped." << "\n";
        } else {
            os << tl._leaks_count << " resource leaks detected, " << i << " MPI calls trapped." << "\n";
        }
    }
    return os;
}

void TransitionList::PrintLog () {
    std::vector<Transition>::iterator iter;
    std::vector<Transition>::iterator iter_end = _tlist.end();
    int i = 0;
    for (iter = _tlist.begin (); iter != iter_end; iter++) {
        iter->PrintLog();
        Scheduler::_logfile << std::endl;
        i++;
    }
    Scheduler::_logfile << _leaks_string.str ();
}

unsigned int TransitionList::size() { return _tlist.size(); }

void TransitionList::eraseFrom(unsigned int s) {
/*
    std::vector <Transition*>::iterator ti = _tlist.begin()+s;
    std::vector <Transition*>::iterator tie = _tlist.end();
    //while(ti++ != tie) 
    for(; ti != tie; ti++) delete *ti;
    _tlist.erase(_tlist.begin()+s, _tlist.end());
*/
}
