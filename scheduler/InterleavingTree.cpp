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
 * File:        InterleavingTree.cpp
 * Description: Implements ISP's POE algorithm
 * Contact:     isp-dev@cs.utah.edu
 */

#include "InterleavingTree.hpp"
#include <sstream>
#include <map>
#include <queue>
#include <string.h>
#include <stack>
#ifndef WIN32
#include <pthread.h>
#endif
#ifdef USE_OPENMP
#include <omp.h>
#endif

#include "Encoding.hpp"
#include "assert.h"

int choiceInLastInterleaving = 0;

// [svs] - added equality checking function for pair of CBs
bool is_eql(std::pair<CB, CB> p1, std::pair<CB, CB> p2)
{
  if(p1.first == p2.first && 
     p1.second == p2.second)
    return true;
  return false;
}


//[grz] std::vector <TransitionList*> Node::_tlist;

//[grz] never called
Node::Node ():has_child (false) {
}

Node::Node (int num_procs) : has_child (false), _level(0),
			     _num_procs(num_procs) {

  //[grz]    backtrack_i = NULL;
  type = GENERAL_NODE;
#ifdef CONFIG_BOUNDED_MIXING
  expand = false;
#endif
  has_aux_coenabled_sends = false;
  //[grz]    tlist_dealloc = true;
  for (int i = 0; i< num_procs ;i++) {
    _tlist.push_back(new TransitionList (i));
  }
			     }

Node::Node (Node &n) {
  //std::cout << "\n";
  *this = n;
}

//dhriti
Node* Node::clone()
{
  Node* n = new Node();

  n->curr_match_set = this->curr_match_set;
  std::transform(_tlist.begin(), _tlist.end(), back_inserter(n->_tlist), cloneFunctor()); // --> Deep copy of transition list
  n->_num_procs = _num_procs;
  n->type = type;
  n->_level = _level;
  n->itree = itree;
  n->wildcard = wildcard;
  n->has_aux_coenabled_sends = has_aux_coenabled_sends;
  n->tlist_dealloc = tlist_dealloc;
  n->has_child = has_child;
  n->ample_set = ample_set;
  n->enabled_transitions = enabled_transitions;
  n->other_wc_matches = other_wc_matches;
  n->_countracker = _countracker;

#ifdef CONFIG_BOUNDED_MIXING
  n->expand = expand;
#endif

  return n;
}

//[grz] this constructor is no longer used
Node::Node (Node &n, bool copyTL) {
  if (copyTL) {
    tlist_dealloc = true;
    for (int i = 0 ; i < _num_procs; i++) {
      TransitionList *tl = new TransitionList (*(n._tlist[i]));
      _tlist.push_back (tl);
    }
  } else {
    for (int i = 0 ; i < _num_procs; i++) {
      //        TransitionList *tl = new TransitionList (*(n._tlist[i]));
      _tlist.push_back (n._tlist[i]);
    }
  }

  _num_procs = n.NumProcs();
  type = GENERAL_NODE;
  _level = (n.GetLevel())+1;
  itree = n.itree;
  has_child = false;
  //[grz]    backtrack_i = NULL;
  *this = n;  
}

Node::~Node() {
  //std::cout << "\t[~Node]\n";
  //[grz]    backtrack();
  //[grz]    if(backtrack_i) delete[] backtrack_i;
  //[grz]    if(tlist_dealloc) {
  
  for(int i=0; i<_num_procs; i++)
    delete this->_tlist[i];

  std::list<CB>().swap(curr_match_set);
  std::vector<std::list<CB> >().swap(ample_set);
  std::vector<std::list<int> >().swap(enabled_transitions);
  std::vector<std::list<CB> >().swap(other_wc_matches);
}


Node &Node::operator= (Node &n) {
  _num_procs = n.NumProcs ();
  for (int i = 0 ; i < _num_procs; i++) {
    _tlist.push_back(new TransitionList(*n._tlist[i]));
  }
  //[grz]    _tlist = n._tlist;
  //[grz]    tlist_dealloc = false;
  //[grz]    backtrack_i = NULL;
  _level = (n.GetLevel())+1;
  has_child = false;
  itree = n.itree;
#ifdef CONFIG_BOUNDED_MIXING
  expand = false;
#endif

  /*subodh, sriram
   *begin CoE
   */

  _countracker = n._countracker; 

  return *this;
}

void Node::deepCopy() {
  int i;
  std::vector <TransitionList*> tlist;
  for(i=0; i<_num_procs; i++) {
    TransitionList *tl = new TransitionList(*(this->_tlist[i]));
    tlist.push_back(tl);
  }
  _tlist = tlist;
  tlist_dealloc = true;
}

void Node::setITree(ITree* new_itree) {
  itree = new_itree;
}

ITree* Node::getITree() {
  return itree;
}

int Node::NumProcs () {
  return _num_procs;
}

int Node::GetLevel () {
  return _level;
}

std::ostream &operator<< (std::ostream &os, Node &n) {

  std::vector <TransitionList *> ::iterator iter;
  std::vector <TransitionList *> ::iterator iter_end = n._tlist.end();

  for (iter = n._tlist.begin (); iter != iter_end; iter++) {
    os << "Transition list for " << (*iter)->GetId () << std::endl;
    os << (*(*iter));
    if (iter + 1 != iter_end) {
      os << std::endl;
    }
  }
  return os;
}

void Node::PrintLog () {
  std::vector <TransitionList *> ::iterator iter;
  std::vector <TransitionList *> ::iterator iter_end = _tlist.end();

  for (iter = _tlist.begin (); iter != iter_end; iter++) {
    (*iter)->PrintLog ();
  }
}

int Node::getTotalMpiCalls() {
  int sum = 0;
  std::vector <TransitionList *> ::iterator iter;
  std::vector <TransitionList *> ::iterator iter_end = _tlist.end();

  for (iter=_tlist.begin(); iter != iter_end; iter++) {
    sum += (*iter)->_tlist.size();
  }
  return sum;
}

bool Node::AllAncestorsMatched (CB &c, std::vector <int> &l) {
   
  bool is_wait_or_test_type = GetTransition(c)->GetEnvelope()->isWaitorTestType();

  std::vector <int>::iterator iter, iter2, iter2_end;
  std::vector <int>::iterator iter_end = l.end();
  for (iter = l.begin (); iter != iter_end; iter++) {
    if (!itree->is_matched[c._pid][(*iter)]) {
      /* == fprs begin == */
      // Wei-Fan Chiang: I don't know why I need to add this line. But, I need it........
      if (_tlist[c._pid]->_tlist[(*iter)].GetEnvelope ()->func_id == PCONTROL) continue;
      /* == fprs end == */
      if (is_wait_or_test_type && !Scheduler::_send_block) {
	if (_tlist[c._pid]->_tlist[(*iter)].GetEnvelope ()->func_id == ISEND) {
	  continue;
	}
      }
      return false;
    } else if (_tlist[c._pid]->_tlist[*iter].GetEnvelope()->func_id == WAIT ||
	       _tlist[c._pid]->_tlist[*iter].GetEnvelope()->func_id == TEST ||
	       _tlist[c._pid]->_tlist[*iter].GetEnvelope()->func_id == WAITALL ||
	       _tlist[c._pid]->_tlist[*iter].GetEnvelope()->func_id == TESTALL) {

      Transition t = _tlist[c._pid]->_tlist[*iter];
      iter2_end = t.get_ancestors().end();
      for (iter2 = t.get_ancestors().begin (); iter2 != iter2_end; iter2++) {
	Envelope* env_f = _tlist[c._pid]->_tlist[*iter2].GetEnvelope();
	Envelope* env_s = GetTransition (c)->GetEnvelope();
	//if (_tlist[c._pid]->_tlist[(*iter2)]->is_matched == false &&
	if(!itree->is_matched[c._pid][(*iter2)] &&
	   env_f->isSendType () && env_s->isSendType () &&
	   env_f->dest == env_s->dest && env_f->comm == env_s->comm &&
	   env_f->stag == env_s->stag) {
	  return false;
	}
      }
    }
  }
  return true;
}

bool Node::AnyAncestorMatched (CB &c, std::vector<int> &l) {
  std::vector <int>::iterator it;

  bool any_match = false;

  Envelope *e = GetTransition (c)->GetEnvelope ();

  std::vector <int>::iterator iter;
  std::vector <int>::iterator iter_end = l.end();

  std::vector <int>::iterator it_end = e->req_procs.end();

  bool is_wait_or_test_type = GetTransition(c)->GetEnvelope()->isWaitorTestType();
  if (is_wait_or_test_type && l.size () == 0) {
    return true;
  }

  for (it = e->req_procs.begin (); it != it_end; it++) {
    //if (_tlist[c._pid]->_tlist[*it]->is_matched == true ||
    if (itree->is_matched[c._pid][*it] || 
	(is_wait_or_test_type && !Scheduler::_send_block && _tlist[c._pid]->_tlist[*it].GetEnvelope()->func_id == ISEND)) {
      any_match = true;
    }
    for (iter = l.begin (); iter != iter_end; iter++) {
      if ((*iter) == *it) {
	iter = l.erase (iter);
	iter_end = l.end ();
	break;
      }
    }
  }
  if (!any_match) {
    return false;
  }

  iter_end = l.end();

  for (iter = l.begin (); iter != iter_end; iter++) {
    if(!itree->is_matched[c._pid][*iter]) {
      return false;
    }
  }

  return true;
}

/* Comment this method out if pthread is desired instead of OpenMP */
/* Note that the use of reverse iterator here is for performance reason 
 * so that we can break off the loop when we hit the last_matched
 * transition - If we stop using reverse iterator - we need to stop using
 * reverse iterator in GetMatchingSends as well - otherwise we won't 
 * be able to preserve the program order matching of receives/sends */
