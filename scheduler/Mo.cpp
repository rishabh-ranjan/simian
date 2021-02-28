#include "Mo.hpp"
#include "utility.hpp"

extern bool is_eql(std::pair<CB, CB> p1, std::pair<CB, CB> p2);

/* constructor */

M::M(){
  
}

std::ostream & operator<<(std::ostream &os, const M &c)
{
  std::multimap<ME, PE, mapComp1>::const_iterator it, it_end;
  it_end = c._MSet.end();
  
  if(c._MSet.empty()){
    os << "EMPTY _MSet" << std::endl;
  }
  else{
    os << "M["<< std::endl;
    for(it = c._MSet.begin(); it != it_end; it ++){
      // os << "[" << (*it).first.first << "," << (*it).second.first << "]" 
      //   << "---" 
      // 	<< "[" << (*it).first.second << "," << (*it).second.second << "]" 
      os << (*it).second.front() << (*it).second.back()
	 << std::endl;
    }
    os <<"]\n";  
  }
  return os;
}



void M::wrToFile(std::ofstream &f)
{

  std::multimap<ME, PE, mapComp1>::const_iterator it, it_end;
  it_end = _MSet.end();
  
  if(_MSet.empty()){
    f << "EMPTY_MSet" << std::endl;
  }
  else{
    for(it = _MSet.begin(); it != it_end; it ++){
      // f << "[" << (*it).first.first << "," << (*it).second.first << "]" 
      // 	<< "[" << (*it).first.second << "," << (*it).second.second << "]" 
      f << (*it).second.front() << (*it).second.back()
	<< std::endl;
    }
  }
}



// Idea: We can also pass the return object by reference

std::set <CB> M::MImage(CB c) {

  std::set<CB> _MImage;
 
  _MIterator it, it_end;
  it_end = _MSet.end();

  //Debug Print
  // std::cout << " _MSet.size =  " << _MSet.size() <<  std::endl;

  int a = c._pid; 
  int b = c._index;
  
  for(it = _MSet.begin(); it != it_end; it++) {
    
    // if ((*it).first == c)
    //   _MImage.insert((*it).second);
    
    // else if((*it).second == c )
    //   _MImage.insert((*it).first);

    // if((*it).first.first == a)  { 
    //   if((*it).second.first == b) 
    // 	_MImage.insert(CB ((*it).first.second, (*it).second.second));
    // }
    // else if ((*it).first.second == a){
    //   if ((*it).second.second == b)
    // 	_MImage.insert(CB ((*it).first.first, (*it).second.first));
    // }

    if((*it).first.first == a)  { 
      if((*it).second.front() == c)
	_MImage.insert((*it).second.back());
    }
    else if ((*it).first.second == a){
      if((*it).second.back() == c)
	_MImage.insert((*it).second.front());
    }

  }

  return _MImage;
}


std::set <CB> M::MImageRestricted(CB c, CB d) {

  std::set<CB> _MImage;
 
  // _MIterator it, it_end;
  // it_end = _MSet.end();

  // //Debug Print
  // // std::cout << " _MSet.size =  " << _MSet.size() <<  std::endl;

  // int a = c._pid; 
  // int b = c._index;
  
  // for(it = _MSet.begin(); it != it_end; it++) {
    
  //   // if ((*it).first == c)
  //   //   _MImage.insert((*it).second);
    
  //   // else if((*it).second == c )
  //   //   _MImage.insert((*it).first);

  //   if((*it).first.first == a)  { 
  //     if((*it).second.first == b && (*it).second.second < d._index ) 
  // 	_MImage.insert(CB ((*it).first.second, (*it).second.second));
  //   }
  //   else if ((*it).first.second == a){
  //     if ((*it).second.second == b && (*it).second.first < d._index)
  // 	_MImage.insert(CB ((*it).first.first, (*it).second.first));
  //   }
  // }

  return _MImage;
}


