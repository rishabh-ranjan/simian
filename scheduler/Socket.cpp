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
 * File:        Socket.cpp
 * Description: Base class for the socket layer
 * Contact:     isp-dev@cs.utah.edu
 */  

#include "Socket.hpp"
#include <iostream>
#include <string.h>

/*
 * For details about the member functions, please look at the header file
 * Socket.hpp
 */

Socket::Socket (bool unix_sockets) : _sfd (INVALID_SOCKET), _is_open (false),
        _unix_sockets (unix_sockets) {

}

Socket::Socket (SOCKET fd, bool unix_sockets) : _sfd (fd), _is_open (true),
        _unix_sockets (unix_sockets) {

}

SOCKET Socket::Open () {

    int        on        = 1;
    int        sock_type = _unix_sockets ? AF_UNIX : AF_INET;
    /*
     * You don't have to open this socket if this is already
     * opened.
     */

    if (_is_open) {
        return 0;
    }

    /*
     * Create the Socket ...
     */
    
    _sfd = socket (sock_type, SOCK_STREAM, 0);
    if (setsockopt (_sfd, SOL_SOCKET, SO_REUSEADDR,
                    (const char *)&on, sizeof (on)) == SOCKET_ERROR) {
        return INVALID_SOCKET;
    }

    if (_sfd != INVALID_SOCKET) {
        _is_open = true;
    }

    return _sfd;
}

SOCKET Socket::get_fd () {
    return _sfd;
}

void Socket::Close () {
    if (!_is_open) {
        return;
    }

#ifndef WIN32
    close (_sfd);
#else
    closesocket (_sfd);
#endif
    _is_open = false;
}

int Socket::Send (std::string packet) {
    if (! _is_open) {
        return -1;
    }
    packet = packet + ":";
    return send (_sfd, packet.c_str(), (int)packet.size(), 0);
}

int Socket::Receive (char *buffer, int len) {

    if (! _is_open) {
        return -1;
    }

    if (len <= 0) {
        return -1;
    }
    int r = recv (_sfd, buffer, len, 0);
    return r;
}

