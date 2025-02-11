#include "../include/args.h"
#include "../include/asn.h"
#include "../include/network.h"
#include <errno.h>
#include <memory.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#if defined(__linux__) && defined(__clang__)
_Pragma("clang diagnostic ignored \"-Wdisabled-macro-expansion\"")
#endif

#define IP_ADDRESS "0.0.0.0"
#define PORT "8000"
#define SIG_BUF 50

    static volatile sig_atomic_t server_running;

static void handle_signal(int sig);
static void process_req(int cfd);
static void send_sys_success(uint8_t buf[], int fd, uint8_t packet_type);
static void send_sys_error(uint8_t buf[], int fd, int err);
static void send_acc_login_success(uint8_t buf[], int fd, uint16_t user_id);

static void handle_signal(int sig)
{
    char message[SIG_BUF];

    snprintf(message, sizeof(message), "Caught signal: %d (%s)\n", sig, strsignal(sig));
    write(STDOUT_FILENO, message, strlen(message));

    if(sig == SIGINT)
    {
        server_running = 0;
        snprintf(message, sizeof(message), "%s\n", "Server is shutting down...");
    }
    write(STDOUT_FILENO, message, strlen(message));
}

int main(int argc, char *argv[])
{
    int              retval;    // Return value
    int              err;       // Error code
    pid_t            pid;       // Process ID
    Arguments        args;      // Arguments
    struct sigaction sa;        // Signal action
    int              sockfd;    // Socket file descriptor

    // Set Ctrl-C override.
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    printf("Server is running... (press Ctrl+C to interrupt)\n");

    memset(&args, 0, sizeof(Arguments));
    args.ip   = IP_ADDRESS;
    args.port = convert_port(PORT, &err);

    parse_args(&args, argc, argv);
    check_args(argv[0], &args);
    printf("Listening on %s: %d\n", args.ip, args.port);
    retval = EXIT_SUCCESS;

    // Connect to the network
    sockfd = sever_network(&args);
    if(sockfd < 0)
    {
        perror("Failed to create server network");
        retval = EXIT_FAILURE;
        goto exit;
    }

    // Wait for clients' connections
    err            = 0;
    server_running = 1;
    while(server_running)
    {
        int                cfd;      // Client file descriptor
        struct sockaddr_in caddr;    // Client address
        socklen_t          csize;    // Client size

        // !!BLOCKING!! Get client connection
        csize = sizeof(struct sockaddr_in);
        memset(&caddr, 0, csize);

        errno = 0;
        cfd   = accept(sockfd, (struct sockaddr *)&caddr, &csize);
        if(cfd < 0)
        {
            perror("Failed to accept client connection");
            continue;
        }

        printf("New connection from: %s:%d\n", inet_ntoa(caddr.sin_addr), caddr.sin_port);

        // Fork the process
        errno = 0;
        pid   = fork();
        if(pid < 0)
        {
            perror("main::fork");
            close(cfd);
            continue;
        }

        if(pid == 0)
        {
            process_req(cfd);
            close(cfd);
            goto exit;
        }

        close(cfd);
    }

exit:
    close(sockfd);
    return retval;
}

static void process_req(int cfd)
{
    int      err;
    header_t header = {0};
    uint8_t  buf[PACKETLEN];

    read(cfd, buf, HEADERLEN);
    decode_header(buf, &header);

    read(cfd, buf + HEADERLEN, header.payload_len);
    err = decode_packet(buf, &header);

    memset(buf, 0, PACKETLEN);
    if(err < 0)
    {
        send_sys_error(buf, cfd, err);
    }

    if(header.packet_type == ACC_LOGIN)
    {
        send_acc_login_success(buf, cfd, 1); /* 1 for testing only */
    }

    if(header.packet_type == ACC_LOGOUT || header.packet_type == ACC_CREATE || header.packet_type == ACC_EDIT)
    {
        send_sys_success(buf, cfd, header.packet_type);
    }
}

static void send_sys_success(uint8_t buf[], int fd, uint8_t packet_type)
{
    int len = encode_sys_success_res(buf, packet_type);
    write(fd, buf, (size_t)len);
}

static void send_sys_error(uint8_t buf[], int fd, int err)
{
    int len = 0;
    len     = encode_sys_error_res(buf, err);
    write(fd, buf, (size_t)len);
}

static void send_acc_login_success(uint8_t buf[], int fd, uint16_t user_id)
{
    int len = encode_acc_login_success_res(buf, user_id);
    write(fd, buf, (size_t)len);
}
