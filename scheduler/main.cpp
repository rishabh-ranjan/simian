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
 * File:        main.cpp
 * Description: Main entry point for the ISP scheduler application
 * Contact:     isp-dev@cs.utah.edu
 */

#include <stdlib.h>
#include "Scheduler.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include "sched-opt.h"
#ifndef WIN32
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
 
#include "simian-core/logger.h"

ull start_time;
std::string dirname;
Logger logger;
std::ofstream statf;
ull scheduler_time;
ull trace_time;
ull path_time;
ull trace_solver_time;
ull path_solver_time;
std::vector< std::pair< int, int >> path_epoch_info;

void child_signal_handler (int signal)
{
  if (signal == SIGCHLD) {
    int statf;
    while (waitpid (-1, &statf, WNOHANG) > 0) {
    }
  }
}
#endif

int main (int argc, char **argv) {

	bool epoch = getenv("NO_EPOCH") ? false : true;
	bool symmetry = getenv("NO_SYMMETRY") ? false : true;

	start_time = unix_time_ms();
	std::string name;
	std::string args;
	for (int i = 0; i < argc; ++i) {
		if (argv[i][0] == '.') {
			name = argv[i];
			for (int j = i+1; j < argc; ++j) {
				args += std::string(argv[j]) + (j == argc-1 ? "" : "_");
			}
			break;
		}
	}
	name = name.substr(4);
	std::string nprocs;
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "-n") == 0) {
			nprocs = argv[i+1];
			break;
		}
	}
	dirname = nprocs + "." + name + "." + args;
	//dirname = std::to_string(start_time);
	mkdir(("runlogs/" + dirname).c_str(), 0755);
	logger.open("runlogs/" + dirname + "/log.txt");
	statf.open("runlogs/" + dirname + "/stats.txt");

	logger << "argc: " << argc;
	for (int i = 0; i < argc; ++i) {
		logger << "argv[ " << i << " ]: " << argv[i];
	}

	statf << "name: " << name << std::endl;
	statf << "nprocs: " << nprocs << std::endl;
	int k;
	statf << "args: ";
	for (k = 0; k < argc; ++k) {
		if (argv[k][0] == '.') {
			break;
		}
	}
	for (++k; k < argc; ++k) {
		statf << argv[k] << ' ';
	}
	statf << std::endl;

  /*
   * Check for arguments and print correct usage.
   */
  int port = 9990;
  int num_clients = 2;
  int kbuffer = 0;
  std::string server("localhost");
  bool send_block = false;
  std::string logfile("");    
  std::string fname;
  int report_progress = 0;
  std::vector<std::string> fargs;
  bool mpicalls = false;
  bool verbose = false;
  bool quiet = false;
  bool unix_sockets = false;
  bool fib = false;
  bool openmp = true;
  bool use_env_only = false;
  bool batch_mode = false;
  te_Exp_Mode explore_mode = EXP_MODE_ALL;
  int  explore_some = 0;
  bool stop_at_deadlock = 0;
  bool debug = false;
  bool no_ample_set_fix = true; // svs -- fix
  int i = 0;
  unsigned bound = 0;
  bool limit_output = 0;
/* == fprs start == */
  bool fprs = false;
/* == fprs end == */

/* svs -- SAT solving begin */
  int encoding = 0;
  bool dimacs = false;
  bool show_formula = false;
  std::string solver(""); 
/* svs -- SAT solving end */

  //initialize the random generator 
  srand ((unsigned) time (0));
  //process autoopts
  int optct = optionProcess( &ispOptions, argc, argv );
  argc -= optct;
  argv += optct;

  
  //CGD
  if(HAVE_OPT(VIEW_PROG_OUT_ONLY)){
    limit_output = 1;
  }
  
  if(HAVE_OPT(PORT)){
    port = OPT_VALUE_PORT;
  }
  if(HAVE_OPT(NUMPROCS)){
    num_clients = OPT_VALUE_NUMPROCS;
  }
  if(HAVE_OPT(KBUFFER)){
    kbuffer = OPT_VALUE_KBUFFER;
  }
  if(HAVE_OPT(HOST)){
    server.assign(OPT_ARG(HOST));
  }
