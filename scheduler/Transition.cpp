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
 * File:        Transition.cpp
 * Description: Implements handling of a single transition in an interleaving
 * Contact:     isp-dev@cs.utah.edu
 */

#include "Transition.hpp"
#include "Scheduler.hpp"

#include <signal.h>

Transition::Transition() {
    DS( t = NULL; );
}

Transition::Transition(const Transition &t_, bool ref_t) {
    *this = t_;
    if(ref_t) t->ref_inc();
}

Transition::Transition(Envelope *e, bool ref_e) {
    t = new Transition_internal(e);
    if(ref_e) t->GetEnvelope()->ref_inc();
}

Transition &Transition::operator=(const Transition &t_) {
    //std::cout << "[Transition operator=]\n";
    t = t_.t;
//    t->ref_inc();
//    t = new Transition_internal(*t_.t);
    return *this;
}

// dhriti
Transition Transition::clone() {
    Transition t;
    t.t = (this->t)->clone();
    return t;
}

std::ostream &operator<<(std::ostream &os, const Transition &t) {
    return os << *t.t;
}

void Transition::ref() {
    t->ref_inc();
    //t->GetEnvelope()->ref_inc();
}

void Transition::unref() {
//    std::cout << "[Trandition.unref] " << t->ref << "\t\t" << t << "\n";
    if(!t->isshared()) {
//        t->ref_dec();
        //if(t == (Transition_internal*)0x808eee8) assert(0);
        delete t;
        DS( t = NULL; )
    } else t->ref_dec();
}

void Transition::copy() {
    //std::cout << "[Trandition.copy enter] " << t << " " << t->ref << "\n";
    if(t->isshared()) {
        t->ref_dec();
        t = new Transition_internal(*t);
    }
    //std::cout << "[Trandition.copy exit] " << t << " " << t->ref << "\n";
/*
        Transition_internal *tt = new Transition_internal(*t);
        delete t;
        t =tt;
        t->ref_inc();
        DS( assert(!t->isshared()); )
*/
}

void Transition::Execute(ServerSocket &s) {
    t->Execute(s);
}

Envelope *Transition::GetEnvelope() const {
    return t->GetEnvelope();
}

bool Transition::AddIntraCB(CB &c) {
    if(t->AddIntraCB_check(c)) {
        copy();
        t->AddIntraCB_mutate(c);
        return true;
    }
    return false;
}

void Transition::PrintLog() const {
    t->PrintLog();
}

void Transition::AddInterCB(CB &c) {
    if(t->AddInterCB_check(c)) {
        copy();
        t->AddInterCB_mutate(c);
    }
}

//bool Transition::isMatched(CB &c) const {
//    return t->isMatched(c);
//}

//void Transition::moveCurrMatching() {
//    copy();
//    t->moveCurrMatching();
//}

void Transition::set_curr_matching(const CB &c) {
    copy();
    t->curr_matching = c;
//    t->curr_matching_set = true;
}

CB &Transition::get_curr_matching() const {
    return t->curr_matching;
}

//std::vector<CB> &Transition::get_matches() const {
//    return t->matches;
//}

std::vector<int> & Transition::get_ancestors() const {
 // svs - removed & -- return by reference and const
    return t->ancestors;
}

std::vector<int> &Transition::mod_ancestors() {
    copy();
    return t->ancestors;
}

//bool &Transition::get_is_matched() const {
//    return t->is_matched;
//}
//
//bool &Transition::get_is_issued() const {
//    return t->is_issued;
//}

#ifdef FIB
std::vector <CB> &Transition::get_inter_cb() {
    return t->_inter_cb;
}

std::vector <CB> &Transition::mod_inter_cb() {
    copy();
    return t->_inter_cb;
}

std::vector <CB> &Transition::get_intra_cb() {
    return t->_intra_cb;
}

std::vector <CB> &Transition::get_cond_intra_cb() {
    return t->_cond_intra_cb;
}
    
void Transition::AddCondIntraCB(CB &c) {
    if(t->AddCondIntraCB_check(c)) {
        copy();
        t->AddCondIntraCB_mutate(c);
    } 
}
#endif

//static int trans_count = 0;

