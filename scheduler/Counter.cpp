#include "Counter.hpp"
#include <iostream>
#include <assert.h>
#include <stdlib.h>

CountDouble::CountDouble() { 
  SndCount = 0;
  RecvCount = 0;
  // WCRecvCount = 0;
}

CountDouble& CountDouble::operator=(const CountDouble &orig) {
    this->SndCount = orig.SndCount;
    this->RecvCount = orig.RecvCount;
    return *this;
}

void CountDouble::debugPrint() {
    std::cout << "SendCount : " << SndCount << std::endl;
    std::cout << "RecvCount : " << RecvCount << std::endl;
    // std::cout << "WCRecvCount : " << WCRecvCount << std::endl;
}

//CountTracker* CountTracker::pinstance = NULL;

// empty counstructor
CountTracker::CountTracker() {}

CountTracker& CountTracker::operator=(const CountTracker &orig) {
    this->_countmap.insert(orig._countmap.begin(), orig._countmap.end());
    this->_wcmap.insert(orig._wcmap.begin(), orig._wcmap.end());
    return *this;
}

int CountTracker::getSendCount(int src, int dst) {
    _processpair _pp(src, dst);

    CountDoubleMapIterator it;
    it = _countmap.find(_pp);

    assert(it != _countmap.end());

    if(it != _countmap.end()) {
        return (*it).second.SndCount;
    }

    return 0;
}


void CountTracker::updateCount(int src, int dst, bool wildcard) {
    _processpair _pp(src, dst);
    CountDoubleMapIterator it, it_end;
    CountDouble triple;

    it = _countmap.find(_pp);
  
    if(it != _countmap.end()) {
        triple.SndCount = (*it).second.SndCount;
        triple.RecvCount = (*it).second.RecvCount;
	
    }

    triple.SndCount++;
   
    if(!wildcard) {
      triple.RecvCount++;
    }
    else {
        _wcmap[dst]++;
    }
    
    if(it != _countmap.end()) {
      // std::cout << "Updating count entry: (" 
      // 		<< src << "," << dst << ")"
      // 		<< std::endl;
      _countmap[_pp] = triple;
    }
    else {
      // std::cout << "Adding count entry: (" 
      // 		<< src << "," << dst << ")"
      // 		<< std::endl;
      _countmap.insert(std::pair<_processpair, CountDouble> (_pp, triple));
    }
}

void CountTracker::addentry(_processpair pp, CountDouble triple) {
    _countmap[pp] = triple;
}

void CountTracker::debugPrint() {
    CountDoubleMapIterator it, _itend;
    _itend = _countmap.end(); 

    std::cout << "\n";
    for(it = _countmap.begin(); it != _itend; ++it) {
        _processpair _pp = it->first;
        CountDouble triple = it->second;

        std::cout << "(" << _pp.first << "," << _pp.second << ")" << std::endl;
        triple.debugPrint();
    }


    WCMapIterator it1, it1_end;
    it1_end = _wcmap.end();
    std::cout << "\nWC count : ";
    for( it1 = _wcmap.begin(); it1 != it1_end; it1++)
      std::cout << "(" << (*it1).first << "," << (*it1).second << ")";
    std::cout << std::endl;
}

CountDouble CountTracker::getCountDouble(int src, int dst) {
    _processpair pp(src, dst);
    CountDoubleMapIterator it;

    it = _countmap.find(pp);
    
    //   assert(it != _countmap.end());
    
    if(it != _countmap.end())
      return it->second;
    else
      {
	CountDouble triple = CountDouble();
	addentry(pp,triple);
	return CountDouble();
      }
}

int CountTracker::getWCCount(int proc) {
    WCMapIterator it;

    it = _wcmap.find(proc);

    //std::cout << "proc : " << proc << std::endl;
    
    // assert(it != _wcmap.end());
    
    if(it != _wcmap.end())
      return it->second;
   
    return 0;
}

int CountTracker::getDiffCount(int _psrc, int _pdst) {
  // deterministic count double for <pid(Scurr), pid(Rcurr)>
  CountDouble _double = getCountDouble(_psrc, _pdst);  
  
  int _wccount = getWCCount(_pdst);  // Rcurr count for pid(Rcurr)
  int _trecv = _double.RecvCount + _wccount;
  int _tsend = _double.SndCount;
  int _diff = _trecv - _tsend;
  return _diff;
}


// destructor
CountTracker::~CountTracker() {
    _countmap.clear();
    _wcmap.clear();
}

