/* network.c */

#include "../include/network.h"
#include "../include/user_db.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void socket_setup(struct sockaddr_storage *addr, socklen_t *addr_len, const Arguments *args);
static int  socket_create(int domain, int type, int protocol);
static int  socket_set(int sockfd);
static int  socket_bind(int sockfd, struct sockaddr_storage *addr, socklen_t addr_len);
static int  socket_listen(int server_fd, int backlog);
static int  socket_connect(int sockfd, struct sockaddr_storage *addr, socklen_t addr_len);

#define BASE_TEN 10
#define ERR_NONE (0)
#define ERR_SOCKET (-1)
#define ERR_SET_OPTION (-2)
#define ERR_BIND (-3)
#define ERR_LISTEN (-4)
#define ERR_CONNECT (-5)

/*
 * Function: server_tcp_setup
 * Description: Sets up a TCP server using the provided arguments.
 * Returns: The server socket descriptor on success or an error code on failure.
 */
int server_tcp(const Arguments *args)
{
    int                     sockfd;
    struct sockaddr_storage addr;
    socklen_t               addr_len;

    addr_len = 0;
    sockfd   = ERR_NONE;

    memset(&addr, 0, sizeof(struct sockaddr_storage));

    /* Set up the address structure */
    socket_setup(&addr, &addr_len, args);

    /* Create the socket */
    sockfd = socket_create(addr.ss_family, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        perror("Failed to create socket");
        sockfd = ERR_SOCKET;
        goto exit;
    }

    /* Set socket options */
    if(socket_set(sockfd) < 0)
    {
        perror("Failed to set socket options");
        sockfd = ERR_SET_OPTION;
        goto exit;
    }

    /* Bind the socket */
    if(socket_bind(sockfd, &addr, addr_len) < 0)
    {
        perror("Failed to bind socket");
        sockfd = ERR_BIND;
        goto exit;
    }
    printf("Bound to socket: %s:%u\n", args->ip, args->port);

    /* Start listening */
    if(start_listen(sockfd, SOMAXCONN) < 0)
    {
        perror("Failed to listen on socket");
        close(sockfd);
        sockfd = ERR_LISTEN;
        goto exit;
    }
    printf("Listening on %s:%u\n", args->ip, args->port);

exit:
    return sockfd;
}

/*
 * Function: server_manager_tcp
 * Description: Connects to the server manager using the provided arguments.
 * Returns: The server manager socket descriptor on success or an error code on failure.
 */
int server_manager_tcp(const Arguments *args)
{
    int                     sm_fd;
    struct sockaddr_storage addr;
    socklen_t               addr_len;

    addr_len = 0;
    sm_fd    = ERR_NONE;

    memset(&addr, 0, sizeof(struct sockaddr_storage));

    /* Set up the address structure */
    socket_setup(&addr, &addr_len, args);

    /* Create the socket */
    sm_fd = socket_create(addr.ss_family, SOCK_STREAM, 0);
    if(sm_fd < 0)
    {
        perror("Failed to create socket");
        sm_fd = ERR_SOCKET;
        goto exit;
    }

    /* Set socket options */
    if(socket_set(sm_fd) < 0)
    {
        perror("Failed to set socket options");
        sm_fd = ERR_SET_OPTION;
        goto exit;
    }

    /* Connect to the server manager */
    if(socket_connect(sm_fd, &addr, addr_len) < 0)
    {
        perror("Failed to connect to server manager");
        sm_fd = ERR_CONNECT;
        goto exit;
    }
    printf("Connected to server manager at %s:%u\n", args->sm_ip, args->sm_port);

exit:
    return sm_fd;
}

