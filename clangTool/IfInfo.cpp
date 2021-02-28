#include "IfInfo.h"
using namespace std;

IfInfo::IfInfo(string varName, Operator op, int dataVal, int start_line_no, int end_line_no, int correspondingRecvLineno, int sendLineNo /*-1*/, int rank /*-1*/, int pid /*-1*/)
{
	this->varNames.push_back(varName);
	this->ops.push_back(op);
	this->literals.push_back(dataVal);
	this->startEndPositions.push_back(std::pair<int, int>(start_line_no, end_line_no)); // start end position
	this->correspondingRecvLineno = correspondingRecvLineno;
	this->rank = rank;
	this->pid = pid;
	this->sendsComingFromIfs.push_back(sendLineNo);
}

IfInfo::IfInfo(string varName, Operator op, int dataVal, int start_line_no, int end_line_no)
{
	this->varNames.push_back(varName);
	this->ops.push_back(op);
	this->literals.push_back(dataVal);
	this->startEndPositions.push_back(std::pair<int, int>(start_line_no, end_line_no)); // start end position
}

IfInfo::IfInfo()
{

}
