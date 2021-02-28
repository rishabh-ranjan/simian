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
 * File:        Scheduler.cpp
 * Description: Scheduler that implements the process scheduling
 * Contact:     isp-dev@cs.utah.edu
 */

#include "simian-core/logger.h"

#include <iostream>
#include "Scheduler.hpp"
#include <sys/types.h>
#ifndef WIN32
#include <sys/select.h>
#include <sys/time.h>
#endif
#include <fstream>
#include <sstream>
#include <iterator>
#include <vector>
/* == fprs begin == */
#include <ctime>
/* == fprs end == */
#include "InterleavingTree.hpp"
/* == fprs begin == */
#define ISP_START_SAMPLING 2
#define ISP_END_SAMPLING 3
/* == fprs end == */
#include <string>

/* svs -- Predictive begin */
#include "Mo.hpp"
//#include "Wu.hpp"
/* svs -- Predictive end */

#ifdef _MSC_VER
#pragma warning( disable : 4127 ) // warning C4127: conditional expression is constant
#endif

#include <cstdlib>

/* svs -- SAT solving begin */
//#include "Encoding.hpp"
//#include "FMEncoding.hpp"
#include "SMTEncoding.hpp"
//#include "SPOEncoding.hpp"
//#include "OptEncoding.hpp"
#include "utility.hpp"
#include "stdlib.h"

#include "simian-core/verifier.h"

// dhriti
int SMTEncoding::procToExplore = 0;
std::deque<std::pair<std::string, expr_vector> > SMTEncoding::seenConstraints = {};
std::vector<expr> SMTEncoding::POSs = {};
context Encoding::c;
/* svs -- SAT solving end */

Scheduler * Scheduler::_instance = NULL;
/*
 * Initialize network variables.
 */
std::string Scheduler::_num_procs = "";
std::string Scheduler::_fname = "";
std::vector<std::string> Scheduler::_fargs = std::vector<std::string>();
std::string Scheduler::_port = "";
std::ofstream Scheduler::_logfile;
std::stringstream Scheduler::_mismatchLog;//CGD
std::string Scheduler::_server = "";
bool Scheduler::_send_block = false;
bool Scheduler::_mpicalls = false;
bool Scheduler::_verbose = false;
bool Scheduler::_quiet = false;
bool Scheduler::_unix_sockets = false;
int  Scheduler::_report_progress = 0;
bool Scheduler::_fib = false;
bool Scheduler::_openmp = false;
bool Scheduler::_env_only = false;
bool Scheduler::_probed = false;
bool Scheduler::_batch_mode = false;
int  Scheduler::interleavings = 0;
int  Scheduler::terminated = 0;
te_Exp_Mode  Scheduler::_explore_mode = EXP_MODE_ALL;
int  Scheduler::_explore_some = 0;
std::vector<int>* Scheduler::_explore_all = NULL;
std::vector<int>* Scheduler::_explore_random = NULL;
std::vector<int>* Scheduler::_explore_left_most = NULL;
bool Scheduler::_stop_at_deadlock = false;
bool Scheduler::_just_dead_lock = false;
bool Scheduler::_deadlock_found = false;
bool Scheduler::_param_set = false;
int  Scheduler::_errorno = 0;
bool Scheduler::_debug = false;
bool Scheduler::_no_ample_set_fix = false;
unsigned Scheduler::_bound = 0;
bool Scheduler::_limit_output = false;
int Scheduler::_kbuffer = 0;
/* == fprs start == */
bool Scheduler::_fprs;
/* == fprs end == */

/* svs -- SAT solving begin */
int Scheduler::_encoding = 0;
bool Scheduler::_errorTrace = false;
bool Scheduler::_dimacs = false;
bool Scheduler::_formula = false;
std::string Scheduler::_solver = "";
/* svs -- SAT solving end */

char** Scheduler::assertsData = NULL;
std::vector<expression> Scheduler::changedConditionals;

fd_set Scheduler::_fds;
SOCKET Scheduler::_lfd = 0;
std::map <SOCKET, int> Scheduler::_fd_id_map;

/*
 * Queues maintained by the Scheduler
 */

std::vector <MpiProc *> Scheduler::_runQ;
Node *Scheduler::root = NULL;

static int redrule1cnt = 0 ;
static int redrule2cnt = 0 ;

//int Scheduler::traceLength = 0;

Scheduler * Scheduler::GetInstance () {

  assert (_param_set);

  /*
   * If Instance is already created, just return it.
   */
  if (_instance != NULL) {
    return _instance;
  }

  logger << "instantiate scheduler...";
	ull tic = unix_time_ms();
	_instance = new Scheduler ();
	//statf << "scheduler instantiation time: " << unix_time_ms() - tic << std::endl;
	scheduler_time += unix_time_ms() - tic;

  if (_errorno != 0) {
    delete _instance;
    exit (_errorno);
  }
  return _instance;
}