void Node::GetEnabledTransitionsOpenMP (std::vector <std::list <int> >&l) {
  l.clear ();

  for (int i = 0; i < NumProcs(); i++) {
    l.push_back (std::list <int> ());
  }

#ifdef USE_OPENMP
#pragma omp parallel for
#endif
  for (int i = 0 ; i < NumProcs () ;i++) {
    std::vector <Transition>::reverse_iterator iter =
      _tlist[i]->_tlist.rbegin();
    std::vector <Transition>::reverse_iterator iter_end;
    CB c(i,0);

    iter_end = _tlist[i]->_tlist.rend();
    int j = (int)_tlist[i]->_tlist.size()-1;
    for (; iter != iter_end; iter++) {
      c._index = j;

      if( !itree->is_matched[i][j] ) {
	std::vector<int> &ancestor_list(GetTransition(c)->get_ancestors());
	if ((iter->GetEnvelope ()->func_id != WAITANY &&
	     iter->GetEnvelope ()->func_id != TESTANY) &&
	    AllAncestorsMatched (c,ancestor_list)) {
	  l[i].push_back (j);
	} else if ((iter->GetEnvelope()->func_id == WAITANY ||
		    iter->GetEnvelope()->func_id == TESTANY) &&
		   AnyAncestorMatched (c,ancestor_list)) {
	  l[i].push_back (j);

	}
      }
      j--;
      if (j <= itree->last_matched[i])
	break;
            
    }
  }
  //std::cout << "\n";

}

/* This is the exact same as the above, just with OpenMP disabled. */
/* Note that the use of reverse iterator here is for performance reason 
 * so that we can break off the loop when we hit the last_matched
 * transition - If we stop using reverse iterator - we need to stop using
 * reverse iterator in GetMatchingSends as well - otherwise we won't 
 * be able to preserve the program order matching of receives/sends */
void Node::GetEnabledTransitionsSingleThreaded (std::vector <std::list <int> > &l) {
  l.clear ();

  for (int i = 0; i < NumProcs(); i++) {
    l.push_back (std::list <int> ());
  }

  for (int i = 0 ; i < NumProcs () ;i++) {
    std::vector <Transition>::reverse_iterator iter =
      _tlist[i]->_tlist.rbegin();
    std::vector <Transition>::reverse_iterator iter_end;
    CB c(i,0);

    iter_end = _tlist[i]->_tlist.rend();
    int j = (int)_tlist[i]->_tlist.size()-1;
    for (; iter != iter_end; iter++) {
      c._index = j;
      if (!itree->is_matched[i][j]) {
	std::vector<int> & ancestor_list(GetTransition(c)->get_ancestors());
	if ((iter->GetEnvelope ()->func_id != WAITANY &&
	     iter->GetEnvelope ()->func_id != TESTANY) &&
	    AllAncestorsMatched (c,ancestor_list)) {
	  l[i].push_back (j);
	} else if ((iter->GetEnvelope()->func_id == WAITANY ||
		    iter->GetEnvelope()->func_id == TESTANY) &&
		   AnyAncestorMatched (c,ancestor_list)) {
	  l[i].push_back (j);

	}
      }
      j--;
      if (j <= itree->last_matched[i])
	break;
            
    }
  }
  //std::cout << "\n";

}

void print(std::vector<std::list <int> > &l) {
  for (unsigned int i = 0 ; i < l.size(); i++) {
    copy (l[i].begin(), l[i].end(), std::ostream_iterator <int> (std::cout, " "));
    std::cout<< std::endl;
  }
}
void print(std::vector<std::list <CB> > &l) {
  std::list <CB>::iterator iter;
  std::list <CB>::iterator iter_end;
    
  for (unsigned int i  = 0 ; i < l.size(); i++) {
    iter_end = l[i].end();
    for (iter = l[i].begin(); iter != iter_end; iter++) {
      std::cout << iter->_pid << "," << iter->_index << " ";
    }
    std::cout << std::endl;
  }
}
/*
  void FreeMemory (std::list<CB> &l) {
  std::list<CB*>::iterator iter;
  std::list<CB*>::iterator iter_end = l.end();
  for(iter = l.begin(); iter != iter_end; iter++) {
  delete *iter;
  }
  }
*/

void *GetEnabledTransitionsThreadC (void* obj) {
    
  struct thread_data *my_data;
  my_data = (struct thread_data*) obj;
  Node *n = my_data->node;
  n->GetEnabledTransitionsThread(my_data->pid, my_data->l);
  //    pthread_exit(NULL);
  return NULL;

}

/*
 * Uncomment this to have a pthread GetEnabledTransitions 
 */
/*
  void Node::GetEnabledTransitions (std::vector <std::list <int> > &l) {

  l.clear();
  pthread_t *threads = new pthread_t [NumProcs()];
  thread_data *t = new thread_data [NumProcs()];
  for (int i = 0; i < NumProcs(); i++) {
  l.push_back(std::list <int> ());
  }
  for (int i = 0; i < NumProcs(); i++) {
  t[i].node = this;
  t[i].pid = i;
  t[i].l = &l[i];
  pthread_create(&threads[i], NULL, GetEnabledTransitionsThreadC, (void *) &t[i]);

  }
  void* status;
  for (int t = 0; t < NumProcs(); t++) {
  pthread_join(threads[t], &status);
  }


  }
*/

/* This function is used when we used pthread instead of OpenMP */

void Node::GetEnabledTransitionsThread(int pid, std::list<int> *l) {
  CB c(0,0);
  std::vector <Transition>::reverse_iterator iter;
  std::vector <Transition>::reverse_iterator iter_end;
    
  iter_end = _tlist[pid]->_tlist.rend();
  c._pid = pid;

  int j = (int)_tlist[pid]->_tlist.size()-1;
  for (iter = _tlist[pid]->_tlist.rbegin (); iter != iter_end; iter++) {
    c._index = j;

    //if ((*iter)->is_matched == false) {
    if (!itree->is_matched[pid][j]){  
      //GetAncestors(c,ancestor_list);
      std::vector<int> &ancestor_list(GetTransition(c)->get_ancestors());
      if ((iter->GetEnvelope()->func_id != WAITANY &&
	   iter->GetEnvelope()->func_id != TESTANY) &&
	  AllAncestorsMatched (c,ancestor_list)) {
	l->push_back (j);
      } else if ((iter->GetEnvelope()->func_id == WAITANY ||
		  iter->GetEnvelope()->func_id == TESTANY) &&
		 AnyAncestorMatched (c,ancestor_list)) {
	l->push_back (j);
      }
    }
    j--;
    //if (j <= _tlist[pid]->last_matched)
    if (j <= itree->last_matched[pid])
      break;
  }
}

void Node::GetWaitorTestAmple (std::vector <std::list <int> > &l) {
  std::list <CB> blist;
  //    #pragma omp parallel for
  for (int i = 0; i < NumProcs (); i++) {
    std::list <int>::iterator iter;
    std::list <int>::iterator iter_end;
    Envelope *e;
    iter_end = l[i].end();
    for (iter = l[i].begin (); iter != iter_end; iter++) {
      e = GetTransition(i, (*iter))->GetEnvelope();

      if (e->isWaitorTestType ()) {
	//std::cout << "FOUND TEST ALL AMPLE!\n";
	//#pragma omp critical
	blist.push_back (CB(i, *iter));
      }
    }
  }
  if (!blist.empty ()) {
    ample_set.push_back (blist);

  }
}

bool Node::GetCollectiveAmple (std::vector <std::list <int> > &l, int collective) {
  std::list <CB> blist;
  std::list <CB> flist;

  for (int i = 0; i < NumProcs (); i++) {
    std::list <int>::iterator iter;
    std::list <int>::iterator iter_end;
        
    iter_end = l[i].end();
    for (iter = l[i].begin (); iter != iter_end; iter++) {
      if (GetTransition (i, *iter)->GetEnvelope ()->func_id == collective) {
	blist.push_back (CB(i, *iter));
      }
    }
  }

  if (collective == FINALIZE) {
    if ((int)blist.size () == NumProcs ()) {
      ample_set.push_back (blist);
      return true;
    }
    return false;
  }

  Envelope *e1;
  Envelope *e2;
  std::list <CB>::iterator iter1;
  int nprocs;
  std::string comm;
  std::list <CB>::iterator iter_end;
  std::list <CB>::iterator iter;
  std::list <CB>::iterator iter3;

  iter_end = blist.end();

  for (iter = blist.begin (); iter != iter_end; iter++) {
    e1 = GetTransition (*iter)->GetEnvelope ();
    nprocs = e1->nprocs;
    comm = e1->comm;
    for (iter1 = blist.begin (); iter1 != iter_end; iter1++) {
      e2 = GetTransition (*iter1)->GetEnvelope ();
      if (e2->comm == e1->comm) {
	flist.push_back (CB(*iter1));
      }
    }
    if ((collective == BCAST || collective == GATHER ||
	 collective == SCATTER || collective == SCATTERV ||
	 collective == GATHERV || collective == REDUCE) && !flist.empty()) {
      std::list <CB>::iterator iter;
      std::list <CB>::iterator iter_end2 = flist.end();

      int root =
	GetTransition (*flist.begin())->GetEnvelope ()->count;
      for (iter = flist.begin (); iter != iter_end2; iter++) {
	int root1 = GetTransition (*iter)->GetEnvelope ()->count;

	if (root != root1) {
	  //FreeMemory(flist);
	  flist.clear ();
	  //FreeMemory(blist);
	  return false;
	}
      }
    }
 
    if (collective == COMM_CREATE || collective == COMM_DUP ||
	collective == CART_CREATE || collective == COMM_SPLIT) {
      /* 0 = COMM_WORLD, 1 = COMM_SELF, 2 = COMM_NULL */
      static int comm_id = 3;
      if (collective == COMM_SPLIT) {
	std::map<int, int> colorcount;

	/* Mark which colors are being used in the map. */
	std::list <CB>::iterator iter;
	std::list <CB>::iterator iter_end2 = flist.end();
	for (iter = flist.begin (); iter != iter_end2; iter++) {
	  colorcount[GetTransition (*iter)->GetEnvelope ()->comm_split_color] = 1;
	}

	/* Assign comm_id's to all of the new communicators. */
	std::map <int, int>::iterator iter_map;
	std::map <int, int>::iterator iter_map_end = colorcount.end();
	for (iter_map = colorcount.begin (); iter_map != iter_map_end; iter_map++) {
	  iter_map->second = comm_id++;
	}

	/* Put the new IDs in the envelopes. */
	for (iter = flist.begin (); iter != iter_end2; iter++) {
	  Envelope *e = GetTransition (*iter)->GetEnvelope ();
	  e->comm_id = colorcount[e->comm_split_color];
	}
      } else {
	int id = comm_id++;

	std::list <CB>::iterator iter;
	std::list <CB>::iterator iter_end2 = flist.end();
	for (iter = flist.begin (); iter != iter_end2; iter++) {
	  GetTransition(*iter)->GetEnvelope()->comm_id = id;
	}
      }
    }

    if ((int)flist.size () == nprocs) {
      break;
    } else {
      //FreeMemory(flist);
      flist.clear ();
    }
  }

  //FreeMemory(blist);
  if (! flist.empty ()) {
    ample_set.push_back (flist);
    //        std::cout << "MATCHED COLLECTIVES!\n";
    return true;
  }
  return false;
}

