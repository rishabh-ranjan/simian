#include "Encoding.hpp"
//#include "solver-src/sat/cardinality.h"

std::stringstream formula;

Encoding::Encoding(ITree *it, M *m/*, propt *_slv*/)
{
  _m = m;
  _it = it;
  last_node = it->_slist[it->_slist.size() -1] ; 
  traceSize = it->_slist.size()-1;
  _deadlock_found = false;
  //slv = _slv;
  //bvUtils = new bv_utilst(*slv);

  // instantiating two variables in the solver memory
  //one = slv->new_variable();
  //zero = slv->new_variable();

  //slv->constraintStream << "one = " << one.get() <<std::endl; 
  //xslv->constraintStream << "zero = " << zero.get() <<std::endl; 
  // setting one to true and zero to false

  //slv->l_set_to(one, true);
  //slv->l_set_to(zero, false);
 
}

std::string Encoding::getLitName (expr a)
{
  std::stringstream res;
  expr one = c.bool_val(true);
  expr zero = c.bool_val(false);
  if( a == one)
    res << "1";
  else if (a == zero)
    res << "0";
  else{
  StrIntPair MnP= lit2sym.find(a)->second;
  res << "X" << MnP.first << "," << MnP.second;
  }
  return res.str();
}

bool isOneOf(int pid, std::vector<std::pair<int, int> > ranksIndices)
{
  std::vector<std::pair<int, int> >::iterator itBegin = ranksIndices.begin();
  std::vector<std::pair<int, int> >::iterator itEnd = ranksIndices.end();
  while(itBegin != itEnd)
  { 
    if((*itBegin).first == pid)
      return true;
    itBegin++;
  }
  return false;
}

int nearestEvent(Envelope* env, std::vector<std::pair<int, int> > r)
{
  int index = env->index;
  int nearest = -1;
  std::vector<std::pair<int, int> >::iterator itBegin = r.begin();
  std::vector<std::pair<int, int> >::iterator itEnd = r.end();
  while(itBegin != itEnd)
  {
    if((*itBegin).second <= index)
    {
      if((*itBegin).second > nearest)
      {
        nearest = (*itBegin).second;
      }
    }
    itBegin++;
  }
  return nearest; // return of -1 means there is no 'relevant' R present above this call in the process
}

// --dhriti
bool Encoding::toBeDropped(Envelope* env, std::vector<std::pair<int, int> > lineNumbers, std::vector<std::pair<int, int> > ranksIndices, std::vector<std::pair<int, int> > allRanksIndices)
{
  if(lineNumbers.size() == 0)
    return false;
  else
  {
    std::vector<std::pair<int, int> >::iterator it = lineNumbers.begin();
    int line = env->linenumber;
    while(it != lineNumbers.end())
    {
      int start = it->first;
      //int end = it->second;
      int a = nearestEvent(env, ranksIndices);
      int b = nearestEvent(env, allRanksIndices);

      if(line>=start /*&& line<=end)*/ && isOneOf(env->id, ranksIndices) && a == b) // Dropping all the match-pairs after the recieve whose data is decoded in an if-statement
        return true;
      it++;
    }
    return false;
  }
}

// -- dhriti
void Encoding::createEset(std::vector<std::pair<int, int> > lineNumbers, std::vector<std::pair<int, int> > ranksIndices, std::vector<std::pair<int, int> > allRanksIndices)
{
  Eset.clear();
  Node* curr = _it->GetCurrNode ();
  for(int i=0; i<curr->_tlist.size(); ++i) // for all processes
  {
    for(int j=0,k=1; k<curr->_tlist[i]->_tlist.size(); ++j,++k) // for all transitions in a process
    {
      Envelope* e = curr->_tlist[i]->_tlist[j].GetEnvelope();
      Envelope* f = curr->_tlist[i]->_tlist[k].GetEnvelope();
      if(e->func_id == IRECV && f->func_id == IRECV && !e->src_wildcard && f->src_wildcard)
      {
        if(!toBeDropped(e, lineNumbers, ranksIndices, allRanksIndices) && !toBeDropped(f, lineNumbers, ranksIndices, allRanksIndices))
        {
          std::cout << "\n!!!!!!!Node: " << e->func << " " << f->func << " !!!!!!!\n";
          Eset.push_back(std::pair<Envelope*, Envelope*>(e, f));
        }
      }
    }
  }
  //std::cout << "\nEset size: " << Eset.size() << "\n";

  /*for(int i =0 ; i < _it->_slist.size()-1; i++) {
    //CB fr = _it->_slist[i]->curr_match_set.back();
    std::cout << "\nCurrent match set: " << _it->_slist[i]->curr_match_set.size() << "\n";
    fflush(stdout);
  }*/

  Node *ncurr;
  std::vector<Node*>::iterator _slist_it, _slist_it_end;
  _slist_it_end = _it->_slist.end();

  // iterate over the state vector for the current trace
  for(_slist_it = _it->_slist.begin(); _slist_it != _slist_it_end; _slist_it++) 
  {
    ncurr = *(_slist_it);

    Envelope *send = ncurr->GetTransition(ncurr->curr_match_set.front())->GetEnvelope();
    Envelope *recv = ncurr->GetTransition(ncurr->curr_match_set.back())->GetEnvelope();

    CB cbSend(send->id, send->index);
    CB cbRecv(recv->id, recv->index);
   
		/*
    std::cout << "Printing the immediate ancestors ..." << std::endl;

    std::vector<int> _alist;
    _alist.push_back(ncurr->curr_match_set.back()._index);
    std::set<int> _aset;
    _aset = getAllAncestorList(ncurr, ncurr->curr_match_set.back()); // ncurr->curr_match_set.back()._pid, _alist);
    std::cout << "The ancestor of " << cbSend << "and" << cbRecv << " is: " << "[";
    std::set<int>::iterator sit, sit_end;
    for(sit = _aset.begin(); sit != _aset.end(); sit++) 
    {
      std::cout << *sit << ",";   
    }
    std::cout << "]\n";
		*/
  }
}

// -- dhriti
void Encoding::createMatchSet(std::vector<std::pair<int, int> > lineNumbers, std::vector<std::pair<int, int> > ranksIndices, std::vector<std::pair<int, int> > allRanksIndices)
{
  /* Displaying line numbers
  std::vector<std::pair<int, int> >::iterator it = lineNumbers.begin();
  std::vector<std::pair<int, int> >::iterator it2 = lineNumbers.end();
  while(it != it2)
  {
    std::cout << (*it).first << "\t" << (*it).second << "\n";
    it++;
  }
  std::cout << "\n_slist size: " << _it->_slist.size() << "\n";
  fflush(stdout); */

  Node *ncurr;
  std::vector<Node*>::iterator _slist_it, _slist_it_end;
  _slist_it_end = _it->_slist.end();

  /*for(_slist_it = _it->_slist.begin(); _slist_it != _slist_it_end; _slist_it++) 
  {
    ncurr = *(_slist_it);

    Envelope *send = ncurr->GetTransition(ncurr->curr_match_set.front())->GetEnvelope();
    Envelope *recv = ncurr->GetTransition(ncurr->curr_match_set.back())->GetEnvelope();

    CB cbSend(send->id, send->index);
    CB cbRecv(recv->id, recv->index);
    
    std::cout << "Printing the immediate ancestors ..." << std::endl;

    std::vector<int> _alist;
    _alist.push_back(ncurr->curr_match_set.back()._index);
    std::set<int> _aset;
    _aset = getAllAncestorList(ncurr, ncurr->curr_match_set.front()); // ncurr->curr_match_set.back()._pid, _alist);
    std::cout << "The ancestor of " << cbSend << "and" << cbRecv << " is: " << "[";
    std::set<int>::iterator sit, sit_end;
    for(sit = _aset.begin(); sit != _aset.end(); sit++) 
    {
      std::cout << *sit << ",";   
    }
    std::cout << "]\n"; 
  }*/

  matchSet.clear(); // Everytime this function gets called, new MatchSet has to be created. Older one has to be cleared
  // for non-SR matches
  for(int i =0 ; i < _it->_slist.size()-1; i++){
    CB front = _it->_slist[i]->curr_match_set.front();
    Transition *t = _it->_slist[i]->GetTransition(front);
    Envelope *env = t->GetEnvelope();
    // check whether match is a non send-recv one
    if(env->isCollectiveType() &&  !(env->func_id == FINALIZE) && !toBeDropped(env, lineNumbers, ranksIndices, allRanksIndices)){
      matchSet.insert(&(_it->_slist[i]->curr_match_set));
    }
  }
  
  //std::cout << "\nRanksIndices: " << ranksIndices.size() << " " << allRanksIndices.size() << "\n";
  //fflush(stdout);

  //for SR matches
  _MIterator mit, mitend;
  mitend = _m->_MSet.end();
  for (mit = _m->_MSet.begin(); mit != mitend; mit++){
    assert(!(*mit).second.empty());
    CB A = (*mit).second.front();
    CB B = (*mit).second.back();
    if(lineNumbers.size() == 0)
      matchSet.insert(&((*mit).second));
    else
    {
      Envelope *envA = last_node->GetTransition(A)->GetEnvelope();
      Envelope *envB = last_node->GetTransition(B)->GetEnvelope();

      if(!toBeDropped(envA, lineNumbers, ranksIndices, allRanksIndices) && !toBeDropped(envB, lineNumbers, ranksIndices, allRanksIndices))
      {    
        matchSet.insert(&((*mit).second));
      }
    } 
  }
}