/* Set up address structure */
static void socket_setup(struct sockaddr_storage *addr, socklen_t *addr_len, const Arguments *args)
{
    in_port_t net_port;
    net_port = htons(args->port);
    memset(addr, 0, sizeof(struct sockaddr_storage));

    /* Try to interpret the address as IPv4 */
    if(inet_pton(AF_INET, args->ip, &((struct sockaddr_in *)addr)->sin_addr) == 1)
    {
        {
            struct sockaddr_in *ipv4_addr;
            ipv4_addr             = (struct sockaddr_in *)addr;
            ipv4_addr->sin_family = AF_INET;
            ipv4_addr->sin_port   = net_port;
            *addr_len             = sizeof(struct sockaddr_in);
        }
    }
    /* If IPv4 fails, try interpreting it as IPv6 */
    else if(inet_pton(AF_INET6, args->ip, &((struct sockaddr_in6 *)addr)->sin6_addr) == 1)
    {
        {
            struct sockaddr_in6 *ipv6_addr;
            ipv6_addr              = (struct sockaddr_in6 *)addr;
            ipv6_addr->sin6_family = AF_INET6;
            ipv6_addr->sin6_port   = net_port;
            *addr_len              = sizeof(struct sockaddr_in6);
        }
    }
    /* If neither IPv4 nor IPv6, set an error */
    else
    {
        fprintf(stderr, "%s is not a valid IPv4 or IPv6 address\n", args->ip);
        exit(EXIT_FAILURE);
    }
}

/* Create a socket */
static int socket_create(int domain, int type, int protocol)
{
    int sockfd;
    sockfd = socket(domain, type, protocol);
    if(sockfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

/* Set socket options */
static int socket_set(int sockfd)
{
    // Get the current flags
    int flag = fcntl(socket, F_GETFL, 0);
    if(flag == -1)
    {
        perror("Failed to get flag");
        return -1;
    }

    // Set the socket to non-blocking
    flag |= O_NONBLOCK;
    if(fcntl(socket, F_SETFL, flag) == -1)
    {
        perror("Failed to set flag");
        return -1;
    }

    // Set the socket to reuse the address
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("Failed to set socket options");
        return -1;
    }
}

/* Bind the socket to an address */
static int socket_bind(int sockfd, struct sockaddr_storage *addr, socklen_t addr_len)
{
    int ret = bind(sockfd, (struct sockaddr *)addr, addr_len);
    return ret;
}

/* Start listening on the socket */
static int socket_listen(int server_fd, int backlog)
{
    int ret = listen(server_fd, backlog);
    return ret;
}

static int socket_connect(int sockfd, struct sockaddr_storage *addr, socklen_t addr_len)
{
    int ret = connect(sockfd, (struct sockaddr *)addr, addr_len);
    return ret;
}

/* Accept a client connection */
int socket_accept(int server_fd, struct sockaddr_storage *client_addr, socklen_t *client_addr_len)
{
    int client_fd;
    client_fd = accept(server_fd, (struct sockaddr *)client_addr, client_addr_len);
    if(client_fd < 0)
    {
        return -1;
    }
    return client_fd;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* Convert port from string to in_port_t */
in_port_t convert_port(const char *binary_name, const char *str)
{
    char     *endptr;
    uintmax_t parsed_value;

    parsed_value = strtoumax(str, &endptr, BASE_TEN);
    if(errno != 0)
    {
        perror("Error parsing in_port_t");
        exit(EXIT_FAILURE);
    }
    if(*endptr != '\0')
    {
        usage(binary_name, EXIT_FAILURE, "Invalid characters in input.");
    }
    if(parsed_value < 1 || parsed_value > UINT16_MAX)
    {
        usage(binary_name, EXIT_FAILURE, "in_port_t value out of range.");
    }
    return (in_port_t)parsed_value;
}

#pragma GCC diagnostic pop

/* Shutdown a socket */
void shutdown_socket(int sockfd, int how)
{
    if(shutdown(sockfd, how) == -1)
    {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}

/* Close a socket; note: obsolete references to user_arr have been removed */
void socket_close(int sockfd)
{
    if(close(sockfd) == -1)
    {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
    printf("Socket %d closed successfully.\n", sockfd);
}
