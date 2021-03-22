#include "SMTEncoding.hpp"
#include <fstream> 
#include <cmath>

#include "simian-core/logger.h"
extern Logger logger;

////////////////////////////////////////////////////////////
/////                                                ///////
////        SMTEncoding                               ///////
////////////////////////////////////////////////////////////
/*
void SMTEncoding::set_width()
{
  width = address_bits();
}

unsigned SMTEncoding::get_width(){
  return width;
}

unsigned SMTEncoding::address_bits()
{
  unsigned res, x=2;
  for(res=1; x<eventSize; res+=1, x*=2);
  return res;
}
*/

enum indices {M, I, CLK, R_CLK, SRC_DEST, TAG, DATA};

expr_vector counter(Encoding::c);

void SMTEncoding::isLitCreatedForCollEvent(Cover* c, expr& m_i, std::string name, int pos)
{
  CB A(c->id, c->index);
  Envelope *envA = last_node->GetTransition(A)->GetEnvelope();
  MatchPtr cMatch = getMPtr(A);
  if(cMatch == NULL)
  {  return; }

  forall_match(lit, (*cMatch)){
    std::map<CB, std::pair<expr_vector, Cover*> >::iterator eit = eventMap.find(*lit);
    if(eit != eventMap.end()){
      if(pos == 0) m_i = (eit->second).first[0];
      else if(pos == 1) m_i = (eit->second).first[1];
      return;
    }
  }
  m_i = new_bool_const(name);
  return;
}

/*
 * This function creates int z3 variables for m, i, tag, src/dest and clk. And put them all in an expr_vector.
 * That expr_vector is inserted in the map with the Cover as the key.
*/
void SMTEncoding::createEventCovers (std::vector<std::pair<int, int> > lineNumbers)
{
  //std::cout << "\nCreate Event Covers";
  fflush(stdout);
  forall_transitionLists(iter, last_node->_tlist){
    forall_transitions(titer, (*iter)->_tlist){

      Envelope *env = (*titer).GetEnvelope();
      //std::cout << "\nbool: " << env->order_id << "\n";
      if(!toBeDropped(env, lineNumbers, ranksIndices, allRanksIndices)) {
        
        // Every boolean variable has a combo of process rank, order, and linenumber in its name
        std::string name = std::to_string(env->id) + "_" + std::to_string(env->index) + "_" + std::to_string(0) + "_" + std::to_string(env->linenumber); 
        // Replaced the order-id of the envelope with 0 in forming the event name: order-id keeps on increasing in interleavings and 
        // makes it difficult to compare two event names belonging to the same event.

        Cover* cover = new Cover(env->id, env->index);
        CB A(env->id, env->index);
        
        if(env->isCollectiveType()) 
        {
          cover->collectiveEvent = true;
          name = "C_" + name;
          expr i = new_bool_const(name + "_i");
          expr m = new_bool_const(name + "_m");
          expr clk = new_int_const(name + "_clk");
          expr r_clk = new_int_const(name + "_rclk");
          s->add(clk >= 0);
          s->add(r_clk >= 0);
          //s->add(clk >= r_clk);
          isLitCreatedForCollEvent(cover, m, name+"_m", 0);
          isLitCreatedForCollEvent(cover, i, name+"_i", 1);
          expr_vector vec(c);
          vec.push_back(m);
          vec.push_back(i);
          vec.push_back(clk);
          vec.push_back(r_clk);
          revEventMap.insert(std::pair<std::pair<expr_vector, Cover*>, CB> (std::pair<expr_vector, Cover*> (vec, cover), A));
          eventMap.insert(std::pair<CB, std::pair<expr_vector, Cover*> > (A, std::pair<expr_vector, Cover*>(vec, cover)));
          eventSize++;
        }

        else if(env->isSendType())
        {
          name = "S_" + name;
          cover->send = true;
          cover->dest = env->id;
          cover->tag = env->stag;
          cover->data = env->data;
          expr data = new_int_const(name + "_data");
          s->add(data == env->data);
          expr dest = new_int_const(name + "_dest");
          //s->add(dest == env->id);
          expr tag = new_int_const(name + "_tag");
          s->add(tag == env->stag);
          expr i = new_bool_const(name + "_i");
          expr m = new_bool_const(name + "_m");
          expr clk = new_int_const(name + "_clk");
          expr r_clk = new_int_const(name + "_rclk");
          expr bufferUsed = new_bool_const(name + "_bufferUsed");
          s->add(clk >= 0);
          s->add(r_clk >= 0);
          //s->add(clk >= r_clk);
          expr_vector vec(c);
          vec.push_back(m);
          vec.push_back(i);
          vec.push_back(clk);
          vec.push_back(r_clk);
          vec.push_back(dest);
          vec.push_back(tag);
          vec.push_back(data);
          vec.push_back(bufferUsed);
          revEventMap.insert(std::pair<std::pair<expr_vector, Cover*>, CB> (std::pair<expr_vector, Cover*> (vec, cover), A));
          eventMap.insert(std::pair<CB, std::pair<expr_vector, Cover*> > (A, std::pair<expr_vector, Cover*>(vec, cover)));
          eventSize++;
        }

        else if(env->isRecvType())
        {
          name = "R_" + name;
          cover->recv = true;
          cover->src = env->src;
          cover->tag = env->rtag;
          expr data = new_int_const(name + "_data");
          expr src = new_int_const(name + "_src");
          //if(env->src != -1) s->add(src == env->src);
          expr tag = new_int_const(name + "_tag");
          if(env->rtag != -1) s->add(tag == env->rtag);
          expr i = new_bool_const(name + "_i");
          expr m = new_bool_const(name + "_m");
          expr clk = new_int_const(name + "_clk");
          expr r_clk = new_int_const(name + "_rclk");
          s->add(clk >= 0);
          s->add(r_clk >= 0);
          //s->add(clk >= r_clk);
          expr_vector vec(c); 
          vec.push_back(m);
          vec.push_back(i);
          vec.push_back(clk);
          vec.push_back(r_clk);
          vec.push_back(src);
          vec.push_back(tag);
          vec.push_back(data);
          revEventMap.insert(std::pair<std::pair<expr_vector, Cover*>, CB> (std::pair<expr_vector, Cover*> (vec, cover), A));
          eventMap.insert(std::pair<CB, std::pair<expr_vector, Cover*> > (A, std::pair<expr_vector, Cover*>(vec, cover)));
          eventSize++;
        }

        else if(env->func_id == FINALIZE) continue;

        else // Example: waitany
        {
          name = "W_" + name;
          expr i = new_bool_const(name + "_i");
          expr m = new_bool_const(name + "_m");
          expr clk = new_int_const(name + "_clk");
          expr r_clk = new_int_const(name + "_rclk");
          s->add(clk >= 0);
          s->add(r_clk >= 0);
          //s->add(clk >= r_clk);
          expr_vector vec(c);
          vec.push_back(m);
          vec.push_back(i);
          vec.push_back(clk);
          vec.push_back(r_clk);
          revEventMap.insert(std::pair<std::pair<expr_vector, Cover*>, CB> (std::pair<expr_vector, Cover*> (vec, cover), A));
          eventMap.insert(std::pair<CB, std::pair<expr_vector, Cover*> > (A, std::pair<expr_vector, Cover*>(vec, cover)));
          eventSize++;
        }

        cover->name = name;
      }
    }
  }
  //std::cout << "\nCreate Event Covers";
}

void SMTEncoding::createOrderLiterals() // Creating clk literals
{
 forall_transitionLists(iter, last_node->_tlist){
    forall_transitions(titer, (*iter)->_tlist){
      if(!toBeDropped((*titer).GetEnvelope(), lineNumbers, ranksIndices, allRanksIndices)) {
        if((*titer).GetEnvelope()->func_id == FINALIZE) continue;
        if((*titer).GetEnvelope()->isCollectiveType()) continue;
        
        Envelope *env = (*titer).GetEnvelope();
        CB A (env->id, env->index); 
        expr a = new_int_const();
        bvEventMap.insert(std::pair<CB, expr>(A, a));
        revBVEventMap.insert(std::pair<expr, CB>(a, A));
      }
    }
  }
  bool flag = false;
  forall_matchSet(mit, matchSet){
    forall_match(lit, (**mit)){
      if(last_node->GetTransition(*lit)->GetEnvelope()->isCollectiveType()){
        flag = true; 
        break;
      }
    }
    if(flag){
      expr e = new_int_const(); 
      collBVMap.insert(std::pair<MatchPtr, expr>((*mit), e));
      revCollBVMap.insert(std::pair<expr, MatchPtr>(e, (*mit)));
      flag = false;
    }
  }
}

/*
void SMTEncoding::printEventMap()
{
  formula << "**** EventMap*****" << std::endl; 
  forall_transitionLists(iter, last_node->_tlist){
    forall_transitions(titer, (*iter)->_tlist){
      Envelope *env = (*titer).GetEnvelope();
      if(env->func_id == FINALIZE) continue;
      CB A (env->id, env->index);
      literalt m = eventMap.find(A)->second.first;
      literalt i = eventMap.find(A)->second.second;
      formula << A << " " << m.get() << "," << i.get() << std::endl;
      formula << "\t *** RevEventMap *** " << std::endl;
      formula << "\t" << m.get() << "--" << revEventMap.find(m)->second <<std::endl;
    }
  }
}
*/

void SMTEncoding::createMatchLiterals()
{
  //std::cout << "\nCreating Match Literals";
  fflush(stdout);
  expr comment = new_bool_const("Creating Match Literals");
  s->add(comment);

  std::stringstream uniquepair;
  std::string matchString;

  forall_matchSet(mit, matchSet)
  {    
    CB A = (**mit).front();
    Envelope* sEnv = last_node->GetTransition(A)->GetEnvelope();
    if(sEnv->isSendType())
    {
      A = (**mit).back();
      Envelope* rEnv = last_node->GetTransition(A)->GetEnvelope();
      if(!toBeDropped(rEnv, lineNumbers, ranksIndices, allRanksIndices) && !toBeDropped(sEnv, lineNumbers, ranksIndices, allRanksIndices)) 
      {
        uniquepair << "s_";
        forall_match(lit, (**mit))
        {
          // match literals only for send and receive
          Envelope* sEnv = last_node->GetTransition(*lit)->GetEnvelope();
          uniquepair << (*lit)._pid;
          uniquepair << (*lit)._index;
        }
        matchString = uniquepair.str();
        if(matchString.size())
        {
          expr s = new_bool_const(matchString);

          //insert in to the map
          matchMap.insert(std::pair<std::string, expr> (matchString, s));
          //revMatchMap.insert(std::pair<expr, std::pai<CB, CB> > (s_m, matchNumeral));
          match2exprMap.insert(std::pair<MatchPtr, expr> (*mit, s));

          // clear out uniquepair
          uniquepair.str("");
          uniquepair.clear();
        }
      }
    }
  }
}