void Encoding::printMatchSet()
{
  formula << "**********MATCHSET*************" << std::endl;
  formula << "MatchSet Size =" << matchSet.size() << std::endl;
  forall_matchSet(mit, matchSet){
    forall_match(lit, (**mit)){
      formula << (*lit);
    }
    formula << std::endl;
  } 
}

bool Encoding::isPresent(MatchPtr m)
{
  MIter mpiter = matchSet.find(m);
  if(mpiter != matchSet.end())
    return true;
  return false;
}

////////////////////////////////////////////////////////////
/////                                                ///////
////        ENCODING 0                               ///////
////////////////////////////////////////////////////////////


// void Encoding0::createMatchLiterals()
// {
//   /* 
//      TRICK: Write to a stringstream: appending it with numbers
//      the way you want and write the contents of a stringstream 
//      back to an integer -- quick hack to just juxtapose numerals.
//    */
  
//   std::stringstream uniquepair;
//   std::string matchNumeral;
  
//   for(int i = 0; i < traceSize-1; i++){
    
//     forall_matchSet(mit, matchSet){
      
//       literalt lt = slv->new_variable();
      
      
//       forall_match(lit, (**mit)){
// 	uniquepair<<(*lit)._pid;
// 	uniquepair<<(*lit)._index;
//       }
//       //      uniquepair << "_" << i; 
//       //uniquepair >> matchNumeral; 
//       matchNumeral = uniquepair.str();

//       // clear out uniquepair
//       uniquepair.str("");
//       uniquepair.clear();      

//       //debug print
//       //std::cout << " =====DEBUG PRINT START: unique symbol=====" <<std::endl; 
//       //std::cout << "uniquepair: " << matchNumeral << ":" << i <<std::endl;
//       //  std::cout << " =====DEBUG PRINT END=====" <<std::endl; 
      
//       //insert in to the map
//       lit2sym.insert(std::pair<literalt, StrIntPair> (lt, StrIntPair(matchNumeral, i)));
//       sym2lit.insert(std::pair<StrIntPair, literalt> (StrIntPair(matchNumeral, i), lt));
//       match2sym.insert(std::pair<MatchPtr, StrIntPair> (*mit, StrIntPair(matchNumeral, i)));
      
//       //debug print
//       //std::cout << " =====DEBUG PRINT START: matchSym Entry=====" <<std::endl; 
//       // std::cout << "match2sym entry: " << (*mit) << ", " << matchNumeral << ":" 
//       // << i <<std::endl;
//       // std::cout << " =====DEBUG PRINT END=====" <<std::endl; 
 
//     }
//   }
// }

// literalt Encoding0::getLiteralFromMatches(MatchPtr mptr, int pos)
// {
//   int  position;
//   std::string matchNumeral;
//   literalt res;

//   std::pair<std::multimap<MatchPtr, StrIntPair>::iterator, 
// 	    std::multimap<MatchPtr, StrIntPair>::iterator > ret; 
//   ret = match2sym.equal_range(mptr);
  
//   for(std::multimap<MatchPtr, StrIntPair>::iterator iter = ret.first;
//       iter != ret.second; iter++){
    
//     matchNumeral = (*iter).second.first;
//     position = (*iter).second.second;
    
//     if(position == pos){

//       // debug print
//       //std::cout << " =====DEBUG PRINT START: literal=====" <<std::endl; 
//       //std::cout << res << std::endl;
//       //std::cout << " =====DEBUG PRINT END: literal=====" <<std::endl; 
//       res = sym2lit.find(StrIntPair(matchNumeral, pos))->second;
//       return res;
//     }
//   }
//   // requires check at callee site whether returned value makes sense
//   return res;
// }

// literalt Encoding0::uniqueAtPosition()
// {
//   literalt x_ap, x_bp;
//   bvt rhs,lhs_rhs; // res;
  
   
//   ///////////////////////////////////////////////////////////////////
//   formula << "****UniqueAtPosition****" << std::endl;

//   for(int i = 0; i < traceSize-1; i++){
//     forall_matchSet(mit, matchSet){
//       // get the literal, matchNumeral, pos entry for the match 
//       x_ap = getLiteralFromMatches(*mit, i);
//       StrIntPair x_ap_MatchPos = lit2sym.find(x_ap)->second;
      
//       // debug print to ostream
//       formula << getLitName(x_ap) << " => "; 
      
//       forall_matchSet(mit1, matchSet){
// 	x_bp = getLiteralFromMatches(*mit1, i);
// 	StrIntPair x_bp_MatchPos = lit2sym.find(x_bp)->second;
// 	/* x_bp and x_ap should share the same position but should have 
// 	   different match numerals
// 	*/
// 	if(x_bp_MatchPos.second == x_ap_MatchPos.second && 
// 	   x_bp_MatchPos.first.compare(x_ap_MatchPos.first) != 0 ) {
// 	  rhs.push_back(slv->lnot(x_bp));
// 	  // debug print to ostream
// 	  formula << "¬" << getLitName(x_bp) << "/\\";
// 	}
//       }
//       lhs_rhs.push_back(slv->limplies(x_ap, slv->land(rhs)));
//       // debug print to ostream
//       formula << std::endl;
//       rhs.clear();
//     }
//     //res.push_back(slv->land(lhs_rhs));
//     //lhs_rhs.clear();
//   }
  
//   ///////////////////////////////////////////////////////////////////////
  
//   return slv->land(lhs_rhs); 
// }


// literalt Encoding0::noRepetition()
// {
  
//   literalt x_ap, x_bpdash;
//   bvt rhs, rhsFinal, lhs_rhs;
  
//   bool flag = false;

//   //////////////////////////////////////////////////////////////////////
//   formula << "****noRepetition****" << std::endl;
//   for(int i = 0; i < traceSize -1; i++){
//     forall_matchSet(mit, matchSet){
//       x_ap = getLiteralFromMatches(*mit, i);
      
//       // debug print to ostream
//       formula << getLitName(x_ap) << "=>"; 
      
//       for(int j = 0; j < i; j++){
// 	forall_matchSet(mitN, matchSet){
// 	  x_bpdash = getLiteralFromMatches(*mitN, j);
// 	  forall_match(lit, (**mit)){
// 	    forall_match(litN, (**mitN)){
// 	      if((*lit) == (*litN)){
// 		flag = true; 
// 		goto RHS;
// 	      }
// 	    }
// 	  }
// 	RHS: if(flag) {
// 	    rhs.push_back(slv->lnot(x_bpdash)); 
// 	    // debug print to ostream
// 	    formula << "¬" << getLitName(x_bpdash) << "/\\"; 
// 	    flag = false;
// 	  }
// 	}
//       }
//       if(rhs.size() ==  0){
// 	rhs.push_back(one); 
// 	// debug print to ostream
// 	formula << getLitName(one) ; 
	
//       }
//       lhs_rhs.push_back(slv->limplies(x_ap, slv->land(rhs)));
//       rhs.clear();
//       // debug print to ostream
//       formula << std::endl;
//     }
//   }
//   ///////////////////////////////////////////////////////////////////
//   return slv->land(lhs_rhs);
// }
 
// literalt Encoding0::partialOrder()
// {

//   literalt x_ap, x_bpdash;
//   bvt rhs, rhsFinal, lhs_rhs ;
  
//   bool flag = false;
//   bool ancestor = false;

//   ///////////////////////////////////
//   formula << "****partialOrder****" << std::endl;
//   for(int i = 0; i < traceSize -1; i++){
//     forall_matchSet(mit, matchSet){
//       x_ap = getLiteralFromMatches(*mit, i);
      
//       // debug print to ostream
//       formula << getLitName(x_ap) << "=>"; 
      
//       forall_transitionLists(iter, last_node->_tlist){
// 	forall_transitions(titer, (*iter)->_tlist){
// 	  Envelope *envB = (*titer).GetEnvelope();
// 	  if(envB->func_id == FINALIZE) continue; // skip the loop for the finalize event
// 	  CB B(envB->id, envB->index);