#ifndef WIN32
  if(HAVE_OPT(US)){ //use unix sockets
    unix_sockets = true;
  }
#endif
  if(HAVE_OPT(BLOCK)){
    send_block = true;
  }
  if(HAVE_OPT(NOBLOCK)){
    send_block = false;
  }
  if(HAVE_OPT(LOGFILE)){
    logfile.assign(OPT_ARG(LOGFILE));
  }
  if(HAVE_OPT(MPICALLS)){
    mpicalls = true;
  }
  if(HAVE_OPT(VERBOSE)){
    verbose = true;
  }
  if(HAVE_OPT(QUIET)){
    quiet = true;
  }
  if(HAVE_OPT(RPT_PROGRESS)){
    report_progress = OPT_VALUE_RPT_PROGRESS;
  }
#ifdef FIB
  if(HAVE_OPT(FIBOPT)){
    fib = true;
  }
#endif
#ifdef USE_OPENMP
  if(HAVE_OPT(DISABLE_OMP)){
    openmp = false;
  }
#endif
  if(HAVE_OPT(ENV)){
    use_env_only = true;
  }
  if(HAVE_OPT(DISTRIBUTED)){
    batch_mode = true;
  }
  if(HAVE_OPT(EXP_MODE)){ //WITH KEYWORD OPTIONS, YOU GET AN INDEX TO THE CHOICE
    explore_mode = (te_Exp_Mode)OPT_VALUE_EXP_MODE;
  }
  if(HAVE_OPT(EXP_SOME)){
    explore_some = OPT_VALUE_EXP_SOME;
  }
  if(HAVE_OPT(STOP_AT_DEADLOCK)){
    stop_at_deadlock = OPT_VALUE_STOP_AT_DEADLOCK;
  } 
#ifdef CONFIG_DEBUG_SCHED
  if(HAVE_OPT(DEBUG_SCHED)) {
    debug = true;
  }
#endif
#ifdef CONFIG_OPTIONAL_AMPLE_SET_FIX
  if(HAVE_OPT(NO_AMPLE_SET_FIX)) {
    no_ample_set_fix = true;
  }
#endif
#ifdef CONFIG_BOUNDED_MIXING
  if(HAVE_OPT(BOUND)) {
    bound = OPT_VALUE_BOUND;
  }
#endif
/* == fprs start == */
  if (HAVE_OPT(FPRS)) {
    fprs = true;
  }
/* == fprs end == */

	/*
	if (HAVE_OPT(EPOCH)) {
		epoch = OPT_VALUE_EPOCH;
	}

	if (HAVE_OPT(SYMMETRY)) {
		symmetry = OPT_VALUE_SYMMETRY;
	}
	*/

/* svs -- SAT encoding begin */
  if(HAVE_OPT(ENCODING)){
    encoding = OPT_VALUE_ENCODING;
  }

 if(HAVE_OPT(DIMACS)){
    dimacs = OPT_VALUE_DIMACS;
  }

 if(HAVE_OPT(SHOW_FORMULA)){
   show_formula = true;
 }
 if(HAVE_OPT(MINISAT)){
   solver = "minisat";
 } 
 else  if(HAVE_OPT(LINGELING)){
   solver = "lingeling";
 } 
 /* svs -- SAT encoding end */

  fname = std::string(argv[i++]);
  std::ostringstream fargs_string;
  for ( ; i < argc; i++) {
    std::string str(argv[i]);
    fargs.push_back(str);
    fargs_string << str << " ";
  }

  std::ostringstream port_strs;
  std::ostringstream num_clientss;
  port_strs << port;
  num_clientss << num_clients;