/*
literalt SMTEncoding::getClkLiteral(CB A, CB B)
{
  Envelope *envA , *envB;
  envA = last_node->GetTransition(A)->GetEnvelope();
  envB = last_node->GetTransition(B)->GetEnvelope();
  
  bvt Abv, Bbv;
  Abv = getEventBV(A);
  Bbv = getEventBV(B);
  
  if(!envA->isCollectiveType() && !envB->isCollectiveType()){
    std::map<std::pair<CB, CB>, literalt >::iterator res;
    res = clkMap.find(std::pair<CB, CB> (A,B));
    if(res != clkMap.end())
      return res->second;
    else {
      literalt c_ab = bvUtils->unsigned_less_than(Abv, Bbv);
      insertClockEntriesInMap(A, B, c_ab);
      return c_ab;
    }
  }
  else if (envA->isCollectiveType() && !envB->isCollectiveType()){
    std::map<std::pair<MatchPtr, CB>, literalt >::iterator res;
    MatchPtr Aptr = getMPtr(A);
    assert (Aptr != NULL);
    res = clkMapCollEvent.find(std::pair<MatchPtr, CB> (Aptr,B));
    if(res != clkMapCollEvent.end())
      return res->second;
    else{
      literalt c_ab = bvUtils->unsigned_less_than(Abv, Bbv);
      insertClockEntriesInMap(A, B, c_ab);
      return c_ab;
    }
  }
  else if(!envA->isCollectiveType() && envB->isCollectiveType()){
    std::map<std::pair<CB, MatchPtr>, literalt >::iterator res;
    MatchPtr Bptr = getMPtr(B);
    assert (Bptr != NULL);
    res = clkMapEventColl.find(std::pair<CB, MatchPtr> (A,Bptr));
    if(res != clkMapEventColl.end())
      return res->second;
    else{
      literalt c_ab = bvUtils->unsigned_less_than(Abv, Bbv);
      insertClockEntriesInMap(A, B, c_ab);
      return c_ab;
    }
  }
  else{
    std::map<std::pair<MatchPtr, MatchPtr>, literalt >::iterator res;
    MatchPtr Aptr = getMPtr (A);
    MatchPtr Bptr = getMPtr(B);
    assert (Aptr != NULL);
    assert (Bptr != NULL);
    res = clkMapCollColl.find(std::pair<MatchPtr, MatchPtr> (Aptr,Bptr));
    if(res != clkMapCollColl.end())
      return res->second;
    else{
      literalt c_ab = bvUtils->unsigned_less_than(Abv, Bbv);
      insertClockEntriesInMap(A, B, c_ab);
      return c_ab;
    }
  }
}
*/

std::pair<expr, expr> SMTEncoding::getMILiteral(CB A)
{
  Envelope *envA = last_node->GetTransition(A)->GetEnvelope();
  expr_vector vec = (eventMap.find(A)->second).first;
  std::pair<expr, expr> p = std::pair<expr, expr>(vec[0], vec[1]); // Returning a pair of vars 'm' and 'i'
  return p;
}

expr_vector SMTEncoding::getMatchExpr(CB A)
{
  std::map<CB, std::pair<expr_vector, Cover*> >::iterator eventIt = eventMap.find(A);
  assert(eventIt != eventMap.end());
  return (eventIt->second).first;
}

MatchPtr SMTEncoding::getMPtr(CB A) 
{
  bool flag = false;
  forall_matchSet(mit, matchSet){
    forall_match(lit, (**mit)){
      if((*lit) == A) {
	     flag = true;
	     break;
      }
    }
    if(flag){
      return (*mit);
    }
  }
  return NULL;
}

/*
std::string SMTEncoding::getClkLitName(literalt lt, CB A, CB B)
{
  Envelope *envA, *envB;
  envA = last_node->GetTransition(A)->GetEnvelope();
  envB = last_node->GetTransition(B)->GetEnvelope();
  
  std::stringstream ss;
  
  if (!envA->isCollectiveType() && !envB->isCollectiveType()){
    std::pair <CB, CB> p = revClkMap.find(lt)->second;
    ss << "C_" << p.first._pid << p.first._index << "_" 
       << p.second._pid << p.second._index;
    return ss.str();
  }
  else if(envA->isCollectiveType() && !envB->isCollectiveType()){
    std::pair<MatchPtr, CB>  p = revClkMapCollEvent.find(lt)->second;
    ss << "C_";
    forall_match(lit, (*(p.first))){
      ss <<  (*lit)._pid << (*lit)._index;
    }
    ss<< "_" << p.second._pid << p.second._index;
    return ss.str();
  }
  else if(!envA->isCollectiveType() && envB->isCollectiveType()){
   std::pair<CB, MatchPtr>  p = revClkMapEventColl.find(lt)->second;
    ss << "C_" << p.first._pid << p.first._index << "_";
    forall_match(lit, (*(p.second))){
      ss << (*lit)._pid << (*lit)._index;
    }
    return ss.str();
  }
  else{
    std::pair<MatchPtr, MatchPtr> p = revClkMapCollColl.find(lt)->second;
    ss << "C_";
    forall_match(lit, (*(p.first))){
      ss << (*lit)._pid << (*lit)._index;
    }
    ss << "_";
    forall_match(lit, (*(p.second))){
      ss << (*lit)._pid << (*lit)._index;
    }
    return ss.str();
  }
}

std::string SMTEncoding::getLitName(literalt lt, int type)
{
  
  std::stringstream ss;
  switch(type){
  case 0:{
    ss << "S_" << revMatchMap.find(lt)->second;
    return ss.str();
  }
    
  case 1:{
    CB A = revEventMap.find(lt)->second;
    ss << "M_" << A._pid << A._index;
    return ss.str();  
  }
    
  case 2: {
    MatchPtr Aptr = revCollMap.find(lt)->second;
    ss << "M_";
    forall_match(lit, (*Aptr)){
      ss << (*lit)._pid  << (*lit)._index;
    }
    return ss.str();
    
  }
  case 3: {
    CB A = revEventMap.find(lt)->second;
    ss << "I_" << A._pid  << A._index;
    return ss.str();
  }
  case 4: {
    MatchPtr Aptr = revCollMap.find(lt)->second;
    ss << "I_";
    forall_match(lit, (*Aptr)){
      ss << (*lit)._pid <<(*lit)._index;
    }
    return ss.str();
  }
  default:
    assert(false);
    
  }
}
*/

/*expr SMTEncoding::getEventBV(CB A) 
{
  Envelope *envA = last_node->GetTransition(A)->GetEnvelope(); 

  if(!envA->isCollectiveType()){
    std::map<CB, expr>::iterator it;
    it = bvEventMap.find(A);
    assert(it != bvEventMap.end());
    return it->second;
  }
  else{
    MatchPtr Aptr = getMPtr(A);
    std::map<MatchPtr, expr>::iterator bv_it;
    bv_it = collBVMap.find(Aptr);
    assert(bv_it != collBVMap.end());
    return bv_it->second;
  }
}*/

expr SMTEncoding::getClk(CB A)
{
  Envelope *envA = last_node->GetTransition(A)->GetEnvelope(); 
  std::map<CB, std::pair<expr_vector, Cover*> >::iterator it;
  it = eventMap.find(A);
  assert(it != eventMap.end());

  return getExprAtIndex((it->second).first, CLK);

  /*if(envA->isCollectiveType()){
    return getExprAtIndex((it->second).first, 2); // clk is at index 2 in collective Covers
  }
  else if(envA->isSendType() || envA->isRecvType())
  {
    return getExprAtIndex((it->second).first, CLK);
  }
  else{
    return getExprAtIndex((it->second).first, 2); 
  }*/
}

expr SMTEncoding::getExprAtIndex(expr_vector e, int index)
{
  return e[index];
}

/*
void SMTEncoding::insertClockEntriesInMap(CB B, CB A,  literalt c_ba)
{
  Envelope * envA = last_node->GetTransition(A)->GetEnvelope();
  Envelope * envB = last_node->GetTransition(B)->GetEnvelope();

  if(!envA->isCollectiveType() && !envB->isCollectiveType()){
    clkMap.insert(std::pair< std::pair<CB,CB>, literalt> (std::pair<CB, CB> (B,A), c_ba));
    revClkMap.insert(std::pair<literalt, std::pair<CB, CB> > (c_ba,std::pair<CB, CB> (B,A)));
  }
  else if (!envA->isCollectiveType() && envB->isCollectiveType()){
    MatchPtr Bptr = getMPtr(B);
    assert (Bptr != NULL);
    clkMapCollEvent.insert(std::pair< std::pair<MatchPtr,CB>, literalt> (std::pair<MatchPtr, CB>
									 (Bptr,A), c_ba));
    revClkMapCollEvent.insert(std::pair<literalt, std::pair<MatchPtr, CB> > 
			      (c_ba,std::pair<MatchPtr, CB> (Bptr,A)));
  } 
  else if(envA->isCollectiveType() && !envB->isCollectiveType()){
    MatchPtr Aptr = getMPtr(A);
    assert (Aptr != NULL);
    clkMapEventColl.insert(std::pair< std::pair<CB ,MatchPtr>, literalt>
			   (std::pair<CB ,MatchPtr> (B,Aptr), c_ba));
    revClkMapEventColl.insert(std::pair<literalt, std::pair<CB, MatchPtr> > 
			      (c_ba, std::pair<CB, MatchPtr> (B,Aptr)));
  }
  else { // both A and B are collectives
    MatchPtr Aptr = getMPtr(A);
    MatchPtr Bptr = getMPtr(B);
    clkMapCollColl.insert(std::pair< std::pair<MatchPtr ,MatchPtr>, literalt> 
			  (std::pair<MatchPtr ,MatchPtr> (Bptr,Aptr), c_ba));
    revClkMapCollColl.insert(std::pair<literalt, std::pair<MatchPtr, MatchPtr> > 
			     (c_ba, std::pair<MatchPtr, MatchPtr> (Bptr,Aptr)));
    
  }
}
*/

bool SMTEncoding::isNecessaryEdgeRequired(Envelope* envA, Envelope* envB)
{
  // If both are Send functions, and the destination of both functions are different, 
  // then there does not need to be an edge between them (yet).

  if(envA->display_name=="Send" && envB->display_name=="Send" && envA->dest!=envB->dest && Scheduler::_kbuffer>0)
  {
    return false;
  }

  return true;
}

void SMTEncoding::partialOrderConstraints()
{
  // Will add the partial order constraints where it is sure to have the clock-order specified.
  //std::cout << "\nPartial Order Constraints";
  fflush(stdout);
  expr comment = new_bool_const("Partial Order Constraints");
  s->add(comment);
  forall_transitionLists(iter, last_node->_tlist){
    forall_transitions(titer, (*iter)->_tlist){
      Envelope * envA = (*titer).GetEnvelope();
      if(!toBeDropped(envA, lineNumbers, ranksIndices, allRanksIndices)) {
        if(envA->func_id == FINALIZE) continue;
        CB A(envA->id, envA->index);
        for(std::vector<int>::iterator vit = (*titer).get_ancestors().begin(); vit != (*titer).get_ancestors().end(); vit ++)
        {
          	CB B (envA->id, *vit);
          	Envelope * envB = last_node->GetTransition(B)->GetEnvelope();
          	if(envB->func_id == FINALIZE) continue;
          	if(isNecessaryEdgeRequired(envA, envB)) // For k-buffering
          	{
              expr orderA = getClk(A);
          	  expr orderB = getClk(B);
              s->add(orderB < orderA); // PPO constraint
            }

          	/*literalt c_ba = bvUtils->unsigned_less_than(Bbv, Abv);
          	insertClockEntriesInMap(B, A, c_ba);
          	slv->l_set_to(c_ba, true);
            */
        }
      }
    }
  }

  // Order of send and receive in the matching pair are same
  /*bool flag = false;
  forall_matchSet(mit, matchSet){
    forall_match(lit, (**mit)){
      if(last_node->GetTransition(*lit)->GetEnvelope()->isSendType() ||
         last_node->GetTransition(*lit)->GetEnvelope()->isRecvType()){
            flag = true; 
            break;
      }
    }

    if(flag){ // hoping it to be a send-receive match
      
      flag = false;
      
      assert((**mit).size() == 2);
      
      CB A = (**mit).front();
      CB B = (**mit).back();
      
      expr orderA(c);
      expr orderB(c);
      
      orderA = getClk(A);
      orderB = getClk(B);

      s->add(orderA == orderB); 
    }
  }*/
}