void Scheduler::SetParams (std::string port, std::string num_clients,
                           std::string server, bool send_block,
                           std::string fname, std::string logfile,
                           std::vector<std::string> fargs,
                           bool mpicalls, bool verbose, bool quiet,
                           bool unix_sockets, int report_progress,
                           bool fib, bool openmp, bool env_only,
                           bool batch_mode, bool stop_at_deadlock,
                           te_Exp_Mode explore_mode, int explore_some,
                           std::vector<int> *explore_all,
                           std::vector<int> *explore_random,
                           std::vector<int> *explore_left_most,
                           bool debug, bool no_ample_set_fix,
                           unsigned bound, bool limit_output
			   /* == fprs start == */
			   , bool fprs,
			   /* == fprs end == */
			   /* svs -- SAT solving begin */
			   int encoding, bool dimacs, bool show_formula, 
			   std::string solver, int kbuffer
			   /* svs -- SAT solving end */
			   ) {//CGD
solver = "z3"; // dhriti
  //solver = "minisat";
  _param_set = true;
  _port = port;
  _num_procs = num_clients;
  _kbuffer = kbuffer;
  _server = server;
  _send_block = send_block;
  _fname = fname;
  _fargs = fargs;
  _mpicalls = mpicalls;
  _verbose = verbose;
  _quiet = quiet;
  _unix_sockets = unix_sockets;
  _report_progress = report_progress;
  _fib = fib;
  _openmp = openmp;
  _env_only = env_only;
  _batch_mode = batch_mode;
  _stop_at_deadlock = stop_at_deadlock;
  _explore_mode = explore_mode; 
  _explore_some = explore_some;
  _explore_all = explore_all;
  _explore_random = explore_random;
  _explore_left_most = explore_left_most;
  _debug = debug;
  _no_ample_set_fix = no_ample_set_fix;
  _bound = bound;
  _limit_output = limit_output;//CGD
  /* == fprs start == */
  _fprs = fprs;
  /* == fprs end == */

/* svs -- SAT solving begin */
  _encoding = encoding;
  _dimacs = dimacs;
  _formula = show_formula;
  _solver = solver;
/* svs -- SAT solving end */

  /* open the logfile for writing */    
  _logfile.open (logfile.c_str());
  if (logfile != "" && ! _logfile.is_open()) {
    if (!quiet) {
      std::cout << "Unable to open file " << logfile << "\n";
    }
    exit (20);
  } else {
    _logfile << num_clients << "\n";
  }
}

Scheduler::Scheduler () :
  ServerSocket (atoi(_port.c_str()),atoi(_num_procs.c_str()),
		_unix_sockets, _batch_mode), order_id(1) {

  _errorno = Start ();

  if (_errorno != 0) {
    if (!_quiet) {
      switch (_errorno) {
      case 13:
	std::cout << "Server unable to open Socket!\n";
	break;
      case 14:
	std::cout << "Server unable to bind to port. The port may already be in use.\n";
	break;
      case 15:
	std::cout << "Server unable to listen on the socket\n";
	break;
      }
    }
    return;
  }

  StartClients ();
  
  _errorno = Accept (); 
  
  if (_errorno != 0) { 
    if (!_quiet) {
      switch (_errorno) {
      case 16:
	std::cout << "Server unable to accept\n";
	break;
      case 17:
	std::cout << "Error reading from socket in server\n";
	break;
      }
    }
    return;
  }

  FD_ZERO (&_fds);
  _lfd = 0;
  for (int i = 0 ; i < atoi (_num_procs.c_str ()); i++) {
    SOCKET fd = GetFD (i);
    FD_SET (fd, &_fds);
    _fd_id_map[fd] = i;
    if (fd > _lfd) {
      _lfd = fd;
    }
  }

  it  = new ITree ( new Node (atoi(_num_procs.c_str ())), ProgName());
  it->sch = this; // [svs] cyclic reference for SAT work
  m = new M();
  //w = new W();
 
  /*
   * Intially all processes are ready to run. Add
   * all procs to the list.
   */

  for (int i = 0 ; i < atoi (_num_procs.c_str ()); i++) {
    _runQ.push_back (new MpiProc (i));
  }
  
 
   //  svs addition for the trace length computation
  //traceLength = 0;

  /* == fprs bedin == */
  it->_is_exall_mode = (bool*) malloc(sizeof(bool)*atoi(_num_procs.c_str()));
  for (int i = 0 ; i < atoi(_num_procs.c_str()) ; i++) it->_is_exall_mode[i] = false;
  /* == fprs end == */
}

std::string Scheduler::ProgName () {

  std::string::size_type pos = _fname.find_last_of ("/", _fname.size ()-1);
  if (pos == std::string::npos) {
    return _fname;
  }
  return _fname.substr (pos+1, _fname.size ()-1);
}

