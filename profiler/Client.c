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
 * File:        Client.c
 * Description: Client communication interface to ISP scheduler
 * Contact:     isp-dev@cs.utah.edu
 */

#include "Client.h"
#include <errno.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif

#define BUFFER_SIZE 256

static SOCKET Open (int unix_sockets) {

    if (unix_sockets) {
        return socket (AF_UNIX, SOCK_STREAM, 0);
    } else {
        return socket (AF_INET, SOCK_STREAM, 0);
    }
}

#ifdef __cplusplus
extern "C" {
#endif

int Send (SOCKET fd, char *buffer, int len) {

    return send (fd, buffer, len, 0);
}

int Receive (SOCKET fd, char *buffer, int len) {

    return recv (fd, buffer, len, 0);
}

SOCKET Connect (int unix_sockets, const char *hostname, int port, const char *sock_file, int id) {

#ifndef WIN32
    struct sockaddr_un    server_addr_un;
#endif
    struct sockaddr_in    server_addr_in;
    struct sockaddr       *server_addr = NULL;
    struct hostent        *server = NULL;
    SOCKET                fd;
    int                   length;

#ifdef WIN32
    /*
     * Initialize Winsock
     */
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR) {
        printf ("Error Starting Winsock\n");
        exit (1);
    }
#endif

    /*
     * Open the Socket endpoint.
     */
    if ((fd = Open (unix_sockets)) == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }

#ifndef WIN32
    if (unix_sockets) {
        memset ((char *)&server_addr_un, '\0', sizeof (server_addr_un));
        server_addr_un.sun_family = AF_UNIX;
        strcpy (server_addr_un.sun_path, sock_file);
        server_addr = (struct sockaddr *)&server_addr_un;
        length = strlen (server_addr_un.sun_path) + sizeof (server_addr_un.sun_family);
    } else {
#endif
        memset ((char *)&server_addr_in, '\0', sizeof (server_addr_in));
        server = gethostbyname (hostname);    

        if ((char *) server == NULL) {
            printf ("Invalid Server name %s\n", hostname);
            return INVALID_SOCKET;
        }

        server_addr_in.sin_family = AF_INET;
        memmove ((char *)&server_addr_in.sin_addr.s_addr, (char *)server->h_addr,
                    server->h_length);
        server_addr_in.sin_port = htons (port);
        server_addr = (struct sockaddr *)&server_addr_in;
        length = sizeof (server_addr_in);
#ifndef WIN32
    }
#endif

    /*
     * Connect to the server.
     */
    if (connect (fd, server_addr, length) < 0) {
      fprintf (stderr, "Client %d unable to connect to server, the erros is: %s\n", id, strerror(errno));
        return INVALID_SOCKET;
    }
    
    return fd;
}

void Disconnect (SOCKET fd) {

#ifndef WIN32
    close (fd);
#else
    closesocket (fd);
    WSACleanup();
#endif
}

#ifdef __cplusplus
}
#endif