void SMTEncoding::matchCollectives()
{
  //std::cout << "\nMatch Collectives";
  fflush(stdout);
  expr comment = new_bool_const("Match Collectives");
  s->add(comment);

  forall_matchSet(mit, matchSet)
  {
    CB A = (**mit).front();
    if(last_node->GetTransition(A)->GetEnvelope()->isCollectiveType()) 
    {
      expr_vector clauses(c);
      expr_vector s_n = getMatchExpr(A);
      forall_match(lit, (**mit))
      {
        if(*lit != A)
        {
          expr_vector s_m = getMatchExpr(*lit);
          clauses.push_back(s_n[2] == s_m[2]);
        }
      }
      array<Z3_ast> args(clauses);
      Z3_ast r = Z3_mk_and(c, args.size(), args.ptr());
      expr res(c, r);
      s->add(res);
    }
  }
}

void SMTEncoding::matchPairs()
{
  //std::cout << "\nMatch Pairs";
  fflush(stdout);
  expr comment = new_bool_const("Match Pairs");
  s->add(comment);

  forall_matchSet(mit, matchSet)
  {
    CB send = (**mit).front();
    CB recv = (**mit).back();

    if(last_node->GetTransition(recv)->GetEnvelope()->isRecvTypeOnly())
    {
      expr_vector s_n = getMatchExpr(recv);
      expr_vector s_m = getMatchExpr(send);

      expr eq1 = (s_n[2] == s_m[2]); //clk
      expr eq2 = (s_n[6] == s_m[6]); //data
      expr eq3 = (s_n[5] == s_m[5]); //tag
      expr S = getMatchLiteral(*mit);

      s->add(implies(S, eq1));
      s->add(implies(S, eq2));
      s->add(implies(S, eq3));

      Cover* crecv = (eventMap.find(recv)->second).second;

      //if (crecv->src == -1 || crecv->tag == -1)
      //{
        expr_vector clauses(c);
        assert((**mit).size() == 2);
        
        std::set<CB> image = _m->MImage(recv);

        for(std::set<CB>::iterator sit = image.begin(); sit != image.end(); sit++) 
        {
          expr_vector s_m = getMatchExpr(*sit);
          expr_vector equation(c);
          //equation.push_back(s_n[2] == s_m[2]); //dest-src
          equation.push_back(s_n[5] == s_m[5]); //tag
          //equation.push_back(s_n[4] == s_m[4]); //clk
          equation.push_back(s_n[6] == s_m[6]); //data

          array<Z3_ast> meet(equation);
          Z3_ast ands = Z3_mk_and(c, meet.size(), meet.ptr());
          expr onepair(c, ands);
          
          clauses.push_back(onepair);
        }
        if(!clauses.empty())
        {
          array<Z3_ast> uni(clauses);
          Z3_ast ors = Z3_mk_or(c, uni.size(), uni.ptr());
          expr allClauses(c, ors);
          s->add(allClauses);
        }
      //}
    }
  }
}

expr SMTEncoding::getMatchLiteral(MatchPtr mptr)
{
  return match2exprMap.find(mptr)->second;
}

void SMTEncoding::createRFSomeConstraint()
{
  //std::cout << "\nRF Some Constraint";
  expr comment = new_bool_const("RF Some Constraint");
  s->add(comment);

  bool flag = false;
  forall_transitionLists(iter, last_node->_tlist)
  {
    forall_transitions(titer, (*iter)->_tlist)
    {
      Envelope* envA = titer->GetEnvelope();
      if(envA->func_id == FINALIZE) continue;
      CB A(envA->id, envA->index);
      expr_vector vars = getMatchExpr(A);
      expr_vector rhs(c); 
      expr m = vars[0];
      if(envA->isRecvType() || envA->isSendType()) 
      {
        forall_matchSet(mit, matchSet)
        {
          forall_match(lit, (**mit))
          {
            if((*lit) == A) 
            {
              flag = true; 
              break;
            }
          }
          if(flag)
          {
            expr S = getMatchLiteral(*mit);
            rhs.push_back(S);
            flag = false; 
          }
        }
        if(!rhs.empty())
        {
          array<Z3_ast> uni(rhs);
          Z3_ast ors = Z3_mk_or(c, uni.size(), uni.ptr());
          expr orsOfS(c, ors);
          s->add(implies(m, orsOfS));
          rhs.empty();
        }
        else 
        {
          /* It is possible that m_a has no match because while checking symbolically, 
             if some other condition can be satisfied we delete some calls */
          // assert(false); // m_a has no match -- NOT POSSIBLE! -- dhriti
        }
      }
    }
  }
}

void SMTEncoding::createMatchConstraint()
{
  //std::cout << "\nMatched Only";
  expr comment = new_bool_const("Matched only");
  s->add(comment);

  bool flag = false;

  forall_matchSet(mit, matchSet)
  {
    expr m(c);
    expr_vector rhs(c);
    if(last_node->GetTransition((**mit).front())->GetEnvelope()->isSendType())
    {
      forall_match(lit, (**mit))
      {
        expr_vector vars = getMatchExpr(*lit);
        m = vars[0];
        rhs.push_back(m);
      }
    }
    if(!rhs.empty())
    {
      expr S = getMatchLiteral(*mit);
      array<Z3_ast> uni(rhs);
      Z3_ast ms = Z3_mk_and(c, uni.size(), uni.ptr());
      expr ands(c, ms);
      s->add(implies(S, ands));
    }
  }
}

void SMTEncoding::atLeastOneSend()
{
  //std::cout << "\nAt Least One Send";
  fflush(stdout);
  expr comment = new_bool_const("atLeastOneSend");
  s->add(comment);

  expr s_m(c);
    
  forall_matchSet(mit, matchSet)
  {
    expr_vector rhs(c);
    CB send = (**mit).front(); // assuming only send-recv matches exist
    CB recv = (**mit).back();
    if(last_node->GetTransition(send)->GetEnvelope()->isSendType()) 
    {
      s_m = getMatchLiteral(*mit);
      rhs.push_back(s_m);
      assert((**mit).size() == 2);
      std::set<CB> image = _m->MImage(send);
      for(std::set<CB>::iterator sit = image.begin(); sit != image.end(); sit++)
      {
        if(recv != (*sit))
        {
          std::stringstream ss;
          ss << "s_" << send._pid << send._index << (*sit)._pid << (*sit)._index;
          expr s_n = matchMap.find(ss.str())->second;
          rhs.push_back(s_n);
        }
      }
      if(!rhs.empty())
      {
        array<Z3_ast> uni(rhs);
        Z3_ast ms = Z3_mk_or(c, uni.size(), uni.ptr());
        expr ors(c, ms);
        s->add(ors);
      }
    }
  }  
}

void SMTEncoding::uniqueMatchSend()
{
  //std::cout << "\nUnique Match Send";
  fflush(stdout);
  expr comment = new_bool_const("Unique Match Send");
  s->add(comment);

  expr s_m(c);
  forall_matchSet(mit, matchSet)
  {
    CB send = (**mit).front(); // assuming only send-recv matches exist
    CB recv = (**mit).back();
    if(last_node->GetTransition(send)->GetEnvelope()->isSendType()) 
    {
      s_m = getMatchLiteral(*mit);
      assert((**mit).size() == 2);
      std::set<CB> image = _m->MImage(send);
      for(std::set<CB>::iterator sit = image.begin(); sit != image.end(); sit++)
      {
        if(recv != (*sit))
        {
          std::stringstream ss;
          ss << "s_" << send._pid << send._index << (*sit)._pid << (*sit)._index;
          expr s_n = matchMap.find(ss.str())->second;
          s->add(implies(s_m, !s_n));
        }
      }
    }
  }
}

void SMTEncoding::uniqueMatchRecv()
{
  //std::cout << "\nUnique Match Receive";
  fflush(stdout);
  expr comment = new_bool_const("Unique Match Receive");
  s->add(comment);

  expr s_m(c);
  forall_matchSet(mit, matchSet)
  {
    CB send = (**mit).front(); // assuming only send-recv matches exist
    CB recv = (**mit).back();
    if(last_node->GetTransition(recv)->GetEnvelope()->isRecvTypeOnly()) 
    {
      s_m = getMatchLiteral(*mit);
      assert((**mit).size() == 2);
      std::set<CB> image = _m->MImage(recv);
      for(std::set<CB>::iterator sit = image.begin(); sit != image.end(); sit++)
      {
        if(send != (*sit))
        {
          std::stringstream ss;
          ss << "s_" << (*sit)._pid << (*sit)._index << recv._pid << recv._index;
          expr s_n = matchMap.find(ss.str())->second;
          s->add(implies(s_m, !s_n));
        }
      }
    }
  }
}

void SMTEncoding::noMoreMatchesPossible()
{
  //std::cout << "\nNo More Matches Possible";
  expr comment = new_bool_const("No More Matches Possible");
  s->add(comment);
  forall_matchSet(mit, matchSet){
    expr_vector vec(c);
    forall_match(lit, (**mit)){
      std::pair<expr,expr> p = getMILiteral(*lit);
      if(last_node->GetTransition(*lit)->GetEnvelope()->isSendType() ||
         last_node->GetTransition(*lit)->GetEnvelope()->isRecvType()){  // Why done only for collectives
            vec.push_back(p.first);
            vec.push_back(!p.second);
      }
    }
    if(!vec.empty()){
      array<Z3_ast> args(vec);
      Z3_ast r = Z3_mk_or(c, vec.size(), args.ptr());
      expr res(c, r);
      //s->add(implies(res, true));
      s->add(res);
    }
  }
}

void SMTEncoding::allAncestorsMatched()
{
  //std::cout << "\nAll Ancestors Matched";
  expr comment = new_bool_const("All Ancestors Matched");
  s->add(comment);
  forall_transitionLists(iter, last_node->_tlist){
    forall_transitions(titer, (*iter)->_tlist){
      expr_vector rhs(c);
      Envelope *envA = titer->GetEnvelope();
      if(!toBeDropped(envA, lineNumbers, ranksIndices, allRanksIndices)) {
        if(envA->func_id == FINALIZE) continue;
        CB A (envA->id, envA->index);
        std::pair<expr, expr> p = getMILiteral(A);
        expr m = p.first;
        expr i = p.second;
        
        std::vector<int> ancs = last_node->GetTransition(A)->get_ancestors();
        for(std::vector<int>::iterator vit = ancs.begin(); vit != ancs.end(); vit++){
          CB B(envA->id, *vit);
          Envelope * envB = last_node->GetTransition(B)->GetEnvelope();
          if(isNecessaryEdgeRequired(envA, envB)) // For k-buffering
          {
            std::pair<expr, expr> q = getMILiteral(B);
            rhs.push_back(q.first);
          }
        }
        if(!rhs.empty()){
          array<Z3_ast> args(rhs);
          Z3_ast r = Z3_mk_and(c, args.size(), args.ptr());
          expr res(c, r);
          expr conjecture1 = implies(res, i);
          expr conjecture2 = implies(i, res);
          s->add(conjecture1 && conjecture2);
        }
        else {
          expr one = c.bool_val(true);
          expr conjecture1 = implies(one, i);
          expr conjecture2 = implies(i, one);
          s->add(conjecture1 && conjecture2);
        }
      }
    }
  }
}