// 	  forall_match(lit, (**mit)){
// 	    if(last_node->isAncestor((*lit), B)){
// 	      ancestor = true; 
// 	      break;
// 	    }
// 	  }
// 	  if(ancestor){
// 	    // debug print to ostream
// 	    formula << "(";
// 	    for(int j = 0; j < i; j++){
// 	      forall_matchSet(mitN, matchSet){
// 		x_bpdash = getLiteralFromMatches(*mitN, j);
// 		forall_match(lit, (**mitN)){
// 		  if((*lit)._pid == envB->id && (*lit)._index == envB->index){ 
// 		    flag = true; break;
// 		  }
// 		}
// 		if(flag) {
// 		  rhs.push_back(x_bpdash); 
// 		  // debug print to ostream
// 		  formula << getLitName(x_bpdash) << "\\/";
// 		  flag = false;
// 		}
// 	      }
// 	    }
// 	    ancestor = false;
// 	    // debug print to ostream
// 	    formula << ") /\\";
// 	  }
// 	  if(rhs.size()){
// 	    rhsFinal.push_back(slv->lor(rhs));
// 	    rhs.clear();
// 	  }
// 	}
//       }
//       if(rhsFinal.size() == 0){
// 	rhsFinal.push_back(one);
// 	// debug print to ostream
// 	formula << getLitName(one) ;
//       }
//       lhs_rhs.push_back(slv->limplies(x_ap, slv->land(rhsFinal)));
//       rhsFinal.clear();
//       // debug print to ostream
//       formula << std::endl;
//     }
//   }
//   ///////////////////////////////////////
//   return slv->land(lhs_rhs);

// }

// literalt Encoding0::extensionNextPos ()
// {
//   bool flag, ancestor;
//   flag = false;
//   ancestor = false;
  
//   literalt x_ap, x_bpdash;
//   bvt rhs,rhsA, lhs_rhs;

//  //////////////////////////////////
//   formula << "****extensionNextPos****" << std::endl;
//   int i = traceSize -2;
//   forall_matchSet(mit, matchSet){
//     x_ap = getLiteralFromMatches(*mit, i);
    
//     // debug print to ostream
//     formula << getLitName(x_ap) << " <=> "; 
    
//     forall_transitionLists(iter, last_node->_tlist){
//       forall_transitions(titer, (*iter)->_tlist){
// 	Envelope *envB = (*titer).GetEnvelope();
// 	if(envB->func_id == FINALIZE) continue; // skip the loop for the finalize event
// 	CB B(envB->id, envB->index);
// 	forall_match(lit, (**mit)){
// 	  if(last_node->isAncestor((*lit), B)){
// 	    ancestor = true; 
// 	    break;
// 	  }
// 	}
// 	if(ancestor){
// 	  // debug print to ostream
// 	  formula << "(";
// 	  for(int j = 0; j < i; j++){
// 	    forall_matchSet(mitN, matchSet){
// 	      x_bpdash = getLiteralFromMatches(*mitN, j);
// 	      forall_match(lit, (**mitN)){
// 		if((*lit)._pid == envB->id && (*lit)._index == envB->index){ 
// 		  flag = true; break;
// 		}
// 	      }
// 	      if(flag) {
// 		rhs.push_back(x_bpdash); 
// 		// debug print to ostream
// 		formula << getLitName(x_bpdash) << "\\/";
// 		flag = false;
// 	      }
// 	    }
// 	  }
// 	  ancestor = false;
// 	  // debug print to ostream
// 	  formula << ") /\\";
// 	}
// 	if(rhs.size()){
// 	  rhsA.push_back(slv->lor(rhs));
// 	  rhs.clear();
// 	}
//       }
//     }
    
//     // debug print to ostream
//     formula << "("; 
    
//     for(int j = 0; j < i; j++){
//       forall_matchSet(mitN, matchSet){
// 	x_bpdash = getLiteralFromMatches(*mitN, j);
// 	forall_match(lit, (**mit)){
// 	  forall_match(litN, (**mitN)){
// 	    if((*lit) == (*litN)){
// 	      flag = true; 
// 	      goto RHS;
// 	    }
// 	  }
// 	}
//       RHS: if(flag) {
// 	  rhsA.push_back(slv->lnot(x_bpdash)); 
// 	  // debug print to ostream
// 	  formula << "¬" << getLitName(x_bpdash) << "/\\"; 
// 	  flag = false;
// 	}
//       }
//     }
//     // svs: FIX TODO-- check the size of rhsA
//     lhs_rhs.push_back(slv->land(slv->limplies(slv->land(rhsA), x_ap), 
// 			       slv->limplies(x_ap, slv->land(rhsA))));
//     rhsA.clear();
//     // debug print to ostream
//     formula << ")"; 
//     formula << std::endl;
    
//   }
    
//   //////////////////////////////////
  
//   return slv->land(lhs_rhs); 
// }

// literalt Encoding0::noMatchAtNextPos()
// {
//   int i = traceSize-2;
//   literalt x_ap; 
//   bvt res;
//   //////////////////////////////////
//   formula << "****noMatchAtNextPos****" << std::endl;
 
//   forall_matchSet(mit, matchSet){
//     x_ap = getLiteralFromMatches(*mit, i);
    
//     // debug print to ostream
//     formula << "¬" << getLitName(x_ap) << "/\\";
    
//     res.push_back(slv->lnot(x_ap));
//     //slv->l_set_to(slv->lnot(x_ap), true);
//   }
  
//   // debug print to ostream
//   formula <<std::endl;
  
//   //////////////////////////////////
//   return slv->land(res);
// }

// void Encoding0::publish()
// {
//  tvt result;
//   literalt x_ap;

//   for(int i = 0; i < traceSize -1; i++){
//     forall_matchSet(mit, matchSet){
//       x_ap = getLiteralFromMatches(*mit, i);
//       switch(slv->l_get(x_ap).get_value()){ 
//       case tvt::TV_TRUE:
// 	formula << getLitName(x_ap) << ":1" << std::endl;
// 	break;
//       case tvt::TV_FALSE:
// 	formula << getLitName(x_ap) << ":0" << std::endl;
// 	break;
//       case tvt::TV_UNKNOWN:
// 	formula << getLitName(x_ap) << ":UNKNOWN" << std::endl;
// 	break;
//       default: assert(false);
//       }
//     }
//   }
// }

// void Encoding0::Encoding_Matches()
// {
//   bvt res; 
  
//   // creating match set and associated literals.
//   createMatchSet();
//   createMatchLiterals();

//   res.push_back(uniqueAtPosition());
//   res.push_back(noRepetition());
//   res.push_back(partialOrder());
//   res.push_back(extensionNextPos());
//   res.push_back(noMatchAtNextPos());
  
//   slv->l_set_to(slv->land(res), true);
  
//   satcheckt::resultt answer = slv->prop_solve();
  
//   switch(answer){
//   case satcheckt::P_UNSATISFIABLE:
//     std::cout << "Formula is UNSAT" <<std::endl;
//     break;
//   case satcheckt::P_SATISFIABLE:
//     std::cout << "Formula is SAT -- DEADLOCK DETECTED" <<std::endl;
//     publish();
//     break;
//     // output the valuations
//   default: 
//     std::cout << "ERROR in SAT SOLVING" <<std::endl;
//     break;
//   }
//   std::cout << formula.str();
//   std::cout << std::endl;
			       

// }



// ////////////////////////////////////////////////////////////
// /////                                                ///////
// ////        ENCODING 1                               ///////
// ////////////////////////////////////////////////////////////



// void Encoding1::createEventLiterals()
// {
  
//   std::stringstream uniquepair;
//   std::string eventNumeral;
  
//   for(int i = 0; i < traceSize-1; i++){
//     forall_transitionLists(iter, last_node->_tlist){
//       forall_transitions(titer, (*iter)->_tlist){
// 	Envelope *env = (*titer).GetEnvelope();
// 	if(env->func_id == FINALIZE) continue;
// 	uniquepair << env->id;
// 	uniquepair << env->index;
// 	//uniquepair >> eventNumeral;
// 	eventNumeral = uniquepair.str();

// 	// clear out uniquepair
// 	uniquepair.str("");
// 	uniquepair.clear();      
	
	
// 	literalt lt = slv->new_variable();
	
// 	//insert in to the map
// 	lit2sym.insert(std::pair<literalt, StrIntPair> (lt, StrIntPair(eventNumeral, i)));
// 	sym2lit.insert(std::pair<StrIntPair, literalt> (StrIntPair(eventNumeral, i), lt));
//       }
//     }
//   }
// }


// literalt Encoding1::getLiteralFromEvents(TIter tit, int pos)
// {
//   literalt res; 
//   Envelope *env = (*tit).GetEnvelope();
//   std::stringstream uniquepair;
//   uniquepair << env->id << env->index;
  
//   res = sym2lit.find(StrIntPair(uniquepair.str(), pos))->second;
//   return res;
// }

// literalt Encoding1::getLiteralFromCB(CB a, int pos)
// {
//   std::stringstream p; 
//   p << a._pid << a._index;

//   std::string eventNumeral;
//   //p >> eventNumeral;
//   eventNumeral = p.str();

//   StrIntPair pair(eventNumeral, pos);
  
//   literalt res = sym2lit.find(pair)->second;
  
//   return res;
// }


// void Encoding1::uniqueAtPosition()
// {
//   literalt x_ap, x_bp; 
//   bvt lhs, rhs;
//   bool flag =false;
//   ////////////////////////////////////////////
//   // debug print to ostream
//   formula << "****uniqueAtPosition****" << std::endl; 
    