void Scheduler::StartClients () {
  std::vector<const char *> cmd;

  interleavings++;
  const char *use_mpirun_env = getenv("USE_MPIRUN");
  bool use_mpirun = use_mpirun_env && !strcmp(use_mpirun_env, "true");

  if (_batch_mode) {
    std::cout << "Please launch the clients on the cluster with the following command:" << std::endl;
    if (use_mpirun) {
      std::cout << "    mpirun ";
    } else {
      std::cout << "    mpiexec ";
    }
    std::cout << "-n " << _num_procs << " "
	      << "\"" << _fname << "\" "
	      << "0 "
	      << _port << " "
	      << _num_procs << " "
	      << _server << " "
	      << (_send_block ? "1" : "0") << " ";
    for (int i = 0; i < int(_fargs.size()); i++) {
      std::cout << _fargs[i] << " ";
    }
    std::cout << std::endl;
    return;
  }

  if (use_mpirun) {
    cmd.push_back("mpirun");
  } else {
    cmd.push_back("mpiexec");
  }
  cmd.push_back("-n");
  cmd.push_back((const char *)_num_procs.c_str());

  if (_env_only) {
    cmd.push_back("-env");
    cmd.push_back("ISP_USE_ENV");
    cmd.push_back("1");
    cmd.push_back("-env");
    cmd.push_back("ISP_HOSTNAME");
#ifndef WIN32
    if (_unix_sockets) {
      cmd.push_back((const char *)GetSocketFile ().c_str());
      cmd.push_back("-env");
      cmd.push_back("ISP_UNIX_SOCKETS");
      cmd.push_back("1");
    }
    else
#endif
      {
	cmd.push_back((const char *)_server.c_str());
	cmd.push_back("-env");
	cmd.push_back("ISP_UNIX_SOCKETS");
	cmd.push_back("0");
	cmd.push_back("-env");
	cmd.push_back("ISP_PORT");
	cmd.push_back((const char *)_port.c_str());
      }
    cmd.push_back("-env");
    cmd.push_back("ISP_NUMPROCS");
    cmd.push_back((const char *)_num_procs.c_str());
    cmd.push_back("-env");
    cmd.push_back("ISP_SENDBLOCK");
    cmd.push_back(_send_block ? "1" : "0");
  }

  cmd.push_back((const char *)_fname.c_str());
  if (!_env_only) {
#ifndef WIN32
    if (_unix_sockets) {
      cmd.push_back("1");
      cmd.push_back(GetSocketFile ().c_str());
    }
    else
#endif
      {
	cmd.push_back("0");
	cmd.push_back((const char *)_port.c_str());
      }
    cmd.push_back((const char *)_num_procs.c_str());
    cmd.push_back((const char *)_server.c_str());
    cmd.push_back(_send_block ? "1" : "0");

    std::stringstream ss_interleaving;
    ss_interleaving << interleavings;
    cmd.push_back(ss_interleaving.str().c_str());
  }

  for (int i = 0; i < int(_fargs.size()); i++) {
    cmd.push_back((const char *)_fargs[i].c_str());
  }
  cmd.push_back(NULL);

#ifndef WIN32
  int gpid = fork ();
  /*
   * Child process must now exec
   */
  if (gpid == 0) {
    if (execvp (cmd[0], (char* const*) &cmd[0]) < 0) {
      cmd[0] = "mpirun";
      if (execvp (cmd[0], (char* const*) &cmd[0]) < 0) {
	if (!_quiet) {
	  std::cout <<"Unable to Start Clients using exec!\n";
	}
	exit (12);
      }
    }
  } else if (gpid < 0) {
    perror(0);
    exit(1);
  }
  _pid = gpid;
#else
  std::string filename("\"" + _fname + "\"");
  DWORD execvp(const char *file, char const **argv);
  if ((_pid = execvp (NULL, &cmd[0])) == 0) {
    if (!_quiet) {
      std::cout <<"Unable to Start Clients using exec!\n";
    }
    exit (12);
  }
#endif

  if (!_quiet && !_limit_output) {//CGD
    //std::cout << "Process started at pid: " << _pid << std::endl;
  }
}