void SMTEncoding::matchImpliesIssued()
{
  //std::cout << "\nMatch Implies Issued";
  expr comment = new_bool_const("Match Implies Issued");
  s->add(comment);
  forall_transitionLists(iter, last_node->_tlist){
    forall_transitions(titer, (*iter)->_tlist){
      Envelope *envA = (*titer).GetEnvelope();
      if(!toBeDropped(envA, lineNumbers, ranksIndices, allRanksIndices)) {
        if(envA->func_id == FINALIZE) continue;
        if(!envA->isCollectiveType()){
        //if(!envA->isBarrier()){ // Unless a call is a Barrier call, m->i
          CB A(envA->id, envA->index);
          expr m = getMILiteral(A).first;
          expr i = getMILiteral(A).second;
          s->add(implies(m, i));
        }
      }
    }
  }
  forall_matchSet(mit, matchSet){
    CB A = (**mit).front();
    Envelope *front = last_node->GetTransition(A)->GetEnvelope();
    //if(front->isCollectiveType() && front->func.compare("Barrier")==0) // Only in case of Barriers, we assume synchronising effect
    if(front->isCollectiveType())
    {
      expr m_match(c);
      expr_vector i_evts(c);
      std::map<CB, std::pair<expr_vector, Cover*> >::iterator eit = eventMap.find(A);
      if(eit != eventMap.end()){
        m_match = (eit->second).first[0];
        forall_match(lit, (**mit)){
          i_evts.push_back(getMILiteral(*lit).second);
        }
        array<Z3_ast> args(i_evts);
        Z3_ast r = Z3_mk_and(c, args.size(), args.ptr());
        expr res(c, r);
        expr conjecture1 = implies(res, m_match); // In the event of barrier matching, match implies all the calls' 'r' and vice-versa
        expr conjecture2 = implies(m_match, res);
        s->add(conjecture1 && conjecture2);
      }
      else
        assert(false);
    }
  }
}

void SMTEncoding::notAllMatched()
{
  //std::cout << "\nNot All Matched";
  expr comment = new_bool_const("Not All Matched");
  s->add(comment);

  expr_vector vec(c);
  forall_transitionLists(iter, last_node->_tlist){
    forall_transitions(titer, (*iter)->_tlist){
      Envelope *envA = titer->GetEnvelope();
      if(!toBeDropped(envA, lineNumbers, ranksIndices, allRanksIndices)) {
        if(envA->func_id == FINALIZE) continue;
        CB A(envA->id, envA->index);
        expr m_a = getMILiteral(A).first;
        //s->add(!m_a);
        vec.push_back(!m_a);
      }
    }
  }
  array<Z3_ast> args(vec);
  Z3_ast r = Z3_mk_or(c, args.size(), args.ptr());
  expr res(c, r);
  s->add(res);
}

void SMTEncoding::waitMatch()
{
  //std::cout << "\nWait Match";
  expr comment = new_bool_const("Wait Match");
  s->add(comment);

  expr_vector rhs(c);
  forall_transitionLists(iter, last_node->_tlist){
    forall_transitions(titer, (*iter)->_tlist){
      Envelope *envA = titer->GetEnvelope();
      if(!toBeDropped(envA, lineNumbers, ranksIndices, allRanksIndices)) {
        if(envA->func_id == FINALIZE) 
          continue;
        CB A(envA->id, envA->index);
        std::pair<expr, expr> p = getMILiteral(A);
        expr m_a = p.first;
        if(envA->func_id == WAIT || envA->func_id == WAITALL) 
        {
          std::vector<int> ancs = titer->get_ancestors();
          for(std::vector<int>::iterator sit = ancs.begin(); sit != ancs.end(); sit++)
          {
            CB B(envA->id, *sit);
            Envelope *envB = last_node->GetTransition(B)->GetEnvelope();
            if(envB->func_id == FINALIZE) continue;
            expr m_b = getMILiteral(B).first;
            rhs.push_back(m_b);
          }
          if(!rhs.empty())
          {
            array<Z3_ast> args(rhs);
            Z3_ast r = Z3_mk_and(c, args.size(), args.ptr());
            expr res(c, r);
            expr conjecture1 = implies(res, m_a);
            expr conjecture2 = implies(m_a, res);
            s->add(conjecture1 && conjecture2);
          }
          else 
          {
            expr one = c.bool_val(true);
            expr conjecture1 = implies(one, m_a);
            expr conjecture2 = implies(m_a, one);
            s->add(conjecture1 && conjecture2);
          }
        }
      }
    }
  }
}

void SMTEncoding::displayMatchSet()
{
  std::cout << "**********MATCHSET*************" << std::endl;
  std::cout << "MatchSet Size =" << matchSet.size() << std::endl;
  forall_matchSet(mit, matchSet){
    forall_match(lit, (**mit)){
      std::cout << (*lit);
    }
    std::cout << std::endl;
  }
}

void SMTEncoding::conditionalMatchesBefore()
{
  //std::cout << "\nConditional Matches Before";
  expr comment = new_bool_const("Conditional Matches Before part 1");
  s->add(comment);
  forall_transitionLists(iter, last_node->_tlist){
    forall_transitions(titer, (*iter)->_tlist){
      Envelope *envA = (*titer).GetEnvelope();
      if(!toBeDropped(envA, lineNumbers, ranksIndices, allRanksIndices)) {
        if(envA->func_id == FINALIZE) continue;
        if(!envA->isCollectiveType()){
          CB A(envA->id, envA->index);
          expr_vector exprVec = getMatchExpr(A);
          expr rclk = exprVec[3];
          expr i = exprVec[1];
          
          std::vector<int> ancs = last_node->GetTransition(A)->get_ancestors();
          CB B(envA->id, ancs[ancs.size()-1]);
          exprVec = getMatchExpr(B);
          expr ancClk = exprVec[2];
          
          expr clkset = rclk == ancClk+1;
          s->add(implies(i, clkset));
        }
      }
    }
  }

  // Creating variables p_ab and q_ab
  std::stringstream pab, qab;
  std::string matchString;

  for(int i=0; i<Eset.size(); ++i)
  {    
    Envelope* recv1 = Eset[i].first;
    Envelope* recv2 = Eset[i].second;
    CB A(recv1->id, recv1->index);
    CB B(recv2->id, recv2->index);
    pab << "p_";
    qab << "q_";
    pab << recv1->id;
    pab << recv1->index;
    pab << recv2->id;
    pab << recv2->index;
    qab << recv1->id;
    qab << recv1->index;
    qab << recv2->id;
    qab << recv2->index;
      
    matchString = pab.str();
    if(matchString.size())
    {
      expr p = new_bool_const(matchString);
      //insert in to the map
      PMap.insert(std::pair<std::string, expr> (matchString, p));
      revPMap.insert(std::pair<std::pair<CB, CB>, expr> (std::pair<CB, CB>(A, B), p));

      matchString = "";
      matchString = qab.str();
      expr q = new_bool_const(matchString);
      //insert in to the map
      QMap.insert(std::pair<std::string, expr> (matchString, q));
      revQMap.insert(std::pair<std::pair<CB, CB>, expr> (std::pair<CB, CB>(A, B), q));

      // clear out uniquepair
      pab.str("");
      pab.clear();
      qab.str("");
      qab.clear();
    }
  }

  comment = new_bool_const("Conditional Matches Before part 2");
  s->add(comment);
  for(int i=0; i<Eset.size(); ++i)
  {
    Envelope* recv1 = Eset[i].first;
    Envelope* recv2 = Eset[i].second;
    CB A(recv1->id, recv1->index);
    CB B(recv2->id, recv2->index);

    expr_vector rhs1(c);
    expr_vector rhs2(c);
    forall_matchSet(mit, matchSet)
    {
      if((**mit).back() == B) 
      {
        CB C = (**mit).front();
        expr_vector exprVecA = getMatchExpr(A);
        expr_vector exprVecB = getMatchExpr(B);
        expr_vector exprVecC = getMatchExpr(C); 
        expr ex1 = (exprVecC[3] > exprVecA[3]);

        forall_matchSet(mit, matchSet)
        {
          if((**mit).back() == A)
          {
            CB D = (**mit).front();
            expr_vector exprVecD = getMatchExpr(D); 
            expr ex2 = (exprVecC[3] > exprVecD[3]);
            rhs2.push_back(ex2);
          }
        }
        if(!rhs2.empty())
        {
          array<Z3_ast> uni(rhs2);
          Z3_ast ms = Z3_mk_or(c, uni.size(), uni.ptr());
          expr ors(c, ms);
          rhs1.push_back(ex1 && ors);
        }
        else
          rhs1.push_back(ex1);        
      }
    }
    if(!rhs1.empty())
    {
      expr S = revPMap.find(std::pair<CB, CB>(A, B))->second;
      array<Z3_ast> uni(rhs1);
      Z3_ast ms = Z3_mk_and(c, uni.size(), uni.ptr());
      expr ands(c, ms);
      s->add(implies(ands, S));
    }
  }

  comment = new_bool_const("Conditional Matches Before part 3");
  s->add(comment);
  for(int i=0; i<Eset.size(); ++i) // For all pairs in Eset?
  {
    Envelope* recv1 = Eset[i].first;
    Envelope* recv2 = Eset[i].second;
    CB A(recv1->id, recv1->index);
    CB B(recv2->id, recv2->index);
    expr pab = revPMap.find(std::pair<CB, CB>(A, B))->second;
    expr orderA = getClk(A);
    expr orderB = getClk(B);
    s->add(implies(pab, orderA<orderB));
  }
}

void SMTEncoding::kBuffering()
{
  //std::cout << "\nK-Buffering Constraints";
  fflush(stdout);
  expr comment = new_bool_const("K-Buffering Constraints");
  s->add(comment);

  counter.push_back(new_int_val(Scheduler::_kbuffer));

  forall_transitionLists(iter, last_node->_tlist){
    forall_transitions(titer, (*iter)->_tlist){
      Envelope* envA = titer->GetEnvelope();
      if(!toBeDropped(envA, lineNumbers, ranksIndices, allRanksIndices))
      {
        if(envA->func_id == FINALIZE) continue;
        if(envA->isSendType())
        {
          CB A(envA->id, envA->index);
          expr_vector vars = getMatchExpr(A);
          expr m = vars[0];
          expr bufferUsed = vars[7];
          //s->add(implies(m && bufferUsed, k==k+1)); // If a Send is matched and if it is using the buffer, then increment 'k'
          Cover* temp = eventMap.find(A)->second.second;
          std::string name = temp->name + "_counter";
          expr x = new_int_const();
          counter.push_back(x);
          s->add( counter[counter.size()-1] == ite(m&&bufferUsed , counter[counter.size()-2]+1 , counter[counter.size()-2]) );
        }
      }
    }
  }
  
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
  forall_transitionLists(iter, last_node->_tlist){
    forall_transitions(titer, (*iter)->_tlist){
      Envelope * envA = (*titer).GetEnvelope();
      if(!toBeDropped(envA, lineNumbers, ranksIndices, allRanksIndices)) 
      {
        if(envA->func_id == FINALIZE) continue;
        if(envA->isSendType())
        {
          CB A(envA->id, envA->index);
          for(std::vector<int>::iterator vit = (*titer).get_ancestors().begin(); vit != (*titer).get_ancestors().end(); vit ++)
          {
            CB B (envA->id, *vit);
            Envelope * envB = last_node->GetTransition(B)->GetEnvelope();
            if(envB->func_id == FINALIZE) continue;
            if(!isNecessaryEdgeRequired(envA, envB)) // For k-buffering
            {
              std::pair<expr, expr> p = getMILiteral(B);
              expr m = p.first;
              expr i = p.second;

              expr_vector vars = getMatchExpr(B);
              expr bufferUsed = vars[7];

              expr orderA = getClk(A);
              expr orderB = getClk(B);
              expr x = new_int_const("x");

              //s->add( ite(i&&counter[counter.size()-1]>0, (x==counter[counter.size()-1]-1)&&bufferUsed, (x==counter[counter.size()-1])&&!bufferUsed&&(orderB<orderA)) ); 
              //counter.push_back(x);

              s->add(implies(i&&counter[counter.size()-1]>0, (x==counter[counter.size()-1]-1)&&bufferUsed) && implies(counter[counter.size()-1]==0, (x==counter[counter.size()-1])&&!bufferUsed&&(orderB<orderA)));
              counter.push_back(x);
            }
          }
        }
      }
    }
  }
}

