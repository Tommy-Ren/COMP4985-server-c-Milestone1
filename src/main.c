#include "../include/args.h"
#include "../include/asn.h"
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

static void setup_signal_handler(void);
static void sigint_handler(int signum);

static volatile sig_atomic_t server_running;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

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

    sockfd = server_tcp_setup(&args);
    if(sockfd < 0)
    {
        perror("Failed to create server network");
        retval = EXIT_FAILURE;
        goto exit;
    }

    sm_fd = client_tcp_setup(&args);
    // if(sm_fd < 0)
    // {
    //     perror("Failed to create server manager network");
    //     retval = EXIT_FAILURE;
    //     goto exit;
    // }

    printf("Connecting to server manager at %s:%d\n", args.sm_ip, args.sm_port);

    setup_signal_handler();
    server_running = 1;

    while(server_running)
    {
        int                     client_fd;
        struct sockaddr_storage client_addr;
        socklen_t               client_addr_len;

        client_addr_len = sizeof(struct sockaddr_storage);
        memset(&client_addr, 0, client_addr_len);

        client_fd = socket_accept(sockfd, &client_addr, &client_addr_len);
        if(client_fd < 0)
        {
            if(!server_running)
            {
                break;
            }
            continue;
        }

        // Fork the process
        pid = fork();
        if(pid < 0)
        {
            perror("Fail to fork");
            remove_user(client_fd);    // Remove user on fork failure
            close(client_fd);
            continue;
        }

        if(pid == 0)    // Child process
        {
            char buffer[1];
            process_req(client_fd);

            // Wait for client to disconnect before cleanup
            while(recv(client_fd, buffer, sizeof(buffer), 0) > 0)
            {
                // Keep reading until the client disconnects
            }

            printf("Client %d disconnected, removing from list.\n", client_fd);
            remove_user(client_fd);
            close(client_fd);
            goto exit;
        }

        close(client_fd);
    }

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
