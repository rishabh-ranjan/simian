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
 * File:        Transition.hpp
 * Description: Implements handling of a single transition in an interleaving
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _TRANSITION_HPP
#define _TRANSITION_HPP

#include "Envelope.hpp"
#include "ServerSocket.hpp"
#include <vector>
#include <cassert>

/*
 * This implements a single transition
 */
class CB {
public:
    int _pid;
    int _index;
    CB () { }

    CB (int i, int ind) : _pid(i), _index(ind) {
    }
    CB(const CB& c) : _pid(c._pid), _index(c._index){}
    bool operator== (const CB &c) {
        return _pid == c._pid && _index == c._index;
    }
    bool operator!= (const CB &c) {
        return !(*this == c);
    }
    friend bool operator< (const CB &a, const CB &b) {
        return a._pid < b._pid || (a._pid == b._pid && a._index < b._index);
    }
    
    friend bool operator> (const CB &a, const CB &b) {
        return a._pid > b._pid || (a._pid == b._pid && a._index > b._index);
    }
    
    
    CB& operator= (const CB &c) {
        this->_pid = c._pid;
        this->_index = c._index;
        return *this;
    }
    void PrintLog(bool pid = false);
    friend std::ostream &operator<< (std::ostream &os, const CB &c);

};

class Transition_internal {
public:
    Transition_internal();
    ~Transition_internal();
    Transition_internal(Envelope * env);
    /*
      * Copy ctor and assignment operator
     */
    Transition_internal(Transition_internal &t);
    Transition_internal& operator=(Transition_internal &);

    // dhriti
    Transition_internal* clone();

    /*
     * Implement the Execute functionality here.
     * This executes the last process instruction enables in
     * backtrack and sends a goahead to the process.
     */

    void Execute (ServerSocket &sock);

    
    inline Envelope *GetEnvelope () {return _env;}
    bool AddIntraCB_check(CB &c);
    void AddIntraCB_mutate(CB &c);
    void PrintLog();
    bool AddInterCB_check(CB &c);
    void AddInterCB_mutate(CB &c);
//    bool isMatched (CB &c);
//    void moveCurrMatching ();
    bool is_curr_matching_set();
    
#ifdef FIB
    bool AddCondIntraCB_check(CB &c);
    void AddCondIntraCB_mutate(CB &c);
#endif
    void ref_inc();
    void ref_dec();
    bool isshared();

    friend std::ostream &operator<< (std::ostream &os, Transition_internal &t);

    std::vector <CB> _inter_cb;
    std::vector <CB> _intra_cb;
#ifdef FIB
    std::vector <CB> _cond_intra_cb;
#endif

    /*
     * What this node is actually matched with
     * makes sense for only sends and receives.
     */
//    std::vector <CB> matches;
    std::vector<int> ancestors;
    CB curr_matching;
//    bool curr_matching_set;
//    bool is_matched;
//    bool is_issued;
    unsigned int ref;
private:
    Envelope     *_env;
};

//std::ostream &operator<< (std::ostream &os, Transition_internal &t);

class Transition {
public:
    Transition();
    Transition(const Transition &t_, bool ref_t=false);
    Transition(Envelope *e, bool ref_e=false);
    
    Transition &operator=(const Transition &t_);
    
    // dhriti
    Transition clone();

    friend std::ostream &operator<<(std::ostream &os, const Transition &t);

    void ref();
    void unref();
    void copy();
    void Execute(ServerSocket &s);
    Envelope *GetEnvelope() const;
    bool AddIntraCB(CB &c);
    void PrintLog() const;
    void AddInterCB(CB &c);
//    bool isMatched(CB &c) const;
    void moveCurrMatching();

    void set_curr_matching(const CB &c);
    CB &get_curr_matching() const;
//    std::vector <CB> &get_matches() const;
    std::vector<int> &get_ancestors() const;
    std::vector<int> &mod_ancestors();
//    bool &get_is_matched() const;
//    bool &get_is_issued() const;
#ifdef FIB
    std::vector <CB> &get_inter_cb();
    std::vector <CB> &mod_inter_cb();
    std::vector <CB> &get_intra_cb();
    std::vector <CB> &get_cond_intra_cb();

    void AddCondIntraCB(CB &c);
#endif

    Transition_internal *t;
//    int issue_id;
};
  
#endif