/* == fprs start == */
/* 
  Scheduler::SetParams(port_strs.str(), num_clientss.str(), server, 
		       send_block, fname,
                       logfile, fargs, mpicalls, verbose, quiet, unix_sockets,
                       report_progress, fib, openmp, use_env_only, 
                       batch_mode, stop_at_deadlock, explore_mode,
                       explore_some, NULL, NULL, NULL, debug, no_ample_set_fix,
                       bound, limit_output);//CGD
*/
  Scheduler::SetParams(port_strs.str(), num_clientss.str(), server, 
		       send_block, fname,
                       logfile, fargs, mpicalls, verbose, quiet, unix_sockets,
                       report_progress, fib, openmp, use_env_only, 
                       batch_mode, stop_at_deadlock, explore_mode,
                       explore_some, NULL, NULL, NULL, debug, no_ample_set_fix,
                       bound, limit_output, fprs, encoding, dimacs, show_formula, solver, kbuffer,
											 epoch, symmetry);
/* == fprs end == */

  if (!quiet && !limit_output) {//CGD
    // std::cout << "ISP - Insitu Partial Order" << std::endl;
    // std::cout << "-----------------------------------------" << std::endl;
    // std::cout << "Command:        " << fname << " " << fargs_string.str() << std::endl;
    // std::cout << "Number Procs:   " << num_clients << std::endl;
    // if (unix_sockets) {
    //   std::cout << "Server:         Local Socket" << std::endl;
    // } else {
    //   std::cout << "Server:         " << server << ":" << port << std::endl;
    // }
    // std::cout << "Blocking Sends: " << (send_block ? "Enabled" : "Disabled") << std::endl;
#ifdef FIB
    if (fib) {
      std::cout << "FIB:            Enabled" << std::endl;
    }
#endif
    std::cout << "-----------------------------------------" << std::endl;
  }

#ifndef WIN32
  // SIGPIPE might be signaled if you read / write from socket that is closed
  // on the other end. This prevents it crashing the program.
  signal(SIGPIPE, SIG_IGN);

  // SIGCHLD will be signaled when the process created with fork() terminates.
  // Must implement a signal handler to wait on that PID.
  signal(SIGCHLD, child_signal_handler);
#endif

  Scheduler::GetInstance ()->StartMC ();
  return 0;
}

#ifdef WIN32
/*
 * This function emulates the execlp function for windows. It
 * returns 0 on failure, or the process PID on success.
 */
DWORD execvp(const char *file, char const **argv) {

  std::stringstream cmdLine;

  /*
   * Construct a command line string for the program.
   */
  for (int i = 0; argv[i]; i++)
    {
      cmdLine << "\"";
      cmdLine << argv[i];
      cmdLine << "\"";
      cmdLine << " ";
    }

  /*
   * CreateProcess API initialization
   */
  STARTUPINFO siStartupInfo;
  PROCESS_INFORMATION piProcessInfo;
  SECURITY_ATTRIBUTES saSecurityAttributes;
  memset(&siStartupInfo, 0, sizeof(siStartupInfo));
  memset(&piProcessInfo, 0, sizeof(piProcessInfo));
  memset(&saSecurityAttributes, 0, sizeof(saSecurityAttributes));

  /*
   * Setup the StartupInfo. Redirect the standard out of the created
   * process to standard error.
   */
  siStartupInfo.cb = sizeof(siStartupInfo);
  siStartupInfo.dwFlags |= STARTF_USESTDHANDLES;
  siStartupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  siStartupInfo.hStdOutput = GetStdHandle(STD_ERROR_HANDLE);
  siStartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

  saSecurityAttributes.nLength = sizeof(saSecurityAttributes);
  saSecurityAttributes.bInheritHandle = FALSE;
  saSecurityAttributes.lpSecurityDescriptor = NULL;

  /*
   * Execute
   */
  if (!CreateProcess(file, const_cast<char*>(cmdLine.str().c_str()),
                     &saSecurityAttributes, &saSecurityAttributes, TRUE,
                     CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &siStartupInfo,
                     &piProcessInfo)) {
    /*
     * CreateProcess failed. Release handles.
     */
    CloseHandle(piProcessInfo.hProcess);
    CloseHandle(piProcessInfo.hThread);

    return 0;
  }

  DWORD ProcessID = piProcessInfo.dwProcessId;

  /*
   * Release handles.
   */
  CloseHandle(piProcessInfo.hProcess);
  CloseHandle(piProcessInfo.hThread);

  return ProcessID;
}
#endif
