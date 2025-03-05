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
static int  process_req(int cfd);
static void send_sys_success(uint8_t buf[], int fd, uint8_t packet_type);
static void send_sys_error(uint8_t buf[], int fd, int err);
static void send_acc_login_success(uint8_t buf[], int fd, uint16_t user_id);

static volatile sig_atomic_t server_running;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

int main(int argc, char *argv[])
{
    Arguments args;
    pid_t     pid;
    int       retval;
    int       sockfd;

    memset(&args, 0, sizeof(Arguments));
    args.ip   = NULL;    // Must be set via command-line args
    args.port = 0;

    parse_args(argc, argv, &args);
    check_args(argv[0], &args);    // Ensures args are valid

    // Initialize user list
    init_user_list();

    printf("Listening on %s:%d\n", args.ip, args.port);    // Confirm correct values

    retval = EXIT_SUCCESS;

    sockfd = server_tcp_setup(&args);
    if(sockfd < 0)
    {
        perror("Failed to create server network");
        retval = EXIT_FAILURE;
        goto exit;
    }

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
            perror("main::fork");
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

static int process_req(int cfd)
{
    header_t header = {0};
    uint8_t  buf[PACKETLEN];
    int      result;

    memset(buf, 0, PACKETLEN);
    if(read(cfd, buf, HEADERLEN) < HEADERLEN)
    {
        return -1;
    }
    decode_header(buf, &header);

    if(read(cfd, buf + HEADERLEN, header.payload_len) < header.payload_len)
    {
        return -1;
    }
    result = decode_packet(buf, &header);

    memset(buf, 0, PACKETLEN);
    if(result < 0)
    {
        send_sys_error(buf, cfd, result);
        return SYS_ERROR;
    }

    if(header.packet_type == ACC_LOGIN)
    {
        send_acc_login_success(buf, cfd, 1); /* 1 for testing only */
        return ACC_LOGIN_SUCCESS;
    }

    if(header.packet_type == ACC_CREATE || header.packet_type == ACC_EDIT)
    {
        send_sys_success(buf, cfd, header.packet_type);
        return SYS_SUCCESS;
    }

    if(header.packet_type == ACC_LOGOUT)
    {
        // no response
        return ACC_LOGOUT;
    }

    if(header.packet_type == CHT_SEND)
    {
        send_sys_success(buf, cfd, header.packet_type);

        // send an example of a chat message from another user.
        send_cht_send(buf, cfd);
        return CHT_SEND;
    }
    return -1;
}

static void send_sys_success(uint8_t buf[], int fd, uint8_t packet_type)
{
    int len = encode_sys_success_res(buf, packet_type);
    write(fd, buf, (size_t)len);
}

static void send_sys_error(uint8_t buf[], int fd, int err)
{
    int len = encode_sys_error_res(buf, err);
    write(fd, buf, (size_t)len);
}

static void send_acc_login_success(uint8_t buf[], int fd, uint16_t user_id)
{
    int len = encode_acc_login_success_res(buf, user_id);
    write(fd, buf, (size_t)len);
}

static void send_cht_send(uint8_t buf[], int fd)
{
    int len = encode_cht_send(buf);
    write(fd, buf, (size_t)len);
}