int Scheduler::Restart () 
{
  // end of modification --svs, sriram 
  /* == fprs begin == */
  if (Scheduler::_fprs) 
  {
    Scheduler::_explore_mode = EXP_MODE_ALL;
  }
  /* == wfchiag end == */
#ifdef FIB
  if (_fib) 
  {
    it->findInterCB ();
    it->FindRelBarriers ();
  }
#endif

  it->GetCurrNode()->PrintLog ();

  //CGD print Mismatched Types
  /* == fprs start == */
  if (_fprs == false) 
  {
    //it->printTypeMismatches();
  }
  /* == fprs end == */

  if (Scheduler::_just_dead_lock) 
  {
    Scheduler::_logfile << Scheduler::interleavings << " DEADLOCK\n";
    Scheduler::_just_dead_lock = false;
    Scheduler::_deadlock_found = true;
  }       
  bool outputused = false;
  std::stringstream outputmsg;
  outputmsg << "-----------------------------------------" << std::endl;
  if (_verbose) 
  {
    outputmsg << *(it->GetCurrNode ());
    outputused = true;
  } 
  else if (!_quiet && !_limit_output) 
  {
    //CGD
    std::vector<TransitionList *>::iterator iter, iter_end;
    iter = it->GetCurrNode ()->_tlist.begin ();
    iter_end = it->GetCurrNode ()->_tlist.end ();
    for (; iter != iter_end; iter++) 
    {
      bool output = false;
      if ((*iter)->_leaks_count != 0) 
      {
        output = true;
        outputmsg << "Rank " << (*iter)->GetId () << ": ";
        outputmsg << (*iter)->_leaks_count << " resource leaks detected.";
      }
      if (_mpicalls) 
      {
        if (!false) 
        {
          outputmsg << "Rank " << (*iter)->GetId () << ": ";
          output = true;
        }
        outputmsg << (*iter)->_tlist.size () << " MPI calls trapped.";
      }
      if (output) 
      {
        outputmsg << std::endl;
      }
      outputused = outputused || output;
    }
  }
  logger << "generate next interleaving...";
	//ull tic = unix_time_ms();
  auto tmp = it->NextInterleaving(Scheduler::changedConditionals);
	// removed because takes ~0ms... dynamic run happens on Restart()
	// which is recorded in firstInterleaving() time
	//statf << "scheduler time: " << unix_time_ms() - tic << std::endl;

  if (tmp)
  {
    if (Scheduler::_deadlock_found && Scheduler::_stop_at_deadlock) 
    {
      std::cout << "Verification stopped. There are still more interleavings to explore\n";
      std::cout << "To continue exploring, please re-run and do not set stop-at-deadlock flag\n";
      ExitMpiProcessAndWait(true);
			statf << "total time: " << unix_time_ms() - start_time << std::endl;
      exit(1);
    }
    if (outputused) 
    {
      outputmsg << "-----------------------------------------" << std::endl;
      std::cout << outputmsg.str();
    }
    it->resetDepth();
#ifdef FIB
    if (_fib) 
    {
      /*
      * At the start of next interleaving clear the taken ample moves
      * in the previous interleaving.
      */
      it->al_curr.clear();
    }
#endif
    int result;

    ServerSocket::Restart ();
    StartClients ();
    if ((result = Accept ()) != 0) 
    {
      if (!_quiet) 
      {
        switch (result) 
        {
          case 16:
          std::cout << "Server unable to accept\n";
          break;
          case 17:
          std::cout << "Error reading from socket in server\n";
          break;
        }
      }
      exit (result);
    }
    for (int i = 0 ; i < atoi (_num_procs.c_str ()); i++) 
    {
      _runQ[i]->_read_next_env = true;
      /* == fprs begin == */
      it->_is_exall_mode[i] = false;
      /* == fprs end == */
    }

    it->ResetMatchingInfo();
    //std::cout << "THERE ARE MORE INTERLEAVINGS!!!\n";
		return 0;
  } 
  else 
  {
		std::cout << "next interleaving not possible. all interleavings exhausted." << std::endl;
    ExitMpiProcessAndWait (true);
    if (!_quiet && !_limit_output) 
    {
      //CGD
      std::string explore_mode = "";
      if (Scheduler::_explore_mode == EXP_MODE_ALL)
        explore_mode = "All Relevant Interleavings";
      else if (Scheduler::_explore_mode == EXP_MODE_RANDOM)
        explore_mode = "Random Choice";
      else if (Scheduler::_explore_mode == EXP_MODE_LEFT_MOST)
        explore_mode = "First Available Choice";

      if (Scheduler::_deadlock_found) 
        outputmsg << "ISP detected deadlock!!!" << std::endl;
      else
        outputmsg << "ISP detected no deadlocks!" << std::endl;

      outputmsg << "Total Explored Interleavings: " << interleavings << std::endl;                
      outputmsg << "Interleaving Exploration Mode: " << explore_mode << std::endl;
      outputmsg << "-----------------------------------------" << std::endl;
      std::cout << outputmsg.str();
    }
#ifdef FIB
    if (_fib) 
    {
      it->printRelBarriers ();
    }
    delete it->last_node;
#endif
    //PrintStats(total_time);
    ExitMpiProcessAndWait (true);
		logger << "done.";
    exit(0);
  }
	return 0;
}


// void PrintStats (unsigned long total_time) {
  
//   std::cout << "Total time = " << 
//     (1.0* total_time)/1000000 << "sec \n";
	

// }

// unsigned long long getTimeElapsed (struct timeval first, 
// 				   struct timeval second) {
//   unsigned long long secs;
//   unsigned long long usecs = 0;

