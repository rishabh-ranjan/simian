#ifndef __SMTENCODING_HPP__
#define __SMTENCODING_HPP__

#ifndef BUFFER_SIZE
#define BUFFER_SIZE 2056
#endif

#include "Encoding.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include "ExpressionStructUtility.hpp"

#include "simian-core/verifier.h"
#include "simian-core/bliss-0.73/graph.hh"

class Cover
{
  public:
    Cover(int id, int index)
    {
      this->id = id;
      this->index = index;
      collectiveEvent = send = recv = false;
    }
    int id;
    int index;
    bool collectiveEvent;
    bool send;
    bool recv;
    int dest;
    int src;
    int tag;
    std::string name;
    int data;
};

class SMTEncoding : public Encoding {
  
public:
  SMTEncoding(
			std::unordered_map< int, std::vector< EpochInfo >>& verified_hashes,
			ITree *it, M *m/*, propt *_slv*/):
		Encoding(it, m/*, _slv*/), width(0), eventSize(0),
		ver(verified_hashes)
  {
    noVars = 1;
    s = new solver(c);
  }

  /*SMTEncoding(const SMTEncoding & smtobj) : Encoding(smtobj._it, smtobj._m, smtobj.slv)
  { 
    std::cout << "\nInside copy constructor\n";
    noVars = smtobj.noVars;
    copy((smtobj.assertsStore).begin(), (smtobj.assertsStore).end(), back_inserter(assertsStore));
  }*/

  expr new_bool_val(bool b)
  {
    return c.bool_val(b);    
  }

  expr new_int_val(int i)
  {
    return c.int_val(i);    
  }

  expr new_bool_const()
  {
    /*std::ostringstream strm;
    strm << noVars;
    std::string name = strm.str();*/
    std::string name = std::to_string(noVars);
    noVars++;
    return c.bool_const(name.c_str());    
  }

  expr new_int_const()
  {
    /*std::ostringstream strm;
    strm << noVars;
    std::string name = strm.str();*/
    std::string name = std::to_string(noVars);
    noVars++;
    return c.int_const(name.c_str());    
  }

  expr new_int_const(std::string name)
  {
    noVars++;
    return c.int_const(name.c_str());    
  }

  expr new_bool_const(std::string name)
  {
    noVars++;
    return c.bool_const(name.c_str());    
  }

  expr new_bv_const(unsigned width)
  {
    std::ostringstream strm;
    strm << noVars;
    std::string name = strm.str();
    //std::string name = std::to_string(noVars);
    noVars++;
    return c.bv_const(name.c_str(), width);
  } 
  
	Verifier ver;
	void init_ver();
	void add_condition(const std::string&);
	/*
	//void test(char** asserts); // rishabh
	//--rishabh
	void init_verifier(Verifier&);
	void add_condition(Verifier&, const std::string&);
	bool run_verifier(Verifier&);
	//rishabh--
	*/

  //creation of literals m_a, i_a,
  // and bitvectors for maintaining clocks
  void createEventCovers(std::vector<std::pair<int, int> >);
  void createMatchLiterals();
  void createOrderLiterals(); // <-- createBVEventLiterals();
  void displayMatchSet(); // variant of printMatchSet in Encoding.cpp
  expr getClkLiteral(CB, CB);
  expr getMatchLiteral(MatchPtr);
  std::pair<expr, expr> getMILiteral(CB);
  expr_vector getMatchExpr(CB);
  std::string getLitName(expr, int);
  std::string getClkLitName(expr, CB, CB);
  expr getEventBV(CB);
  expr getClk(CB);
  expr getExprAtIndex(expr_vector, int);
  MatchPtr getMPtr(CB);
  void isLitCreatedForCollEvent(Cover*, expr &, std::string, int);

  void encodingConditionals(/*Verifier&, */char**, int);
  void encodingPartialOrders();
  bool checkResult();
  bool possibleMatchAvailable(std::vector<expression>);
  bool changeConditionals(std::vector<expression> &);
  void generateConstraints(bool);
  void createDependencies();
    

  void insertClockEntriesInMap(CB, CB, expr);

