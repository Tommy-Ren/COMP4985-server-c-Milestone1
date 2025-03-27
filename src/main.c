#include "../include/args.h"
#include "../include/network.h"
#include "../include/user_db.h"
#include "../include/utils.h"    // Declares setup_signal_handler() and server_running
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// #define SM_MAX_ATTEMPTS 5    // Maximum number of connection attempts to server manager
// #define SM_RETRY_DELAY 1     // Delay in seconds between attempts
#define SM_FD 3

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

    // === NEW!!! HARD CODED THE SERVER MANAGER FILE DESCRIPTOR BECAUSE THE SERVER STARTER HANDLES IT ===

    // Try to connect to the server manager up to SM_MAX_ATTEMPTS times.
    sm_fd = SM_FD;
    //    for(int attempt = 0; attempt < SM_MAX_ATTEMPTS; attempt++)
    //    {
    //        printf("Connecting to server manager on %s:%d (attempt %d)...\n", args.sm_ip, args.sm_port, attempt + 1);
    //        sm_fd = server_manager_tcp(&args);
    //        if(sm_fd >= 0)
    //        {
    //            break;
    //        }
    //        fprintf(stderr, "Attempt %d: Failed to connect to server manager, retrying in %d second(s)...\n", attempt + 1, SM_RETRY_DELAY);
    //        sleep(SM_RETRY_DELAY);
    //    }
    //    if(sm_fd < 0)
    //    {
    //        fprintf(stderr, "Failed to connect to server manager after %d attempts, continuing without it.\n", SM_MAX_ATTEMPTS);
    //    }
    //    else
    //    {
    printf("Connected to server manager.\n");
    //    }

    // Start handling client connections (and optionally sending diagnostics to the server manager)
    handle_connections(sockfd, sm_fd);

    return EXIT_SUCCESS;
}
