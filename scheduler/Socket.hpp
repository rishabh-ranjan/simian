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
 * File:        Socket.hpp
 * Description: Base class for the socket layer
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <sys/types.h>
#include <iostream>
#include <string>
#include <errno.h>
#ifndef WIN32
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#else
#include <winsock2.h>
#endif

#define BUFFER_SIZE 2560
/*
 * Wrapper Code for the basic Socket system Calls. This creates and closes
 * sockets. Creates TCP sockets ONLY. This must be inherited by the Client and
 * Server side communication implementation.
 */

class Scheduler;
class Socket {

public:
    /*
     * Constructor. This takes the socket type as the parameter.
     * Assuming that domain is AF_INET and protocol field is 0.
     */
    Socket (bool unix_sockets);

    /*
     * Single parameter constructor. Takes the Socket identifier as its
     * parameter. The socket identifier must be an already opened socket
     * Invoking Open on this socket does not have any effect.
     */
    Socket (SOCKET fd, bool unix_sockets);

    /*
     * Open: This Opens the socket and also returns the return code.
     */
    SOCKET Open ();

    /*
     * Close: Closes the socket endpoint.
     */
    void Close ();

    /*
     * get_fd: Returns the socket descriptor of the socket.
     */
    SOCKET get_fd ();

    /*
     * Send : Send the packet.
     */
    int Send (std::string packet);

    /*
     * Receive: Receive the packet
     */

    int Receive (char *packet, int len);

private:
    SOCKET  _sfd;
    bool    _is_open;
    bool    _unix_sockets;
};

#endif
