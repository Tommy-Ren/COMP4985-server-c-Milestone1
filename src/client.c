//
// Created by Kiet & Tommy on 12/1/25.
//

#include "../include/client.h"
#include "../include/server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void client(void)
{
    printf("CLIENT\n");
}

int connect_client(char *serverInformation[])
{
    const char        *server_ip   = serverInformation[0];
    const char        *server_port = serverInformation[1];
    struct sockaddr_in server_address;
    int                port;
    char              *endptr;
    const int          decimalBase   = 10;
    int                client_socket = socket(AF_INET, SOCK_STREAM, 0);

    printf("IP: %s\n", server_ip);
    printf("PORT: %s\n", server_port);

    if(client_socket == -1)
    {
        perror("Socket create failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;

    if(inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0)
    {
        perror("Invalid IP address");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Convert the port from string to integer
    port                    = (int)strtol(server_port, &endptr, decimalBase);
    server_address.sin_port = htons((uint16_t)port);

    if(connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server at %s:%s\n", server_ip, server_port);

    // Now you can send and receive data through client_socket
    // handle send/recv
    sleep(decimalBase * decimalBase * decimalBase);
    // Close the client socket when done
    close(client_socket);

    return 0;
}
