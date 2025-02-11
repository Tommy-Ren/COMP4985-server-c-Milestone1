#include "../include/args.h"
#include "../include/asn.h"
#include "../include/server.h"
#include <errno.h>
#include <memory.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define IP_ADDRESS "0.0.0.0"
#define PORT "8000"

typedef struct data_t
{
    int                fd;
    int                cfd; /* client fd */
    struct sockaddr_in addr;
    socklen_t          addr_len;
} data_t;

static volatile sig_atomic_t server_running;

static void sigint_handler(int signum);
static void handle_signal(int sig);
static void process_req(const data_t *d);
static void send_sys_success(uint8_t buf[], int fd, uint8_t packet_type);
static void send_sys_error(uint8_t buf[], int fd, int err);
static void send_acc_login_success(uint8_t buf[], int fd, uint16_t user_id);

static void sigint_handler(int signum)
{
    server_running = 0;
}

static void handle_signal(int sig)
{
    if(sig == SIGINT)
    {
        server_running = 0;
        printf("\nShutting down server...");
    }
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

    printf("Server is server_running... (press Ctrl+C to interrupt)\n");

    memset(&args, 0, sizeof(Arguments));
    args.ip   = IP_ADDRESS;
    args.port = convert_port(PORT, &err);

    parse_args(&args, argc, argv);
    check_arguemnts(args);
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
        int                connfd;
        struct sockaddr_in connaddr;
        socklen_t          connsize;

        // !!BLOCKING!! Get client connection
        connsize = sizeof(struct sockaddr_in);
        memset(&connaddr, 0, connsize);

        errno  = 0;
        connfd = accept(sockfd, (struct sockaddr *)&connaddr, &connsize);
        if(connfd < 0)
        {
            // perror("main::accept");
            continue;
        }

        printf("New connection from: %s:%d\n", inet_ntoa(connaddr.sin_addr), connaddr.sin_port);

        // Fork the process
        errno = 0;
        pid   = fork();
        if(pid < 0)
        {
            perror("main::fork");
            close(connfd);
            continue;
        }

        if(pid == 0)
        {
            retval = request_handler(connfd);
            close(connfd);
            goto exit;
        }

        close(connfd);
    }

exit:
    close(sockfd);
    return retval;
}

static void process_req(const data_t *d)
{
    header_t header = {0};
    uint8_t  buf[PACKETLEN];
    int      result;

    read(d->cfd, buf, HEADERLEN);
    decode_header(buf, &header);

    read(d->cfd, buf + HEADERLEN, header.payload_len);
    result = decode_packet(buf, &header);

    memset(buf, 0, PACKETLEN);
    if(result < 0)
    {
        send_sys_error(buf, d->cfd, result);
    }

    if(header.packet_type == ACC_LOGIN)
    {
        send_acc_login_success(buf, d->cfd, 1); /* 1 for testing only */
    }

    if(header.packet_type == ACC_LOGOUT || header.packet_type == ACC_CREATE || header.packet_type == ACC_EDIT)
    {
        send_sys_success(buf, d->cfd, header.packet_type);
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
