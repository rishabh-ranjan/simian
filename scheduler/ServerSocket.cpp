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
 * File:        ServerSocket.cpp
 * Description: Implements sockets for the server side
 * Contact:     isp-dev@cs.utah.edu
 */

#include "ServerSocket.hpp"
#include <iostream>
#ifndef WIN32
#include <sys/wait.h>
#endif
using namespace std;
/*
 * Implementation of Server side sockets. Please see ServerSocket.hpp
 * for the documentation of the member functions.
 */

ServerSocket::ServerSocket (int port, int num_clients, bool unix_sockets, bool batch_mode) :
        _serv_port (port), _num_clients (num_clients), _unix_sockets (unix_sockets),
        _batch_mode (batch_mode), _serv_sock (unix_sockets) {

}

ServerSocket::~ServerSocket(){
#ifdef WIN32
	WSACleanup();
#endif
}

int ServerSocket::Start () {

#ifdef WIN32
    /*
     * Initialize Winsock
     */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR) {
        exit (12);
    }
#endif

    /*
     * Open the Server side socket to listen.
     */

    if (_serv_sock.Open () == INVALID_SOCKET) {
        return 13;
    }
    /*
     * Initialize the server address structure and bind the
     * socket to the port.
     */
#ifndef WIN32
    if (_unix_sockets) {
        memset ((char *)&_serv_addr_un, '\0', sizeof (_serv_addr_un));
        _serv_addr_un.sun_family = AF_UNIX;
        strcpy (_serv_addr_un.sun_path, GetSocketFile ().c_str() );

        if (bind (_serv_sock.get_fd (), (struct sockaddr *)&_serv_addr_un,
                  strlen (_serv_addr_un.sun_path) + sizeof (_serv_addr_un.sun_family))
                == SOCKET_ERROR) {
            return 14;
        }
    } else {
#endif
        memset ((char *)&_serv_addr_in, '\0', sizeof (_serv_addr_in));
        _serv_addr_in.sin_family = AF_INET;
        _serv_addr_in.sin_addr.s_addr = INADDR_ANY;
        _serv_addr_in.sin_port = htons ((u_short)_serv_port);

        if (bind (_serv_sock.get_fd (), (struct sockaddr *)&_serv_addr_in,
                  sizeof (_serv_addr_in)) == SOCKET_ERROR) {
            return 14;
        }
#ifndef WIN32
    }
#endif

    /*
     * Server Start listening on the socket.
     */

    if (listen (_serv_sock.get_fd (), 5) == SOCKET_ERROR) {
        return 15;
    }
    return 0;
}


// This function accepts 2*n number of connections if there are n number of clients.
// First n connections are accepted from profiler (by profiling MPI_Init) and next n connections are 
// accepted from GenerateAssumes (to send the data of all 'if' conditions encountered in a process).

int ServerSocket::Accept () {

    for (int i = 0 ; i < _num_clients; i++) {

        char             buffer[BUFFER_SIZE];
        struct sockaddr_in     cli_addr;
        SOCKET        nfd;
        int            id;
        int            cli_len;

        memset (buffer, '\0', BUFFER_SIZE);
        cli_len    = sizeof (cli_addr);

        /*
         * Accept connections from the clients and get their id's.
         * Insert the ids -> socket mapping.
         */
        nfd = accept (_serv_sock.get_fd (),
#ifndef WIN32
                      (struct sockaddr *) &cli_addr,
                      reinterpret_cast <socklen_t *> (&cli_len));
#else
                      (SOCKADDR*) &cli_addr, &cli_len);
#endif
        if (nfd == SOCKET_ERROR) {
#ifdef DEBUG
			printf("Socket error on scheduler: %d\n", WSAGetLastError());
#endif
            return 16;
        }

        Socket *nsock = new Socket (nfd, _unix_sockets);

        int len;
        len = nsock->Receive (buffer, BUFFER_SIZE);
        if (len == 0 || len == SOCKET_ERROR) {
            return 17;
        }
        id = atoi (buffer);
        nsock->Send (goback);
        _cli_socks[id] = nsock; 
        if (_batch_mode) {
            std::cout << "Received connection from rank " << id << "." << std::endl;
        }
    }
    return 0;
}

