/*
*	Dhriti Khanna (dhritik@iiitd.ac.in)
*
*	Class for storing 'if' data. Whenever an'if' is encountered, an object of this class is created and 
*	pushed into the stack being maintained for storing such 'ifs'.
*/

#ifndef IFINFO_H
#define IFINFO_H

#include <vector>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include "CommonUtils.h"

class IfInfo
{
	public:
			int rank;
			int pid;
			std::vector<std::string> varNames;
			int correspondingRecvLineno;
			std::vector<Operator> ops;
			std::vector<int> literals; // For now we are comparing with only integer values
			std::vector<std::pair<int, int> > startEndPositions;
			std::vector<int> sendsComingFromIfs;

			IfInfo(std::string varName, Operator op, int dataVal, int start_line_no, int end_line_no, int correspondingRecvLineno, int sendLineNo=-1, int rank=-1, int pid=-1);
			IfInfo(std::string varName, Operator op, int dataVal, int start_line_no, int end_line_no);
			IfInfo();
};

#endif