bool Node::getNonWildcardReceive (std::vector <std::list <int> > &l) {
  for (int i = 0; i < NumProcs (); i++) {
    std::list <int>::iterator iter;
    std::list <int>::iterator iter_end;
        
    iter_end= l[i].end();
    for (iter = l[i].begin (); iter != iter_end; iter++) {
      //            std::cout << *(GetTransition(i, *iter)->GetEnvelope()) << std::endl;
      //            std::cout << "id is" << GetTransition(i, *iter)->GetEnvelope()->func_id << std::endl;
            
      if (GetTransition (i, *iter)->GetEnvelope ()->isRecvType ()) {
	if (GetTransition (i, *iter)->GetEnvelope ()->src != WILDCARD) {
	  CB tempCB(i, *iter);
	  CB c1;
	  if(getMatchingSend (c1, l, tempCB)) {
	    std::list <CB> ml;
	    ml.push_back (c1);
	    ml.push_back (CB(i, *iter));
	    ample_set.push_back (ml);
	    return true;
	  }
	}
      }
    }
  }
  return false;
}

/* The use of reverse_iterator here is necessary to preserve program-order
 * matching - This is based on the fact that GetEnabledTransistions also
 * uses a reverse iterator */
bool Node::getMatchingSend (CB &res, std::vector <std::list <int> > &l, CB &c) {
  //    std::cout <<"here in getMatchingSend\n";
  int src = GetTransition(c)->GetEnvelope ()->src;
  std::list <int>::reverse_iterator iter;
  std::list <int>::reverse_iterator iter_end = l[src].rend();
  //std::cout << "Finding match for a receive " << *c << "|" << *GetTransition(*c) << "|\n";
  Envelope *e;
  Envelope *c_env = GetTransition(c)->GetEnvelope();
  for (iter = l[src].rbegin (); iter != iter_end; iter++) {
    e = GetTransition(src, (*iter))->GetEnvelope();

    if (e->isSendType () &&
	e->dest == c._pid &&
	e->comm == c_env->comm &&
	(e->stag == c_env->rtag || c_env->rtag == WILDCARD)) {
      //std::cout << "Found a matching send!!! " << "\n";
      res._pid = src;
      res._index = *iter;
      return true;
    }
  }
  return false;
}

bool Node::getAllMatchingSends (std::vector <std::list <int> > &l, CB &c, 
                                std::vector <std::list <CB> >& ms) {
  DR( std::cout << " [getAllMatchingSends begin]\n\trecv=" << c << "\n"; )

    std::list <int>::reverse_iterator iter;
  std::list <int>::reverse_iterator iter_end;
  Envelope *e;
  Envelope *c_env = GetTransition(c)->GetEnvelope();
  bool found_ample = false;
  std::set<CB> *sends = &itree->matched_sends[c];

  DR(
     std::set <CB>::iterator si = sends->begin();
     std::set <CB>::iterator sie = sends->end();
     std::cout << "\tmatched_sends:"; 
     for(; si!=sie; si++)
       std::cout << " " << *si;
     std::cout << "\n";
     )

    for (int i = 0; i < NumProcs (); i++) {

      iter_end = l[i].rend();
      for (iter = l[i].rbegin (); iter != iter_end; iter++) {
	e = GetTransition(i, *iter)->GetEnvelope();
	
	if (e->isSendType () &&
	    e->dest == c._pid &&
	    e->comm == c_env->comm &&
	    (e->stag == c_env->rtag || c_env->rtag == WILDCARD) && (
// #ifdef CONFIG_OPTIONAL_AMPLE_SET_FIX
// 								    Scheduler::_no_ample_set_fix ? true :
// #endif
								    (sends->find(CB(i, *iter)) == sends->end()))) {
	  std::list <CB> ml;
	  ml.push_back (CB(i, *iter));
	  ml.push_back (c);
	  //                #pragma omp critical
	  ms.push_back (ml);
	  found_ample = true;
	  break;
	}
      }
    }
  DR( std::cout << " [getAllMatchingSends end]\n"; )
    return found_ample;
}

void Node::GetallSends (std::vector <std::list <int> > &l) {
  bool first = false;

  //    #pragma omp parallel for 
  for (int i = 0; i < NumProcs (); i++) {
    std::list <int>::iterator iter;
    std::list <int>::iterator iter_end;

    iter_end = l[i].end();

    for (iter = l[i].begin (); iter != iter_end; iter++) {
      if (GetTransition (i, (*iter))->GetEnvelope ()->isRecvType ()) {
	if (GetTransition (i, (*iter))->GetEnvelope ()->src == WILDCARD) {
	  //std::cout << "RECV Transition is " << *GetTransition(*(*iter)) << "\n";
	  CB tempCB(i, *iter);
	  //getAllMatchingSends (l, tempCB);
	  if (getAllMatchingSends (l, tempCB, 
				   // #ifdef CONFIG_OPTIONAL_AMPLE_SET_FIX
				   //                             Scheduler::_no_ample_set_fix ? ample_set :
				   // #endif
				   (!first) ? ample_set:other_wc_matches))  {
	    first = true;
	  }
	}
      }
    }
  }
  other_wc_matches.clear();
}

void Node::GetReceiveAmple (std::vector <std::list <int> > &l) {
  //std::cout << "In GetReceiveAmple\n";
  if (getNonWildcardReceive (l)) {
    return;
  }
    
  GetallSends (l);
}

bool Node::GetAmpleSet () {
  if (! ample_set.empty ()) {
    return true;
  }
  //std::vector <std::list <int> > enabled_transitions;

  GetEnabledTransitions (enabled_transitions);
    
  /*
   * See if there is a Barrier set ready to go!
   */
  GetCollectiveAmple (enabled_transitions, BARRIER);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, BCAST);
  if (! ample_set.empty ())
    return true;


  GetCollectiveAmple (enabled_transitions, SCATTER);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, GATHER);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, SCATTERV);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, GATHERV);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, ALLGATHER);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, ALLGATHERV);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, ALLTOALL);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, ALLTOALLV);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, SCAN);
  if (! ample_set.empty ())
    return true;

  /* == fprs begin == */
  GetCollectiveAmple (enabled_transitions, EXSCAN);
  if (! ample_set.empty ())
    return true;
  /* == fprs end == */

  GetCollectiveAmple (enabled_transitions, REDUCE);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, REDUCE_SCATTER);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, ALLREDUCE);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, FINALIZE);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, CART_CREATE);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, COMM_CREATE);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, COMM_DUP);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, COMM_SPLIT);
  if (! ample_set.empty ())
    return true;

  GetCollectiveAmple (enabled_transitions, COMM_FREE);
  if (! ample_set.empty ())
    return true;

  GetWaitorTestAmple (enabled_transitions);
  if (! ample_set.empty ()) {
    return true;
  }
  GetReceiveAmple (enabled_transitions);
  if (! ample_set.empty ())
    return true;

  /* Special case for Test & Iprobe calls 
   * If no call can progress and 
   * If there's a test call then we can match it and return false. 
   * Also need to remove the CB edges */

  std::list<CB> test_list;
  for (int i = 0; i < NumProcs(); i++) {
    Transition t = _tlist[i]->_tlist.back();
    if (t.GetEnvelope()->func_id == TEST ||
	t.GetEnvelope()->func_id == TESTALL ||
	t.GetEnvelope()->func_id == TESTANY ||
	t.GetEnvelope()->func_id == IPROBE) {
      test_list.push_back(CB (i, (int)_tlist[i]->_tlist.size()-1));

      //Need to clean up the CB edge here
      //Each of this transition's ancestors will have an edge
      //to each of the transition's descendants
      std::vector<int>::iterator iter = t.get_ancestors().begin();
      std::vector<int>::iterator iter_end = t.get_ancestors().end();
      for (; iter != iter_end; iter++) {
	std::vector<CB>::iterator iter2 = t.get_intra_cb().begin();
	std::vector<CB>::iterator iter2_end = t.get_intra_cb().end();
	for (; iter2 != iter2_end; iter2++) {
	  Transition* descendant = GetTransition(*iter2);
	  Transition* ancestor = GetTransition(i, *iter);
	  descendant->mod_ancestors().push_back(*iter);
	  ancestor->AddIntraCB(*iter2);
	}
      }
    }
  }
  if (!test_list.empty()) {
    ample_set.push_back(test_list);
  }
  if (!ample_set.empty())
    return true;

    
  return false;
}