//   if (first.tv_usec > second.tv_usec) {
//     usecs = 1000000 - first.tv_usec + second.tv_usec;
//     second.tv_sec--;
//   } else {
//     usecs = second.tv_usec - first.tv_usec;
//   } //   secs = (second.tv_sec - first.tv_sec);
//   secs *= 1000000;
//   return (secs + usecs);
// }

int Scheduler::acceptAsserts()
{
  int num_procs = atoi(_num_procs.c_str());
  assertsData = new char* [num_procs];

  for(int i=0; i<num_procs; ++i)
  {
    assertsData[i] = new char[BUFFER_SIZE];
    memset(assertsData[i], '\0', BUFFER_SIZE);
  }

  for (int id=0; id<num_procs; ++id)
  { 
    char buffer[BUFFER_SIZE];
    memset(buffer, '\0', BUFFER_SIZE);
    if(Receive (id, buffer, BUFFER_SIZE) <= 0)
    {
      //printf("\nScheduler unable to accept in acceptAsserts %d\n", id);
      return -1;
    }

    if (strcmp(buffer, "") != 0) // something to save
    {
      //std::cout << "Received data in acceptAsserts: " << buffer << "\n";
      // This assertsData also contain the line numbers of the MPI_Send calls which are fired from within 'if' or 'else' blocks
      strncpy(Scheduler::assertsData[id], buffer, strlen(buffer));
    }

    Send (id, "saved");
  }
	return 0;
}

void Scheduler::putInExploredCategory(std::vector<expression> changedConditionals)
{
  std::vector<expression>::iterator it = changedConditionals.begin();
  std::vector<expression>::iterator it2 = changedConditionals.end();
  solver* dummy = new solver(smt->c);
  expr_vector clauses(smt->c);
  while(it != it2)
  { 
    expression e = (*it);
    expr x = (smt->c).int_const(e.varName.c_str());

    expr singleAssertion(smt->c);
    int literal = e.lit;

    if(e.opr == "==")
      singleAssertion = (x == (smt->c).int_val(literal)); 
    
    else if(e.opr == "!=")
      singleAssertion = !(x == (smt->c).int_val(literal)); 

    else if(e.opr == "<")
      singleAssertion = (x < (smt->c).int_val(literal)); 
      
    else if(e.opr == ">")
      singleAssertion = (x > (smt->c).int_val(literal)); 
      
    else if(e.opr == "<=")
      singleAssertion = (x <= (smt->c).int_val(literal)); 
      
    else if(e.opr == ">=")
      singleAssertion = (x >= (smt->c).int_val(literal)); 

    else std::cout << "\nOperator in a conditional not supported by tool\n"; 

    dummy->add(singleAssertion);
    //std::cout << "\nConditionals in putInExploredCategory " << singleAssertion << "\n";
    clauses.push_back(singleAssertion);
    it++;
  }
    // Push the expr containing 'and' of all the clauses found
  if(!clauses.empty())
  {
    array<Z3_ast> args(clauses);
    Z3_ast r = Z3_mk_and(smt->c, clauses.size(), args.ptr());
    expr res(smt->c, r);
    SMTEncoding::POSs.push_back(res);
  }
}

void Scheduler::finish(bool result) {
	ExitMpiProcessAndWait(true);
	if (result) {
		std::cout << "Verification successful: program is free of deadlocks." << std::endl;
		logger << "verdict: deadlock free";
		statf << "deadlock: no" << std::endl;
	} else {
		std::cout << "DEADLOCK detected!" << std::endl;
		logger << "verdict: deadlock present";
		statf << "deadlock: yes" << std::endl;
	}

	ull total_time = unix_time_ms() - start_time;
	
	int total_epochs = 0;
	int unique_epochs = 0;
	int total_syms = 0;
	statf << "epochs: ";
	for (const auto& p: verified_hashes) {
		for (const EpochInfo& epoch_info: p.second) {
			statf << "(" << epoch_info.size << "," << epoch_info.syms << "," << epoch_info.reps << ") ";
			total_epochs += epoch_info.reps;
			unique_epochs += 1;
			total_syms += epoch_info.syms;
			trace_solver_time += epoch_info.solver_time;
		}
	}
	statf << std::endl;
	statf << "total epochs: " << total_epochs << std::endl;
	statf << "unique epochs: " << unique_epochs << std::endl;
	statf << "repeated epochs: " << total_epochs - unique_epochs << std::endl;
	statf << "total generators: " << total_syms << std::endl;

	if (!path_epoch_info.empty()) {
		statf << "path epochs: ";
		for (const auto& p: path_epoch_info) {
			statf << "(" << p.first << "," << p.second << ") ";
		}
		statf << std::endl;

		statf << "explored paths: " << interleavings << std::endl;
		statf << "terminated paths: " << terminated << std::endl;
		statf << "scheduler time: " << scheduler_time << std::endl;
		statf << "trace time: " << trace_time << std::endl;
		statf << "trace solver time: " << trace_solver_time << std::endl;
		statf << "path time: " << path_time << std::endl;
		statf << "path solver time: " << path_solver_time << std::endl;
		statf << "total time: " << total_time << std::endl;
		statf << "misc time: " << total_time - scheduler_time - path_time - trace_time << std::endl;
	} else {
		statf << "total time: " << total_time << std::endl;
		statf << "scheduler time: " << scheduler_time << std::endl;
		statf << "processing time: " << trace_time - trace_solver_time << std::endl;
		statf << "solver time: " << trace_solver_time << std::endl;
		statf << "misc time: " << total_time - scheduler_time - trace_time << std::endl;
	}

	exit(0);
}

