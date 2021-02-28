#ifndef __MO__HPP__
#define __MO__HPP__

#include "InterleavingTree.hpp"
#include <vector>
#include <set>
//#include <map>
#include <cassert>

typedef std::list<CB> PE;

typedef std::pair<int, int> ME; //

struct mapComp1 {
  bool operator () ( const ME & fst, const ME& snd) const
    
  {
    if (fst.first < snd.first) return true;
    else if (fst.first == snd.first 
	     && fst.second < snd.second) return true;
    else
      return false;
    
  }

};

//typedef std::multimap<ME, ME, mapComp1>_M;
typedef std::multimap<ME, PE, mapComp1>_M;
//typedef std::multimap<ME,ME, mapComp1>::iterator _MIterator;
typedef std::multimap<ME,PE, mapComp1>::iterator _MIterator;

// typedef std::vector<ME> _M;
// typedef std::vector<ME> ::iterator _MIterator;

class M {

public:

  // default constructor
  M();
  ~M(){}

  /*
   * Constructs the ME between MPI calls
   * ITree*: interleaving tree to iterate through the states
   */
  void _MConstruct(ITree*);

  /*
   * Checks if a ME exists in Mo set
   * ME to check
   * returns iterator pointing to the ME object
   */

  _MIterator IsPresent( std::pair<CB, CB> & );
  /*
   * removes the ME from Mo set
   * ME to be removed
   */
  void RemoveME(std::pair<CB, CB>);

  /*
   * adds ME to the Mo set
   * ME to be added
   */
  void AddME (std::pair<CB, CB>);

  /*
   * Should add comment
   */
  void UpdateAncs(Node*, int, int, std::set<int>, int&);

  /*
   * should add comment
   */
  void UpdateDesc(Node*, Node*, int, int, std::set<int>, int&);

  /*
   * should add comment
   */
  bool Update(std::pair<CB, CB>);

  /*
   * Constructs _MSet for all operations lying in the Ancestor of Scurr --- Rcurr  match
   */
  void ConstructAncs(Node*, Node*);

  /*
   * Constructs _MSet for all operations lying in the Descendant of Scurr --- Rcurr  match
   */
  void ConstructDesc(Node*, Node*);

  /*
   * Prints the elements in Mo set
   */
  void DebugPrint();

  /*
   * Computes the M(c) -- all potential matching partners of c.
   */
 
  std::set<CB> MImage(CB c);

  /*
   * Computes the M(c) -- all potential matching partners of c restricted
   * before the index of d.
   */
  std::set<CB> MImageRestricted(CB c, CB d);


  /*
   * should add comment
   */

  bool CheckMImage(CB c , int pid);

  /*
   * should add comment
   */

  std::set<CB> MImageNotFrom(CB c, int);

  /*
   * Computes first operation in M(c) wrt pid
   */
  CB FirstM(std::set<CB>, int);

  /*
   * Computes last operation in M(c) wrt pid
   */
  CB LastM(std::set<CB>, int);

  std::set<CB> collectMo(std::set<CB> l);
 
  /*
   * << overloaded to print ME in the desired format
   */
  friend std::ostream &operator<< (std::ostream &os, const M &c);

  void wrToFile(std::ofstream &filename);
 
  std::list<CB> & getCBLst(_MIterator iter);


  void getAllAncsSends(Node *n, std::set<int> & snd_set);
  
  void getAllAncsRecvs(Node *n, std::set<int> & recv_set);
  
  void NewConstructAncs(Node *ncurr, Node* nlast);
  
  void NewConstructDesc(Node* ncurr, Node* nlast) ;

  void getAllDescSends(Node *ncurr, Node *nlast, std::set<int> &snd_set);

  void getAllDescRecvs(Node *ncurr, Node *nlast,  std::set<int> & recv_set);

  _M _MSet;
};



#endif