void SMTEncoding::generateConstraints(bool flag)
{
  //displayMatchSet();
  s->add(k>=0); // the 'k' is for k-buffering; 'k' declared in SMTEncoding.hpp
  s->add(k==Scheduler::_kbuffer);

  createEventCovers(lineNumbers);
  /*set_width();*/
  /*createOrderLiterals(); */ // Creating clk literals <-- createBVEventLiterals();  I have created clk literals in createEventCovers only
  createMatchLiterals();
  matchCollectives();  // Uncomment this for building the semantics for collectives
  
  gettimeofday(&constGenStart, NULL);

  partialOrderConstraints(); // <-- createClkLiterals(); + createRFConstraint(); partial order constraint + clock difference + clock equality
  matchPairs(); // RF Constraint
  uniqueMatchSend(); // unique match for send constraint
  uniqueMatchRecv(); // unique match for recv constraint
  //atLeastOneSend(); // at least one of the match sets must be true

  // The problem with atLeastOneSend is that when a deadlock is indeed present, it will not detect it because one of the s variables will be false 
  // and this rule will try to set it to true. So, whole result will be UNSAT.

  createRFSomeConstraint(); // Match correct
  createMatchConstraint(); //Matched Only
  matchImpliesIssued(); // Match only  issued 
  allAncestorsMatched(); // All ancestors matched 
  //conditionalMatchesBefore(); // Conditional matches before condition encoded
  //kBuffering();

  if(flag) // These are the safety conditions. We drop these when checking for other combinations of conditionals encountered
  {
    noMoreMatchesPossible(); // No more matches possible
    notAllMatched();  // not all matched
  }

  // This property is for conditional matches before
  /*for(int i=0; i<Eset.size(); ++i) 
  {
    Envelope* recv1 = Eset[i].first;
    Envelope* recv2 = Eset[i].second;
    CB A(recv1->id, recv1->index);
    CB B(recv2->id, recv2->index);

    expr_vector exprVecA = getMatchExpr(A);
    expr_vector exprVecB = getMatchExpr(B);

    s->add(implies(true, exprVecB[2]<exprVecA[2])); // Wildcard receive matches before deterministic receive
    //s->add(true && !(exprVecB[2]<exprVecA[2])); // Negation of the condition written above
  }*/

  waitMatch();
  gettimeofday(&constGenEnd, NULL);
  getTimeElapsed(constGenStart, constGenEnd);
}

void storeRankIndex(std::vector<std::pair<int, int> >& r, std::string name)
{
  int pid, index;

  std::istringstream f(name);
  std::string str;    
  
  getline(f, str, '_');
  getline(f, str, '_');
  pid = std::stoi(str);

  getline(f, str, '_');
  index = std::stoi(str);

  r.push_back(std::pair<int, int> (pid, index));
}

void SMTEncoding::encodingConditionals(/*Verifier &ver, */char** assertsData, int size)
{
  //std::cout << "\nEncodingSymbolicInformation\n";
  fflush(stdout);
  expr comment = new_bool_const("Encoding Symbolic Information");
  s->add(comment);
  
  // Generate z3 clauses for asserts data

  assertsStore.clear(); // This function executes whenever there is a new execution of the program. So we clear the assertsStore every time.

  std::string varName;
  int rank, numAsserts, opr, literal, startlineno, endlineno, correspondingRecv, sendLineNo;

  for(int i=0; i<size; ++i) // size: number of processes
  {
    expr_vector clauses(c);
    //std::cout << "\nassertsData: " << assertsData[i] << "\n";
    fflush(stdout);
    std::vector<std::pair<expr, std::pair<int, int> > > localAsserts;

    // Read the data in the string and form asserts
    std::istringstream  iss;
    iss.str(std::string(assertsData[i]));
    iss >> rank;
    iss.ignore();
    iss >> numAsserts;
    //std::cout << "numAsserts: " << numAsserts << "\n";
    std::vector<Cover*> Cbs;
    while(numAsserts > 0)
    {
      iss.ignore();
      iss >> varName;
      iss.ignore();
      iss >> opr;
      iss.ignore();
      iss >> literal;
      iss.ignore();
      iss >> startlineno;
      iss.ignore();
      iss >> endlineno;
      iss.ignore();
      iss >> correspondingRecv;
      iss.ignore();
      iss >> sendLineNo;

	//std::cout << "\nReached Here!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
	//fflush(stdout);

      std::string name;

      // Matching the name of the variable with z3 name
      forall_transitionLists(iter, last_node->_tlist)
      {
        forall_transitions(titer, (*iter)->_tlist)
        {
          if(!toBeDropped((*titer).GetEnvelope(), lineNumbers, ranksIndices, allRanksIndices) && (*titer).GetEnvelope()->isRecvType())
          {  
            Envelope *env = (*titer).GetEnvelope();
            if(env->linenumber == correspondingRecv && env->id == rank)
            {
              CB A (env->id, env->index);
              std::map<CB, std::pair<expr_vector, Cover*> >::iterator eit = eventMap.find(A);

              if(eit != eventMap.end())
              {
                Cover* c = eit->second.second;
                Cbs.push_back(c);
              }
            }
          }
        }
      }

      /*int numOfAssertsPerReceive=1;
      if(Cbs.size() < numAsserts)
        numOfAssertsPerReceive = numAsserts/Cbs.size(); // This is just a jugaad. Do it meaningfully later
      
      std::cout << numOfAssertsPerReceive << "\n";
      fflush(stdout);
      */
      numAsserts--;

			/*
      std::vector<Cover*>::iterator dkBegin = Cbs.begin();
      std::vector<Cover*>::iterator dkEnd = Cbs.end();
      while(dkBegin != dkEnd)
      {
        std::cout << (*dkBegin)->name << ' ' << literal << "\n";
        dkBegin++;
      }
      fflush(stdout);
			*/
      int i=0;
      //int j=0;
      do
      {
        name = Cbs[i]->name;
        // Z3 assertion      
        if (varName.find("MPI_TAG") != std::string::npos) 
          name = name + "_tag";
        else
          name = name + "_data";

        storeRankIndex(allRanksIndices, name);

        expr x = new_int_const(name);

        expr singleAssertion(c);

        switch (opr)
        {
          case 0:
                  singleAssertion = (x == new_int_val(literal)); 
                  break;
          case 1:
                  singleAssertion = !(x == new_int_val(literal)); 
                  break;
          case 2:
                  singleAssertion = (x < new_int_val(literal)); 
                  break;
          case 3:
                  singleAssertion = (x > new_int_val(literal)); 
                  break;
          case 4:
                  singleAssertion = (x <= new_int_val(literal)); 
                  break;
          case 5:
                  singleAssertion = (x >= new_int_val(literal)); 
                  break;

          default: std::cout << "\nOperator in a conditional not supported by tool\n"; 
        }
        s->add(singleAssertion);

				//std::clog << "+ " << singleAssertion << std::endl;
				std::ostringstream oss;
				oss << singleAssertion;
				add_condition(oss.str());

        //std::cout << "\nConditionals of this branch: " << singleAssertion << "\n";
        localAsserts.push_back(std::pair<expr, std::pair<int, int> > (singleAssertion, std::pair<int, int>(startlineno, endlineno)));
        clauses.push_back(singleAssertion);
        i++;
        if(i<Cbs.size())
        {
          numAsserts--;
          iss.ignore();
          iss >> varName;
          iss.ignore();
          iss >> opr;
          iss.ignore();
          iss >> literal;
          iss.ignore();
          iss >> startlineno;
          iss.ignore();
          iss >> endlineno;
          iss.ignore();
          iss >> correspondingRecv;
          iss.ignore();
      		iss >> sendLineNo;
        }
        /*j++;
        if(j==numOfAssertsPerReceive)
        {
          i++;
          j=0;
        }*/
      } while(i<Cbs.size());
      Cbs.clear();      
    }
    // Push the expr containing 'and' of all the clauses found
    if(!clauses.empty())
    {
      array<Z3_ast> args(clauses);
      Z3_ast r = Z3_mk_and(c, clauses.size(), args.ptr());
      expr res(c, r);
      POSs.push_back(res);
      assertsStore.push_back(std::pair<int, std::vector<std::pair<expr, std::pair<int, int> > > > (rank, localAsserts));
    }
  }
}

/*void SMTEncoding::encodingConditionals(char** assertsData, int size)
{
  std::cout << "\nEncodingSymbolicInformation\n";
  fflush(stdout);
  expr comment = new_bool_const("Encoding Symbolic Information");
  s->add(comment);
  
  // Generate z3 clauses for asserts data

  assertsStore.clear(); // This function executes whenever there is a new execution of the program. So we clear the assertsStore every time.

  std::string varName;
  int rank, numAsserts, opr, literal, startlineno, endlineno, correspondingRecv;

  for(int i=0; i<size; ++i) // size: number of processes
  {
    expr_vector clauses(c);
    std::cout << "\nassertsData: " << assertsData[i] << "\n";
    fflush(stdout);
    std::vector<std::pair<expr, std::pair<int, int> > > localAsserts;

    // Read the data in the string and form asserts
    std::istringstream  iss;
    iss.str(std::string(assertsData[i]));
    iss >> rank;
    iss.ignore();
    iss >> numAsserts;
    std::cout << "numAsserts: " << numAsserts << "\n";
    std::vector<Cover*> Cbs;
    while(numAsserts > 0)
    {
      iss.ignore();
      iss >> varName;
      iss.ignore();
      iss >> opr;
      iss.ignore();
      iss >> literal;
      iss.ignore();
      iss >> startlineno;
      iss.ignore();
      iss >> endlineno;
      iss.ignore();
      iss >> correspondingRecv;

      std::string name;

      // Matching the name of the variable with z3 name
      forall_transitionLists(iter, last_node->_tlist)
      {
        forall_transitions(titer, (*iter)->_tlist)
        {
          if(!toBeDropped((*titer).GetEnvelope(), lineNumbers, ranksIndices, allRanksIndices) && (*titer).GetEnvelope()->isRecvType())
          {  
            Envelope *env = (*titer).GetEnvelope();
            if(env->linenumber == correspondingRecv && env->id == rank)
            {
              CB A (env->id, env->index);
              std::map<CB, std::pair<expr_vector, Cover*> >::iterator eit = eventMap.find(A);

              if(eit != eventMap.end())
              {
                Cover* c = eit->second.second;
                Cbs.push_back(c);
              }
            }
          }
        }
      }

      int numOfAssertsPerReceive=1;
      if(Cbs.size() < numAsserts)
        numOfAssertsPerReceive = numAsserts/Cbs.size(); // This is just a jugaad. Do it meaningfully later

      std::cout << numOfAssertsPerReceive << "\n";
      fflush(stdout);

      numAsserts--;

      std::vector<Cover*>::iterator dkBegin = Cbs.begin();
      std::vector<Cover*>::iterator dkEnd = Cbs.end();
      while(dkBegin != dkEnd)
      {
        std::cout << (*dkBegin)->name << "\n";
        dkBegin++;
      }
      fflush(stdout);
      int i=0;
      int j=0;
      do
      {
        name = Cbs[i]->name;
        // Z3 assertion      
        if (varName.find("MPI_TAG") != std::string::npos) 
          name = name + "_tag";
        else
          name = name + "_data";

        storeRankIndex(allRanksIndices, name);

        expr x = new_int_const(name);

        expr singleAssertion(c);

        switch (opr)
        {
          case 0:
                  singleAssertion = (x == new_int_val(literal)); 
                  break;
          case 1:
                  singleAssertion = !(x == new_int_val(literal)); 
                  break;
          case 2:
                  singleAssertion = (x < new_int_val(literal)); 
                  break;
          case 3:
                  singleAssertion = (x > new_int_val(literal)); 
                  break;
          case 4:
                  singleAssertion = (x <= new_int_val(literal)); 
                  break;
          case 5:
                  singleAssertion = (x >= new_int_val(literal)); 
                  break;

          default: std::cout << "\nOperator in a conditional not supported by tool\n"; 
        }
        s->add(singleAssertion);
        std::cout << "\nConditionals of this branch: " << singleAssertion << "\n";
        localAsserts.push_back(std::pair<expr, std::pair<int, int> > (singleAssertion, std::pair<int, int>(startlineno, endlineno)));
        clauses.push_back(singleAssertion);
  
        if(i<Cbs.size())
        {
          numAsserts--;
          iss.ignore();
          iss >> varName;
          iss.ignore();
          iss >> opr;
          iss.ignore();
          iss >> literal;
          iss.ignore();
          iss >> startlineno;
          iss.ignore();
          iss >> endlineno;
          iss.ignore();
          iss >> correspondingRecv;
        }
        //i++;
        j++;
        if(j==numOfAssertsPerReceive)
        {
          i++;
          j=0;
        }
      } while(i<Cbs.size());
      Cbs.clear();      
    }
    // Push the expr containing 'and' of all the clauses found
    if(!clauses.empty())
    {
      array<Z3_ast> args(clauses);
      Z3_ast r = Z3_mk_and(c, clauses.size(), args.ptr());
      expr res(c, r);
      POSs.push_back(res);
      assertsStore.push_back(std::pair<int, std::vector<std::pair<expr, std::pair<int, int> > > > (rank, localAsserts));
    }
  }
}*/

