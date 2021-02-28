#ifndef __UTILITY__
#define __UTILITY__

#include <InterleavingTree.hpp>
#include <sys/select.h>
#include <sys/time.h>

#include <unistd.h>
#include <ios>
#include <iostream>
#include <fstream>
#include <string>

/* */
std::set<int> getImmediateAncestorList (Node *ncurr, int _pid, std::vector<int> &_list);

/* */
std::set<int> getAllAncestorList(Node *ncurr, CB c);

/* */
std::set<int> getImmediateDescendantList (Node *ncurr, int _pid, std::vector<int> &_list);

/* */
std::set<int> getAllDescendantList (Node *ncurr, CB c);

unsigned long long getTimeElapsed (struct timeval first, struct timeval second);

void PrintStats (unsigned long total_time);

size_t getCurrentRSS( );
size_t getPeakRSS( );
void process_mem_usage(double& vm_usage, double& resident_set);
#endif