void Scheduler::StartMC () { 
  /*
   * First generate one single interleaving.
   * Use timeouts for this incase the processes
   * have deadlocked!
   */
  /* == fprs begin == */
  unsigned ran_seed = time(0);
  /* == fprs end == */
	//std::unordered_map< int, std::vector< EpochInfo >> verified_hashes;
  for (;;) 
  {
		logger << '\n';
		logger << "interleaving: " << interleavings;
		//std::cout << "\n-----------------------------------------" << std::endl;
		//std::cout << "\n~ INTERLEAVING: " << interleavings << " ~\n" << std::endl;
		//std::cout << "-----------------------------------------\n" << std::endl;
    /* == fprs begin == */
    if (Scheduler::_fprs) 
    {
      srand(ran_seed);
      Scheduler::_explore_mode = EXP_MODE_RANDOM;
    }
    /* == fprs end == */


		logger << "generate interleaving...";
		//std::cout << "generating interleaving..." << std::endl;
		ull tic = unix_time_ms();
    auto tmp = generateFirstInterleaving ();
		//statf << "scheduler time: " << unix_time_ms() - tic << std::endl;
		scheduler_time += unix_time_ms() - tic;

    if (tmp == 1)
    {
			terminated++;
			logger << "detected terminated interleaving";
      // This is a terminated interleaving
      ExitMpiProcessAndWait (true);
      delete it;
      it = previousITree->clone();

      // Set Encoding's _it = it (without calling the constructor)
      smt->_it = it;
      smt->last_node = (smt->_it)->_slist[(smt->_it)->_slist.size() -1] ; 

			logger << "";
			logger << "change conditionals...";
			ull tic = unix_time_ms();
      Scheduler::changedConditionals.clear();
			auto tmp = smt->changeConditionals(Scheduler::changedConditionals);
			//statf << "path change time: " << unix_time_ms() - tic << std::endl;
			path_time += unix_time_ms() - tic;

      if(tmp) // Returns true if some conditions can be changed and scheduler can be started again
      {
				logger << "restarting scheduler with changed conditionals...";
				//std::cout << "Restarting Scheduler with changed conditionals..." << std::endl;
        Restart();
        continue;
      }
      else
      {
				logger << "path conditionals exhausted";
				finish(true);
      }
    }
    
    if(Scheduler::_just_dead_lock) {
			logger << "deadlock detected in dynamic execution";
			finish(false);
		}
    
    // modification for CoE -- svs, sriram 
    m->_MConstruct(it);

    /*
    **  Accept the 'asserts' data from clients. -- dhriti
    */
    acceptAsserts();

    if(Scheduler::_encoding == 0)
    {
			logger << "start trace verification...";
      //std::cout << "verifying..." << std::endl;
			ull tic = unix_time_ms();
      smt = new SMTEncoding(verified_hashes, it, m/*, slv*/);

			logger << "generate Hermes constraints...";
      smt->encodingPartialOrders();

			logger << "start Simian verifier...";
			smt->init_ver();
      smt->encodingConditionals(Scheduler::assertsData, atoi(_num_procs.c_str()));
			bool res = smt->ver.verify();
			smt->ver.preserve_conditions();
			//statf << "trace verification time: " << unix_time_ms() - tic << std::endl;
			trace_time += unix_time_ms() - tic;
			logger << "Simian verfication result: " << (res ? "deadlock free" : "deadlock present");

      if (res)
      {
				logger << "change conditionals...";
				ull tic = unix_time_ms();
        Scheduler::changedConditionals.clear();
				smt->ver.clear_conditions();
				auto tmp = smt->changeConditionals(Scheduler::changedConditionals);
				//statf << "path change time: " << unix_time_ms() - tic << std::endl;
				path_time += unix_time_ms() - tic;

        if(tmp) // Returns true if some conditions can be changed and scheduler can be started again
        {
					logger << "restarting scheduler with changed conditionals...";
					//std::cout << "restarting scheduler with changed conditionals..." << std::endl;
          Restart();
          continue;
        }
        else
        {
					logger << "path conditionals exhausted";
					finish(true);
        }
      }
      else
      {
				finish(false);
      }
    }
  }
}