int ServerSocket::Send (int id, std::string packet) {
    std::map <int, Socket*>::iterator iter = _cli_socks.find (id);
    //assert (iter != _cli_socks.end ());
    return ((*iter).second)->Send (packet);
}

int ServerSocket::Restart () {

    std::map <int, Socket*>::iterator iter;

    /*
     * Close all the client sockets.
     */
    for (iter = _cli_socks.begin (); iter != _cli_socks.end (); iter++) {
        (*iter).second->Close ();
        delete (*iter).second;
    }
    /*
     * Clear up the mapping of clients to sockets.
     */
    _cli_socks.clear();
    /*
     * Restart the server
     */
#ifndef WIN32
    if (_unix_sockets) {
      _serv_sock.Close();
      
      if (!_serv_sock_filename.empty())
        unlink(_serv_sock_filename.c_str());
      
      _serv_sock_filename = "";
      Start();
    }
#endif
    return 0;
}

void ServerSocket::Stop () {

    std::map <int, Socket*>::iterator iter;

    /*
     * Send the stop command to all client sockets and then close the socket.
     */
    for (iter = _cli_socks.begin (); iter != _cli_socks.end (); iter++) {
        (*iter).second->Send(stop);
        (*iter).second->Close ();
    }

#ifdef WIN32
    //WSACleanup (); //TODO: remove me
#else
    if (!_serv_sock_filename.empty ()) {
        unlink (_serv_sock_filename.c_str ());
    }
    if (_unix_sockets)
      _serv_sock_filename = "";

#endif
}

int ServerSocket::Receive (int id, char *buffer, int len) {
    std::map <int, Socket*>::iterator iter = _cli_socks.find (id);
    assert (iter != _cli_socks.end ());
    int r = ((*iter).second)->Receive (buffer, len);
    return r;
}

SOCKET ServerSocket::GetFD (int id) {
    std::map <int, Socket*>::iterator iter = _cli_socks.find (id);
    assert (iter != _cli_socks.end());
    return ((*iter).second)->get_fd ();
}

void ServerSocket::Close () {
    std::map <int, Socket *>::iterator    iter;

    for (iter = _cli_socks.begin (); iter != _cli_socks.end (); iter++) {
        (*iter).second->Close ();
    }

#ifdef WIN32
    //WSACleanup (); //TODO: remove me -- added to destructor
#else
    
    if (!_serv_sock_filename.empty ()) {
      unlink (_serv_sock_filename.c_str ());
    }
    if (_unix_sockets)
      _serv_sock_filename= "";

#endif
}

void ServerSocket::ExitMpiProcessAndWait (bool terminate) {

#ifndef WIN32
    signal(SIGCHLD, SIG_DFL);
#endif
    if (terminate) {
        Stop ();
    } else {
        Close ();
    }
#ifndef WIN32
    int stat;
    waitpid (_pid, &stat, 0);
    void child_signal_handler (int signal);
    signal(SIGCHLD, child_signal_handler);
#else
    HANDLE h = OpenProcess(SYNCHRONIZE, false, _pid);
    if (h != NULL) {
        WaitForSingleObject(h, INFINITE);
        CloseHandle(h);
    }
#endif
}

#ifndef WIN32
std::string ServerSocket::GetSocketFile () {

    if (_serv_sock_filename.empty ()) {
        char filename[] = "/tmp/isp.XXXXXX";
        int fd = mkstemp (filename);
        if (fd >= 0) {
            close (fd);
            unlink (filename);
            _serv_sock_filename.insert (0, filename);
        }
    }

    return _serv_sock_filename;
}
#endif
