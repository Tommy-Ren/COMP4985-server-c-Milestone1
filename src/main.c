#include "../include/args.h"
#include "../include/network.h"
#include "../include/user_db.h"
#include "../include/utils.h"    // Declares setup_signal_handler() and server_running
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    Arguments args;
    int       sockfd;
    int       sm_fd;    // Server manager socket descriptor

    memset(&args, 0, sizeof(Arguments));
    args.ip   = NULL;
    args.port = 0;

    parse_args(argc, argv, &args);

    printf("Listening on %s:%d\n", args.ip, args.port);

    // Set up signal handler
    setup_signal_handler();

    sockfd = server_tcp(&args);
    if(sockfd < 0)
    {
        perror("Failed to create server network.");
        exit(EXIT_FAILURE);
    }

    printf("Connecting to server manager on %s:%d\n", args.sm_ip, args.sm_port);
    sm_fd = server_manager_tcp(&args);
    if(sm_fd < 0)
    {
        fprintf(stderr, "Failed to connect to server manager, continuing without it.\n");
        sm_fd = -1;    // Mark sm_fd as invalid so that diagnostics won't be sent.
    }

    // handle_connections() handles both server manager and client connections
    handle_connections(sockfd, sm_fd);

    return EXIT_SUCCESS;
}