bool SMTEncoding::checkResult() // Returns true if the encoding is satisfiable, else returns false
{
  switch (s->check()) 
  {
    case unsat:   std::cout << "\nUNSATISFIABLE\n"; break;
    case sat:     std::cout << "\nSAT: DEADLOCK DETECTED\n"; break;
    case unknown: std::cout << "\nunknown\n"; break;
  }

  //std::cout << (*s) << "\n";

  if(s->check() == sat)
  {
    model m = s->get_model();
    //std::cout << m << "\n";
    return true; // Found a deadlock, stop exploring further permutations (states)
  }
  else if(s->check() == unsat)
  {
    return false; // Call a function to change one of the conditional asserts and drop the MPI calls in an 'if' branch
  }
}

// Check whether this set of conditionals ('assumes') is even possible.
bool SMTEncoding::isSatisfied(solver* s)
{
	//std::clog << "Checking validity of new path constraints" << std::endl;
  //std::cout << "\nInside isSatisfied\n" << (*s) << "\n";

	/*
  switch (s->check()) 
  {
    case unsat:   std::cout << "\nUNSATISFIABLE\n"; break;
    case sat:     std::cout << "\nSAT: EXPLORE THIS VARIANT\n"; break;
    case unknown: std::cout << "\nunknown\n"; break;
  }
	*/

	//std::cout << "\nInside isSatisfied\n" << (*s) << "\n";
  if(s->check() == sat)
  {
    //model m = s->get_model();
    //std::cout << m << "\n";
    return true; // Explore this variant
  }
  else if(s->check() == unsat)
  {
    return false;
  }
}

bool SMTEncoding::isExplored(expr e)
{
  context ct;
  solver *dummy = new solver(ct);
  expr a = to_expr(ct, Z3_translate(c, e, ct));
  // Check whether it is even possible to satisfy the changed conditional (it should not be trivially unsat: x==1 and x==2)
  dummy->push();
  dummy->add(a);
  if(dummy->check() == unsat)
  {
    //std::cout << "\nReturning because unsat\n";
    delete dummy;
    return true;
  }
  dummy->pop();
  for(std::vector<expr>::iterator it=SMTEncoding::POSs.begin(); it!=SMTEncoding::POSs.end(); ++it)
  {
    dummy->push();
    expr b = to_expr(ct, Z3_translate(c, (*it), ct));
    //std::cout << "\nInside isExplored: " << a << "\t" << SMTEncoding::POSs.size() << "\t" << b << "\n";
    expr conjecture = a == b;
    dummy->add(!conjecture);
    //std::cout << "\nInside isExplored\n" << (*dummy) << "\n";
    if(dummy->check() == unsat)
    {
      delete dummy;
      return true;
    }
    dummy->pop();
  }
  delete dummy;
  //std::cout << "\nThis is not yet explored\n";
  //fflush(stdout);
  return false;
}

// Convert expr into the data structure expression; form changedConditionals
void visit(expr_vector ev, std::vector<expression> & changedConditionals) 
{
  for (int i=0; i<ev.size(); ++i)
  {
    expr e = ev[i];
    bool neg = false;
    if (e.is_app()) 
    {
      func_decl f = e.decl();
      std::string opr = f.name().str();

      if(opr == "not")
      {
        e = e.arg(0);
        f = e.decl();
        opr = f.name().str();
        neg = true;
      }

      expression ex;
      if(opr == "<")
      {
        //ex.lit = e.arg(1).decl().name().str();
        ex.varName = e.arg(0).decl().name().str();
        Z3_get_numeral_int(e.arg(1).ctx(), e.arg(1), &ex.lit);
        if(neg) ex.opr = ">="; else ex.opr = "<";
        changedConditionals.push_back(ex);
      }
      else if(opr == "<=")
      {
        //ex.lit = e.arg(1).decl().name().str();
        ex.varName = e.arg(0).decl().name().str();
        Z3_get_numeral_int(e.arg(1).ctx(), e.arg(1), &ex.lit);
        if(neg) ex.opr = ">"; else ex.opr = "<=";
        changedConditionals.push_back(ex);
      }
      else if(opr == ">")
      {
        //ex.lit = e.arg(1).decl().name().str();
        ex.varName = e.arg(0).decl().name().str();
        Z3_get_numeral_int(e.arg(1).ctx(), e.arg(1), &ex.lit);
        if(neg) ex.opr = "<="; else ex.opr = ">";
        changedConditionals.push_back(ex);
      }
      else if(opr == ">=")
      {
        //ex.lit = e.arg(1).decl().name().str();
        ex.varName = e.arg(0).decl().name().str();
        Z3_get_numeral_int(e.arg(1).ctx(), e.arg(1), &ex.lit);
        if(neg) ex.opr = "<"; else ex.opr = ">=";
        changedConditionals.push_back(ex);
      }
      else if(opr == "=")
      {
        //ex.lit = e.arg(1).decl().name().str();
        ex.varName = e.arg(0).decl().name().str();
        Z3_get_numeral_int(e.arg(1).ctx(), e.arg(1), &ex.lit);
        if(neg) ex.opr = "!="; else ex.opr = "==";
        changedConditionals.push_back(ex);
      }
      else if(opr == "!=")
      {    
        //ex.lit = e.arg(1).decl().name().str();
        ex.varName = e.arg(0).decl().name().str();
        Z3_get_numeral_int(e.arg(1).ctx(), e.arg(1), &ex.lit);
        if(neg) ex.opr = "=="; else ex.opr = "!=";
        changedConditionals.push_back(ex);
      }
      /*else
      {
        unsigned num = e.num_args();
        for (unsigned i = 0; i < num; i++) 
        {
          visit(e.arg(i), changedConditionals);
        }
      }*/
    }
  }
}