  //constraint generation functions
  void partialOrderConstraints(); // <-- createClkLiterals(); partial order constraint + clock differenc
  void createRFSomeConstraint(); // Match correct
  void createMatchConstraint();//Matched Only
  void matchPairs();// match pairs for wildcard receives
  void matchCollectives(); // match collectives
  void atLeastOneSend(); // at least one of the match sets must be true
  void uniqueMatchSend();// unique match for send constraint
  void uniqueMatchRecv(); // unique match for recv constraint
  void noMoreMatchesPossible(); // No more matches possible
  void allAncestorsMatched(); // All ancestors matched  
  void notAllMatched(); // not all matched
  void matchImpliesIssued();// match only  issued
  void waitMatch(); // matches when waits have 
  void conditionalMatchesBefore(); // Conditional matches before condition encoded
  void kBuffering(); // Implementing k-buffering semantics

  bool isNecessaryEdgeRequired(Envelope*, Envelope*);
  bool isSatisfied(solver*);
  bool isExplored(expr);

#if 0  
  void createSerializationConstraint();
  void createFrConstraint();
  void createUniqueMatchConstraint();
  void alternativeUniqueMatchConstraint();
  void allFstIssued();
  void transitiveConstraint();
  void allAncestorsMatched();
  void totalOrderOnSends();
  void totalOrderOnRacingSends();
  void nonOverTakingMatch();
  void makingMatchesHappenSooner();
#endif

  //printing
  void printEventMap();
  void publish();
  

  // bitvector related functions
  void set_width();
  unsigned get_width();
  void create_bv(expr &);
  unsigned address_bits();
  
public:

  std::map<std::pair<expr_vector, Cover*>, CB> revEventMap;
  std::map<CB, std::pair<expr_vector, Cover*> > eventMap;
  
  std::map<expr, MatchPtr> revCollMap;
  std::map<MatchPtr,std::pair<expr, expr> > collEventMap;

  std::map<std::string, expr> matchMap;
  std::map<expr, std::string> revMatchMap;
  std::map<MatchPtr, expr> match2exprMap;

  std::map<expr, std::pair<CB, CB> > revClkMap;
  std::map<std::pair<CB, CB>, expr > clkMap; 

  std::map<expr, std::pair<MatchPtr, CB> > revClkMapCollEvent;
  std::map<std::pair<MatchPtr, CB>, expr > clkMapCollEvent; 
  
  std::map<expr, std::pair<CB, MatchPtr> > revClkMapEventColl;
  std::map<std::pair<CB, MatchPtr>, expr > clkMapEventColl; 
  
  std::map<expr, std::pair<MatchPtr, MatchPtr> > revClkMapCollColl;
  std::map<std::pair<MatchPtr, MatchPtr>, expr > clkMapCollColl; 

  std::map<CB, expr> bvEventMap;
  std::map<expr, CB> revBVEventMap;
  
  std::map<MatchPtr, expr> collBVMap;
  std::map<expr, MatchPtr> revCollBVMap;

  std::map<std::string, expr> PMap;
  std::map<std::pair<CB, CB>, expr> revPMap;
  std::map<std::string, expr> QMap;
  std::map<std::pair<CB, CB>, expr> revQMap;  
  
  unsigned width; 
  unsigned eventSize;  

  solver* s;
  unsigned noVars;
  expr k = new_int_const("kBuffer");

  //dhriti
  // Vector of pairs of expressions and the start,end line numbers of the corresponding 'if'
  std::vector<std::pair<int, std::vector<std::pair<expr, std::pair<int, int> > > > > assertsStore; // Used to store the conditions (of which we create the combinations)
  std::vector<std::pair<int, int> > lineNumbers; // Functions between these line numbers are to be ignored whenever we form Match-sets or Event Covers
  std::vector<std::pair<int, int> > ranksIndices; // Functions after this index in rank's process are to be ignored --> For loops
  std::vector<std::pair<int, int> > allRanksIndices; // Ranks and indices of all wildcard receives after which an 'if' is present
  static std::vector<expr> POSs; // Keep a vector of POSs to keep track of what has been seen so far, we do not want to repeat exploring some set of conditions
  static std::deque<std::pair<std::string, expr_vector> > seenConstraints;
  static int procToExplore;
  std::vector<std::pair<CB, CB> > dependencies;
 };


#endif
