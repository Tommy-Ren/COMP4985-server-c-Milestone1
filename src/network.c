/* network.c */

#include "../include/network.h"
#include "../include/user_db.h"
#include <arpa/inet.h>
#include <errno.h>
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

static void socket_setup(struct sockaddr_storage *addr, socklen_t *addr_len, const Arguments *args, int *err);
static void sm_socket_setup(struct sockaddr_storage *addr, socklen_t *addr_len, const Arguments *args, int *err);
static int  socket_create(int domain, int type, int protocol);
static void socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port);
static void start_listen(int server_fd, int backlog);

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
int server_tcp_setup(const Arguments *args)
{
    int                     err;
    int                     sockfd;
    struct sockaddr_storage addr;
    socklen_t               addr_len;

    addr_len = 0;
    err      = ERR_NONE;

    memset(&addr, 0, sizeof(struct sockaddr_storage));

    /* Set up the address structure */
    socket_setup(&addr, &addr_len, args, &err);

    /* Create the socket */
    sockfd = socket_create(addr.ss_family, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        errno = err;
        perror("Failed to create socket");
        sockfd = ERR_SOCKET;
        goto exit;
    }

    /* Set socket options */
    errno = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("Failed to set socket options");
        sockfd = ERR_SET_OPTION;
        goto exit;
    }

    /* Bind the socket */
    socket_bind(sockfd, &addr, args->port);
    if(sockfd < 0)
    {
        perror("Failed to bind socket");
        sockfd = ERR_BIND;
        goto exit;
    }

    /* Start listening */
    start_listen(sockfd, SOMAXCONN);
    if(listen(sockfd, SOMAXCONN) < 0)
    {
        perror("Failed to start listening");
        close(sockfd);
        sockfd = ERR_LISTEN;
        goto exit;
    }

exit:
    return sockfd;
}

int client_tcp_setup(const Arguments *args)
{
    int                     err;
    int                     sm_fd;
    struct sockaddr_storage addr;
    socklen_t               addr_len;

    addr_len = 0;
    err      = ERR_NONE;

    memset(&addr, 0, sizeof(struct sockaddr_storage));

    /* Set up the address structure */
    sm_socket_setup(&addr, &addr_len, args, &err);

    /* Create the socket */
    sm_fd = socket_create(addr.ss_family, SOCK_STREAM, 0);
    if(sm_fd < 0)
    {
        errno = err;
        perror("Failed to create socket to server manager");
        sm_fd = ERR_SOCKET;
        goto exit;
    }

    /* Set socket options */
    errno = 0;
    if(connect(sm_fd, (struct sockaddr *)&addr, addr_len) < 0)
    {
        perror("Failed to connect to server manager");
        sm_fd = ERR_CONNECT;
        goto exit;
    }

exit:
    return sm_fd;
}

/* Set up address structure */
static void socket_setup(struct sockaddr_storage *addr, socklen_t *addr_len, const Arguments *args, int *err)
{
    in_port_t net_port;
    net_port = htons(args->port);
    memset(addr, 0, sizeof(struct sockaddr_storage));
    *err = 0;

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
        *err = errno;
    }
}

static void sm_socket_setup(struct sockaddr_storage *addr, socklen_t *addr_len, const Arguments *args, int *err)
{
    in_port_t net_port;
    net_port = htons(args->sm_port);
    memset(addr, 0, sizeof(struct sockaddr_storage));
    *err = 0;

    /* Try to interpret the address as IPv4 */
    if(inet_pton(AF_INET, args->sm_ip, &((struct sockaddr_in *)addr)->sin_addr) == 1)
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
    else if(inet_pton(AF_INET6, args->sm_ip, &((struct sockaddr_in6 *)addr)->sin6_addr) == 1)
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
        fprintf(stderr, "%s is not a valid IPv4 or IPv6 address\n", args->sm_ip);
        *err = errno;
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

/* Bind the socket to an address */
static void socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port)
{
    char      addr_str[INET6_ADDRSTRLEN];
    socklen_t addr_len;
    void     *vaddr;
    in_port_t net_port;

    net_port = htons(port);

    if(addr->ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4_addr;
        ipv4_addr           = (struct sockaddr_in *)addr;
        addr_len            = sizeof(*ipv4_addr);
        ipv4_addr->sin_port = net_port;
        vaddr               = (void *)&(((struct sockaddr_in *)addr)->sin_addr);
    }
    else if(addr->ss_family == AF_INET6)
    {
        struct sockaddr_in6 *ipv6_addr;
        ipv6_addr            = (struct sockaddr_in6 *)addr;
        addr_len             = sizeof(*ipv6_addr);
        ipv6_addr->sin6_port = net_port;
        vaddr                = (void *)&(((struct sockaddr_in6 *)addr)->sin6_addr);
    }
    else
    {
        fprintf(stderr, "Internal error: addr->ss_family must be AF_INET or AF_INET6, was: %d\n", addr->ss_family);
        exit(EXIT_FAILURE);
    }

    if(inet_ntop(addr->ss_family, vaddr, addr_str, sizeof(addr_str)) == NULL)
    {
        perror("inet_ntop");
        exit(EXIT_FAILURE);
    }

    if(bind(sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        perror("Binding failed");
        fprintf(stderr, "Error code: %d\n", errno);
        exit(EXIT_FAILURE);
    }

    printf("Bound to socket: %s:%u\n", addr_str, port);
}

/* Start listening on the socket */
static void start_listen(int server_fd, int backlog)
{
    if(listen(server_fd, backlog) == -1)
    {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Listening for incoming connections...\n");
}

/* Accept a client connection */
int socket_accept(int server_fd, struct sockaddr_storage *client_addr, socklen_t *client_addr_len)
{
    int  client_fd;
    char client_host[NI_MAXHOST];
    char client_service[NI_MAXSERV];

    errno     = 0;
    client_fd = accept(server_fd, (struct sockaddr *)client_addr, client_addr_len);
    if(client_fd == -1)
    {
        if(errno != EINTR)
        {
            perror("Failed to accept client connection");
        }
        return -1;
    }

    if(getnameinfo((struct sockaddr *)client_addr, *client_addr_len, client_host, NI_MAXHOST, client_service, NI_MAXSERV, 0) == 0)
    {
        user_obj *user;
        printf("Accepted a new connection from %s:%s\n", client_host, client_service);
        user = new_user();
        if(!user)
        {
            close(client_fd);
            return -1;
        }
        user->id = client_fd; /* Assign socket FD as user ID for now */
        if(add_user(user) == -1)
        {
            free(user);
            close(client_fd);
            return -1;
        }
    }
    else
    {
        printf("Unable to get client information\n");
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

    errno        = 0;
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