//   for(int i = 0; i < traceSize-1; i++){
//     forall_matchSet(mit, matchSet){
//       formula << "(";
//       forall_match(lit, (**mit)) {
// 	x_ap = getLiteralFromCB((*lit), i); 
// 	formula << getLitName(x_ap) << "/\\";
// 	lhs.push_back(x_ap);
//       }
//       formula << ") =>";
//       forall_transitionLists(iter, last_node->_tlist){
// 	forall_transitions(titer, (*iter)->_tlist){
// 	  x_bp = getLiteralFromEvents(titer, i);
// 	  if((*titer).GetEnvelope()->func_id == FINALIZE) continue;
// 	  //	  if(x_ap == x_bp) continue;
// 	  for(bvt::iterator bit = lhs.begin(), bitend = lhs.end(); 
// 	      bit != bitend; bit++){
// 	    if((*bit) == x_bp){ flag =true; break;}
// 	  }
// 	  if(flag) { flag = false; continue;}
// 	  else{
// 	    rhs.push_back(slv->lnot(x_bp));
// 	    formula << "¬" << getLitName(x_bp) << "/\\";
// 	  }
// 	}
//       }
//       slv->l_set_to(slv->limplies(slv->land(lhs), slv->land(rhs)), true); 
//       lhs.clear();
//       rhs.clear();
//       formula << std::endl;
//     }
//   }
//   ///////////////////////////////////////////
  
// }

// void Encoding1::noRepetition()
// {
//   literalt x_ap, x_aprime;
//   bvt rhs;
  
//   /////////////////////////////////////////
//   formula << "****noRepetition****" << std::endl; 
//   for(int i = 0; i < traceSize-1; i++){
//     forall_transitionLists(iter, last_node->_tlist){
//       forall_transitions(titer, (*iter)->_tlist){
// 	x_ap = getLiteralFromEvents(titer, i);
// 	if((*titer).GetEnvelope()->func_id == FINALIZE) continue;
// 	formula << getLitName(x_ap) << " => ";
// 	for(int j=0; j<i; j++){
// 	  x_aprime = getLiteralFromEvents(titer, j);
// 	  rhs.push_back(slv->lnot(x_aprime));
// 	  formula << "¬" << getLitName(x_aprime) << "/\\";
// 	}
// 	if(rhs.size()){
// 	  slv->l_set_to(slv->limplies(x_ap, slv->land(rhs)), true);
// 	  rhs.clear();
// 	}
// 	else {
// 	  slv->l_set_to(slv->limplies(x_ap, one), true);
// 	  formula << getLitName(one) << "/\\";
// 	}
// 	formula << std::endl; 
//       }
//     }
//   }
//   ////////////////////////////////////////
// }

// void Encoding1::partialOrder()
// {
//   literalt x_ap, x_bprime;
//   bvt rhs;
//   //////////////////////////////////////
//   formula << "****PartialOrder****" << std::endl; 
//   for(int i = 0; i < traceSize-1; i++){
//     forall_transitionLists(iter, last_node->_tlist){
//       forall_transitions(titer, (*iter)->_tlist){
// 	if((*titer).GetEnvelope()->func_id == FINALIZE) continue;
// 	x_ap = getLiteralFromEvents(titer,i);
// 	Envelope *envA = (*titer).GetEnvelope();
// 	CB A(envA->id, envA->index);
// 	forall_transitions(titerN, (*iter)->_tlist){
// 	  Envelope *envB = (*titerN).GetEnvelope();
// 	  if(envB->func_id == FINALIZE) continue;
// 	  CB B(envB->id, envB->index);
// 	  if(last_node->isAncestor(A,B)){
// 	    formula << getLitName(x_ap) << " => ";
// 	    for(int j=0; j<i; j++){
// 	      x_bprime = getLiteralFromEvents(titerN, j);
// 	      formula << getLitName(x_bprime) << "\\/";
// 	      rhs.push_back(x_bprime);
// 	    }
// 	    if(rhs.size()){
// 	      slv->l_set_to(slv->limplies(x_ap, slv->lor(rhs)), true);
// 	      rhs.clear();
// 	    }   
// 	    else {
// 	      slv->l_set_to(slv->limplies(x_ap, zero), true);
// 	      formula << getLitName(zero);
// 	    }
// 	    formula << std::endl;
// 	  }
// 	}
//       }
//     }
//   }
//   ////////////////////////////////////
// }


// void Encoding1::stutteringAtPosition()
// {
//   literalt x_ap, x_bp; 
//   bool flag = false;
//   bvt rhs, rhsFinal;
  
//   ////////////////////////////////////////////////
//   formula << "****stutteringAtPosition****" << std::endl; 
//   for(int i = 0; i < traceSize-1; i++){
//     forall_transitionLists(iter, last_node->_tlist){
//       forall_transitions(titer, (*iter)->_tlist){
// 	x_ap = getLiteralFromEvents(titer, i);
// 	if((*titer).GetEnvelope()->func_id == FINALIZE) continue;
// 	formula << getLitName(x_ap) << " => ";
// 	forall_matchSet(mit, matchSet){
// 	  formula << "(";
// 	  forall_match(lit, (**mit)){
// 	    x_bp = getLiteralFromCB((*lit), i);
// 	    rhs.push_back(x_bp);
// 	    if(x_ap == x_bp) flag = true;
// 	  }
// 	  if(flag){
// 	    rhsFinal.push_back(slv->land(rhs));
// 	    for(bvt::iterator bit = rhs.begin(); bit != rhs.end(); bit++){
// 	      formula << getLitName(*bit) << "/\\";
// 	    }
// 	    flag = false;
// 	  }
// 	  formula << ") \\/ ";
// 	  rhs.clear();
// 	}
// 	if(rhsFinal.size()){
// 	  slv->l_set_to( slv->limplies(x_ap, slv->lor(rhsFinal)), true);
// 	  rhsFinal.clear();
// 	}
// 	formula << std::endl;
//       }
//     }
//   }
//   ///////////////////////////////////////////////
// }

// void Encoding1::extensionNextPos()
// {
//   int i = traceSize-2;
//   literalt x_ap, x_bpdash, x_apdash;
//   bvt lhs, rhsA, rhsB, temp;
//   //////////////////////////////////////
//   formula << "****extensionNextPos****" << std::endl; 
//   forall_matchSet(mit, matchSet){
//     formula << "(" ; 
//     forall_match(lit, (**mit)) {    
//       x_ap = getLiteralFromCB((*lit), i);
//       formula << getLitName(x_ap) << " /\\";
//       lhs.push_back(x_ap); 
//     }
//     formula << ") <=> (" ;
//     forall_match(lit, (**mit)) {
//       for(int j=0; j < i; j++){
// 	x_apdash = getLiteralFromCB((*lit), j);
// 	rhsB.push_back(slv->lnot(x_apdash));
// 	formula << "¬" << getLitName(x_apdash) << "/\\";
//       } 
//       if(rhsB.empty()){
// 	rhsB.push_back(one); 
// 	formula << getLitName(one) << "/\\";
//       }
//     }
//     formula << ") /\\ (" ;
//     forall_match(lit, (**mit)){
//       forall_transitionLists(iter, last_node->_tlist){
// 	forall_transitions(titer, (*iter)->_tlist){
// 	  Envelope *envB = (*titer).GetEnvelope();
// 	  if(envB->func_id == FINALIZE) continue;	  
// 	  CB B (envB->id, envB->index);
// 	  if(last_node->isAncestor((*lit),B)){
// 	    formula << "(" ;
// 	    for (int j=0; j<i; j++){
// 	      x_bpdash = getLiteralFromEvents(titer, j);
// 	      temp.push_back(x_bpdash);
// 	      formula << getLitName(x_bpdash) << "\\/";
// 	    }
	   
// 	    if(!temp.empty()){
// 	      formula << ") /\\";
// 	      rhsA.push_back(slv->lor(temp));
// 	      temp.clear();
// 	    }
// 	    else{
// 	      rhsA.push_back(zero);
// 	      formula << getLitName(zero) << ") /\\";
// 	    }
// 	  }
// 	}
//       }
//     }
//     // if(rhsA.empty()){
//     //   rhsA.push_back(one);
//     //   formula << getLitName(one) << ") /\\";
//     // }
//     if(!rhsA.empty())
//       rhsB.push_back(slv->land(rhsA));

//     slv->l_set_to(slv->land(slv->limplies(slv->land(lhs), slv->land(rhsB)), 
// 			  slv->limplies(slv->land(rhsB), slv->land(lhs))), true);
//     rhsB.clear();
//     lhs.clear();
//     rhsA.clear();
//     formula << std::endl;
//   }
//   /////////////////////////////////////
  
// }


// void Encoding1::noMatchAtNextPos()
// {
//   int i = traceSize-2;
//   literalt x_ap;
//   bvt res, resF;
//   /////////////////////////////////////
//   formula << "****noMatchAtNextPos****" << std::endl; 
//   forall_matchSet(mit, matchSet){
//     formula << "(";
//     forall_match(lit, (**mit)) {    
//       x_ap = getLiteralFromCB((*lit), i);
//       res.push_back(slv->lnot(x_ap));
//       formula << "¬" << getLitName(x_ap) << "\\/";
//     }
//     formula << ") /\\";
//     resF.push_back(slv->lor(res)); 
//     res.clear();
//   }
//   formula <<std::endl;
//   slv->l_set_to(slv->land(resF), true);
//  /////////////////////////////////////