int Scheduler::generateFirstInterleaving ()
{
	struct timeval tv;
	Transition *t;
	tv.tv_usec = 0;
	tv.tv_sec = 2;
	int count = atoi(_num_procs.c_str ());
	int nprocs = count;

	while (count) 
	{
		for (int  i = 0 ; i < nprocs; i++) 
		{
			if (_runQ[i]->_read_next_env) 
			{
				t = getTransition(i);
				/* == fprs begin == */    
				// IMPORTANT NOTE: the constant ISP_START_SAMPLING and ISP_END_SAMPLEING here should be corresponded to the ISP_START_SAMPLING and ISP_END_SAMPLING defined in isp.h

				{
					if (t->GetEnvelope()->func_id == PCONTROL && t->GetEnvelope()->stag == ISP_START_SAMPLING) 
					{
						it->_is_exall_mode[i] = true;
					}
					if (t->GetEnvelope()->func_id == PCONTROL && t->GetEnvelope()->stag == ISP_END_SAMPLING) 
					{
						it->_is_exall_mode[i] = false;
					}
				}
				/* == fprs end == */
				//DS( std::cout << " [generateFirstInterleaving 1] e=" << t->GetEnvelope() << " e->func_id=" << t->GetEnvelope()->func_id << "\n"; )
				if (!(it->GetCurrNode ())->_tlist[i]->AddTransition (t)) 
				{
					ExitMpiProcessAndWait (true);

					if (!Scheduler::_quiet) 
					{
						/* ISP expects the same transitions on restart
						* to ensure the correctness of DPOR
						* backtracking - If the program changes its
						* control flow path upon restarting, ISP will
						* not work */
						std::cout << "ERROR! TRANSITIONS ON RESTARTS NOT SAME!!!!\n";
					}
					exit (21);
				}

        //std::cout << "\nSize of the s list: " << (it->_slist).size();

				//DS( std::cout << " [generateFirstInterleaving 2] e=" << t->GetEnvelope() << " e->func_id=" << t->GetEnvelope()->func_id << "\n"; )

				/* Some memory leak can happen here if we're not in
				the first interleaving. T has a "NEW" envelope
				which is never freed When this leak is fixed,
				please remove this comment */
				//Envelope * env; 
				//env = t->GetEnvelope();
				//CB ct(env->id, env->index);
				//std::cout<< "\n" << it->GetCurrNode ()->getAllAncestors(ct).size() << std::endl;
				
        if (! t->GetEnvelope()->isBlockingType()) {
				//if (! (t->GetEnvelope()->isBlockingType() || t->GetEnvelope()->isCollectiveType())) //dhriti
					Send (i, goback);
				} // dhriti: I have to send 'goback' signal in case of collectives also, because otherwise the program hangs and 
          // even ISP can not generate even first interleaving.
				else 
				{
					_runQ[i]->_read_next_env = false;
				}
				if (t->GetEnvelope ()->func_id == FINALIZE) 
				{
					//std::cout << "setting no more read for " << i <<
					//"\n";
					_runQ[i]->_read_next_env = false;
					count--;
				}
				delete t;
			}
		}
		std::list <int> l;
		std::list <int>::iterator iter;
		std::list <int>::iterator iter_end;

		//[grz] would and 'if' be enough here? --> NO (answered by dhriti, we are calling CHECK for every matching of ample sets (as the program is progressing))
		while (!hasMoreEnvelopeToRead ()) 
		{ 
      int x = it->CHECK(*this, l, changedConditionals); 
      
      if (x==1 || x==2)
  		{
        //std::cout << "\nInside CHECK\n";
        /*// I want to delete all the so-far seen transitions from all processes in this interleaving
        for (int  i = 0 ; i < nprocs; i++) 
        {
          it->GetCurrNode()->_tlist[i]->eraseFrom(0);
        }
        // Also, the current node must point to the last-node
        // Delete the transitions from _slist too --> Slist of good interleaving is saved at the end of this interleaving.*/
        // At the time of a terminated interleaving, that Slist is restored in startMC
        return x;
      }
			iter_end = l.end();

			/* This takes advantage of the fact that if we have
			PROBE/IPROBE, the size of l has to be less than 2, in
			which the first one is the send, the second one is the
			PROBE/IPROBE */
			/* In the case of PROBE/IPROBE, the iprobe call is the
			only call that should be allowed to go ahead */
			if (Scheduler::_probed) 
			{
				assert (l.size() <= 2);
				_runQ[l.back()]->_read_next_env = true;
				Scheduler::_probed = false;
			}
			else 
			{
				for (iter = l.begin (); iter != iter_end; iter++) 
				{
					_runQ[*iter]->_read_next_env = true;
				}
				//Scheduler::traceLength ++;
			}
		}
	}

	// Reached end of interleaving without any deadlocks - update InterCB
	#ifdef CONFIG_OPTIONAL_AMPLE_SET_FIX
	if(!Scheduler::_no_ample_set_fix)
	#endif
  
	it->ProcessInterleaving();
  
  //delete previousITree;
  previousITree = it->clone();

  return 0;
}