bool M::CheckMImage(CB c , int pid)
{
  
  // std::vector <std::pair <CB, CB> > ::iterator it, it_end;
  _MIterator it, it_end;
  
  it_end = _MSet.end();

  //Debug Print
  // std::cout << " _MSet.size =  " << _MSet.size() <<  std::endl;

  for(it = _MSet.begin(); it != it_end; it++) {
    
    //    if((*it).first == c && (*it).second._pid != pid) 
    if((*it).first.first == c._pid && 
       (*it).second.front()._index == c._index && 
       (*it).first.second != pid) 
      return false;
    
    //  else if( (*it).second == c && (*it).first._pid != pid) {
    else if((*it).first.second == c._pid && 
	    (*it).second.back()._index == c._index && 
	    (*it).first.first != pid) 
      return false;
  }
  
  return true;
  
}



std::set<CB> M::MImageNotFrom(CB c, int from){
  
  std::set<CB> _coi;
  // std::vector <std::pair <CB, CB> >::iterator it, it_end;
  // it_end = _MSet.end();
 
  // //std::cout<< " In findCoEnabledImageNotFrom func with D1's entry: " << c << " and its CoI coming from other than " <<from <<std::endl;

  // for(it = _MSet.begin(); it != it_end; it++) {
  //   if(((*it).first == c) && (*it).second._pid != from) {
  //     _coi.insert((*it).second);
  //   }
  //  else if( (*it).second == c && (*it).first._pid != from) {
  //     _coi.insert((*it).first);
  //   }
  // }

  return _coi;


}

CB M::FirstM(std::set<CB> _coi, int pid) {

  std::set<CB>::iterator it, it_end;
  it_end = _coi.end();

  CB _min(pid, 50000);

  for(it = _coi.begin(); it != it_end; it++) {
    if((*it)._pid == pid && (*it)._index < _min._index)
      _min = *it;
  }
  return _min;
}

CB M:: LastM(std::set<CB> _coi, int pid) {
 

  std::set<CB>::iterator it, it_end;
  it_end = _coi.end();

  CB _max(pid, -1);

  for(it = _coi.begin(); it != it_end; it++) {
    if((*it)._pid == pid && (*it)._index > _max._index)
      _max = *it;
  }

  return _max;
}

_MIterator M:: IsPresent(std::pair<CB, CB> & p)
{
  // std::pair <int, int> a = std::pair<int, int>(p.first._pid, p.second._pid);
  
  // std::pair <int, int> b = std::pair<int, int>(p.first._index, p.second._index);
 
  // if(a.first == -1 || a.second == -1 ||
  //    b.first == -1 || b.second == -1) 
  //   assert(false);
  
  // _MIterator it, it_end;
  
  
  // it_end = _MSet.end();
  
  // std::pair <_MIterator, _MIterator> ret;
  
  // ret = _MSet.equal_range(a);
  

  // for(it = ret.first; it != ret.second; it++)
  //   if((*it).second.first == b.first && (*it).second.second == b.second)
  //     return it;

  // return it_end; 
  int fPID = p.first._pid;
  int sPID = p.second._pid;

  if(fPID == -1 || sPID == -1)
    assert(false);

  _MIterator it, it_end;
  it_end = _MSet.end();
  
  std::pair <_MIterator, _MIterator> ret;
  
  ret = _MSet.equal_range(std::pair<int, int>(fPID, sPID));
  
  for(it = ret.first; it != ret.second; it++){
    
    if((*it).second.front() == p.first && (*it).second.back() == p.second)
      return it;
  }
  return it_end;
}


void M::RemoveME(std::pair <CB, CB> p){
  _MIterator i = IsPresent(p);
   
  if(i != _MSet.end()){
    _MSet.erase(i);

    //Debug Print
      // std::cout << "removed a match edge: "
      // 		<< p.first  
      // 		<<  " --- "  
      // 		<< p.second
      // 		<< std::endl;


  }
  
}

void M::AddME (std::pair<CB, CB> p)
{

  //  _MSet.push_back(p);
  std::pair <int, int> a = std::pair<int, int>(p.first._pid, p.second._pid);
  
  //std::pair <int, int> b = std::pair<int, int>(p.first._index, p.second._index);
  std::list <CB> b; 
  b.push_back(p.first);
  b.push_back(p.second);
  
  // _MIterator it = IsPresent(p);

  //if(it == _MSet.end()){
      _MSet.insert(std::pair <ME, PE> (a,b));
      
    //Debug Print
    // std::cout << "added a M^o edge: "
    // 		<< p.first  
    // 		<<  " ---> "  
    // 		<< p.second
    // 		<< std::endl;
}