Transition *Node::GetTransition (CB &c) {
  Transition* t = GetTransition(c._pid, c._index);
  return t;
  //    return _tlist[c._pid]->_tlist[c._index];
}

Transition *Node::GetTransition (int pid, int index) {
  return &_tlist[pid]->_tlist[index];
}

/* [svs] addition */
bool Node::Present(int a, std::vector<int> l)
{
  std::vector<int>::iterator it; 
  for(it = l.begin(); it != l.end(); it ++){
    if ( (*it) == a )
      return true;
  }
  return false;
}

std::set<int> Node::getImmediateDescendants (CB c)
{
  std::set<int> _descendant;
  
  Transition *_t = GetTransition(c);
  std::vector<CB> _tdesc = _t->get_intra_cb();
  for(std::vector<CB>::iterator it = _tdesc.begin();
      it != _tdesc.end(); it++){
    _descendant.insert((*it)._index);	  
  }    
  return _descendant;
}

std::set<int> Node:: getAllDescendants (CB c)
{
  std::set<int> _alldescendants;
  std::stack<int> st;
  st.push(c._index);

  while(!st.empty()) {
    
    int index = st.top();
    st.pop();
    
    CB tmp(c._pid, index);
    
    std::set<int> _idesc;
    _idesc = getImmediateDescendants(tmp);
    
    std::set<int>::iterator it_end, it;
    it_end = _idesc.end();
    
    for(it = _idesc.begin(); it != it_end; it++) {
      _alldescendants.insert(*it);
      st.push(*it);
    }
  }

  return _alldescendants;
}


std::set<int> Node::getAllAncestors(CB c)
{
  std::set<int> res;
 
 std::list <CB> stack; 
 stack.push_back(c);
   
 while (!stack.empty()) { 
   CB tmp = stack.back();
   assert(tmp._pid != -1) ;
   stack.pop_back();

   Transition * t = GetTransition(tmp);
   std::vector<int>::iterator iter = t->get_ancestors().begin();
 
   std::vector<int>::iterator iter_end = t->get_ancestors().end();
   
   
   for (; iter != iter_end; iter++) {
     
     int i = *iter;
     
     res.insert(i);
     CB c1(c._pid, i);
     stack.push_back(c1);
   }
 }
 
 return res;
 
}

// checks if c2 is an ancestor of c1. 
bool Node::isAncestor(CB c1, CB c2)
{
  std::set<int> res;
  
  if(c1._pid == c2._pid){
    
    res = getAllAncestors(c1);
    //std::cout << "Ancestors of  "<< c1 << ":"  << res.size() << std::endl;
    
    std::set<int>::iterator rit, rit_end; 
    
    //  rit = t->get_ancestors().begin();
    rit_end = res.end();
    
    for(rit = res.begin(); rit != rit_end; rit ++){
      //std::cout << "Looking at Ancestor " << CB(c1._pid, *rit) << std::endl;
      if(c2._index == *rit){
	//std::cout << "Found the ancestor " << c2 << std::endl;
	return true;
      }
    }
  }
  return false;
}

/*End of addition -- [svs] */

// dhriti
ITree* ITree::clone()
{
  ITree* it = new ITree();
  it->mismatchTypeList = mismatchTypeList;
  std::transform((this->_slist).begin(), (this->_slist).end(), back_inserter(it->_slist), cloneFunctor());
  it->depth = depth;

#ifdef FIB
  it->last_node = GetCurrNode();
  it->al_curr = al_curr;
  it->FRBlist = FRBlist;
  it->FIBlist = FIBlist;
  it->blist = blist;
#endif 

  std::vector<bool*>::iterator itBegin = is_matched.begin();
  while(itBegin != is_matched.end())
  {
    (it->is_matched).push_back(new bool(**itBegin));
    itBegin++;
  }
  itBegin = is_issued.begin();
  while(itBegin != is_issued.end())
  {
    (it->is_issued).push_back(new bool(**itBegin));
    itBegin++;
  }
  it->last_matched = new int(*last_matched);
  it->_is_exall_mode = new bool(*_is_exall_mode);

  it->aux_coenabled_sends = aux_coenabled_sends; 
  it->matched_sends = matched_sends;

#ifdef CONFIG_BOUNDED_MIXING
  it->expanded = expanded;
#endif

  //it->traceFile = traceFile; ---> Later: Connect new traceFile with the previous one (with 'continue writing' mode on)
  it->sch = sch; 
  it->have_wildcard = have_wildcard;
  it->pname = pname;
  //(it->_slist[0])->setITree(it);

  return it;
}

ITree::ITree (Node *n, std::string name) {

  pname = name;
  _slist.push_back (n);
  depth = 0;
  have_wildcard = false;
#ifdef CONFIG_BOUNDED_MIXING
  expanded = 0;
#endif 
  for (int i = 0; i < n->NumProcs(); i ++) {
    is_matched.push_back(new bool [MAX_TRANSITIONS]());
    is_issued.push_back(new bool [MAX_TRANSITIONS]());
  }
  last_matched = new int[n->NumProcs()]();
  for (int j = 0; j < n->NumProcs(); j++)
    last_matched[j] = -1;
    
  n->setITree(this);
}

ITree::~ITree () {

  //std::cout << "\nIn ITree destructor 1\n";
  //fflush(stdout);
  std::list<CB>().swap(mismatchTypeList);
  for (std::vector<Node*>::iterator it = _slist.begin() ; it != _slist.end(); ++it)
  {
    delete (*it);
  } 
  _slist.clear();

//std::cout << "\nIn ITree destructor 2\n";
//fflush(stdout);

#ifdef FIB
  //delete last_node;  std::vector<std::list<CB> >().swap(al_curr);
  std::vector<std::list<CB> >().swap(FRBlist);
  std::vector<std::list<CB> >().swap(FIBlist);
  std::vector<CB>().swap(blist);
#endif

//std::cout << "\nIn ITree destructor 3\n";
//fflush(stdout);

  std::vector<bool*>::iterator itBegin = is_matched.begin();
  for ( ; itBegin != is_matched.end(); ++itBegin)
  {
     delete (*itBegin);
  } 
  is_matched.clear();

//std::cout << "\nIn ITree destructor 4\n";
//fflush(stdout);

  itBegin = is_issued.begin();
  for ( ; itBegin != is_issued.end(); ++itBegin)
  {
     delete (*itBegin);
  } 
  is_issued.clear();

//std::cout << "\nIn ITree destructor 5\n";
//fflush(stdout);

  delete last_matched;
  delete _is_exall_mode;

  std::map <CB, std::list <CB> >().swap(aux_coenabled_sends);
  std::map <CB, std::set <CB> >().swap(matched_sends);

//std::cout << "\nIn ITree destructor 6\n";
//fflush(stdout);
}

void ITree::ResetMatchingInfo() {
  for (int i = 0; i< GetCurrNode()->NumProcs(); i++) {
    delete [] is_matched[i];
    delete [] is_issued[i];
    is_matched[i] = new bool[MAX_TRANSITIONS]();
    is_issued[i] = new bool[MAX_TRANSITIONS]();
  }
  delete [] last_matched;

  last_matched = new int[GetCurrNode()->NumProcs()]();
  for (int j = 0; j < GetCurrNode()->NumProcs(); j++)
    last_matched[j] = -1;


}

Node *ITree::GetCurrNode () {
  assert((int)depth < (int)_slist.size());
  return _slist[depth];
}

void ITree::print_CBL(std::list <CB> cbl)
{
  std::list<CB >::iterator it;
  Node * n = GetCurrNode();
  Transition *t;
  Envelope *env;
  std::cout<<"\nMATCHED:[";
  for(it = cbl.begin(); it!= cbl.end(); it++){
    t = n->GetTransition(*it);
    env = t->GetEnvelope();
    if (env->isSendType () ||
	env->isRecvType ())
      std::cout<<"Proc:" << (*it)._pid <<", File: " << env->filename
	       << "," << env->linenumber; ;
  }
  std::cout<<"]\n\n";
  
}