// }

// void Encoding1::publish()
// {
//  tvt result;
//   literalt x_ap;
  

//   for(int i = 0; i < traceSize -1; i++){
//     forall_transitionLists(iter, last_node->_tlist){
//       forall_transitions(titer, (*iter)->_tlist){
// 	if((*titer).GetEnvelope()->func_id == FINALIZE) continue;
// 	x_ap = getLiteralFromEvents(titer, i);
// 	switch(slv->l_get(x_ap).get_value()){ 
// 	case tvt::TV_TRUE:
// 	  formula << getLitName(x_ap) << ":1" << std::endl;
// 	  break;
// 	case tvt::TV_FALSE:
// 	  formula << getLitName(x_ap) << ":0" << std::endl;
// 	  break;
// 	case tvt::TV_UNKNOWN:
// 	  formula << getLitName(x_ap) << ":UNKNOWN" << std::endl;
// 	  break;
// 	default: assert(false);
// 	}
//       }
//     }
//   }
// }

// void Encoding1::Encoding_Events()
// {
//   // creating match set and associated literals.
//   createMatchSet();
//   createEventLiterals();
//   gettimeofday(&constGenStart, NULL);  
//   uniqueAtPosition();
//   noRepetition();
//   partialOrder();
//   stutteringAtPosition();
//   extensionNextPos();
//   noMatchAtNextPos();
//   gettimeofday(&constGenEnd, NULL);  
//   std::cout << formula.str();
//   std::cout << std::endl;
  
//   formula.str("");
//   formula.clear();
  
//   gettimeofday(&solverStart, NULL);
//   satcheckt::resultt answer = slv->prop_solve();
//   gettimeofday(&solverEnd, NULL);
  
//   formula << "Number of Clauses: " << static_cast<cnf_solvert *>(slv.get())->no_clauses() << std::endl;
//   formula << "Number of Variables: " << slv->no_variables() << std::endl;
//   formula << "Constraint Generation Time: "
// 	  << (getTimeElapsed(constGenStart, constGenEnd)*1.0)/1000000 << "sec \n";
//   formula << "Solving Time: " << (getTimeElapsed(solverStart, solverEnd)*1.0)/1000000 << "sec \n";
  

//   switch(answer){
//   case satcheckt::P_UNSATISFIABLE:
//     formula << "Formula is UNSAT" <<std::endl;
//     break;
//   case satcheckt::P_SATISFIABLE:
//     formula  << "Formula is SAT -- DEADLOCK DETECTED" <<std::endl;
//     publish();
//     break;
//     // output the valuations
//   default: 
//     formula << "ERROR in SAT SOLVING" <<std::endl;
//     break;
//   }

//   std::cout << formula.str();
//   std::cout << std::endl;
  
// }


// ////////////////////////////////////////////////////////////
// /////                                                ///////
// ////        ENCODING 2                               ///////
// ////////////////////////////////////////////////////////////

// void Encoding2::createMatchLiterals()
// {
//   std::stringstream uniquepair;
//   std::string matchNumeral;
//   bool flag = false;

//   for(int i = 0; i < traceSize-1; i++){
//     forall_matchSet(mit, matchSet){
//       forall_match(lit, (**mit)){
// 	uniquepair<<(*lit)._pid;
// 	uniquepair<<(*lit)._index;
// 	if(last_node->GetTransition((*lit))->GetEnvelope()->isCollectiveType()) 
// 	  flag = true;
//       }
//       if(flag){ // add only collective match-literals
// 	matchNumeral = uniquepair.str();
// 	flag = false;
// 	literalt lt = slv->new_variable();
// 	//insert in to the map
// 	lit2sym.insert(std::pair<literalt, StrIntPair> (lt, StrIntPair(matchNumeral, i)));
// 	sym2lit.insert(std::pair<StrIntPair, literalt> (StrIntPair(matchNumeral, i), lt));
// 	match2sym.insert(std::pair<MatchPtr, StrIntPair> (*mit, StrIntPair(matchNumeral, i)));
//       }
//       // clear out uniquepair
//       uniquepair.str("");
//       uniquepair.clear();      
//     }
//   }
// }

// void Encoding2::createEventLiterals()
// {
//   std::stringstream uniquepair;
//   std::string eventNumeral;

//   for(int i = 0; i < traceSize-1; i++){
//     forall_transitionLists(iter, last_node->_tlist){
//       forall_transitions(titer, (*iter)->_tlist){
// 	Envelope *env = (*titer).GetEnvelope();
// 	if(env->func_id == FINALIZE) continue;

// 	if(!(env->isCollectiveType())){ 
// 	  uniquepair << env->id;
// 	  uniquepair << env->index;
// 	  //uniquepair >> eventNumeral;
// 	  eventNumeral = uniquepair.str();
// 	  literalt lt = slv->new_variable();
	  
// 	  //insert in to the map
// 	  lit2sym.insert(std::pair<literalt, StrIntPair> (lt, StrIntPair(eventNumeral, i)));
// 	  sym2lit.insert(std::pair<StrIntPair, literalt> (StrIntPair(eventNumeral, i), lt));
// 	}
// 	  // clear out uniquepair
// 	uniquepair.str("");
// 	uniquepair.clear();      
//       }
//     }
//   }
// }

// literalt Encoding2::getLiteralFromEvents(TIter tit, int pos)
// {
//   literalt res; 
//   Envelope *env = (*tit).GetEnvelope();
//   std::stringstream uniquepair;
//   uniquepair << env->id << env->index;
  
//   res = sym2lit.find(StrIntPair(uniquepair.str(), pos))->second;
//   return res;
// }

// literalt Encoding2::getLiteralFromCB(CB a, int pos)
// {
//   std::stringstream p; 
//   p << a._pid << a._index;

//   std::string eventNumeral;
//   //p >> eventNumeral;
//   eventNumeral = p.str();

//   StrIntPair pair(eventNumeral, pos);
  
//   literalt res = sym2lit.find(pair)->second;
  
//   return res;
// }

// literalt Encoding2::getLiteralFromMatches(MatchPtr mptr, int pos)
// {
//   int  position;
//   std::string matchNumeral;
//   literalt res;

//   std::pair<std::multimap<MatchPtr, StrIntPair>::iterator, 
// 	    std::multimap<MatchPtr, StrIntPair>::iterator > ret; 
//   ret = match2sym.equal_range(mptr);
  
//   for(std::multimap<MatchPtr, StrIntPair>::iterator iter = ret.first;
//       iter != ret.second; iter++){
    
//     matchNumeral = (*iter).second.first;
//     position = (*iter).second.second;
    
//     if(position == pos){
//       res = sym2lit.find(StrIntPair(matchNumeral, pos))->second;
//       return res;
//     }
//   }
//   // requires check at callee site whether returned value makes sense
//   return res;
// }

// MatchPtr Encoding2::getMatchPtrFromEvent(CB a)
// {
//   bool flag = false;
//   MatchPtr res;
//   forall_matchSet(mit, matchSet){
//     forall_match(lit, (**mit)){
//       if((*lit) == a){
// 	flag = true;
// 	break;
//       }
//     }
//     if(flag) {
//       res = *mit;
//       break;
//     }
//   }
//   return res;
// }


// void Encoding2::uniqueAtPosition()
// {
//   literalt x_ap, x_bp, x_np; 
//   bvt lhs, rhs, rhsPrint;
//   bool collective = false;
//   bool flag = false;
//   ////////////////////////////////////////////
//   // debug print to ostream
//   formula << "****uniqueAtPosition****" << std::endl; 
    
//   for(int i = 0; i < traceSize-1; i++){
//     forall_matchSet(mit, matchSet){

//       forall_match(lit, (**mit)) {
// 	if(!(last_node->GetTransition(*lit)->GetEnvelope()->isCollectiveType())){
// 	  x_ap = getLiteralFromCB((*lit), i); 
// 	  lhs.push_back(x_ap);
// 	}
// 	else {
// 	  collective = true; 
// 	  break;
// 	}
//       }
//       if(!collective){ // matched events are not collective
// 	assert(lhs.size() != 0);
// 	forall_transitionLists(iter, last_node->_tlist){
// 	  forall_transitions(titer, (*iter)->_tlist){
// 	    if((*titer).GetEnvelope()->isCollectiveType() ||
// 	       (*titer).GetEnvelope()->func_id == FINALIZE) continue;
// 	    x_bp = getLiteralFromEvents(titer, i);
// 	    for(bvt::iterator bit = lhs.begin(), bitend = lhs.end(); 
// 		bit != bitend; bit++){
// 	      if((*bit) == x_bp){ flag =true; break;}
// 	    }
// 	    if(flag) { flag = false; continue;}
// 	    else{
// 	      rhs.push_back(slv->lnot(x_bp));
// 	      rhsPrint.push_back(x_bp);
// 	    }
// 	  }
// 	}
// 	forall_matchSet(mitN, matchSet){
// 	  forall_match(lit, (**mitN)){
// 	    if(last_node->GetTransition(*lit)->GetEnvelope()->isCollectiveType()){
// 	      flag = true;
// 	      break;
// 	    }
// 	  }
// 	  if(flag){
// 	    x_np = getLiteralFromMatches(*mitN, i);
// 	    rhs.push_back(slv->lnot(x_np));
// 	    rhsPrint.push_back(x_np);
// 	    flag=false;
// 	  }
// 	}
// 	if(!rhs.empty()){
// 	  //formula output
// 	  formula << "((";
// 	  for(bvt::iterator bit = lhs.begin(); bit != lhs.end(); bit++){
// 	    formula << getLitName(*bit) << " & ";
// 	  }
// 	  formula << ") -> ("; 
// 	  for(bvt::iterator bit = rhsPrint.begin(); bit != rhsPrint.end(); bit++){
// 	    formula << "!" << getLitName(*bit) << " & ";
// 	  }
// 	  formula << ")) &" << std::endl; 
	  