bool M::Update(std::pair <CB, CB> _pcer) {
  _MIterator it = IsPresent(_pcer);
 
  if(it == _MSet.end()){
    // _MSet.push_back(_pcer);
    AddME(_pcer);
    return true;
  }

  // return false, _pcer is present
   return false;
}



void M::UpdateAncs(Node* ncurr, int _psrc, int _pdst, std::set<int> _aset, int &_diff) 
{
  std::set<int>::reverse_iterator _sit, _sitend;
  _sitend = _aset.rend();
  
  // reverse iterate on the set to find immediate ancestors

  for(_sit = _aset.rbegin(); _sit != _sitend; _sit++) {
    
    CB  _cb (_pdst, *_sit);

    Transition *_t = ncurr->GetTransition(_cb);
    
    Envelope *env = _t->GetEnvelope(); 
    
    /* Find Ancestor receives that can match Scurr */
    if(env->isRecvTypeOnly()) {
    
      if(!(env->src==WILDCARD)) { // env is recv and is deterministic

	if(env->src == _psrc) {  // check is sourcing from Scurr's pid
	  
	  if (Update(std::pair<CB, CB> (ncurr->curr_match_set.front(), _cb)))
	    _diff--;                       
	}
      
      }
      else { // env is a wildcard

	if (Update(std::pair<CB, CB> (ncurr->curr_match_set.front(), _cb))) 
	  _diff--;
      }
    
    }
    
    if(_diff == 0)
      return;
  }
}

void M::UpdateDesc(Node* ncurr, Node* nlast, int _psrc, int _pdst, std::set<int> _aset, int &_diff) 
{
    
  std::set<int>::iterator _sit, _sitend;
  _sitend = _aset.end();

  // reverse iterate on the set to find immediate ancestors
  for(_sit = _aset.begin(); _sit != _sitend; _sit++) {
    
    //    CB _cb = *(new CB(_pdst, *_sit));
    CB _cb (_pdst, *_sit);
    
    Transition *_t = nlast->GetTransition(_cb);
    
    //std::cout << "_cb: " << _cb << std::endl;
    //std::cout << "ncurr_curr_match_set : " << ncurr->curr_match_set.front() << std::endl;
   
    Envelope *env = _t->GetEnvelope();
    
    if(env->isRecvTypeOnly()){
    
      if(!env->src_wildcard) {
	
	if(env->src == _psrc) {
	  
	  if (Update(std::pair<CB, CB> (ncurr->curr_match_set.front(), _cb)))
	    _diff--;                       
	}
      }
      else {
	
	if (Update(std::pair<CB, CB> (ncurr->curr_match_set.front(), _cb))) 
	  _diff--;
      }
         
    }
    
    if(_diff == 0)
      return;
  }
}

void M::getAllDescSends(Node *n, Node *nlast, std::set<int> &snd_set)
{
  Envelope *senv; 
  senv = n->GetTransition(n->curr_match_set.front())->GetEnvelope();
  snd_set = nlast->getAllDescendants(n->curr_match_set.front());
  // delete from the ancestors set if not send type				
  for(std::set<int>::iterator sit = snd_set.begin(); 
      sit != snd_set.end();){
    CB cb_for_sit(n->curr_match_set.front()._pid, *sit);
    Envelope *env_for_sit;
    env_for_sit = nlast->GetTransition(cb_for_sit)->GetEnvelope();
    
    if (!env_for_sit->isSendType() || (env_for_sit->dest != senv->dest)){
      std::set<int>::iterator del_iter;
      del_iter = sit;
      ++sit;
      snd_set.erase(del_iter);
      continue;
    }
    else
      ++sit;
  } // end for

}


void M:: getAllAncsSends(Node *n, std::set<int> & snd_set)
{
  Envelope *senv; 
  senv = n->GetTransition(n->curr_match_set.front())->GetEnvelope();
  snd_set = n->getAllAncestors(n->curr_match_set.front());
  // delete from the ancestors set if not send type				
  for(std::set<int>::iterator sit = snd_set.begin(); 
      sit != snd_set.end();){
    CB cb_for_sit(n->curr_match_set.front()._pid, *sit);
    Envelope *env_for_sit;
    env_for_sit = n->GetTransition(cb_for_sit)->GetEnvelope();
    
    if (!env_for_sit->isSendType() || (env_for_sit->dest != senv->dest)){
      std::set<int>::iterator del_iter;
      del_iter = sit;
      ++sit;
      snd_set.erase(del_iter);
      continue;
    }
    else
      ++sit;
  } // end for
}