bool matches(std::list<CB> sendRecv, std::vector<expression> changedConditionals, Node* n)
{
  //std::cout << "\nIn matches: " << sendRecv.back() << "\n";
	// sendRecv is of size 2, because we check the size in CHECK() before invoking this function

	// Extract the Send part of sendRecv duo
	Envelope* sendEnv = n->GetTransition(sendRecv.front())->GetEnvelope();
	// Extract the Recv part of sendRecv duo
	Envelope* recvEnv = n->GetTransition(sendRecv.back())->GetEnvelope();

	std::vector<expression>::iterator it = changedConditionals.begin();
  std::vector<expression>::iterator it2 = changedConditionals.end();

  // Parsing the changedConditionals
  for(; it!=it2; ++it)
  {
    expression ex = *it;
    std::string varName = ex.varName;
    char* cstr = new char[varName.length() + 1];
    strcpy(cstr, varName.c_str());
    char* token;
    token = strtok(cstr, "_");
    token = strtok(NULL, "_");
    int pidConditional = atoi(token);
    token = strtok(NULL, "_");
    int indexConditional = atoi(token);

    if(pidConditional==recvEnv->id && indexConditional==recvEnv->index)
    {
    	// We have found a recv which must be matched with only those sends which are sending some different data/tag 

    	if(varName.substr( varName.length()-3 ) == "tag")
    	{
    		// Check the tag of the send's envelope
    		int sendTag = sendEnv->stag;
    		//std::cout << "Send's tag: " << sendTag;
    		if(ex.opr == "==")
    		{
    			if(sendTag != ex.lit)
    			 return false;
    		}
    		else if(ex.opr == "!=")
    		{
    			if(sendTag == ex.lit)
    				return false;
    		}
    		else if(ex.opr == ">")
    		{
    			if(sendTag <= ex.lit)
    				return false;
    		}
    		else if(ex.opr == ">=")
    		{
    			if(sendTag < ex.lit)
    				return false;
    		}
    		else if(ex.opr == "<")
    		{
    			if(sendTag >= ex.lit)
    				return false;
    		}
    		else if(ex.opr == "<=")
    		{
    			if(sendTag > ex.lit)
    				return false;
    		}
    	}
    	else if(varName.substr( varName.length()-4 ) == "data")
    	{
        //std::cout << "\nVarName" << ex.varName << ex.opr << ex.lit << " " << sendEnv->data << "\n";
    		// Check the data of the send's envelope
    		int sendData = sendEnv->data;
    		if(ex.opr == "==")
    		{
    			if(sendData != ex.lit)
    				return false;
    		}
    		else if(ex.opr == "!=")
    		{
    			if(sendData == ex.lit)
    				return false;
    		}
    		else if(ex.opr == ">")
    		{
    			if(sendData <= ex.lit)
    				return false;
    		}
    		else if(ex.opr == ">=")
    		{
    			if(sendData < ex.lit)
    				return false;
    		}
    		else if(ex.opr == "<")
    		{
    			if(sendData >= ex.lit)
    				return false;
    		}
    		else if(ex.opr == "<=")
    		{
    			if(sendData > ex.lit)
    				return false;
    		}
    	}
    }
  }
  return true;
}

// -- changed by dhriti
int ITree::CHECK (ServerSocket &sock, std::list <int> &l, std::vector<expression> changedConditionals) 
{
	bool probe_flag = false;
	int choice = 0; // default is EXP_MODE_LEFT_MOST
	Node *n = GetCurrNode();
  fflush(stdout);
	Envelope *env;
	if (n->GetAmpleSet()) 
	{
		std::list<CB> cbl;

		// modify the ample set
		if (Scheduler::_fprs) 
		{
			std::vector< std::list<CB> > new_ample_set;
			std::vector< std::list<CB> > dont_care_set;
			std::vector< std::list<CB> >::iterator as_iter;
			Envelope *this_env;
			for (as_iter = n->ample_set.begin() ; as_iter != n->ample_set.end() ; as_iter++) 
			{
				bool dont_care = true;
				std::list<CB>::iterator cbiter;
				for (cbiter = (*as_iter).begin() ; cbiter != (*as_iter).end() ; cbiter++) 
				{
					this_env = n->GetTransition(*cbiter)->GetEnvelope();
					if (this_env->in_exall) 
					{
						new_ample_set.push_back(*as_iter);
						dont_care = false;
						break;
					}	
				}

				if (dont_care) 
				{
					dont_care_set.push_back(*as_iter);	
				}
			}
			/*
			std::cout << "======== ample set size : " << n->ample_set.size() << std::endl;
			std::cout << "======== new ample set size : " << new_ample_set.size() << std::endl;
			std::cout << "======== don't care size : " << dont_care_set.size() << std::endl;
			*/
			int choice_dc = 0;
			if (dont_care_set.size() > 1) choice_dc = rand() % dont_care_set.size();
			if (dont_care_set.size() > 0) new_ample_set.push_back(dont_care_set.at(choice_dc));

			n->ample_set.clear();
			n->ample_set = new_ample_set;

			cbl = n->ample_set.at(choice);
		}
		else if(Scheduler::_errorTrace)
		{
		}
		else // dhriti: I have made the changes only here because only this (else part) is actually executed
		{
  		/* == fprs end == */
			if (n->ample_set.size() > 1 && Scheduler::_explore_mode == EXP_MODE_RANDOM) // dhriti: The mode is not RANDOM
			{
				choice = rand() % n->ample_set.size();
			}
			/* == fprs start == */
      while(choice < (int)n->ample_set.size())
      {
        cbl = n->ample_set.at(choice);
        if(!matches(cbl, changedConditionals, n))
        {
          //std::cout << "\n\n\neliminating something from ample_set\n\n\n";
          choice++;
        }
        else
          break;
      }
      if(choice == (int)n->ample_set.size()) // This means that we do not have any eligible choice available. There is no non-redundant send left to match with this receive
      {
        //std::cout << "\n*** TERMINATING THIS INTERLEAVING ***" << std::endl;
        return 1;
      }
      // Saving the choice in choiceInLastInterleaving (if this is the wildcard receive before the 'if')
      std::vector<expression>::iterator it = changedConditionals.begin();
      std::vector<expression>::iterator it2 = changedConditionals.end();
      if(it!=it2)
      {
        expression ex = *it;
        std::string varName = ex.varName;
        int pidConditional = varName.at(2) - '0';
        int indexConditional = varName.at(4) - '0';
        //std::cout << pidConditional << " " << indexConditional << "\n";
        if(n->isWildcardNode() && n->wildcard._pid==pidConditional && n->wildcard._index==indexConditional)
        {
          choiceInLastInterleaving = choice;
        }
      }
      
  	} 
		/* == fprs end == */
    //------------------Till this point we have selected the choice amonst ample sets available----------------------------------------------
		
    /*
		* sriram modification
		* cbl.front -> if sendtype -> update sendcount
		* cbl.back -> if recv (check if func_id is RECV or IRECV) -> update recv or recv(*) counts
		*/

		/* size = 2 => cbl.front is send
		*             cbl.back is recv
		*/

		n->curr_match_set = cbl;

		if(cbl.size() == 2) 
		{
			// check if the front is send type
			//Envelope *_esrc = n->GetTransition(cbl.front())->GetEnvelope();
			//Envelope *_edst = n->GetTransition(cbl.back())->GetEnvelope();
			//int _psrc = cbl.front()._pid;
			//int _pdst = cbl.back()._pid;

			// if(_esrc->isSendType()) {
			// 	if(_edst->func_id == IRECV || _edst->func_id == RECV) {
			// 	  if(_edst->src_wildcard)
			// 	    n->_countracker.updateCount(_psrc, _pdst, true);            
			// 	  else 
			// 	    n->_countracker.updateCount(_psrc, _pdst, false);
			// 	}
			// }

			// else if (_esrc->func_id == BARRIER)
			//   n->curr_match_set = cbl;
			//else
			//  n->curr_match_set = cbl;
		} 

		#ifdef FIB
		// if (Scheduler::_fib) { // [svs] -- changes done for predictive analysis
		al_curr.push_back(cbl);
		//}
		#endif
		if (depth >= (int)_slist.size ()-1) 
		{
			//            if (Scheduler::_explore_mode == EXP_MODE_ALL && 
			//                GetCurrNode()->ample_set.size() > 1) {
			Node *n_ = new Node(*n);
      
      n_->wildcard._pid = n->wildcard._pid; // dhriti
      n_->wildcard._index = 0; // dhriti

			_slist.push_back(n_);
			n->type = GENERAL_NODE;
			if(cbl.size() == 2) 
			{
				env = n->GetTransition(cbl.back())->GetEnvelope();
				if ((env->func_id == RECV || env->func_id == IRECV) && env->src == WILDCARD) 
				{
					have_wildcard = true;
					n->type = WILDCARD_RECV_NODE;
					n->wildcard._pid = env->id;
					n->wildcard._index = env->index;
					//n_->deepCopy();
					//                } else if ((env->func_id == PROBE || env->func_id == IPROBE) && env->src == WILDCARD) {
				} 
				else if (env->func_id == PROBE && env->src == WILDCARD) 
				{
					have_wildcard = true;
					n->type = WILDCARD_PROBE_NODE;
					n->wildcard._pid = env->id;
					n->wildcard._index = env->index;
					//n_->deepCopy();
				}
			}
		} 
		else 
		{
			if(n->isWildcardNode()) 
			{
				env = n->GetTransition(cbl.back())->GetEnvelope();
				n->wildcard._pid = env->id;
				n->wildcard._index = env->index;
			}
		}
		depth++;
		n = GetCurrNode();

		/*
		if(cbl.size() == 2) {
		} else
		n->type = GENERAL_NODE;
		*/

		/* If the match set has a send and receive - the send is the first item, the receive second */
		/* Here we get the source of the send, so that we can rewrite the wildcard */
		int source = cbl.front ()._pid;

		if (n->GetTransition(cbl.front())->GetEnvelope ()->isSendType ()) 
		{
			/* if it's a probe call, we don't issue the send, so no 
			need to update the matching of the send */
			if (n->GetTransition(cbl.back())->GetEnvelope ()->func_id != PROBE &&
			n->GetTransition(cbl.back())->GetEnvelope ()->func_id != IPROBE) 
			{
				n->GetTransition(cbl.front())->set_curr_matching(cbl.back());
			} else 
			{
				probe_flag = true;
				Scheduler::_probed = true;
			}
			n->GetTransition(cbl.back())->set_curr_matching(cbl.front());
		}

		//Check if the data types match CGD
		if (n->GetTransition(cbl.front())->GetEnvelope()->isSendType ())
		{
			int currType = n->GetTransition(cbl.front())->GetEnvelope()->data_type;
			int matchType = n->GetTransition(cbl.back())->GetEnvelope()->data_type;
			if(currType == matchType)
			{
				n->GetTransition(cbl.front())->GetEnvelope()->typesMatch = true;
				n->GetTransition(cbl.back())->GetEnvelope()->typesMatch = true;
			}
			else
			{
				n->GetTransition(cbl.front())->GetEnvelope()->typesMatch = false;
				n->GetTransition(cbl.back())->GetEnvelope()->typesMatch = false;

				Envelope *env = n->GetTransition(cbl.front())->GetEnvelope ();
				Scheduler::_mismatchLog << env->filename << " "
				<< env->linenumber << std::endl;
				env = n->GetTransition(cbl.back())->GetEnvelope ();
				Scheduler::_mismatchLog << env->filename << " " << env->linenumber << std::endl;
			}
		}
		else if(n->GetTransition(cbl.front())->GetEnvelope()->data_type == -1)
		{
			Envelope *env = n->GetTransition(cbl.front())->GetEnvelope();
			Scheduler::_mismatchLog << env->filename << " "	<< env->linenumber << std::endl;
		}

		std::list <CB>::iterator iter;
		std::list <CB>::iterator iter_end = cbl.end();
		bool test_iprobe_calls_only = true;
		for (iter = cbl.begin (); iter != iter_end; iter++) 
		{
			env = n->GetTransition (*iter)->GetEnvelope ();
			if (env->func_id != IPROBE && env->func_id != TEST &&
			env->func_id != TESTALL && env->func_id != TESTANY) 
			{
				test_iprobe_calls_only = false;
				break;
			}
		}

		for (iter = cbl.begin (); iter != iter_end; iter++) 
		{
			Transition *tn = n->GetTransition (*iter);
			env = tn->GetEnvelope ();

			if (test_iprobe_calls_only) source = -1;

			/* If the other call is a probe, we only issue the send
			* but leave the send un-matched! */
			if (!probe_flag || !env->isSendType()) 
			{
				is_matched[iter->_pid][iter->_index] = true;
				n->_tlist[iter->_pid]->_ulist.remove(iter->_index);
			}

			/* increase the last matched transition to the latest matched transition */

			int temp_last_matched = last_matched[iter->_pid];
			while (temp_last_matched < (int) (n->_tlist[iter->_pid]->_tlist.size()-1) 
							&& is_matched[iter->_pid][temp_last_matched + 1]) 
			{
				temp_last_matched ++;
			}
			last_matched[iter->_pid] = temp_last_matched;

			/* if the call has been issued earlier, do not issue it again */
			/* What happens here: If we issue a pair of Send/Probe, do
			* not allow the scheduler to read the next envelope of
			* the Send process. If we issue a pair of Send/Recv, allow the 
			* scheduler to read the next envelope of the send process */

			if (is_issued[iter->_pid][iter->_index]) 
			{
				if (env->isSendType() && env->isBlockingType() && !Scheduler::_probed) 
				{
					Scheduler::_runQ[iter->_pid]->_read_next_env = true;
				}
				continue;
			}
			std::ostringstream oss;

			oss << goahead << " " << env->index << " " << source;

			env->Issued ();
			if (env->func_id == COMM_CREATE || env->func_id == COMM_DUP	|| env->func_id == CART_CREATE || env->func_id == COMM_SPLIT) 
			{
				oss << " " << env->comm_id;
			} 
			else 
			{
				oss << " " << 0;
			}

			if (env-> isBlockingType ()) 
			{
				oss << " " << 2 ;
				l.push_back (iter->_pid);
			} 
			else 
			{
				oss << " " << 1 ;
			}
			//std::cout <<"From scheduler to rank " << (*iter)._pid << " " << oss.str() << "\n";
      //fflush(stdout);
			sock.Send (iter->_pid, oss.str());
			is_issued[iter->_pid][iter->_index]= true;
		}
	} 
	else 
	{
		sock.ExitMpiProcessAndWait (true);

		if (!Scheduler::_quiet && !Scheduler::_limit_output) 
		{
			std::cout << "-----------------------------------------" << std::endl;
			if (n->getTotalMpiCalls() < CONSOLE_OUTPUT_THRESHOLD)
				std::cout << *n << std::endl;

			std::cout << "No matching MPI call found!" << std::endl;
			std::cout << "Detected a DEADLOCK in interleaving " << 
			Scheduler::interleavings << std::endl;
			//          std::cout << "Killing program " << pname << std::endl;

			std::cout << "-----------------------------------------" << std::endl;
		}
		Scheduler::_just_dead_lock = true;
		//exit (1);
		return 1;
	}
	DR( std::cout << " [CHECK end]\n"; )
	return 0;
}