// 	  slv->l_set_to(slv->limplies(slv->land(lhs),slv->land(rhs)) , true); 
// 	  rhs.clear();
// 	  rhsPrint.clear();
// 	}
// 	lhs.clear();
//       }
//       else{ // for collective events
// 	x_np = getLiteralFromMatches(*mit, i);
// 	forall_matchSet(mitN, matchSet){
// 	  x_bp = getLiteralFromMatches(*mitN,i);
// 	  forall_match(lit, (**mitN)){
// 	    if(last_node->GetTransition(*lit)->GetEnvelope()->isCollectiveType()){
// 	      flag = true;
// 	      break;
// 	    }
// 	  }
// 	  if(flag){
// 	    if(x_bp != x_np){
// 	      rhs.push_back(slv->lnot(x_bp));
// 	      rhsPrint.push_back(x_bp);
// 	    }
// 	    flag=false;
// 	  }
// 	}
// 	forall_transitionLists(iter, last_node->_tlist){
// 	  forall_transitions(titer, (*iter)->_tlist){
// 	    if((*titer).GetEnvelope()->isCollectiveType() ||
// 	       (*titer).GetEnvelope()->func_id == FINALIZE) continue;
// 	    x_ap = getLiteralFromEvents(titer, i);
// 	    rhs.push_back(slv->lnot(x_ap));
// 	    rhsPrint.push_back(x_ap);
// 	  }
// 	}
// 	if(!rhs.empty()){
// 	  //formula output
// 	  formula << "(";
// 	  formula << getLitName(x_np);
// 	  formula << " -> ("; 
// 	  for(bvt::iterator bit = rhsPrint.begin(); bit != rhsPrint.end(); bit++){
// 	    formula << "!" << getLitName(*bit) << " & ";
// 	  }
// 	  formula << ")) &" << std::endl; 

// 	  slv->l_set_to(slv->limplies(x_np, slv->land(rhs)), true);
// 	  rhs.clear();
// 	  rhsPrint.clear();
// 	}
// 	collective =false;
//       }
//     }
//   }
//   ///////////////////////////////////////////
  
// }

// void Encoding2::noRepetition()
// {
//   literalt x_ap, x_apdash;
//   bvt rhs;
//   bvt rhsPrint;
//   bool flag = false;
//   bool flag1 = false;
//   ////////////////////////////////////////////
//   formula << "****noRepetition****" << std::endl; 

//   for(int i = 0; i < traceSize-1; i++){
//     forall_transitionLists(iter, last_node->_tlist){
//       forall_transitions(titer, (*iter)->_tlist){
// 	if((*titer).GetEnvelope()->isCollectiveType() ||
// 	   (*titer).GetEnvelope()->func_id == FINALIZE) continue;
// 	x_ap = getLiteralFromEvents(titer, i);
// 	for (int j = 0; j < i ; j++){
// 	  x_apdash = getLiteralFromEvents(titer, j);
// 	  rhs.push_back(slv->lnot(x_apdash));
// 	  rhsPrint.push_back(x_apdash);
// 	}
// 	if(!rhs.empty())
// 	  slv->l_set_to(slv->limplies(x_ap, slv->land(rhs)), true);
// 	else {
// 	  slv->l_set_to(slv->limplies(x_ap, one), true);
// 	}
// 	// // formula output
// 	formula << "(" << getLitName(x_ap) << " -> ( ";
// 	if(!rhsPrint.empty())
// 	  for(bvt::iterator bit = rhsPrint.begin(); bit != rhsPrint.end(); bit++){
// 	    formula << "!" << getLitName(*bit) << " & ";
// 	  }       
// 	else
// 	  formula << getLitName(one);
// 	formula << ")) & " << std::endl;
// 	rhs.clear();
// 	rhsPrint.clear();
//       }
//     }
    
//     forall_matchSet(mit, matchSet){
//       forall_match(lit, (**mit)){
// 	if(last_node->GetTransition(*lit)->GetEnvelope()->isCollectiveType()){
// 	  flag = true;
// 	  break;
// 	}	
//       }
//       if (flag){
// 	x_ap = getLiteralFromMatches(*mit, i);
// 	for (int j = 0; j <i; j ++){
// 	  x_apdash = getLiteralFromMatches(*mit, j);
// 	  rhs.push_back(slv->lnot(x_apdash));
// 	  rhsPrint.push_back(x_apdash);
// 	}
// 	if(!rhs.empty())
// 	  slv->l_set_to(slv->limplies(x_ap, slv->land(rhs)), true);
// 	else 
// 	  slv->l_set_to(slv->limplies(x_ap, one), true);

// 	// // formula output
// 	formula <<"(" << getLitName(x_ap) << " -> ( ";
// 	if(!rhs.empty())
// 	  for(bvt::iterator bit = rhsPrint.begin(); bit != rhsPrint.end(); bit++){
// 	    formula << "!" << getLitName(*bit) << " & ";
// 	  }       
// 	else
// 	  formula << getLitName(one);
// 	formula << ")) & " << std::endl;
	
// 	rhs.clear();
// 	rhsPrint.clear();	
// 	flag = false;
//       }
//     }
//   }
//   ////////////////////////////////////////////

// }

// void Encoding2::partialOrder()
// {
//   bvt rhs;
//   literalt x_ap, x_bp;

//   ////////////////////////////////////////////
//   formula << "****partialOrder****" << std::endl; 

//   for(int i = 0; i < traceSize-1; i++){

//     forall_transitionLists(iter, last_node->_tlist){
//       forall_transitions(titer, (*iter)->_tlist){
// 	if((*titer).GetEnvelope()->func_id == FINALIZE) continue;
// 	else if(!((*titer).GetEnvelope()->isCollectiveType())){
// 	  x_ap = getLiteralFromEvents(titer, i);
// 	  CB A(titer->GetEnvelope()->id, titer->GetEnvelope()->index);
// 	  forall_transitions(titerN, (*iter)->_tlist){
// 	    if((*titerN).GetEnvelope()->func_id == FINALIZE) continue;
// 	    CB B(titerN->GetEnvelope()->id, titerN->GetEnvelope()->index);
// 	    if(last_node->isAncestor(A, B)){
// 	      if(!((*titerN).GetEnvelope()->isCollectiveType())){
		
// 		for(int j =0 ; j<i; j++){
// 		  x_bp = getLiteralFromEvents(titerN, j);
// 		  rhs.push_back(x_bp);
// 		}
// 		if(!rhs.empty())
// 		  slv->l_set_to(slv->limplies(x_ap,slv->lor(rhs)),true);
// 		else
// 		  slv->l_set_to(slv->limplies(x_ap, zero),true);
		  
// 		// //formula output
// 		formula << "(" << getLitName(x_ap) << " -> ( " ;
// 		if(!rhs.empty())
// 		  for(bvt::iterator bit = rhs.begin(); bit != rhs.end(); bit++){
// 		    formula << getLitName(*bit) << " | ";
// 		  }
// 		else
// 		  formula << getLitName(zero); 
// 		formula << " )) &" << std::endl;
		
// 		rhs.clear();
// 	      }
// 	      else { //titerN is collective type
// 		MatchPtr Bptr = getMatchPtrFromEvent(B);
// 		for(int j=0; j<i; j++){
// 		  x_bp = getLiteralFromMatches(Bptr, j);
// 		  rhs.push_back(x_bp);
// 		}
// 		if(!rhs.empty())
// 		  slv->l_set_to(slv->limplies(x_ap, slv->lor(rhs)),true);
// 		else 
// 		  slv->l_set_to(slv->limplies(x_ap, zero),true);
		
// 		// //formula output
// 		formula << "(" << getLitName(x_ap) << " -> ( " ;
// 		if(!rhs.empty())
// 		  for(bvt::iterator bit = rhs.begin(); bit != rhs.end(); bit++){
// 		    formula << getLitName(*bit) << " | ";
// 		  }
// 		else
// 		  formula << getLitName(zero); 
// 		formula << " )) &" << std::endl;
		