void M::getAllDescRecvs(Node *n, Node *nlast, std::set<int> & recv_set)
{
  int sndr_id; 
  sndr_id = n->GetTransition(n->curr_match_set.front())->GetEnvelope()->id;
  
  recv_set = nlast->getAllDescendants(n->curr_match_set.back());

  // delete from descendantss 
  for(std::set<int>::iterator sit =recv_set.begin(); 
      sit != recv_set.end(); ){
    
    CB cb_for_sit(n->curr_match_set.back()._pid, *sit);
    Envelope *env_for_sit;
    env_for_sit = nlast->GetTransition(cb_for_sit)->GetEnvelope();
    
    if (!env_for_sit->isRecvTypeOnly() || 
	(env_for_sit->src != sndr_id && 
	 env_for_sit->src != WILDCARD)){
      std::set<int>::iterator del_iter;
      del_iter = sit;
      ++sit;
      recv_set.erase(del_iter);
      continue;
    }
    else
      ++sit;
  } // end for
}

void M::getAllAncsRecvs(Node *n, std::set<int> & recv_set)
{
  int sndr_id; 
  sndr_id = n->GetTransition(n->curr_match_set.front())->GetEnvelope()->id;
  
  recv_set = n->getAllAncestors(n->curr_match_set.back());
  // delete from ancestors 
  for(std::set<int>::iterator sit =recv_set.begin(); 
      sit != recv_set.end(); ){
    
    CB cb_for_sit(n->curr_match_set.back()._pid, *sit);
    Envelope *env_for_sit;
    env_for_sit = n->GetTransition(cb_for_sit)->GetEnvelope();
    
    if (!env_for_sit->isRecvTypeOnly() || 
	(env_for_sit->src != sndr_id && 
	 env_for_sit->src != WILDCARD)){
      std::set<int>::iterator del_iter;
      del_iter = sit;
      ++sit;
      recv_set.erase(del_iter);
      continue;
    }
    else
      ++sit;
  } // end for
}

void M::NewConstructAncs(Node *ncurr, Node* nlast)
{

  if(ncurr->curr_match_set.back()._pid >= ncurr->NumProcs() || 
     ncurr->curr_match_set.back()._pid < 0){
    //assert(false);
    return;
  }
  Envelope *renv;
  Envelope *senv;
  if (!ncurr->curr_match_set.empty()){
    renv = ncurr->GetTransition(ncurr->curr_match_set.back())->GetEnvelope();
    senv = ncurr->GetTransition(ncurr->curr_match_set.front())->GetEnvelope();
    
    int _psrc = ncurr->curr_match_set.front()._pid; // sending process pid
    
    int _pdst = ncurr->curr_match_set.back()._pid;  // recv process pid

    if(renv->isRecvTypeOnly()){
      
      /* Add the match-set Scurr-Rcurr to the Mo set */
      bool flag = Update(std::pair<CB, CB> (ncurr->curr_match_set.front(),
					    ncurr->curr_match_set.back() ));
      
      /* 
	 1) Get the ancestor sends of curr_match_set.front() which have
	    the same destination
	 2) Get all ancestor receives of curr_match_set.back() that syntactically 
	    match the curr_match_set.front()
	 NOTE: possible optimization here -- memoization
      */
      std::set<int> _all_ancestors_of_send;
      std::set<int> _all_ancs_recv_that_match_send;
      
      getAllAncsSends(ncurr, _all_ancestors_of_send);
      getAllAncsRecvs(ncurr, _all_ancs_recv_that_match_send);
      
      /* get differential count between <Scurr, Rcurr> */
      int _diff = _all_ancs_recv_that_match_send.size() - _all_ancestors_of_send.size();
    
	 // debug print
	 //std::cout << " is = " << _diff << std::endl;
	 
	 std::set<int> _aset;
	 std::vector<int> _alist;
	 
	 _alist.push_back(ncurr->curr_match_set.back()._index);
       
       // debug print
       //
       //_aset = getAllAncestorList(ncurr, ncurr->curr_match_set.back());
       //std::cout << ncurr->curr_match_set.back() << std::endl;
	 
       while(_diff > 0) {
	 
  	 _aset = getImmediateAncestorList(ncurr, _pdst, _alist);
  	 
	 if(_aset.empty()) {
  	   // debug print
  	   //
  	   //std::cout << "breaking out from empty set\n";
  	   break;
  	 }
     
  	 UpdateAncs(ncurr, _psrc, _pdst, _aset, _diff);

  	 //std::cout << "diff after CeR : " << _diff << std::endl;
  	 // assert(_diff == 0);
	 
	 _alist.clear();
	 
	 for(std::set<int>::iterator it = _aset.begin(); it != _aset.end(); it++){
	   
	   _alist.push_back(*it);
	   
	 }
	 
       }
      
    }
  }
}

