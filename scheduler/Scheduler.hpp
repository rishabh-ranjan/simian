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
 * File:        Scheduler.hpp
 * Description: Scheduler that implements the process scheduling
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _SCHEDULER_HPP
#define _SCHEDULER_HPP
/*removed to use sched-opt.h: te_Exp_Mode
#define EXPLORE_ALL 0
#define EXPLORE_RANDOM 1
#define EXPLORE_LEFT_MOST 2
*/
#define EXPLORE_SOME 3

#ifdef CONFIG_DEBUG_SCHED
#define DR(code) if(Scheduler::_debug) { code }
#define DS(code) { code }
#else
#define DR(code)
#define DS(code)
#endif

// Need forward declarations to eliminate circular dependencies of header files.
class Node;
class ITree;
class M;
//class W;
//class FMEncoding;
class SMTEncoding;
//class SPOEncoding;
//class OptEncoding;
class propt;

#include <list>
#include "ServerSocket.hpp"
#include "MpiProc.hpp"
#include "name2id.hpp"
#include <vector>
#include "Transition.hpp"
#include <fstream>
#include <iostream>
#include "sched-opt.h"
#include <cassert>
#include <set>
#include "ExpressionStructUtility.hpp"

#include <unordered_map>
#include "simian-core/verifier.h"


/* 
 *  HELPER FUNCTIONS
 */

#define MAX_T 500000

class cloneFunctor // --> A utility class to make clone of an object
{
  public:
  template <typename T>
    T* operator() (T* a) 
    {
        return a->clone();
    }
  template <typename T>
    T operator() (T a) 
    {
        return a.clone();
    }
};

/*
 * This inherits from the ServerSocket so that the Scheduler itself acts as
 * the  server and receives client requests directly to it.
 *
 * Implementing Scheduler as a singleton.
 */
class Scheduler : public ServerSocket {

public:
  /*
   * Returns a pointer to a scheduler object
   */
  static Scheduler* GetInstance ();
  /*
   * Starts the scheduling process.
   */
  void  StartMC ();
	void finish(bool);
  /*
   * SetParams - This function must be invoked before any other
   * function can be invoked!
   */
/* == fprs start == */
/* 
  static void SetParams (std::string port, std::string num_clients,
                         std::string server, bool send_block, std::string fname,
                         std::string logfile, std::vector<std::string> fargs,
                         bool mpicalls, bool verbose, bool quiet, bool unix_sockets, 
                         int report_progress, bool fib, bool openmp, 
                         bool use_env_only,
                         bool batch_mode, bool stop_at_deadlock, 
                         te_Exp_Mode explore_mode, int explore_some, 
                         std::vector<int>* explore_all,
                         std::vector<int>* explore_random, std::vector<int>* explore_left_most,
                         bool debug, bool no_ample_set_fix, unsigned bound, bool limit_output);
*/
  static void SetParams (std::string port, std::string num_clients,
                         std::string server, bool send_block, std::string fname,
                         std::string logfile, std::vector<std::string> fargs,
                         bool mpicalls, bool verbose, bool quiet, bool unix_sockets, 
                         int report_progress, bool fib, bool openmp, 
                         bool use_env_only,
                         bool batch_mode, bool stop_at_deadlock, 
                         te_Exp_Mode explore_mode, int explore_some, 
                         std::vector<int>* explore_all,
                         std::vector<int>* explore_random, std::vector<int>* explore_left_most,
                         bool debug, bool no_ample_set_fix, unsigned bound, bool limit_output, bool fprs, int encoding, bool dimacs, bool show_formula, std::string solver, int kbuffer);
/* == fprs end == */
  /*
   * Processes that can run are here.
   */
  static std::vector <MpiProc *> _runQ;

  /*begin boolean formula file modification --[svs] */
  // Encoding0 *e0;  
  // Encoding1 *e1;
  // Encoding2 *e2;
  propt *slv;
  //FMEncoding *fm;
  SMTEncoding *smt;
  SMTEncoding *previousSMT;
  //SPOEncoding *spo;
  //OptEncoding *opt;
  /*end of modification --[svs] */
  
  std::string getProgName(){ return this->ProgName();}
  std::string getNumProcs() {return this->_num_procs;}
private:
  /*
   * Constructor: The constructor requires the port number it
   * must run on and the number of clients (MPI processes) that
   * it must schedule.
   *
   * Need to figure out how Scheduler will integrate with the
   * DPOR interleaver (for that matter any interleaver!)
   */
  
  Scheduler ();
  
  /*
   * copy constructor
   */
  Scheduler (const Scheduler &);

  /*
   * Assignment operator.
   */

  Scheduler& operator= (const Scheduler &);
  /*
   * Start Clients
   */
  void StartClients ();
    
  int Restart ();
  std::string ProgName ();
  int generateFirstInterleaving ();
  void putInExploredCategory(std::vector<expression>);
  Transition *getTransition (int);
  bool hasMoreEnvelopeToRead ();
  int acceptAsserts();

/* [svs] == SAT encoding */
//  void  generateErrorTrace();
/* [svs] == end SAT encoding */

  ITree* it;
  ITree* previousITree;

  /* begin CeR modification -- svs */
  M *m;
  //W *w;
  /* end of modification --svs */


  static Scheduler*               _instance;
  static std::string              _fname;
  static std::vector<std::string> _fargs;
  static std::string              _port;
  static std::string              _server;
  static bool                     _param_set;
  static int                      _errorno;
  static fd_set                   _fds;
  static SOCKET                   _lfd;
  static std::map <SOCKET, int>   _fd_id_map;
  static Node                     *root;

	std::unordered_map< int, std::vector< EpochInfo >> verified_hashes;
  
public: 
  static std::string              _num_procs;
  static int                      interleavings;
  static int                      terminated;
  static bool                     _send_block;
  static bool                     _mpicalls;
  static bool                     _verbose;
  static bool                     _quiet;
  static bool                     _unix_sockets;
  static int                      _report_progress;
  static bool                     _fib;
  static std::ofstream            _logfile;
  static std::stringstream        _mismatchLog;//CGD
  static bool                     _openmp;
  static bool                     _env_only;
  static bool                     _batch_mode;
  static bool                     _probed;
  static te_Exp_Mode              _explore_mode;
  static int                      _explore_some;
  static bool                     _just_dead_lock;
  static bool                     _stop_at_deadlock;
  static bool                     _deadlock_found;
  static bool                     _debug;
  static bool                     _no_ample_set_fix;
  static unsigned                 _bound;
  static bool											_limit_output;//CGD
  static int                      _kbuffer;
  
  //  static int  traceLength;
  int                                    order_id;
  static std::vector<int>                *_explore_all;
  static std::vector<int>                *_explore_random;
  static std::vector<int>                *_explore_left_most;
/* == fprs start == */
  static bool                     _fprs;
/* == fprs end == */
  
/* svs == SAT encoding */
  static int _encoding;
  static bool _errorTrace;
  static bool _dimacs;
  static bool _formula;
  static std::string _solver;
  /* svs -- end SAT encoding */
  static char** assertsData; //dhriti
  static std::vector<expression> changedConditionals; //dhriti
};
#endif
