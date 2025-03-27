//
// Created by eckotic on 3/24/25.
//

#include "../include/server_starter.h"
#include "../include/args.h"
#include "../include/network.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define SM_PORT 9000

int main(int argc, char *argv[])
{
    Arguments args;
    int       socket;
    int       server;

    parse_args(argc, argv, &args);
    args.sm_port = SM_PORT;    // hard coded server manager port

    printf("Trying to connect to %s:%d\n", args.sm_ip, args.sm_port);
    socket = server_manager_tcp(&args);

    if(socket < 0)
    {
        printf("server_manager_tcp error\n");
        exit(EXIT_FAILURE);
    }

    // seperate later into a switch statement for "start"
    server = fork();
    if(server == -1)
    {
        perror("fork");
        kill(server, SIGINT);
        close(socket);
        exit(EXIT_FAILURE);
    }
    if(server == 0)
    {
        printf("Starting server...\n");
        execv("./main", argv);
    }
    else
    {
        //        while(1)
        //        {
        // seperate later into switch statement for "stop"
        sleep(3);
        printf("Stopping server...\n");
        kill(server, SIGINT);
        wait(NULL);
        printf("Server stopped!\n");
        //        }
    }
    close(socket);
    close(server);
}