// 		rhs.clear();
// 	      }
// 	    }
// 	  }
// 	}
// 	else{  // titer is collective type
// 	  CB A(titer->GetEnvelope()->id, titer->GetEnvelope()->index);
// 	  MatchPtr Aptr = getMatchPtrFromEvent(A);
// 	  x_ap = getLiteralFromMatches(Aptr, i);
// 	  forall_transitions(titerN, (*iter)->_tlist){
// 	    if((*titerN).GetEnvelope()->func_id == FINALIZE) continue;
// 	    CB B(titerN->GetEnvelope()->id, titerN->GetEnvelope()->index);
// 	    if(last_node->isAncestor(A, B)){
// 	     if(!((*titerN).GetEnvelope()->isCollectiveType())){
// 	       for(int j =0 ; j<i; j++){
// 		  x_bp = getLiteralFromEvents(titerN, j);
// 		  rhs.push_back(x_bp);
// 	       }
// 	       if(!rhs.empty())
// 		 slv->l_set_to(slv->limplies(x_ap,slv->lor(rhs))  ,true);
// 	       else 
// 		 slv->l_set_to(slv->limplies(x_ap,zero)  ,true);
	       
// 	       // //formula output
// 	       	formula << "(" << getLitName(x_ap) << " -> ( " ;
// 	       	if(!rhs.empty())
// 	       	  for(bvt::iterator bit = rhs.begin(); bit != rhs.end(); bit++){
// 	       	    formula << getLitName(*bit) << " | ";
// 	       	  }
// 	       	else
// 	       	  formula << getLitName(zero); 
// 	       	formula << " )) &" << std::endl;
		
// 		rhs.clear();
// 	      }
// 	      else { //titerN is collective type
// 		MatchPtr Bptr = getMatchPtrFromEvent(B);
// 		for(int j=0; j<i; j++){
// 		  x_bp = getLiteralFromMatches(Bptr, j);
// 		  rhs.push_back(x_bp);
// 		}
// 		if(!rhs.empty())
// 		  slv->l_set_to(slv->limplies(x_ap, slv->lor(rhs)),true);
// 		else 
// 		  slv->l_set_to(slv->limplies(x_ap, zero),true);
		
// 		// //formula output
// 		formula << "(" << getLitName(x_ap) << " -> ( " ;
// 		if(!rhs.empty())
// 		  for(bvt::iterator bit = rhs.begin(); bit != rhs.end(); bit++){
// 		    formula << getLitName(*bit) << " | ";
// 		  }
// 		else
// 		  formula << getLitName(zero); 
// 		formula << " )) &" << std::endl;

// 		rhs.clear();
// 	      }
// 	    }
// 	  }
// 	}
//       }
//     }
//   }
//   ////////////////////////////////////////////// 
// }


// void Encoding2::stutteringAtPosition()
// {
//  literalt x_ap, x_bp; 
//   bool flag = false;
//   bvt rhs, rhsFinal;
//   std::list<bvt> rhsPrint;
  
//   ////////////////////////////////////////////////
//   formula << "****stutteringAtPosition****" << std::endl; 
//   for(int i = 0; i < traceSize-1; i++){
//     forall_transitionLists(iter, last_node->_tlist){
//       forall_transitions(titer, (*iter)->_tlist){
// 	if((*titer).GetEnvelope()->isCollectiveType() ||
// 	   (*titer).GetEnvelope()->func_id == FINALIZE) continue;
// 	x_ap = getLiteralFromEvents(titer, i);
// 	forall_matchSet(mit, matchSet){
// 	  forall_match(lit, (**mit)){
// 	    x_bp = getLiteralFromCB((*lit), i);
// 	    rhs.push_back(x_bp);
// 	    if(x_ap == x_bp) flag = true;
// 	  }
// 	  if(flag){
// 	    assert(!rhs.empty());
// 	    rhsPrint.push_back(rhs);
// 	    rhsFinal.push_back(slv->land(rhs));
// 	    flag = false;
// 	  }
// 	  rhs.clear();
// 	}
// 	if(!rhsFinal.empty()){
// 	  slv->l_set_to( slv->limplies(x_ap, slv->lor(rhsFinal)), true);
// 	  rhsFinal.clear();
// 	}
	
// 	// formula output
// 	if(!rhsPrint.empty()){
// 	  formula << "( " << getLitName(x_ap) << " -> ( ";
// 	  for(std::list<bvt>::iterator lbit = rhsPrint.begin(); 
// 	      lbit != rhsPrint.end(); lbit ++){
// 	    formula << "(" ;
// 	    for(bvt::iterator bit = (*lbit).begin(); bit!=(*lbit).end(); bit++){
// 	      formula << getLitName(*bit) << " & ";
// 	    }	   
// 	    formula << ") | "; 
// 	  }
// 	  formula << ")) &" << std::endl;
// 	  rhsPrint.clear();
// 	}
//       }
//     }
//   }
// }

// void Encoding2::noMatchAtNextPos()
// {
//   int i = traceSize-2; 
//   literalt x_ap;
//   bvt res;
//   std::list<bvt> Print;
//   bool flag =false;
//   //////////////////////////////////////////////
//   formula << "****noMatchAtNextPos****" << std::endl; 
//   forall_matchSet(mit, matchSet){
//     bvt temp;
//     forall_match(lit, (**mit)){
//       if(last_node->GetTransition(*lit)->GetEnvelope()->isCollectiveType()){
// 	flag = true;
// 	break;
//       }
//       else{
// 	x_ap = getLiteralFromCB((*lit), i);
// 	res.push_back(slv->lnot(x_ap));
// 	temp.push_back(x_ap);
//       }
//     }
//     if (flag){
//       bvt t;
//       x_ap = getLiteralFromMatches(*mit, i);
//       t.push_back(x_ap);
//       slv->l_set_to(slv->lnot(x_ap), true);
//       Print.push_back(t);
//       flag = false;
//     }
//     else{
//       assert(!res.empty());
//       slv->l_set_to(slv->lor(res), true); 
//       Print.push_back(temp);
//       res.clear();
//       temp.clear();
//     }
//   }
  
//   // formula output 
//   for(std::list<bvt>::iterator lbit = Print.begin(); 
//       lbit != Print.end(); lbit ++){
//     formula << "(" ;
//     for(bvt::iterator bit = (*lbit).begin(); bit!=(*lbit).end(); bit++){
//       formula << "!" << getLitName(*bit) << " | ";
//     }	   
//     formula << ") & "; 
//   }
//   ////////////////////////////////////////////// 

// }

