//
// Created by tommy on 2/11/25.
//

#ifndef NETWORK_H
#define NETWORK_H

#include "../include/args.h"
#include <netinet/in.h>
#define ERR_NONE -1
#define ERR_NO_DIGITS -2
#define ERR_OUT_OF_RANGE -3
#define ERR_INVALID_CHARS -4
#define ERR_SOCKET -1
#define ERR_SET_OPTION -2
#define ERR_BIND -3
#define ERR_LISTEN -4

// Declare network content
struct network
{
    int                     sockfd;
    struct sockaddr_storage server_addr;
    struct sockaddr_storage client_addr;
    socklen_t               server_addr_len;
    socklen_t               client_addr_len;
};

// Function to set up address
int sever_network(const Arguments *args);

// Convert port if network socket as input
in_port_t convertPort(const char *str, int *err);

// free all resources
void null_free(void **ptr);

#endif    // NETWORK_H