/* This looks complicated, but the algorithm is as follows: Go through
 * all the interleaving nodes, delete the first match-set of each node
 * This is what we've covered so far.  If the node is empty after
 * this, simply remove that node.  If there is one node that has more
 * than one match sets, which means we have to cover another interleaving, return true
 */
bool ITree::NextInterleaving (std::vector<expression> changedConditionals) 
{
  ClearInterCB();

  int i = depth+1;
  assert(i == (int) _slist.size());

  //#ifdef FIB
  int oldDepth = depth;
  last_node = GetCurrNode ();
  //last_node->deepCopy();
  //#endif

  /* == fprs begin == */ 
  if (Scheduler::_fprs) 
  {
    if (Scheduler::_explore_mode == EXP_MODE_RANDOM) return true;
    else if (Scheduler::_explore_mode == EXP_MODE_LEFT_MOST) return false;
  }   
  else 
  {
    if (Scheduler::_explore_mode == EXP_MODE_RANDOM || Scheduler::_explore_mode == EXP_MODE_LEFT_MOST)
      return false;
  }
  /* == fprs end == */

	/*
  for(int j=0; j<i; ++j)
  {
    Node *n = _slist[j];
    //std::cout << "\nIn nextInterleaving for loop: " << n->wildcard._pid << " " << n->wildcard._index << "\n";
  }
	*/

  while (i-- > 0) 
  {
    /* erase the first match-set */
    Node *n = _slist[i];

    std::vector<expression>::iterator it = changedConditionals.begin();
    expression ex = *it;
    std::string varName = ex.varName;
    char* cstr = new char[varName.length() + 1];
    strcpy(cstr, varName.c_str());
    char* token;
    token = strtok(cstr, "_");
    token = strtok(NULL, "_");
    int pidConditional = atoi(token);
    token = strtok(NULL, "_");
    int indexConditional = atoi(token);

    if(n->wildcard._pid==pidConditional && n->wildcard._index==indexConditional)
    {
      if (!( n->ample_set.empty() && ((!n->isWildcardNode() || (n->isWildcardNode() && aux_coenabled_sends[n->wildcard].empty()))))) 
      {
        //std::cout << "\nIn nextInterleaving: " << pidConditional << " " << indexConditional << "\n";
        //fflush(stdout);
        #ifdef CONFIG_BOUNDED_MIXING
        if (n->isWildcardNode() && Scheduler::_bound && !n->expand && expanded < Scheduler::_bound) 
        {
          n->expand = true;
          expanded++;
        }
        if(Scheduler::_bound && n->expand || !Scheduler::_bound) 
        {
          //std::cerr << "_bound=" << Scheduler::_bound << " n->expand=" << n->expand << " expanded=" << expanded << "\n";
          #endif
          //#ifdef FIB
          delete last_node;
          //#endif
          /* if the ample set is not empty - extra interleaving needed */

          // std::cout << "******************\n";
          // print(_slist[i-1]->ample_set);
          // std::cout << "******************\n";
          // std::cerr << n->ample_set.empty() << std::endl;
          return true;
          #ifdef CONFIG_BOUNDED_MIXING
        }
        #endif
      }
    }

    /* if ample_set is empty, delete that node */
    if(n->isWildcardNode()) 
    {
      #ifdef CONFIG_BOUNDED_MIXING
      if(Scheduler::_bound && n->expand) 
      {
        expanded--;
      }
      #endif
      matched_sends[n->wildcard].clear();
    }
    #ifndef FIB
    delete *(_slist.end()-1);
    //delete n;
    #else
    if (depth != oldDepth) 
    {
      delete *(_slist.end()-1);
    }
    #endif
    _slist.pop_back ();
    depth--;
    //std::cout << "\nPopping from slist: " << depth << "\n";
    fflush(stdout);
    i = (int)_slist.size();
  }
  //DR (std::cout << " [NextInterleaving end]\n"; )
  return false;
}

/* This method is called at the end of each interleaving
 * The following task is accomplished:
 * Add the interCB edges to all the nodes
 * Find out the sends for which buffering would make it illegible 
 * to match with some wildcard receives.
 * Side effects: interCB edges are added, ample set will be changed 
 * if the sends could become a potential match due to buffering
 */
void ITree::ProcessInterleaving () {

  /* Only do this if we have seen a wildcard receive before
   * Otherwise we'll just kill performance!!! */
  if (have_wildcard) {
    /* Go back to each transition and add interCB edges 
     * Note that we could not do this on the fly as we had not
     * seen all the possible matchings */
    AddInterCB ();

    /* Find all the sends that could have been co-enabled by buffering */
    FindCoEnabledSends ();
  }
}

/* Precond: c has to be of type Wait or Test 
 * Return a CB edge that points to a send 
 * if this Wait/Waitall is for a send request, return NULL otherwise. 
 * Whichever method that calls this will have 
 * to make sure to clean up this returned edge to avoid memory leak */
bool ITree::findSendOfThisWait (CB &res, CB &c) {
  Envelope *e = GetCurrNode ()->GetTransition (c)->GetEnvelope ();
  if (e->isWaitorTestType ()) {
    for (unsigned int i = 0 ; i < e->req_procs.size (); i++) {
      CB wc (c._pid, e->req_procs[i]);
      if (GetCurrNode()->GetTransition(wc)->GetEnvelope()->isSendType()) {
	res = wc;
	return true;
      }
    }
  }    
  return false;
}

/* going through all the tlist, return the max number of MPI calls
 * Side effects: None */
