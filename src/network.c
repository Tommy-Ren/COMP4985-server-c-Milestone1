#include "../include/network.h"
#include <arpa/inet.h>
#include <errno.h>
#include <memory.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


static void setup_addr(struct sockaddr_storage *addr, socklen_t *addr_len, const Arguments *args, int *err);
static int  create_socket(struct sockaddr_storage *addr, socklen_t addr_len, int *err);
static void socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port);

// Function to set up the network server
int sever_network(const Arguments *args)
{
    // Initialize variables
    int                     err;
    int                     sockfd;
    struct sockaddr_storage addr;
    socklen_t               addr_len;

    addr_len = 0;
    err      = 0;

    memset(&addr, 0, sizeof(struct sockaddr_storage));

    // Set up the address structure
    setup_addr(&addr, &addr_len, args, &err);

    // Create the socket
    sockfd = create_socket(&addr, addr_len, err);
    if(sockfd < 0)
    {
        errno = err;
        perror("Failed to create socket");
        sockfd = ERR_SOCKET;
        goto exit;
    }

    // Set socket options
    errno = 0;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
    {
        perror("Failed to set socket options");
        sockfd = ERR_SET_OPTION;
        goto exit;
    }

    // Bind the socket
    errno = 0;
    socket_bind(sockfd, &addr, args->port);
    if(sockfd < 0)
    {
        perror("Failed to bind socket");
        close(sockfd);
        sockfd = ERR_BIND;
        goto exit;
    }

    // Start listening
    errno = 0;
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

// Function to open network socket
static int create_socket(struct sockaddr_storage *addr, socklen_t addr_len, int *err)
{
    int sockfd;

    errno = 0;

    sockfd = socket(addr->ss_family, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        *err = errno;
        return -1;
    }

    return sockfd;
}

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
}

// Set up address structure
static void setup_addr(struct sockaddr_storage *addr, socklen_t *addr_len, const Arguments *args, int *err)
{
    in_port_t net_port = htons(args->port);
    memset(addr, 0, sizeof(struct sockaddr_storage));
    *err = 0;

    // Try to interpret the address as IPv4
    if(inet_pton(AF_INET, args->ip, &((struct sockaddr_in *)addr)->sin_addr) == 1)
    {
        struct sockaddr_in *ipv4_addr = (struct sockaddr_in *)addr;
        ipv4_addr->sin_family         = AF_INET;
        ipv4_addr->sin_port           = net_port;
        *addr_len                     = sizeof(struct sockaddr_in);
    }
    // If IPv4 fails, try interpreting it as IPv6
    else if(inet_pton(AF_INET6, args->ip, &((struct sockaddr_in6 *)addr)->sin6_addr) == 1)
    {
        struct sockaddr_in6 *ipv6_addr = (struct sockaddr_in6 *)addr;
        ipv6_addr->sin6_family         = AF_INET6;
        ipv6_addr->sin6_port           = net_port;
        *addr_len                      = sizeof(struct sockaddr_in6);
    }
    // If neither IPv4 nor IPv6, set an error
    else
    {
        fprintf(stderr, "%s is not a valid IPv4 or IPv6 address\n", args->ip);
        *err = errno;
    }
}

// Function to convert port if network socket as type
in_port_t convertPort(const char *str, int *err)
{
    in_port_t port;
    char     *endptr;
    long      val;

    *err  = ERR_NONE;
    port  = 0;
    errno = 0;
    val   = strtol(str, &endptr, 10);

    // Check if no digits were found
    if(endptr == str)
    {
        *err = ERR_NO_DIGITS;
        return port;
    }

    // Check for out-of-range errors
    if(val < 0 || val > UINT16_MAX)
    {
        *err = ERR_OUT_OF_RANGE;
        return port;
    }

    // Check for trailing invalid characters
    if(*endptr != '\0')
    {
        *err = ERR_INVALID_CHARS;
        return port;
    }

    port = (in_port_t)val;
    return port;
}

// Free all network resouces
void null_free(void **ptr)
{
    if(ptr != NULL && *ptr != NULL)
    {
        free(*ptr);
        *ptr = NULL;
    }
}