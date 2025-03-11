#ifndef NETWORK_H
#define NETWORK_H

#include "../include/args.h"
#include "../include/message.h"
#include "../include/user_db.h"

int server_tcp(const Arguments *args);
int server_manager_tcp(const Arguments *args);

int       socket_accept(int server_fd, struct sockaddr_storage *client_addr, socklen_t *client_addr_len);
in_port_t convert_port(const char *binary_name, const char *str);

#endif    // NETWORK_H