size_t ITree::getMaxTlistSize() {
  size_t ret = GetCurrNode()->_tlist[0]->_tlist.size();
  for (size_t i = 1; i < GetCurrNode()->_tlist.size(); i++) {
    if (ret < GetCurrNode()->_tlist[i]->_tlist.size())
      ret = GetCurrNode()->_tlist[i]->_tlist.size();
  }
  return ret;
}

size_t ITree::getTCount() {
  size_t ret = GetCurrNode()->_tlist[0]->_tlist.size();
  for (size_t i = 1; i < GetCurrNode()->_tlist.size(); i++) {
    ret += GetCurrNode()->_tlist[i]->_tlist.size();
  }
  return ret;
}

/* Return true if there exists a path from src to destination
 * We do not consider the wait node's since the sends are buffered
 * Side effects: visited array is used but not cleaned up.
 * The expectation is that whichever method that uses visited will
 * have to initialized it.
 * We're using BFS here, because DFS would result in a very deep
 * recursion.
 */
bool ITree::FindNonSendWaitPath (bool ** visited, CB &src, CB &dest) {
	/*
  DR( 
     std::cout << "  [FindNonSendWaitPath]\n"
     << "\t\tsrc=" << src << " dest=" << dest << "\n";
      )
			*/

    if (src == dest)
      return true;
  if (src._pid == dest._pid && src._index > dest._index) 
    return false;

  std::vector <CB>::iterator iter;
  std::vector <CB>::iterator iter_end;
  std::queue <CB> worklist;
  CB current(0,0);

  /* initialize the visited array */
  for (int i = 0; i < GetCurrNode()->NumProcs(); i++) {
    for (size_t j = 0; j < getMaxTlistSize(); j ++)
      visited[i][j] = false;
  }


  /* doing a BFS search */
  worklist.push(src);
  //DR( std::cout << "\tworklist "; )
    while (!worklist.empty()) {
      CB c;
      current = worklist.front();
      //DR( std::cout << current << " "; )
        worklist.pop();
      if (current == dest) {
	//DR( std::cout << "\n"; )
	  return true;
      }
      visited[current._pid][current._index]= true;
      /* now add all the intraCB edges of current into the worklist */
      Transition t = *GetCurrNode()->GetTransition(current);
      iter = t.get_intra_cb().begin();
      iter_end = t.get_intra_cb().end();
      for (; iter != iter_end; iter++) {
	if (findSendOfThisWait(c, *iter)) {
	  visited[iter->_pid][iter->_index] = true;
	}
	if (!visited[iter->_pid][iter->_index]) {
	  worklist.push(*iter);
	  visited[iter->_pid][iter->_index] = true;
                                
	}
      }
      /* now add all the interCB edges of current into the worklist */
      iter = t.get_inter_cb().begin();
      iter_end = t.get_inter_cb().end();
      for (; iter != iter_end; iter++) {
	if (findSendOfThisWait(c, *iter)) {
	  //delete c;
	  visited[iter->_pid][iter->_index] = true;
	}

	if (!visited[iter->_pid][iter->_index]) {
	  worklist.push(*iter);
	  visited[iter->_pid][iter->_index] = true;
                                
	}
      }
    }
  //DR( std::cout << "\n"; )
    
    return false;
    
}

void ITree::FindCoEnabledSends () {
  //DR( std::cout << " [FindCoEnabledSends begin]\n"; )
    /*
      int ** transition_map = new int* [NumProcs()];
      for (int i = 0; i < NumProcs(); i++) {
      transition_map[i]= new int [GetCurrNode()->_tlist[i].size()];
      }
    */
    std::vector <Node *>::iterator iter;
  std::vector <Node *>::iterator iter_end = _slist.end ();

  /* keep visited CB edges here 
   * This array will be used by FindNonSenWait 
   * We don't want to alloc/re-alloc too many times
   */
  bool** visited = new bool* [GetCurrNode()->NumProcs()];
  for (int i = 0; i < GetCurrNode()->NumProcs(); i++) {
    visited[i] = new bool [getMaxTlistSize()];
  }

  for (iter = _slist.begin (); iter != iter_end-1; iter++) {
    Node *n = (*iter);
        
    /* Only care about nodes with wild card receive */
    if (!n->isWildcardNode()) continue;
    //[grz]        if(!n->GetTransition(n->wildcard)->GetEnvelope()->func_id == IPROBE) continue;

    CB send = n->ample_set.front().front();
    //[grz]        if(!n->GetTransition(send)->GetEnvelope()->isSendType()) continue;
    //DR( std::cout << "\tsend: " << send << "\trecv: " << n->wildcard << "\n"; )

      std::set <CB> *prev_sends = &matched_sends[n->wildcard];
    prev_sends->insert(send);
    std::map <CB, std::list <CB> >::iterator csi = aux_coenabled_sends.find(n->wildcard);
    if(csi != aux_coenabled_sends.end())
      csi->second.remove(send);

    Envelope *renv = GetCurrNode ()->GetTransition ((n->wildcard))->GetEnvelope ();
    for (int i = 0 ; i < n->NumProcs (); i++) {
      if (i == n->wildcard._pid) {
	continue;
      }
      size_t tlist_size = GetCurrNode()->_tlist[i]->_tlist.size();
      for (unsigned int j = 0 ; j < tlist_size; j++) {
	Envelope *e = GetCurrNode ()->_tlist[i]->_tlist[j].GetEnvelope ();
	if (! e->isSendType ()) {
	  continue;
	}
	if (e->dest == n->wildcard._pid && 
	    (e->stag == renv->rtag || renv->rtag == WILDCARD) && 
	    e->comm == renv->comm) {
	  CB c(i, e->index);
	  std::list <int>::iterator it;
	  std::list <int>::iterator it_end = n->enabled_transitions[i].end();
	  bool sendcoenabled = false;
	  for (it = n->enabled_transitions[i].begin (); it != it_end; it++) {
	    if (*it >= e->index )  {
	      sendcoenabled = true;
	      break;
	    }
	  }
	  if (!sendcoenabled) {

	      bool overtaking = (c._pid ==
				 GetCurrNode()->GetTransition(n->wildcard)->
				 get_curr_matching()._pid);
	      if (overtaking || 
		  FindNonSendWaitPath (visited, n->wildcard, c)) {

		  break;
	      } else {

		  if(prev_sends->find(c) == prev_sends->end()) {
		    n->has_aux_coenabled_sends = true;
		    std::list <CB> *sends = &aux_coenabled_sends[n->wildcard];
		    std::list <CB>::iterator si = sends->begin();
		    std::list <CB>::iterator sie = sends->end();
		    while(si != sie && *si < c) si++;
		    if(si == sie || *si != c) sends->insert(si, c);
		  }
	      }
	  }
	}   
      }
    }
  }
  /* freeing memory */
  for (int i = 0; i < GetCurrNode()->NumProcs(); i++) {
    delete [] visited[i];
  }
  delete [] visited;
  //DR( std::cout << " [FindCoEnabledSends end]\n"; )
    }

void ITree::ClearInterCB () {
  Node *n = GetCurrNode ();

  for (int i = 0 ; i < n->NumProcs (); i++) {
    for (unsigned int j = 0;  j < n->_tlist[i]->_tlist.size(); j++) {
      std::vector<CB>::iterator iter;
      n->_tlist[i]->_tlist[j].mod_inter_cb().clear();
    }
  }
}

void ITree::AddInterCB () {
  std::vector <Node *>::iterator iter;
  std::vector <Node *>::iterator iter_end = _slist.end ();

  for (iter = _slist.begin(); iter != iter_end; iter++) {
    if ((*iter)->ample_set.size() == 0) {
      continue;
    }
    std::list <CB> mset = (*iter)->ample_set.front();     
        
    if (mset.size() <= 1) {
      continue;
    }
    if (mset.size() == 2) {
      Envelope *e = (*iter)->GetTransition (mset.back())->GetEnvelope ();
      if ((e->func_id == IRECV || e->func_id == RECV) && 
	  e->src == WILDCARD) {
	GetCurrNode()->GetTransition(mset.back())->AddInterCB(mset.front());
	//          std::cout <<"Added edges for " << *(mset.back()) << " and " << *(mset.front()) << " \n";
	continue;
      } 
    } 
    std::list <CB>::iterator it1;
    std::list <CB>::iterator it_end = mset.end();
    bool only_sends = true;
    for (it1 = mset.begin(); it1 != it_end ; it1++) {
      if (! (*iter)->GetTransition(*it1)->GetEnvelope()->isSendType()) 
	only_sends = false;
    }
    if (only_sends)
      continue;

    for (it1 = mset.begin(); it1 != it_end ; it1++) {
      std::list <CB>::iterator it2;
      std::list <CB>::iterator it2_end = mset.end();
      for (it2 = mset.begin(); it2 != it2_end ; it2++) {
	if (it2 != it1) {
	  GetCurrNode()->GetTransition(*it1)->AddInterCB(*it2);
	  //  std::cout <<"Added edges for " << *(*it1) << " and " << *(*it2) << " \n";

	}   
      }
    }
  } 
  //    std::cout << "Leaving AddInterCB\n";
}

bool ITree:: checkIfSATAnalysisRequired()
{
  /* see if the program has any wildcard
     if yes -- then check if the ample-set at any
     node is greater > 2
  */
  if(have_wildcard){
    return true;
  }
  //std::cout << "SAT analysis is NOT REQUIRED: no wildcards" <<std::endl;
  return false;
}

void ITree::resetDepth () {
  depth = 0;
  for (size_t i = 0; i < _slist[depth]->_tlist.size(); i ++) {
    _slist[depth]->_tlist[i]->last_matched = -1;
  }
}

