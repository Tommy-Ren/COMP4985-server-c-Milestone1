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

static void setup_signal_handler(void);
static void sigint_handler(int signum);
static void process_req(int cfd);
static void send_sys_success(uint8_t buf[], int fd, uint8_t packet_type);
static void send_sys_error(uint8_t buf[], int fd, int err);
static void send_acc_login_success(uint8_t buf[], int fd, uint16_t user_id);

#define IP_ADDRESS "0.0.0.0"
#define PORT "8000"
static volatile sig_atomic_t server_running;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

int main(int argc, char *argv[])
{
    Arguments args;      // Arguments
    pid_t     pid;       // Process ID
    int       retval;    // Return value
    int       sockfd;    // Socket file descriptor

    memset(&args, 0, sizeof(Arguments));
    args.ip   = IP_ADDRESS;
    args.port = convert_port(argv[0], PORT);

    parse_args(argc, argv, &args);
    check_args(argv[0], &args);

    printf("Listening on %s:%d\n", args.ip, args.port);
    retval = EXIT_SUCCESS;

    // Connect to the network
    sockfd = server_tcp_setup(&args);
    if(sockfd < 0)
    {
        perror("Failed to create server network");
        retval = EXIT_FAILURE;
        goto exit;
    }

    // Wait for clients' connections
    setup_signal_handler();
    server_running = 1;
    while(server_running)
    {
        int                     client_fd;          // Client file descriptor
        struct sockaddr_storage client_addr;        // Client address
        socklen_t               client_addr_len;    // Client size

        // !!BLOCKING!! Get client connection
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
            perror("main::fork");
            close(client_fd);
            continue;
        }

        // If child process, process the request
        if(pid == 0)
        {
            process_req(client_fd);
            close(client_fd);
            goto exit;
        }

        close(client_fd);
    }

exit:
    close(sockfd);
    return retval;
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
