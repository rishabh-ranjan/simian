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
 * File:        ServerSocket.hpp
 * Description: Implements sockets for the server side
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _SERVER_SOCKET_HPP_
#define _SERVER_SOCKET_HPP_

#include <sys/types.h>
#include "Socket.hpp"
#include <cassert>
#include <string>
#include <map>
#include <stdlib.h>
#ifndef WIN32
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#else
#include <winsock2.h>
typedef DWORD pid_t;
#endif

const std::string goahead = "1";
const std::string loop = "2";
const std::string goback = "2";
const std::string restart = "3";
const std::string stop = "4";

class ServerSocket {

public:
    /*
     * Constructor - This creates the server instance.
     * Requires the port number as input along with the
     * number of clients connecting to it.
     */

    ServerSocket (int port, int clients, bool unix_sockets, bool batch_mode);

    /*
     * The startup code for server. This does all the server Initialization.
     *
     * - Opening the Socket
     * - Binding the Socket
     * - Listening on the Socket.
     *
     * Returns 0 if successful and -1 on error.
     */
    virtual ~ServerSocket();

    int Start ();

    /*
     * Accept: Accept all the client connections and get their
     * Ids. We must divide the listening and accept into two
     * different calls because the server must start listening
     * before it forks to create a client process.
     *
     * Returns 0 on success and -1 on failure.
     */

    int Accept ();

    /*
     * Send : Since the server is connected to multiple clients,
     * we need the client id to get the required client socket.
     */
    int Send (int id, std::string packet);
    /*
     * Receive : Since the server is connected to multiple clients,
     * we need the client id to get the required client socket.
     */

    int Receive (int id, char *buffer, int len);

    /*
     * Restart: Restart the Server by closing all the previous
     * Sockets and reopening them again. This is similar to Start.
     * Accept must be invoked after this as it is needed for Start.
     *
     * Returns 0 on success and -1 on failure.
     */

    virtual int Restart ();

    /*
     * Stop: Sends the stop command to all clients to make them
     * terminate.
     */

    virtual void Stop ();

    /*
     * GetFD: Get the socket id for the given client.
     */
    SOCKET GetFD (int id);

    /*
     * Close : Close the Socket connections with all clients.
     */
    void Close ();

    /*
     * ExitMpiProcessAndWait: Exits the profiled process and waits until
     * it actually exits before returning. If terminate is true, it calls
     * Stop () to terminate, otherwise it calls Close ().
     */
    void ExitMpiProcessAndWait (bool terminate);

#ifndef WIN32
    /*
     * GetSocketFile: If Unix Sockets are used, it returns the
     * for this socket. If one does not exist, one is created.
     */
    std::string GetSocketFile ();
#endif

private:
    int                     _serv_port;
    int                     _num_clients;
    bool                    _unix_sockets;
    bool                    _batch_mode;
    struct sockaddr_in      _serv_addr_in;
#ifndef WIN32
    struct sockaddr_un      _serv_addr_un;
#endif
    Socket                  _serv_sock;
    std::map <int, Socket*> _cli_socks;
    std::map <int, Socket*> _cli_socks_asserts;
    std::string             _serv_sock_filename;

protected:
    pid_t                   _pid;

};
#endif