#ifdef FIB
void Node::updateInterHB (std::list <CB> &l) {
  std::list <CB>::iterator iter;
  std::list <CB>::iterator iter_end = l.end ();

  for (iter = l.begin (); iter != iter_end; iter++) {
    std::list <CB>::iterator iter1;
    std::list <CB>::iterator iter1_end = l.end ();

    for (iter1 = l.begin (); iter1 != iter1_end; iter1++) {
      std::vector <CB>::iterator iter2, iter2_end;

      if (*iter == *iter1) {
	continue;
      }
      Transition t = _tlist[iter1->_pid]->_tlist[iter1->_index];
            
      /* Intra CB's */
      iter2_end = t.get_intra_cb().end ();
      for (iter2 = t.get_intra_cb().begin (); iter2 != iter2_end; iter2++) {
	_tlist[iter->_pid]->_tlist[iter->_index].
	  //AddInterCB (GetTransition(*(*iter2)));
	  AddInterCB(*iter2);
      }

      /* Inter CB's */
      iter2_end = t.get_inter_cb().end ();
      for (iter2 = t.get_inter_cb().begin (); iter2 != iter2_end; iter2++) {
	_tlist[iter->_pid]->_tlist[iter->_index].
	  AddInterCB(*iter2);
      }
    }
  }    
}

void ITree::findInterCB () {
  std::vector<std::list<CB> >::reverse_iterator iter;
  std::vector<std::list<CB> >::reverse_iterator iter_end = al_curr.rend ();
  Node *n = GetCurrNode();

  for (iter = al_curr.rbegin (); iter != iter_end; iter++) {
    n->updateInterHB (*iter);
  }
}

bool ITree::visited (CB &c, std::vector<CB> &l) {
  std::vector<CB>::iterator iter;
  std::vector<CB>::iterator iter_end = l.end ();

  for (iter = l.begin(); iter != iter_end; iter++) {
    if (c == *iter) {
      return true;
    }
  }
  return false;
}

bool ITree::findAdjacent (CB &res, CB &c, std::vector<CB> &l) {
  Transition t = *GetCurrNode()->GetTransition(c);
  std::vector<CB>::iterator iter;
  std::vector<CB>::iterator iter_end = t.get_intra_cb().end ();

  for (iter = t.get_intra_cb().begin(); iter != iter_end; iter++) {
    if (!visited(*iter,l) && iter->_pid != -1) {
      res = *iter;
      return true;
    }
  }

  iter_end = t.get_inter_cb().end();
  for (iter = t.get_inter_cb().begin(); iter != iter_end; iter++) {
    if (!visited(*iter,l) && (iter->_pid != -1)) {
      res = *iter;
      return true;
    }
  }
  return false;
}

void ITree::checkBarrier(CB &c, CB &c1, std::vector<CB> &s)
{
  Node *n = GetCurrNode();
  bool check_flag = false;
  std::vector<CB>::iterator iter;
  std::vector<CB>::iterator iter_end = s.end ();

  if (n->GetTransition(c)->GetEnvelope()->isSendType() &&
      n->GetTransition(c1)->GetEnvelope()->isRecvType() &&
      n->GetTransition(c1)->GetEnvelope()->src == WILDCARD &&
      n->GetTransition(c)->GetEnvelope()->dest == 
      n->GetTransition(c1)->GetEnvelope()->id) {

    // check stack
    for(iter = s.begin (); iter != iter_end; iter++)
      {
	if(*iter == c1) {
	  check_flag = false;
	  break;
	}
	if(*iter == c) {
	  check_flag = true;
	}
	//if(n->GetTransition(**iter)->GetEnvelope()->func_id == BARRIER &&
	//    !visited(*iter, blist) && check_flag) {
	//    // add a check for the only barrier
	//    // br = *iter;
	//    std::cout<< "barrier at line " << n->GetTransition(**iter)->GetEnvelope()->linenumber << " got added\n";
	//}
	blist.push_back(*iter);
      }
    // if(br != NULL){
    //     //std::cout<<"added the barrier :"<<*(n->GetTransition(*br))<<"\n";
    //     blist.push_back(br);
    //       }
  }
  else if(n->GetTransition(c)->GetEnvelope()->isRecvType() &&
	  n->GetTransition(c1)->GetEnvelope()->isSendType() &&
	  n->GetTransition(c)->GetEnvelope()->src == WILDCARD &&
	  n->GetTransition(c1)->GetEnvelope()->dest == 
	  n->GetTransition(c)->GetEnvelope()->id) {
    // check stack
    for(iter = s.begin(); iter != iter_end; iter++)
      {
	if (*iter == c1) {
	  check_flag = false;
	  break;
	}
	if (*iter == c) {
	  check_flag = true;
	}
	if (n->GetTransition(*iter)->GetEnvelope()->func_id == BARRIER &&
	    !visited(*iter, blist) && check_flag) {
	  // add a check for the only barrier
	  // br = *iter;
	  blist.push_back(*iter);
	}
      }
  }
}

void ITree::Dfs (CB &c) {
  std::vector<CB> s;
  std::vector<CB> visited;

  s.push_back(c);
  visited.push_back(c);
  while (!s.empty()) {
    CB c1 = s.back();
    //std::cout << "looking at transition: "<<*(n->GetTransition(*c1))<<"\n";
    checkBarrier(c, c1, s);

    CB adj;
    if(findAdjacent(adj, c1, visited)) {
      //std::cout<<"visiting adj: "<<*(n->GetTransition(*adj)) << "\n";
      visited.push_back(adj);
      s.push_back(adj);
    } else {
      s.pop_back();
    }
  }
}

void ITree::forDfs (std::list<CB> &l) {
  std::list<CB>::iterator iter;
  std::list<CB>::iterator iter_end = l.end ();

  for (iter = l.begin(); iter != iter_end; iter++) {
    Transition t = *GetCurrNode()->GetTransition(*iter);
    if (t.GetEnvelope()->isRecvType() && t.GetEnvelope()->src == WILDCARD)
      {
	Dfs(*iter);
      }
  }
}

void ITree::getBarrierList () {
  bool flag = false;
  std::list<CB> temp;
  std::vector<CB>::iterator iter, iter_end;
  std::vector<std::list<CB> >::iterator iter1, iter1_end;
  std::list<CB>::iterator iter2, iter2_end;

  iter_end = blist.end ();
  for (iter = blist.begin (); iter != iter_end; iter++) {
        
    iter1_end = al_curr.end ();
    for (iter1 = al_curr.begin (); iter1 != iter1_end; iter1++) {
            
      iter2_end = iter1->end ();
      for (iter2 = iter1->begin (); iter2 != iter2_end; iter2++) {
	if(*iter2 == *iter) {
	  flag = true;
	  break;
	}
      }

      if(flag)
	{
	  for (iter2 = iter1->begin (); iter2 != iter2_end; iter2++) {
	    //  if(!visited(*iter,blist) && 
	    //              (n->GetTransition(*c)->GetEnvelope()->linenumber !=
	    //               n->GetTransition(**iter)->GetEnvelope()->linenumber))
	    temp.push_back(*iter2);
	  }
	  break;
	}
    }
    flag = false;
  }
  if(!present(temp, FRBlist) && (temp.size() != 0))
    {
      //std::cout<<"addition to the FRBlist: temp size "<<temp.size()<<"\n";
      FRBlist.push_back(temp);
    }
  //  for(iter1 = temp.begin();
  //       iter1 != temp.end();
  //       iter1 ++)
  //     blist.push_back(*iter1);
}

bool ITree::present(std::list<CB> &l, std::vector<std::list<CB> > &b) {
  std::vector<std::list<CB> >::iterator iter, iter_end;
  std::list<CB>::iterator iter2, iter2_end;

  if (l.empty ()) {
    return false;
  }
  CB c = l.front();
  // std::cout<< " I am in present function \n";
  iter_end = b.end ();
  for (iter = b.begin (); iter != iter_end; iter++) {
    iter2_end = iter->end ();
    for (iter2 = iter->begin (); iter2 != iter2_end; iter2++) {
      if(c == *iter2) {
	return true;
      }
    }
  }
  return false;
}

void ITree::FindRelBarriers()
{
  std::vector<std::list<CB> >::iterator iter;
  std::vector<std::list<CB> >::iterator iter_end = al_curr.end ();
    
  // Path search from every Send and Receive*
  for (iter = al_curr.begin (); iter != iter_end; iter++)
    {
      forDfs(*iter);
    }
  // std::cout<< "DFS done successfully\n";
  //  if(!blist.empty())
  //     {
  //       std::cout << "Barrier list not empty \n";
  //       Print();
  //     }
}

//CGD
void ITree::printTypeMismatches()
{
  if(Scheduler::_mismatchLog.str().length() != 0){
    Scheduler::_logfile << "[TYPEMISMATCH]" << std::endl;
    Scheduler::_logfile << Scheduler::_mismatchLog.str();
    std::cout << "-----------------------------------------\n";
    std::cout << "WARNING - The following calls don't share the same data type with their matching calls:\n";
    std::cout << Scheduler::_mismatchLog.str();
  }
}



void ITree::getFIB()
{
  std::vector<std::list<CB> >::iterator iter;
  std::vector<std::list<CB> >::iterator iter_end = al_curr.end ();

  for (iter = al_curr.begin (); iter != iter_end; iter++) {
    CB c = iter->front ();
    if (last_node->GetTransition(c)->GetEnvelope()->func_id == BARRIER) {
      if (!present(*iter, FRBlist)) {
	FIBlist.push_back(*iter);
      }
    }
  }
}

void ITree::printRelBarriers()
{
  std::vector<std::list<CB> >::iterator iter, iter_end;
  std::list<CB>::iterator iter2, iter2_end;

  getBarrierList();
    
  getFIB();
}

#endif