bool SMTEncoding::possibleMatchAvailable(std::vector<expression> changedConditionals)
{
  //std::cout << "\nIn possibleMatchAvailable, size: " << changedConditionals.size();
  // Find unique variables present in changedConditionals. We have to check the satisfiability for every such variable
  std::set<std::string> uniqueVars;
  std::vector<expression>::iterator itBegin = changedConditionals.begin();
  std::vector<expression>::iterator itEnd = changedConditionals.end();

  while(itBegin != itEnd)
  {
    uniqueVars.insert((*itBegin).varName);
    itBegin++;
  }
  //std::cout << "\nIn possibleMatchAvailable, uniqueVars size: " << uniqueVars.size();
  std::set<std::string>::iterator setItBegin = uniqueVars.begin();

  while(setItBegin != uniqueVars.end())
  {
    bool flag2 = false; // This flag being false means that for this R, there is no send confirming to conditionals
                        // If at the end of the loop, this flag remains false, then there is indeed no send confirming to conditionals, and we must return false
    // These are the pid and index of the recv present in conditionals
    int pid, index;

    std::istringstream f((*setItBegin));
    std::string str;    
    getline(f, str, '_');
    getline(f, str, '_');
    pid = std::stoi(str);    

    getline(f, str, '_');
    index = std::stoi(str);    

    forall_matchSet(mit, matchSet)
    {
      if((**mit).size() == 2) // Its send recv
      {
        int _pid = (**mit).back()._pid;
        int _index = (**mit).back()._index; 

        if(pid == _pid && index == _index) // (**mit)'s front is an ample set
        {
          CB A((**mit).front()._pid, (**mit).front()._index); // Send's CB
          std::map<CB, std::pair<expr_vector, Cover*> >::iterator eventIt = eventMap.find(A);
          Cover* sendCover = ((*eventIt).second).second;

          itBegin = changedConditionals.begin();
          itEnd = changedConditionals.end();

          bool flag = true;

          // Parsing the changedConditionals
          for(; itBegin!=itEnd; ++itBegin)
          {
            expression ex = *itBegin;
            std::string varName = ex.varName;
            int pidConditional = varName.at(2) - '0';
            int indexConditional = varName.at(4) - '0';

            if(pidConditional==_pid && indexConditional==_index)
            {
              if(varName.substr( varName.length()-3 ) == "tag")
              {
                // Check the tag of the send's envelope
                int sendTag = sendCover->tag;
                //std::cout << "Send's tag: " << sendTag;
                if(ex.opr == "==")
                {
                  if(sendTag != ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
                else if(ex.opr == "!=")
                {
                  if(sendTag == ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
                else if(ex.opr == ">")
                {
                  if(sendTag <= ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
                else if(ex.opr == ">=")
                {
                  if(sendTag < ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
                else if(ex.opr == "<")
                {
                  if(sendTag >= ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
                else if(ex.opr == "<=")
                {
                  if(sendTag > ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
              }
              else if(varName.substr( varName.length()-4 ) == "data")
              {
                //std::cout << "\nVarName" << ex.varName << ex.opr << ex.lit << " " << sendCover->data << "\n";
                // Check the data of the send's envelope
                int sendData = sendCover->data;
                if(ex.opr == "==")
                {
                  if(sendData != ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
                else if(ex.opr == "!=")
                {
                  if(sendData == ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
                else if(ex.opr == ">")
                {
                  if(sendData <= ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
                else if(ex.opr == ">=")
                {
                  if(sendData < ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
                else if(ex.opr == "<")
                {
                  if(sendData >= ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
                else if(ex.opr == "<=")
                {
                  if(sendData > ex.lit)
                  {
                    flag = false;
                    break;
                  }
                }
              }
            }
          }
          if (flag == true)
          {
            flag2 = true;
            break;
          }
        }
      }
    }
    setItBegin++;

    if (flag2 == false)
      return false;
  }
  return true;
}

bool SMTEncoding::changeConditionals(std::vector<expression> & changedConditionals)
{
  //while(procToExplore < assertsStore.size())
  //{
  std::vector<std::pair<expr, std::pair<int, int> > > assumes;
  //for(int l=0; l<assertsStore.size(); ++l)
  for(int l=assertsStore.size()-1; l>=0; --l)
  {
    assumes.insert(assumes.end(), (assertsStore[l].second).begin(), (assertsStore[l].second).end());
  }
  int size = assumes.size();
  //std::cout << "\nassumes size: " << size << "\n";
  
  // Find a variation of the conditions in the assumes which has not been explored before and see if that variant can be satisfied
  for (int i=size-1; i>=0; --i)
  {
    // Remove everything from the top of the stack till i (excluding i)
    std::string nameVariable;
    expr es = (assumes[i].first);
    if (es.is_app()) 
    {
      func_decl f = es.decl();
      std::string opr = f.name().str();
      if(opr == "not")
      {
        es = es.arg(0);
      }
      nameVariable = es.arg(0).decl().name().str();
    }
    if(!seenConstraints.empty())
    {
      //std::cout << "\nseenConstraints.size at the time of removing: " << seenConstraints.size() << "\n";
      int y = i;
      while(y < size-1 && !seenConstraints.empty()) // This technique won't work in nested if-else
      {
        //std::cout << "\nRemoving: " << seenConstraints.front().first << "\n";
        seenConstraints.pop_front();
        y++;
      }
    }


    s = new solver(c);
    expr_vector e(c);
    lineNumbers.clear();
    ranksIndices.clear();
    s->push();
		//Verifier ver(false);
		//init_verifier(ver);

		// Invert the ith entry; if ith entry depends upon any prior condition, change the i. Make i point to that prior condition
		
		std::string str;
    //s->add(!(assumes[j].first));
    es = (assumes[i].first);
    if (es.is_app()) 
    {
      func_decl f = es.decl();
      std::string opr = f.name().str();
      if(opr == "not")
      {
        es = es.arg(0);
      }
      str = es.arg(0).decl().name().str();
      storeRankIndex(ranksIndices, str);
      
      // Add this constraint in the stack with the matching variable name
      if(!seenConstraints.empty())
      {
        std::pair<std::string, expr_vector> p = seenConstraints.front();
        std::deque<std::pair<std::string, expr_vector> > temp;

        int s = seenConstraints.size();
        while(str != p.first)
        {
          temp.push_front(p);
          seenConstraints.pop_front();
          if (!seenConstraints.empty()) 
            p = seenConstraints.front();
          else break;
        }
        if(temp.size() == s) // No match found
        {
          // Push the remaining expr_vectors back into the stack
          while(!temp.empty())
          {
            seenConstraints.push_front(temp.front());
            temp.pop_front();
          }
          expr_vector e(c);
          e.push_back(!(assumes[i].first));
          seenConstraints.push_front(std::pair<std::string, expr_vector>(str, e));
        }
        else // The name matches, add the expr into the expr_vector of the matched varible
        {
          seenConstraints.pop_front();
          expr_vector e = p.second;
          e.push_back(!(assumes[i].first));
          seenConstraints.push_front(std::pair<std::string, expr_vector>(str, e));
          // Push the remaining expr_vectors back into the stack
          while(!temp.empty())
          {
            seenConstraints.push_front(temp.front());
            temp.pop_front();
          }
        }
      }
      else
      {
        // First entry in the stack
        expr_vector e(c);
        e.push_back(!(assumes[i].first));
        seenConstraints.push_front(std::pair<std::string, expr_vector>(str, e));
      }
    }
    //std::cout << "\nAdding: " << str << "\n";

    //e.push_back(!(assumes[j].first));
    lineNumbers.push_back(assumes[i].second); // These are the line numbers between which all MPI calls have to be ignored

		ver.clear_conditions();
    // Let the other conditionals be put as they are (below ith)
    for(int j=i-1; j>=0; --j)
    {
      s->add(assumes[j].first);

			//std::clog << "- " << assumes[j].first << std::endl;
			std::ostringstream oss;
			oss << assumes[j].first << std::endl;
			add_condition(oss.str());

      e.push_back(assumes[j].first);
    }

    //std::cout << "\nseenConstraints.size: " << seenConstraints.size() << "\n";
    for(int k=seenConstraints.size()-1; k>=0; --k)
    {
      expr_vector ev = seenConstraints[k].second;
      visit(ev, changedConditionals);
			for (const auto &x: ev) {
				//std::clog << "~ " << x << std::endl;
				std::ostringstream oss;
				oss << x << std::endl;
				add_condition(oss.str());
			}
			
			//std::clog << "---" << std::endl;

      if(!ev.empty())
      {
        array<Z3_ast> args(ev);
        Z3_ast r = Z3_mk_and(c, ev.size(), args.ptr());
        expr previousConstraints(c, r);
        s->add(previousConstraints);
      }

      for(int x=0; x<ev.size(); ++x)
        e.push_back(ev[x]);
    }

    if(!e.empty())
    {
      array<Z3_ast> args(e);
      Z3_ast r = Z3_mk_and(c, e.size(), args.ptr());
      expr thisVariant(c, r);
      //std::cout << "\nThis variant: " << thisVariant << "\n";
      //visit(e, changedConditionals); 
      //s->add(thisVariant);

			/*
      std::pair<std::string, expr_vector> p = seenConstraints.front();
      std::cout << "\n" << p.first << " " << p.second << "\n";
      fflush(stdout);
			*/
      
      //std::cout << "Changed Conditionals in SMTEncoding" << "\n";
			/*
      std::vector<expression>::iterator it = changedConditionals.begin();
      std::vector<expression>::iterator it2 = changedConditionals.end();

      for(; it!=it2; ++it)
      {
        expression ex = *it;
        std::cout << ex.varName << ex.opr << ex.lit << "\n";
      }
      fflush(stdout);
			*/

      //std::cout << "\nChanged Conditionals in changeConditionals: " << (*s) << "\n";
      //fflush(stdout);
      if(!isExplored(thisVariant))
      {
        //createMatchSet(lineNumbers, ranksIndices, allRanksIndices); // If you are going to comment this line, please remove lineNumbers argument from all the calls. Remove lineNumbers
        //createEset(lineNumbers, ranksIndices, allRanksIndices);
        //std::cout << "\n----------------------------------Solver's content:-----------------------------------------\n";
        //std::cout << (*s) << "\n";
        //generateConstraints(false);  // argument 'false' indicates that we are calling generateConstraints from this function
				//bool res = isSatisfied(s);
				//std::cout << "checking feasibility of changed conditionals... ";
				//fflush(stdout);
				logger << "checking feasibility of changed conditionals...";
				clock_t tic = clock();
				bool vres = ver.check_feasibility();
				clock_t toc = clock();
				double ela = double(toc-tic)/CLOCKS_PER_SEC;
				if (vres) {
					logger << "verdict: feasible";
					//std::cout << "feasible." << std::endl;
				} else {
					logger << "verdict: not feasible";
					//std::cout << "not feasible." << std::endl;
				}
				//std::cout << "feasibility checking time: " << ela << " s" << std::endl;
        if(vres)
        {
          //if(possibleMatchAvailable(changedConditionals))
          POSs.push_back(thisVariant);
          delete s;
          return true;
        }
        else 
        {
          POSs.push_back(thisVariant);
          //std::cout << "\nCan not go this route\n";
          //fflush(stdout);
        }
      }
      //else std::cout << "\nAlready explored\n";
    }
    changedConditionals.clear();
    delete s;
  }
  // Exhausted all combinations in the process; move to next process
  seenConstraints.clear();
  //procToExplore++;
  //}
  //std::cout << "\nReturning false from changeConditionals\n";
  fflush(stdout);
  return false;
}

// This version of changeConditionals invert the conditions process by process
/*bool SMTEncoding::changeConditionals(std::vector<expression> & changedConditionals)
{
  while(procToExplore < assertsStore.size())
  {
    std::vector<std::pair<expr, std::pair<int, int> > > localAsserts = assertsStore[procToExplore].second;
    int size = localAsserts.size();
    std::cout << "\nlocalAsserts size: " << size << "\n";
    
    // Find a variation of the conditions in this localAsserts which has not been explored before and see if that variant can be satisfied
    for (int i=size-1; i>=0; --i)
    {
      // Remove everything from the top of the stack till i (excluding i)
      std::string nameVariable;
      expr es = (localAsserts[i].first);
      if (es.is_app()) 
      {
        func_decl f = es.decl();
        std::string opr = f.name().str();
        if(opr == "not")
        {
          es = es.arg(0);
        }
        nameVariable = es.arg(0).decl().name().str();
      }
      if(!seenConstraints.empty())
      {
        std::cout << "\nseenConstraints.size at the time of removing: " << seenConstraints.size() << "\n";
        int y = i;
        while(y < size-1 && !seenConstraints.empty()) // This technique won't work in nested if-else
        {
          std::cout << "\nRemoving: " << seenConstraints.front().first << "\n";
          seenConstraints.pop_front();
          y++;
        }
        // Remove only if seenConstraints contain a record with a variable of name nameVariable
        // bool flag = false;
        // for(int y=0; y< seenConstraints.size(); ++y)
        // {
        //   if(seenConstraints[y].first == nameVariable)
        //   {
        //     flag = true;
        //     break;
        //   }
        // }
        // if(flag)
        // {
        //   std::pair<std::string, expr_vector> p = seenConstraints.front();
        //   std::string nameTopVariable = p.first;
        //   while(nameTopVariable != nameVariable)
        //   {
        //     seenConstraints.pop_front();
        //     if(!seenConstraints.empty())
        //     {
        //       p = seenConstraints.front();
        //       nameTopVariable = p.first;
        //     }
        //     else break; 
        //   }
        // }
      }

      s = new solver(c);
      expr_vector e(c);
      lineNumbers.clear();
      ranksIndices.clear();
      s->push();

      std::string str;
      for(int j=i; j>=0; --j)
      {
        if(j != i)
        {
          s->add(localAsserts[j].first);
          e.push_back(localAsserts[j].first);
        }
        else
        {
          //s->add(!(localAsserts[j].first));
          expr es = (localAsserts[j].first);
          if (es.is_app()) 
          {
            func_decl f = es.decl();
            std::string opr = f.name().str();
            if(opr == "not")
            {
              es = es.arg(0);
            }
            str = es.arg(0).decl().name().str();
            storeRankIndex(ranksIndices, str);
            
            // Add this constraint in the stack with the matching variable name
            if(!seenConstraints.empty())
            {
              std::pair<std::string, expr_vector> p = seenConstraints.front();
              std::deque<std::pair<std::string, expr_vector> > temp;

              int s = seenConstraints.size();
              while(str != p.first)
              {
                temp.push_front(p);
                seenConstraints.pop_front();
                if (!seenConstraints.empty()) 
                  p = seenConstraints.front();
                else break;
              }
              if(temp.size() == s) // No match found
              {
                // Push the remaining expr_vectors back into the stack
                while(!temp.empty())
                {
                  seenConstraints.push_front(temp.front());
                  temp.pop_front();
                }
                expr_vector e(c);
                e.push_back(!(localAsserts[j].first));
                seenConstraints.push_front(std::pair<std::string, expr_vector>(str, e));
              }
              else // The name matches, add the expr into the expr_vector of the matched varible
              {
                seenConstraints.pop_front();
                expr_vector e = p.second;
                e.push_back(!(localAsserts[j].first));
                seenConstraints.push_front(std::pair<std::string, expr_vector>(str, e));
                // Push the remaining expr_vectors back into the stack
                while(!temp.empty())
                {
                  seenConstraints.push_front(temp.front());
                  temp.pop_front();
                }
              }
            }
            else
            {
              // First entry in the stack
              expr_vector e(c);
              e.push_back(!(localAsserts[j].first));
              seenConstraints.push_front(std::pair<std::string, expr_vector>(str, e));
            }
          }
          std::cout << "\nAdding: " << str << "\n";

          //e.push_back(!(localAsserts[j].first));
          lineNumbers.push_back(localAsserts[j].second); // These are the line numbers between which all MPI calls have to be ignored
        }
      }
      std::cout << "\nseenConstraints.size: " << seenConstraints.size() << "\n";
      for(int k=seenConstraints.size()-1; k>=0; --k)
      {
        expr_vector ev = seenConstraints[k].second;
        visit(ev, changedConditionals);
        if(!ev.empty())
        {
          array<Z3_ast> args(ev);
          Z3_ast r = Z3_mk_and(c, ev.size(), args.ptr());
          expr previousConstraints(c, r);
          s->add(previousConstraints);
        }

        for(int x=0; x<ev.size(); ++x)
          e.push_back(ev[x]);
      }

      if(!e.empty())
      {
        array<Z3_ast> args(e);
        Z3_ast r = Z3_mk_and(c, e.size(), args.ptr());
        expr thisVariant(c, r);
        std::cout << "\nThis variant: " << thisVariant << "\n";
        //visit(e, changedConditionals); 
        //s->add(thisVariant);

        std::pair<std::string, expr_vector> p = seenConstraints.front();
        std::cout << "\n" << p.first << " " << p.second << "\n";
        fflush(stdout);
        
        std::cout << "Changed Conditionals in SMTEncoding" << "\n";
        std::vector<expression>::iterator it = changedConditionals.begin();
        std::vector<expression>::iterator it2 = changedConditionals.end();

        for(; it!=it2; ++it)
        {
          expression ex = *it;
          std::cout << ex.varName << ex.opr << ex.lit << "\n";
        }
        fflush(stdout);

        //std::cout << "\nChanged Conditionals in changeConditionals: " << (*s) << "\n";
        //fflush(stdout);
        if(!isExplored(thisVariant))
        {
          createMatchSet(lineNumbers, ranksIndices, allRanksIndices); // If you are going to comment this line, please remove lineNumbers argument from all the calls. Remove lineNumbers
          //createEset(lineNumbers, ranksIndices, allRanksIndices);
          std::cout << "\n----------------------------------Solver's content:-----------------------------------------\n";
          std::cout << (*s) << "\n";
          generateConstraints(false);  // argument 'false' indicates that we are calling generateConstraints from this function
          fflush(stdout);
          bool res = isSatisfied(s);
          if(res)
          {
            //if(possibleMatchAvailable(changedConditionals))
            POSs.push_back(thisVariant);
            delete s;
            return true;
          }
          else 
          {
            POSs.push_back(thisVariant);
            std::cout << "\nCan not go this route\n";
            fflush(stdout);
          }
        }
        else std::cout << "\nAlready explored\n";
      }
      changedConditionals.clear();
      delete s;
    }
    // Exhausted all combinations in the process; move to next process
    //changedConditionals.clear();
    //delete s;
    procToExplore++;
    seenConstraints.clear();
  }
  std::cout << "\nReturning false from changeConditionals\n";
  fflush(stdout);
  return false;
}*/

// This function is used to change one condition and drop the appropriate match-pairs from Match Set
/*bool SMTEncoding::changeConditionals(std::vector<expression> & changedConditionals)
{
  int numProcs = assertsStore.size();

  for(int proc=0; proc<numProcs; ++proc)
  {
    int rank = assertsStore[proc].first;
    std::vector<std::pair<expr, std::pair<int, int> > > localAsserts = assertsStore[proc].second;
    int size = localAsserts.size();
    std::cout << "localAsserts.size() " << size << "\n";
    int numCombsToExplore = pow(2, size) - 1;
    s = new solver(c);

    // Find a variation of the conditions in this localAsserts which has not been explored before and see if that variant can be satisfied
    for (int i=1; i<=numCombsToExplore; ++i)
    { 
      expr_vector e(c);
      lineNumbers.clear();
      ranksIndices.clear();
      s->push();

      int ONE = 1;
      for(int j=0; j<size; ++j)
      {
        if((ONE&i) == 0)
        {
          s->add(localAsserts[j].first);
          e.push_back(localAsserts[j].first);
        }
        else
        {
          s->add(!(localAsserts[j].first));
          expr es = (localAsserts[j].first);
          if (es.is_app()) 
          {
            func_decl f = es.decl();
            std::string opr = f.name().str();
            if(opr == "not")
            {
            es = es.arg(0);
            }
            std::string str = es.arg(0).decl().name().str();
            storeRankIndex(ranksIndices, str);
          }

          e.push_back(!(localAsserts[j].first));
          lineNumbers.push_back(localAsserts[j].second); // These are the line numbers between which all MPI calls have to be ignored
        }
        ONE = ONE << 1;
      }
      
      std::cout << "\nChanged Conditionals in changeConditionals: " << (*s) << "\n";
      //Z3_string Z3Formula = Z3_benchmark_to_smtlib_string(c, "benchmark", "", "", "", 0, 0, 0);
      fflush(stdout);
      std::stringstream buffer;
      buffer << (*s) << "\n";
      //std::cout << buffer.str();

      if(!e.empty())
      {
        array<Z3_ast> args(e);
        Z3_ast r = Z3_mk_and(c, e.size(), args.ptr());
        expr thisVariant(c, r);

        s->add(thisVariant);

        visit(thisVariant, changedConditionals);

        if(!isExplored(thisVariant))
        {
          createMatchSet(lineNumbers, ranksIndices, allRanksIndices); // If you are going to comment this line, please remove lineNumbers argument from all the calls. Remove lineNumbers
          //createEset(lineNumbers, ranksIndices, allRanksIndices);
          generateConstraints(false);  // argument 'false' indicates that we are calling generateConstraints from this function
          bool res = isSatisfied(s);
          if(res)
          {
            //if(possibleMatchAvailable(changedConditionals))
            POSs.push_back(thisVariant);
            delete s;
            return true;
          }
          else 
          {
            POSs.push_back(thisVariant);
            std::cout << "\nCan not go this route\n";
          }
        }
        else std::cout << "\nAlready explored\n";
      }
      changedConditionals.clear();
      s->pop();
    }
  }

  // Exhausted all combinations
  changedConditionals.clear();
  delete s;
  return false;
}*/

/*
Finds out which wildcrd Recv call depends on which wildcard Recv
*/
void SMTEncoding::createDependencies()
{
	for(int i =0 ; i < _it->_slist.size()-1; i++)
	{
    CB back = _it->_slist[i]->curr_match_set.back();
    Transition *t = _it->_slist[i]->GetTransition(back);
    Envelope *env = t->GetEnvelope();
    //std::cout << "\ncreateDependencies: " << env->func << " : " << env->dest << "\n";
    if(strcmp((env->func).c_str(), "Recv") == 0)
    {
    	/* If this is a wildcard receive A, then go to its matching send. */
    	/* If that send is coming from an 'if' block, find its corresponding Recv B */
    	/* Put (A, B) in dependencies vector */
    	if(env->src_wildcard)
    	{
    		CB matching_send = _it->_slist[i]->curr_match_set.front();
    		t = _it->_slist[i]->GetTransition(matching_send);
    		Envelope *env_matching_send = t->GetEnvelope();

    		char** assertsData = Scheduler::assertsData;
    		int numProcs = atoi(Scheduler::_num_procs.c_str());

    		for(int i=0; i<numProcs; ++i)
				{
					int numAsserts, rank;
					// Read the data in the string
					std::istringstream  iss;
					iss.str(std::string(assertsData[i]));
					iss >> rank;
					iss.ignore();
					iss >> numAsserts;
					bool flag = false;
					while(numAsserts > 0)
					{
					  std::string varName;
					  int opr, literal, startlineno, endlineno, correspondingRecv, sendLineNo;
					  iss.ignore();
					  iss >> varName;
					  iss.ignore();
					  iss >> opr;
					  iss.ignore();
					  iss >> literal;
					  iss.ignore();
					  iss >> startlineno;
					  iss.ignore();
					  iss >> endlineno;
					  iss.ignore();
					  iss >> correspondingRecv;
					  iss.ignore();
					  iss >> sendLineNo;

					  // Matching the sendLineNo with the line number in the envelope of matching Send of the wildcard recv
						if(sendLineNo == env_matching_send->linenumber)
						{
							CB A (rank, (env_matching_send->index)-1); // This technique is wrong; correct it later
    					//std::cout << "\nSend index: " << env_matching_send->id << " " << env_matching_send->index << " Receive index: " << A._pid << " "  << A._index << "\n";
    					dependencies.push_back(std::pair<CB, CB>(back, A)); 
    					// So this dependencies vector contains new CB for A, not the original pointer to the CB A
    					// Take care of this when finding something from eventMap using this CB, correct result will not be obtained
							flag = true;
							break;
						}
						numAsserts--;
					}
					if(flag)
						break;
				}
    	}
    }
	}

	//std::cout << "\nDependencies size: " << dependencies.size() << "\n";
	    
	/*
  // Display the dependencies
  for(std::vector<std::pair<CB, CB> >::iterator it = dependencies.begin(); it!=dependencies.end(); ++it)
  {
  	std::pair<CB, CB> dependency = *it;
  	std::cout << "\nDependency first: " << dependency.first._pid << " " << dependency.first._index << "\n";
  	std::cout << "\nDependency second: " << dependency.second._pid << " " << dependency.second._index << "\n";
  }
	*/
}

void SMTEncoding::encodingPartialOrders()
{
  createMatchSet();
  createEset();
  
  /*
  ** Create a set of the dependencies between Send and wildcard Receive calls
  */
  createDependencies();
  generateConstraints(true);
  
  /* Uncomment this block if you run Mopper++ for a single trace, like Mopper
  gettimeofday(&solverStart, NULL);

  switch (s->check()) 
  {
    case unsat:   std::cout << "UNSATISFIABLE\n"; break;
    case sat:     std::cout << "SAT: DEADLOCK DETECTED\n"; break;
    case unknown: std::cout << "unknown\n"; break;
  }

  //std::cout << "\n\nRules:\n" << (*s) << "\n\n";

  gettimeofday(&solverEnd, NULL);

  if(s->check())
  {
    model m = s->get_model();
    //std::cout << m << "\n";
  }

  std::cout << "Constraint Generation Time: " << (getTimeElapsed(constGenStart, constGenEnd)*1.0)/1000000 << " sec \n";
  std::cout << "Constraint Solving Time: " << (getTimeElapsed(solverStart, solverEnd)*1.0)/1000000 << " sec \n";
  std::cout << "Total: " << (getTimeElapsed(constGenStart, constGenEnd)*1.0)/1000000 + (getTimeElapsed(solverStart, solverEnd)*1.0)/1000000 << " sec \n";
  */

  /*if(Scheduler::_formula==true){
    std::ofstream formulaFile;
    std::stringstream ss;
    ss << _it->sch->getProgName() << "."; 
    if(Scheduler::_send_block)
      ss << "b.";
    ss << _it->sch->getNumProcs() << ".formula";
    formulaFile.open(ss.str().c_str());
    formulaFile << formula.str();
    formulaFile.close();
  }
  formula.str("");
  formula.clear();
  
  std::cout << "********* SAT VALUATIONS ************" << std::endl;
  std::cout << "Number of Clauses: " <<  static_cast<cnf_solvert *>(slv)->no_clauses() << std::endl;
  std::cout << "Number of Variables: " << slv->no_variables() << std::endl;
  std::cout << "Constraint Generation Time: "
	  << (getTimeElapsed(constGenStart, constGenEnd)*1.0)/1000000 << "sec \n";
  
  gettimeofday(&solverStart, NULL);
  satcheckt::resultt answer = slv->prop_solve();
  gettimeofday(&solverEnd, NULL);
  switch(answer){
  case satcheckt::P_UNSATISFIABLE:
    formula << "Formula is UNSAT" <<std::endl;
    break;
  case satcheckt::P_SATISFIABLE:
    formula  << "Formula is SAT -- DEADLOCK DETECTED" <<std::endl;
    _deadlock_found = true;
    //publish();
    break;
    // output the valuations
  default: 
    formula << "ERROR in SAT SOLVING" <<std::endl;
    break;
  }
  //std::cout << //slv->constraintStream.str();
  
  std::cout << "Solving Time: " << (getTimeElapsed(solverStart, solverEnd)*1.0)/1000000 << "sec \n";
  size_t peakSize = getPeakRSS();
  std::cout << "Mem (MB): " << peakSize/(1024.0*1024.0) << std::endl;
  std::cout << formula.str();
  std::cout << std::endl;*/  
}
