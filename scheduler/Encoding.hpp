#ifndef __ENCODING_HPP__
#define __ENCODING_HPP__

#include "Mo.hpp"
#include "InterleavingTree.hpp"
#include <map>
#include <stack>
#include "utility.hpp"
//#include "util/threeval.h"
/*
#include "solver-src/flattening/bv_utils.h"
#include <solver-src/sat/satcheck_minisat2.h>
#include <solver-src/sat/satcheck_lingeling.h>
#include <solver-src/sat/dimacs_cnf.h>
*/
#include <memory>
#include "z3++.h"

using namespace z3;

typedef std::list<CB> * MatchPtr; 
typedef std::list<CB> Match; 

typedef std::pair<std::string, int> StrIntPair;

typedef std::set<MatchPtr>::iterator MIter;
typedef Match::iterator LIter;

typedef std::vector<TransitionList *>::iterator TLIter;
typedef std::vector<Transition>::iterator TIter;

//typedef satcheck_minisat_no_simplifiert satcheckt;
//typedef satcheck_minisat_simplifiert satcheck_simplifiert;

#define forall_matchSet(it, MT) \
  for (MIter it = MT.begin(), it_end = MT.end(); \
       it != it_end; it++)

#define forall_match(it, M)			 \
  for (LIter it = M.begin(), it_end = M.end(); \
       it != it_end; it++)


#define forall_transitionLists(it, TL) \
  for (TLIter it = TL.begin(), it_end = TL.end(); \
       it != it_end; it++) 

#define forall_transitions(it, T) \
  for (TIter it = T.begin(), it_end = T.end(); \
       it != it_end; it++)

class Encoding{
public:
  Encoding() {}
  Encoding(ITree *it, M *m/*, propt * */); 
  ~Encoding(){}

  virtual std::string getLitName(expr a);
  void createMatchSet(std::vector<std::pair<int, int> > = std::vector<std::pair<int, int> >(), 
                      std::vector<std::pair<int, int> > = std::vector<std::pair<int, int> >(), 
                      std::vector<std::pair<int, int> > = std::vector<std::pair<int, int> >());

  void createEset(std::vector<std::pair<int, int> > = std::vector<std::pair<int, int> >(), 
                  std::vector<std::pair<int, int> > = std::vector<std::pair<int, int> >(), 
                  std::vector<std::pair<int, int> > = std::vector<std::pair<int, int> >());

  bool toBeDropped(Envelope*, std::vector<std::pair<int, int> >, std::vector<std::pair<int, int> >, std::vector<std::pair<int, int> >);
  void printMatchSet();
  bool isPresent(MatchPtr m);

  Node* last_node; 
  int traceSize;
  M* _m;
  ITree *_it; 
  //literalt one, zero;
  //satcheckt slv;
  //propt * slv;
  //satcheck_simplifiert  slv;
  //bv_utilst * bvUtils;  

  std::set<MatchPtr> matchSet;
  std::map<StrIntPair, expr> sym2lit; 
  std::map<expr, StrIntPair > lit2sym;
  std::vector<std::pair<Envelope*, Envelope*> > Eset; 
  bool _deadlock_found;
  struct timeval constGenStart, constGenEnd, solverStart, solverEnd;

  static context c;
};

// class Encoding0 : public Encoding{
// public: 
//   Encoding0(ITree *it, M *m): Encoding(it, m) {} 
  
//   //main encoding function
//   void Encoding_Matches();
  
//   // helper functions for Encoding_Matches
//   literalt uniqueAtPosition();
//   literalt noRepetition ();
//   literalt partialOrder();
//   literalt extensionNextPos();
//   literalt noMatchAtNextPos();
  

//   literalt uniqueEvents();
//   literalt noRepetitionEvents();
//   literalt partialOrderEvents();
//   literalt extentionEvents();
//   literalt noMatchEvents();
  
//   //helper functions
//   void createMatchLiterals(); 
//   literalt getLiteralFromMatches(MatchPtr, int);

//   //  IntPair getMatchNumeralPositionFromMatches(MatchPtr, int);

//   void publish();

// protected: 
//   std::multimap<MatchPtr, StrIntPair > match2sym; // symbol:= (match, pos)
// };

// class Encoding1 : public Encoding{

// public:
//   Encoding1(ITree *it, M *m): Encoding(it, m) {} 

  
//   //main encoding function
//   void Encoding_Events();
  
//   // helper functions for Encoding_Matches
//   void stutteringAtPosition();
//   void uniqueAtPosition();
//   void noRepetition ();
//   void partialOrder();
//   void extensionNextPos();
//   void noMatchAtNextPos();

//   // general helper functions
//   void createEventLiterals();
//   literalt getLiteralFromEvents(TIter, int);
//   literalt getLiteralFromCB(CB, int);
//   void publish();
// };

// class Encoding2 : public Encoding {
// public:
//   Encoding2(ITree *it, M *m): Encoding(it, m) {} 

  
//   //main encoding function
//   void Encoding_Mixed();
  
//   // helper functions for Encoding_Matches
//   void stutteringAtPosition();
//   void uniqueAtPosition();
//   void noRepetition ();
//   void partialOrder();
//   void extensionNextPos();
//   void noMatchAtNextPos();

//   void createMatchLiterals(); 
//   void createEventLiterals();
//   void createBVEventLiterals();

//   literalt getLiteralFromEvents(TIter, int);
//   literalt getLiteralFromMatches(MatchPtr, int);
//   literalt getLiteralFromCB(CB, int);
//   MatchPtr getMatchPtrFromEvent(CB a);
//   void publish();
// protected: 
//   std::multimap<MatchPtr, StrIntPair > match2sym; // symbol:= (match, pos)
//   };

extern std::stringstream formula;

#endif