void M::ConstructAncs(Node* ncurr, Node* nlast) 
{
  
  if(ncurr->curr_match_set.back()._pid >= ncurr->NumProcs() || 
     ncurr->curr_match_set.back()._pid < 0)
    return;
  
  Envelope *env;
  if (!ncurr->curr_match_set.empty()){
    
    env = ncurr->GetTransition(ncurr->curr_match_set.back())->GetEnvelope();
    
    if(env->isRecvTypeOnly()){
      
      /* Add the match-set Scurr-Rcurr to the Mo set */
      bool flag = Update(std::pair<CB, CB> (ncurr->curr_match_set.front(),
					    ncurr->curr_match_set.back() ));
      
      
      //std::cout << *env <<std::endl;
      
      int _psrc = ncurr->curr_match_set.front()._pid; // sending process pid
      
      int _pdst = ncurr->curr_match_set.back()._pid;  // recv process pid
      
      // Debug print
      // std::cout << "diff for CB pair: [" 
      // 	   << ncurr->curr_match_set.front() << ","
      // 	   << ncurr->curr_match_set.back() << " ]";	  
      
      /* get differential count between <Scurr, Rcurr> */
      int _diff = ncurr->_countracker.getDiffCount(_psrc, _pdst);
      
      // debug print
      //std::cout << " is = " << _diff << std::endl;
      
      std::set<int> _aset;
      std::vector<int> _alist;
      
      _alist.push_back(ncurr->curr_match_set.back()._index);
	 
      // debug print
      //
      //_aset = getAllAncestorList(ncurr, ncurr->curr_match_set.back());
      //std::cout << ncurr->curr_match_set.back() << std::endl;
      
      while(_diff > 0) {
	
	_aset = getImmediateAncestorList(ncurr, _pdst, _alist);
  	
	if(_aset.empty()) {
	  // debug print
	  //
	  //std::cout << "breaking out from empty set\n";
	  break;
  	 }
	
	UpdateAncs(ncurr, _psrc, _pdst, _aset, _diff);
	
	//std::cout << "diff after CeR : " << _diff << std::endl;
	// assert(_diff == 0);
	 
	_alist.clear();
	
	for(std::set<int>::iterator it = _aset.begin(); 
	    it != _aset.end(); it++){
	   
	  _alist.push_back(*it);
	  
	}
	
      }
      // assert(_diff == 0);
    }
  }
}

void M::NewConstructDesc(Node* ncurr, Node* nlast) 
{
  if(ncurr->curr_match_set.back()._pid >= ncurr->NumProcs()  )
    return;
  
  if (!ncurr->curr_match_set.empty()){
    Envelope* renv = ncurr->GetTransition(ncurr->curr_match_set.back())->GetEnvelope();
    
    if(renv->isRecvTypeOnly()){
      
      int _psrc = ncurr->curr_match_set.front()._pid; // sending process pid
      int _pdst = ncurr->curr_match_set.back()._pid;  // recv process pid
      
      std::set<int> _all_descendants_of_send;
      std::set<int> _all_descendant_recv_that_match_send;
      
      //[svs]: its cruicial to operate with nlast since
      // for any other node the _intra_cb info is not complete
      getAllDescSends(ncurr, nlast, _all_descendants_of_send);
      getAllDescRecvs(ncurr, nlast, _all_descendant_recv_that_match_send);
      
      /* get differential count between <Scurr, Rcurr> */
      int _ldiff = _all_descendant_recv_that_match_send.size() -
	_all_descendants_of_send.size();
      
      std::set<int> _dset;
      std::vector<int> _dlist;
      
      _dlist.push_back(ncurr->curr_match_set.back()._index);
      
      while(_ldiff > 0) {
	_dset = getImmediateDescendantList(nlast, _pdst, _dlist);
	
	if(_dset.empty()) {
	  break;
	}
	
	UpdateDesc(ncurr, nlast, _psrc, _pdst, _dset, _ldiff);
	
	_dlist.clear();
	
	for(std::set<int>::iterator it = _dset.begin(); it != _dset.end(); it++){
	  _dlist.push_back(*it);                
	}
      }
    }
  }
}

