#include "../include/args.h"
#include "../include/network.h"
#include "../include/user_db.h"
#include "../include/utils.h"    // Declares setup_signal_handler() and server_running
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    Arguments args;
    int       sockfd;
    int       sm_fd;    // No initial assignment needed

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
    }

    // handle_connections() handle server manager and clients
    handle_connections(sockfd, sm_fd);

    // When a shutdown signal is received, handle_clients() returns.
    if(sm_fd >= 0)
    {
        close(sm_fd);
        sm_fd = -1;
    }
    if(sockfd >= 0)
    {
        close(sockfd);
        sockfd = -1;
    }

    return EXIT_SUCCESS;
}
