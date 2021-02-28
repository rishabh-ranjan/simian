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
 * File:        Client.h
 * Description: Client communication interface to ISP scheduler
 * Contact:     isp-dev@cs.utah.edu
 */

#ifndef _CLIENT_H
#define _CLIENT_H

#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
typedef int SOCKET;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#else
#include <winsock2.h>
#endif
#include <stdio.h>

/*
 * Open a socket, connect to the server and send the MPI id to 
 * the server.
 *
 * Returns -1 on failure. On success returns the socked handle.
 */

#ifdef __cplusplus
extern "C" {
#endif

SOCKET Connect (int unix_sockets, const char *server, int port,
        const char *sockfile, int id);

/*
 * Send: Send data to the server.
 * Returns the amount of data sent.
 */

int Send (SOCKET fd, char *buffer, int len);

/*
 * Receive: Receive data from server.
 * Returns the amount of data received from the server.
 */

int Receive (SOCKET fd, char *buffer, int len);

/*
 * Disconnect: Close connection to the server.
 */

void Disconnect (SOCKET fd);

#ifdef __cplusplus
}
#endif

#endif