Transition_internal::Transition_internal (Envelope * env) {
    _env = env;
    //_env->ref_inc();
    ref = 1;
    curr_matching = CB(-1,-1);
//    curr_matching_set = false;
//    is_matched = false;
//    is_issued = false;
//    trans_count++;  //[grz] maybe this shouldn't be done at creation time
//    env_dealloc = true;
}

Transition_internal::Transition_internal () {
    _env = (Envelope*)1;
    ref = 1;
    curr_matching = CB(-1,-1);
//    curr_matching_set = false;
//    is_matched = false;
//    is_issued = false;
//    env_dealloc = false;
//    trans_count++;
}

Transition_internal::Transition_internal (Transition_internal &t) {
    //    highest_ancestor = t.highest_ancestor;
//    env_dealloc = false;
    (*this) = t;
//    trans_count++;
    //        printf("trans_count : %d\n, size of trans %d", trans_count, sizeof(Transition));
}

Transition_internal::~Transition_internal() {
//    std::cerr << "[~Transition_internal]\n";
//    if(!valid) 
//    std::cerr << "[~Transition_internal] invalid\n";
//    valid = false;
//    std::cout << "[~Trandition_internal] " << _env->ref << "\t\t" << _env << "\n";

    if(!_env->isshared()) {
        //_env->ref_dec();
        delete _env;
        DS( _env = NULL; )
    } else _env->ref_dec();

//    if(env_dealloc) {
//        delete _env;
//        DS( _env = NULL; )
//    }
}

// dhriti
Transition_internal* Transition_internal::clone()
{
    Transition_internal* t = new Transition_internal();
    
    t->ancestors = this->ancestors;
    t->_intra_cb = this->_intra_cb;
    t->_inter_cb = this->_inter_cb;
    t->_cond_intra_cb = this->_cond_intra_cb;
    t->ref = this->ref;
    t->curr_matching = this->curr_matching;
    t->_env = (this->_env)->clone();

    return t;
}

Transition_internal &Transition_internal::operator= (Transition_internal &t) {
    /* avo 06/02/08 fixing memory leaks */
    /* why can't we delete this envelope here? Something else must be using it */

//    if(env_dealloc) {
//        delete _env; //[grz] this currently will not be executed
//        //DS( std::cout << "[Transition operator=] env_dealloc\n"; )
//        DS( _env = NULL; )
//    }

//    env_dealloc = true;
    ref = 1;
//    is_matched = t.is_matched;
//    is_issued = t.is_issued;

    curr_matching = t.curr_matching;

    //printf("*******Creating env in Transition\n");

    _env = t.GetEnvelope();
    _env->ref_inc();
//    _env = new Envelope(*t.GetEnvelope()); //[grz] only necessary for fib?

    ancestors = t.ancestors;
    _intra_cb = t._intra_cb;
    _inter_cb = t._inter_cb;
    _cond_intra_cb = t._cond_intra_cb;

//    matches = t.matches;
    return *this;
}

//void Transition_internal::moveCurrMatching () {
//    matches.push_back (curr_matching);
//}

//bool Transition_internal::isMatched (CB &c) {
//    std::vector <CB>::iterator iter;
//    std::vector <CB>::iterator iter_end = matches.end();
//    for (iter = matches.begin (); iter != iter_end; iter++) {
//        if (iter->_pid == c._pid) {
//            return true;
//        }
//    }
//    return false;
//}

void Transition_internal::Execute (ServerSocket &sock) {

    /*
     * Execute the process in the _backtrack.
     */
    _env->Issued ();
    sock.Send (_env->id, goahead);
}

/* Already inlined in the header file
Envelope *Transition::GetEnvelope () {
    return _env;
}
*/

std::ostream &operator<< (std::ostream &os, Transition_internal &t) {
    std::vector <CB>::iterator iter;
    std::vector <CB>::iterator iter_end = t._intra_cb.end();
    os << *(t.GetEnvelope ());
    os << "{";
    for (iter = t._intra_cb.begin(); iter != iter_end; iter++) {
        os << *iter;
    }
    os << "} {";
    iter_end = t._inter_cb.end();
    for (iter = t._inter_cb.begin(); iter != iter_end; iter++) {
        os << *iter;
    }
    os << "}";
    if (t.is_curr_matching_set() && (t.GetEnvelope ()->isSendType () ||
                            t.GetEnvelope ()->isRecvType ())) {
        os << "\t Matched [" << t.curr_matching._pid
           << ","<< t.curr_matching._index << "]";
    }
    return os;
}

