//
// Created by Kiet & Tommy on 12/1/25.
//

#ifndef MAIN_SERVER_H
#define MAIN_SERVER_H

#include <signal.h>

// struct to hold the info for server
struct serverInformation
{
    int   fd;
    char *ip;
    char *port;
};

// struct to hold the info for client
struct clientInformation
{
    int fd;
};

void server(void);
int  server_setup(char *passedServerInfo[]);
int  socket_create(void);
int  socket_bind(struct serverInformation server);
int  server_close(struct serverInformation server);
int  client_close(int client);
int  send_response_resource(int client_socket, const char *content, size_t content_length);
int  send_response_head(int client_socket, size_t content_length);
int  send_response_post(int client_socket, const char *resPath);
void start_listen(int server_fd);
int  handle_connection(int server_fd, struct clientInformation clients[], int *numClients);
int  get_req_response(int client_socket, const char *filePath);
int  head_req_response(int client_socket, const char *filePath);
int  post_req_response(int client_socket, const char *filePath, const char *data);
int  run_server(void);
#endif    // MAIN_SERVER_H
