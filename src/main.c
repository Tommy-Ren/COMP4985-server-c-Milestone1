#include "../include/network.h"
#include <memory.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    Arguments args;
    pid_t     pid;
    int       retval;
    int       sockfd;
    int       sm_fd;    // Server manager file descriptor

    memset(&args, 0, sizeof(Arguments));
    args.ip   = NULL;
    args.port = 0;

    parse_args(argc, argv, &args);

    // Initialize user list
    init_user_list();

    printf("Listening on %s:%d\n", args.ip, args.port);

    retval = EXIT_SUCCESS;

    setup_signal_handler();

    sockfd = server_tcp(&args);
    if(sockfd < 0)
    {
        perror("Failed to create server network.\n");
        retval = EXIT_FAILURE;
        goto exit;
    }

    printf("Connecting to server manager on %s:%d\n", args.sm_ip, args.sm_port);
    sm_fd = server_manager_tcp(&args);
    // if(sm_fd < 0)
    // {
    //     perror("Failed to connect to server manager\n");
    //     retval = EXIT_FAILURE;
    //     goto exit;
    // }

    // Connect to server manager
    // handle_sm(sm_fd, sockfd);

    // Handle client connections
    handle_clients(sockfd);

exit:
    close(sm_fd);
    close(sockfd);
    exit(retval);
}
