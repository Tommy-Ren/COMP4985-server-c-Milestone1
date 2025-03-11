#include "../include/args.h"
#include "../include/network.h"
#include "../include/user_db.h"    // Include user database header
#include <errno.h>
#include <memory.h>
#include <netinet/in.h>
#include <signal.h>
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

    sockfd = server_tcp(&args);
    if(sockfd < 0)
    {
        perror("Failed to create server network.\n");
        retval = EXIT_FAILURE;
        goto exit;
    }

    sm_fd = server_manager_tcp(&args);
    // if(sm_fd < 0)
    // {
    //     perror("Failed to connect to server manager\n");
    //     retval = EXIT_FAILURE;
    //     goto exit;
    // }

    printf("Connecting to server manager at %s:%d\n", args.sm_ip, args.sm_port);

    setup_signal_handler();

    // Connect to server manager
    // handle_sm(sm_fd, sockfd);

    // Handle client connections
    handle_clients(sockfd);

exit:
    close(sm_fd);
    close(sockfd);
    exit(retval);
}

static void setup_signal_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = sigint_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void sigint_handler(int signum)
{
    server_running = 0;
}

#pragma GCC diagnostic pop
