#include "GenerateAssumes.h"
//#include "../profiler/Profiler.c"
using namespace std;

#define BUFFER_SIZE 1024

extern const char      *goahead;
extern SOCKET          fd;

ProcessIfs::ProcessIfs()
{
	connectionFd = -1;
}

void ProcessIfs::storeInfo(std::string variable, Operator op, int literal, int start_line_no, int end_line_no, int correspondingRecvLineno, int sendLineNo)
{
	ifs.push_back(new IfInfo(variable, op, literal, start_line_no, end_line_no, correspondingRecvLineno, sendLineNo));
    //std::cout << "\nInside storeInfo: " << start_line_no << " " << end_line_no << "\n";
	//displayInfo();
}

void ProcessIfs::displayInfo()
{
	for(std::vector<IfInfo*>::iterator it = ifs.begin(); it != ifs.end(); ++it) 
	{
        for(std::vector<std::string>::iterator jt = (*it)->varNames.begin(); jt != (*it)->varNames.end(); ++jt) 
		  std::cout << (*jt) << "\n"; 
	}
}

int ProcessIfs::connectToScheduler(int my_rank)
{
	int fd=-1;
	if ((fd = Connect (0/*unix_sockets*/, "localhost"/*host*/, 9999/*port*/, NULL/*sockfile*/, my_rank)) == INVALID_SOCKET) 
	{
        printf ("Client %d Unable to connect\n", my_rank);
        return -1;
    }
    connectionFd = fd;
    char sbuff[BUFFER_SIZE];
    memset (sbuff, '\0', BUFFER_SIZE);
    sprintf (sbuff, "%d\n", my_rank);
    if (Send (connectionFd, sbuff, strlen (sbuff)) < 0) {
        printf ("Client %d unable to send id to server\n", my_rank);
        return -1;
    }

	while (1) 
	{
        char rbuff[BUFFER_SIZE];
        memset (rbuff, '\0', BUFFER_SIZE);
        Receive (connectionFd, rbuff, BUFFER_SIZE);
        if(strcmp(rbuff, "2:") == 0) // goback signal (server has accepted the connection)
        	return fd;
    }
    
    return fd;
}

int ProcessIfs::transferInfoToScheduler(int my_rank)
{
    //cout << "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
    char buffer[BUFFER_SIZE];
    memset (buffer, '\0', BUFFER_SIZE);
    /* Information to be sent to the server:
     * 1. Rank
     * 2. Number of Ifs data: asserts
     * 3. Data in the vector
    */
    sprintf (buffer, "%d %lu ", my_rank, ifs.size());
    for(int i=0; i<ifs.size(); ++i)
    {
        IfInfo* ifInfo = ifs[i];
        std::pair<int, int> startEndPosition = ifInfo->startEndPositions.back();
        sprintf (buffer+strlen(buffer), "%s %d %d %d %d %d %d", (ifInfo->varNames.back()).c_str(), ifInfo->ops.back(), ifInfo->literals.back(), startEndPosition.first, startEndPosition.second, ifInfo->correspondingRecvLineno, ifInfo->sendsComingFromIfs.back());
    }
    //std::cout << "The buffer is: " << buffer;
    if(Send (fd, buffer, sizeof(buffer)) <= 0) 
    {
        printf ("Client unable to send in SendRecv: %d\n", fd);
        return -1;
    }

    // And receive the confirmation that the sent data has been received by the server
    while (1) 
    {
        char rbuff[BUFFER_SIZE];
        memset (rbuff, '\0', BUFFER_SIZE);
        if (Receive (fd, rbuff, BUFFER_SIZE) <= 0)
        {
        	printf("Client unable to receive in SendRecv\n");
        	return -1;
        }
        if (strcmp(rbuff, "saved") == 0)
        	return 1;
        
        return -1;
    }
}