// void Encoding2::extensionNextPos()
// {
//   int i = traceSize-2;
//   bvt rhs, rhsFinal, lhs; 
//   bvt  rhsBPrint; 
//   std::list <bvt> rhsAPrint;
//   literalt x_ap, x_apdash, x_bp, x_bpdash;
//   bool flag = false;
//   ////////////////////////////////////////////// 
//   formula << "****extensionNextPos****" << std::endl; 
//   forall_matchSet(mit, matchSet){
//     forall_match(lit, (**mit)) {    
//       x_ap = getLiteralFromCB((*lit), i);
//       if(!(last_node->GetTransition(*lit)->GetEnvelope()->isCollectiveType())){
// 	lhs.push_back(x_ap);
// 	for(int j=0; j < i; j++){
// 	  x_apdash = getLiteralFromCB((*lit), j);
// 	  rhsBPrint.push_back(x_apdash);
// 	  rhsFinal.push_back(slv->lnot(x_apdash));
// 	}
// 	if(rhsFinal.empty()){
// 	  rhsFinal.push_back(one);
// 	  rhsBPrint.push_back(one);
// 	  }
//       }
//       else {// i.e event is part of collective match
// 	flag = true;
// 	break;
//       }
//     }
//     if(!flag){
//       assert(!lhs.empty());
//       assert(!rhsFinal.empty());
//       forall_match(lit, (**mit)){
// 	forall_transitionLists(iter, last_node->_tlist){
// 	  forall_transitions(titer, (*iter)->_tlist){
// 	    Envelope *envB = (*titer).GetEnvelope();
// 	    if(envB->func_id == FINALIZE) continue;
// 	    CB B (envB->id, envB->index);
// 	    if(last_node->isAncestor((*lit),B)){
// 	      bvt temp;
// 	      if(!((*titer).GetEnvelope()->isCollectiveType())){
// 		for (int j=0; j<i; j++){
// 		  x_bpdash = getLiteralFromEvents(titer, j);
// 		  temp.push_back(x_bpdash);
// 		} 
// 		if(!temp.empty()){
// 		  rhs.push_back(slv->lor(temp));
// 		  rhsAPrint.push_back(temp);
// 		}
// 		else {
// 		  rhs.push_back(zero);
// 		  temp.push_back(zero);
// 		  rhsAPrint.push_back(temp);
// 		}
// 	      }
// 	      else{
// 		MatchPtr Bptr = getMatchPtrFromEvent(B);
// 		for (int j=0; j<i; j++){
// 		  x_bpdash = getLiteralFromMatches(Bptr, j);
// 		  temp.push_back(x_bpdash);
// 		}
// 		if(!temp.empty()){
// 		  rhs.push_back(slv->lor(temp));
// 		  rhsAPrint.push_back(temp);
// 		  temp.clear();
// 		}
// 		else{
// 		  rhs.push_back(zero);
// 		  temp.push_back(zero);
// 		  rhsAPrint.push_back(temp);
// 		  temp.clear();
// 		}
// 	      }
// 	    }
// 	    if(rhs.empty()){
// 	      rhs.push_back(one);
// 	      rhsAPrint.push_back(rhs);
// 	    }
// 	    rhsFinal.push_back(slv->land(rhs));
// 	    rhs.clear();
// 	  }
// 	}
//       }
//       if(!lhs.empty() && !rhsFinal.empty()){
// 	slv->l_set_to(slv->land(slv->limplies(slv->land(lhs), slv->land(rhsFinal)), 
// 			      slv->limplies(slv->land(rhsFinal), slv->land(lhs))), true);
// 	//formula output
// 	formula << "( (" ;
// 	for(bvt::iterator bit = lhs.begin(); bit != lhs.end(); bit++){
// 	  formula << getLitName(*bit) << " & ";
// 	}
// 	formula << ") <-> ("; 
// 	for(bvt::iterator bit = rhsBPrint.begin(); bit != rhsBPrint.end(); bit++){
// 	  formula << "!" << getLitName(*bit) << " & ";
// 	}
// 	formula << ") & (";
// 	for(std::list<bvt>::iterator lbit = rhsAPrint.begin(); lbit != rhsAPrint.end(); 
// 	    lbit++){
// 	  formula << "(";
// 	  for(bvt::iterator bit = (*lbit).begin(); bit != (*lbit).end(); bit++){
// 	    formula << getLitName(*bit) << " | ";  
// 	  }
// 	  formula << ") & ";
// 	}
// 	formula << ")) & " <<std::endl;
// 	rhsAPrint.clear(); rhsBPrint.clear();
// 	lhs.clear();
// 	rhsFinal.clear();
//       }
//     }
//     else{ // 
//       assert(lhs.empty());
//       x_ap = getLiteralFromMatches(*mit, i);
//       forall_match(lit, (**mit)) {
// 	forall_transitionLists(iter, last_node->_tlist){
// 	  forall_transitions(titer, (*iter)->_tlist){
// 	    Envelope *envB = (*titer).GetEnvelope();
// 	    if(envB->func_id == FINALIZE) continue;
// 	    CB B (envB->id, envB->index);
// 	    if(last_node->isAncestor((*lit), B)){
// 	      bvt temp;  
// 	      if(last_node->GetTransition(B)->GetEnvelope()->isCollectiveType()){
// 		MatchPtr Bptr = getMatchPtrFromEvent(B);
// 		for(int j=0;j<i;j++){
// 		  x_bp = getLiteralFromMatches(Bptr, j);
// 		  temp.push_back(x_bp);
// 		}
// 		if(!temp.empty()){
// 		  rhs.push_back(slv->lor(temp));
// 		  rhsAPrint.push_back(temp);
// 		  temp.clear();
// 		}
// 		else{
// 		  rhs.push_back(zero);
// 		  temp.push_back(zero);
// 		  rhsAPrint.push_back(temp);
// 		  temp.clear();
// 		}
// 	      }
// 	      else{
// 		for(int j=0;j<i;j++){
// 		  x_bp = getLiteralFromCB(B, j);
// 		  temp.push_back(x_bp);
// 		}
// 		if(!temp.empty()){
// 		  rhs.push_back(slv->lor(temp));
// 		  rhsAPrint.push_back(temp);
// 		  temp.clear();
// 		}
// 		else{
// 		  rhs.push_back(zero);
// 		  temp.push_back(zero);
// 		  rhsAPrint.push_back(temp);
// 		  temp.clear();
// 		}
// 	      }
// 	    }
// 	    if(rhs.empty()){
// 	      rhs.push_back(one);
// 	      rhsAPrint.push_back(rhs);
// 	    }
// 	    rhsFinal.push_back(slv->land(rhs));
// 	    rhs.clear();
// 	  }
// 	}
//       }
      
//       for(int j=0; j<i; j++){
// 	x_bp = getLiteralFromMatches(*mit,j);
// 	rhsFinal.push_back(slv->lnot(x_bp));
// 	rhsBPrint.push_back(x_bp);
//       }
      
//       if(!rhsFinal.empty()){
// 	slv->l_set_to(slv->land(slv->limplies(x_ap, slv->land(rhsFinal)), 
// 			      slv->limplies(slv->land(rhsFinal), x_ap)), true);
// 	rhsFinal.clear();
	
// 	//formula output
// 	formula << "( " ;
// 	formula << getLitName(x_ap) << " <-> ("; 
// 	for(bvt::iterator bit = rhsBPrint.begin(); bit != rhsBPrint.end(); bit++){
// 	  formula << "!" << getLitName(*bit) << " & ";
// 	}
// 	formula << ") & (";
// 	for(std::list<bvt>::iterator lbit = rhsAPrint.begin(); lbit != rhsAPrint.end(); 
// 	    lbit++){
// 	  formula << "(";
// 	  for(bvt::iterator bit = (*lbit).begin(); bit != (*lbit).end(); bit++){
// 	    formula << getLitName(*bit) << " | ";  
// 	  }
// 	  formula << ") & ";
// 	}
// 	formula << ")) & " <<std::endl;
// 	rhsAPrint.clear(); rhsBPrint.clear();
//       }
//       flag = false;
//     }
//   }
//   ///////////////////////////////////////////////////////////////////////

// }



// void Encoding2::publish()
// {
//   tvt result;
//   literalt x_ap;
//   bool flag = false;
  
//   for(int i = 0; i < traceSize -1; i++){
//     forall_transitionLists(iter, last_node->_tlist){
//       forall_transitions(titer, (*iter)->_tlist){
// 	if((*titer).GetEnvelope()->func_id == FINALIZE) continue;
// 	if(!((*titer).GetEnvelope()->isCollectiveType())){
// 	  x_ap = getLiteralFromEvents(titer, i);
// 	  switch(slv->l_get(x_ap).get_value()){ 
// 	  case tvt::TV_TRUE:
// 	    formula << getLitName(x_ap) << ":1" << std::endl;
// 	    break;
// 	  case tvt::TV_FALSE:
// 	    formula << getLitName(x_ap) << ":0" << std::endl;
// 	    break;
// 	  case tvt::TV_UNKNOWN:
// 	    formula << getLitName(x_ap) << ":UNKNOWN" << std::endl;
// 	    break;
// 	  default: assert(false);
// 	  }
// 	}
//       }
//     }
//     forall_matchSet(mit, matchSet){
//       forall_match(lit, (**mit)){
// 	if(last_node->GetTransition(*lit)->GetEnvelope()->isCollectiveType()){
// 	  flag = true;
// 	  break;
// 	}	
//       }
//       if(flag){
// 	x_ap = getLiteralFromMatches(*mit, i);
// 	switch(slv->l_get(x_ap).get_value()){ 
// 	case tvt::TV_TRUE:
// 	  formula << getLitName(x_ap) << ":1" << std::endl;
// 	  break;
// 	case tvt::TV_FALSE:
// 	  formula << getLitName(x_ap) << ":0" << std::endl;
// 	  break;
// 	case tvt::TV_UNKNOWN:
// 	  formula << getLitName(x_ap) << ":UNKNOWN" << std::endl;
// 	  break;
// 	default: assert(false);
// 	}
// 	flag = false;
//       }
//     }
//   }
// }


// void Encoding2::Encoding_Mixed()
// {
//   createMatchSet();
//   //  printMatchSet();
//   createMatchLiterals();
//   createEventLiterals();

//   gettimeofday(&constGenStart, NULL);
//   stutteringAtPosition();  
//   uniqueAtPosition();
//   noRepetition();
//   partialOrder();
//   extensionNextPos();
//   noMatchAtNextPos();
//   gettimeofday(&constGenEnd, NULL);
  
//   // std::cout << formula.str();
//   // std::cout << std::endl;
  
//   std::cout << "********* SAT VALUATIONS ************" << std::endl;

//   formula.str("");
//   formula.clear();

//   gettimeofday(&solverStart, NULL);  
//   satcheckt::resultt answer = slv->prop_solve();
//   gettimeofday(&solverEnd, NULL);  

//   formula << "Number of Clauses: " << static_cast<cnf_solvert *>(slv.get())->no_clauses() << std::endl;
//   formula << "Number of Variables: " << slv->no_variables() << std::endl;
//   formula << "Constraint Generation Time: "
// 	  << (getTimeElapsed(constGenStart, constGenEnd)*1.0)/1000000 << "sec \n";
//   formula << "Solving Time: " << (getTimeElapsed(solverStart, solverEnd)*1.0)/1000000 << "sec \n";

//   switch(answer){
//   case satcheckt::P_UNSATISFIABLE:
//     std::cout << "Formula is UNSAT" <<std::endl;
//     break;
//   case satcheckt::P_SATISFIABLE:
//     std::cout << "Formula is SAT -- DEADLOCK DETECTED" <<std::endl;
//     publish();
//     break;
//     // output the valuations
//   default: 
//     std::cout << "ERROR in SAT SOLVING" <<std::endl;
//     break;
//   }
 
//   std::cout << formula.str();
//   std::cout << std::endl;
 
// }
// 