std::ostream &operator<< (std::ostream &os, const CB &c) {
    os << "[" << c._pid << ", "<< c._index << "]";
    
    return os;
}

void Transition_internal::PrintLog () {
    std::vector <CB>::iterator iter;
    std::vector <CB>::iterator iter_end = _intra_cb.end();
    Scheduler::_logfile << Scheduler::interleavings << " " << GetEnvelope()->id
                                                                 << " "    ;
    GetEnvelope ()->PrintLog();
    Scheduler::_logfile << "{ ";
    for (iter = _intra_cb.begin(); iter != iter_end; iter++) {
        iter->PrintLog();
        Scheduler::_logfile << " ";
    }
    Scheduler::_logfile << "} { ";
    iter_end = _inter_cb.end();
    for (iter = _inter_cb.begin(); iter != iter_end; iter++) {
        iter->PrintLog(true);
        Scheduler::_logfile << " ";
    }
    Scheduler::_logfile << "}";
    if (is_curr_matching_set() && (GetEnvelope ()->isSendType () ||
                            GetEnvelope ()->isRecvType ())) {
        Scheduler::_logfile << " Match: " << curr_matching._pid
                  << " " << curr_matching._index;
    } else {
        Scheduler::_logfile << " Match: " << -1
                  << " " << -1;

    }
    Scheduler::_logfile << " File: " << GetEnvelope ()->filename.length ()
                        << " " << GetEnvelope ()->filename
                        << " " << GetEnvelope ()->linenumber;
}

void CB::PrintLog (bool pid) {
    if (!pid)
        Scheduler::_logfile << _index ;
    else 
        Scheduler::_logfile << "[ " << _pid << " " <<_index 
                    << " ]" ;
}

bool Transition_internal::AddIntraCB_check(CB &c) {
    std::vector <CB>::iterator iter;
    std::vector <CB>::iterator iter_end = _intra_cb.end();

    for (iter = _intra_cb.begin (); iter != iter_end; iter++) {
        if (*iter == c) {
            return false;
        }
    }
    iter_end = _inter_cb.end();
    for (iter = _inter_cb.begin (); iter != iter_end; iter++) {
        if (*iter == c) {
            return false;
        }
    }
    return true;
}

void Transition_internal::AddIntraCB_mutate(CB &c) {
    _intra_cb.push_back(c);
}

bool Transition_internal::AddInterCB_check(CB &c) {
    std::vector <CB>::iterator iter;
    std::vector <CB>::iterator iter_end = _intra_cb.end();

    if (c._pid == -1 && c._index == -1) {
        _inter_cb.push_back (c);
        return false;
    }
    for (iter = _intra_cb.begin (); iter != iter_end; iter++) {
        if (*iter == c) {
            return false;
        }
    }
    iter_end = _inter_cb.end();
    for (iter = _inter_cb.begin (); iter != iter_end; iter++) {
        if (*iter == c) {
            return false;
        }
    }
    return true;
}
void Transition_internal::AddInterCB_mutate(CB &c) {
    _inter_cb.push_back(c);
}

#ifdef FIB
bool Transition_internal::AddCondIntraCB_check(CB &c)
{
    std::vector <CB>::iterator iter;

    for (iter = _cond_intra_cb.begin ();
            iter != _cond_intra_cb.end (); iter++) {
        if (*iter == c) {
            return false;
        }
    }
 
    for (iter = _inter_cb.begin ();
            iter != _inter_cb.end (); iter++) {
        if (*iter == c) {
            return false;
        }
    }
    return true;
}

void Transition_internal::AddCondIntraCB_mutate(CB &c) {
    _cond_intra_cb.push_back(c);
}
#endif

bool Transition_internal::is_curr_matching_set() {
    return curr_matching != CB(-1,-1);
}

void Transition_internal::ref_inc() {
    ref++;
    //std::cout << "  Transition_internal:ref_inc " << this << " " << ref << "\n";
}

void Transition_internal::ref_dec() {
    DS( assert(ref>1); )
    ref--;
    //std::cout << "  Transition_internal:ref_dec " << this << " " << ref << "\n";
}

bool Transition_internal::isshared() {
    DS( assert(ref>0); )
    return ref != 1;
}