void M::ConstructDesc(Node* ncurr, Node* nlast) {
  
  if(ncurr->curr_match_set.back()._pid >= ncurr->NumProcs()  )
    return;
  
  if (!ncurr->curr_match_set.empty()){
    Envelope* env = ncurr->GetTransition(ncurr->curr_match_set.back())->GetEnvelope();
    
    if(env->isRecvTypeOnly()){
      
      int _psrc = ncurr->curr_match_set.front()._pid; // sending process pid
      int _pdst = ncurr->curr_match_set.back()._pid;  // recv process pid
      
      int ldiffa = nlast->_countracker.getDiffCount(_psrc, _pdst); 
      int ldiffb = ncurr->_countracker.getDiffCount(_psrc, _pdst);
      
      int _ldiff = ldiffa - ldiffb;
      
      // debug print
      //
      //std::cout << "_ldiff : " << _ldiff << std::endl;
      //nlast->_countracker.debugPrint();
      //ncurr->_countracker.debugPrint();
      //
      //debug print
      //
      //std::set<int> _dset = getAllDescendantList(nlast, ncurr->curr_match_set.back());
      //std::cout << ncurr->curr_match_set.back() << std::endl;
      
      std::set<int> _dset;
      std::vector<int> _dlist;
      
      _dlist.push_back(ncurr->curr_match_set.back()._index);
      
      while(_ldiff > 0) {
	
	_dset = getImmediateDescendantList(nlast, _pdst, _dlist);
	
	if(_dset.empty()) {
	  break;
	}
	
	UpdateDesc(ncurr, nlast, _psrc, _pdst, _dset, _ldiff);
	
	_dlist.clear();
	
	for(std::set<int>::iterator it = _dset.begin(); it != _dset.end(); it++){
	  _dlist.push_back(*it);                
	}
      }
      // assert(_ldiff == 0);
    }
  }
}

void M::_MConstruct(ITree *itree) {

  Node *nlast, *ncurr;

  // get the last node of itree
  nlast = itree->_slist[itree->_slist.size()-1];

  std::vector<Node*>::iterator _slist_it, _slist_it_end;

  _slist_it_end = itree->_slist.end();

  // iterate over the state vector for the current trace
  for(_slist_it = itree->_slist.begin();
      _slist_it != _slist_it_end; _slist_it++) {
  
    ncurr = *(_slist_it);
  
    // debug print
    //   ncurr->_countracker.debugPrint();
    
    // std::cout<< "Going in Ancs construction ..." <<std::endl;
    NewConstructAncs(ncurr, nlast);
    // std::cout<< "success in Ancs construction" <<std::endl;
    NewConstructDesc(ncurr, nlast);
  }
}


std::set<CB> M::collectMo(std::set<CB> l){

  std::set<CB> ret;

  std::set<CB>::iterator it, ite;
  
  ite = l.end();
  
  for(it = l.begin(); it != ite; it ++){
    std::set<CB> tmp = MImage(*it);
    ret.insert(tmp.begin(), tmp.end());
    
  }
  
  // std::cout<< "The size of CollectMo set is:" 
  // 	   << ret.size()
  // 	   <<std::endl;
  
  
  return ret;

}

std::list<CB> & M::getCBLst(_MIterator iter)
{
  // std::list<CB> res;

  // CB c1((*iter).first.first, (*iter).second.first);
  // CB c2((*iter).first.second, (*iter).second.second);
  // res.push_back(c1);
  // res.push_back(c2);
  
  // return res;
  
  return (*iter).second;
  
  
}