bool Scheduler::hasMoreEnvelopeToRead () {

  for (int  i = 0 ; i < atoi (_num_procs.c_str ()); i++) {
    if (_runQ[i]->_read_next_env) {
      return true;
    }
  }
  return false;
}

Transition *Scheduler::getTransition (int id) {

  Envelope *e = NULL;
  char    buffer[BUFFER_SIZE];
   
  do { 
    memset(buffer, '\0', BUFFER_SIZE);
    Receive (id, buffer, BUFFER_SIZE);
    /* == fprs start == */
    e = CreateEnvelope (buffer, id, order_id++, it->_is_exall_mode[id]);
    /* == fprs end == */
    if (!e) {
      it->GetCurrNode ()->PrintLog ();
      ExitMpiProcessAndWait (true);
      if (!_quiet) {
      	std::cout << *(it->GetCurrNode ()) << "\n";
      	std::cout << "Receiving empty buffer - Terminating ...\n";
      }
      exit (22);
    }
    //DS( std::cout << " [getTransition] e=" << e << " e->func_id=" << e->func_id << "\n"; )
    if (e->func_id == LEAK) {
      std::stringstream& leaks = it->GetCurrNode ()->_tlist[id]->_leaks_string;
      it->GetCurrNode ()->_tlist[id]->_leaks_count++;
      leaks << Scheduler::interleavings << " " << id << " Leak ";
      leaks << e->display_name << " { } { } Match: -1 -1 File: ";
      leaks << e->filename.length () << " " << e->filename << " " << e->linenumber << std::endl;
      Send (id, goback);
      order_id--;
    }
  } while (e->func_id == LEAK);

  bool error = false;
  switch (e->func_id) {

  case ASSERT:
    it->GetCurrNode ()->PrintLog ();
    _logfile << interleavings << " "
	     << e->id << " "
	     << "ASSERT "
	     << "Message: " << e->display_name.length() << " " << e->display_name << " "
	     << "Function: " << e->func.length() << " " << e->func << " "
	     << "File: " << e->filename.length() << " " << e->filename << " " << e->linenumber << "\n";
    ExitMpiProcessAndWait (true);
    if (!_quiet) {
      std::cout << "-----------------------------------------" << std::endl;
      std::cout << *(it->GetCurrNode ()) << std::endl;
      std::cout << "Program assertion failed!" << std::endl;
      std::cout << "Rank " << e->id << ": " << e->filename << ":" << e->linenumber
		<< ": " << e->func << ": " << e->display_name << std::endl;
      std::cout << "Killing program " << ProgName() << std::endl;
      std::cout << "-----------------------------------------" << std::endl;
      std::cout.flush();
    }
    exit (3);
    break;

  case ABORT:
    it->GetCurrNode()->_tlist[id]->AddTransition(new Transition(e));
    it->GetCurrNode ()->PrintLog ();
    ExitMpiProcessAndWait (true);
    if (!_quiet) {
      std::cout << "-----------------------------------------" << std::endl;
      std::cout << *(it->GetCurrNode ()) << std::endl;
      std::cout << "MPI_Abort on rank " << e->id << " caused abort on all ranks." << std::endl;
      std::cout << "Killing program " << ProgName() << std::endl;
      std::cout << "-----------------------------------------" << std::endl;
      std::cout.flush();
    }
    exit (4);
    break;

  case SEND:
  case ISEND:
  case SSEND:
  case RSEND:
    if (!e->dest_wildcard &&
	(e->dest < 0 || e->dest >= atoi (_num_procs.c_str ()))) {
      error = true;
    }
    break;
  case IRECV:
  case IPROBE:
  case PROBE:
  case RECV:
    if (!e->src_wildcard &&
	(e->src < 0 || e->src >= atoi (_num_procs.c_str ()))) {
      error = true;
    }
    break;
  case SCATTER:
  case GATHER:
  case SCATTERV:
  case GATHERV:
  case BCAST:
  case REDUCE:
    if (e->count < 0 || e->count >= atoi (_num_procs.c_str ())) {
      error = true;
    }
    break;
  }
  if (error) {
    it->GetCurrNode ()->_tlist[id]->AddTransition(new Transition(e));
    it->GetCurrNode ()->PrintLog ();
    ExitMpiProcessAndWait (true);
    if (!_quiet) {
      std::cout << "-----------------------------------------" << std::endl;
      std::cout << *(it->GetCurrNode ()) << std::endl;
      std::cout << "Rank " << id << ": " << "Invalid rank in MPI_" << e->func << " at "
		<< e->filename << ":" << e->linenumber << std::endl;
      std::cout << "Killing program " << ProgName() << std::endl;
      std::cout << "-----------------------------------------" << std::endl;
      std::cout.flush();
    }
    exit (5);
  }

    return new Transition(e);
}

