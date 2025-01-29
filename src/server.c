
//
// Created by Kiet & Tommy on 12/1/25.
//

#include "../include/server.h"
#include "../include/fileTools.h"
#include "../include/httpRequest.h"
#include "../include/stringTools.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <ndbm.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1024
// #define STATUS_INTERNAL_SERVER_ERR 500
// #define STATUS_OK 200
// #define STATUS_RES_CREATED 201
// todo handle the different cases in the response :: 404, 504, etc
// todo re-comment code for more clarity
// todo break down functions into smaller functions to pinpoint errors, etc
// todo move functions to appropriate modules
// todo implement read fully --> wait until all bits are read (wait until end of
// req \r\n\r\n for reading)

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static volatile sig_atomic_t exit_flag = 0;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static struct pollfd fds[SOMAXCONN];

void server(void)
{
    printf("SERVER\n");
}

static int set_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL, 0);
    if(flags == -1)
    {
        perror("fcntl F_GETFL");
        return -1;
    }
    if(fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl F_SETFL O_NONBLOCK");
        return -1;
    }
    return 0;
}

int server_setup(char *passedServerInfo[])
{
    struct serverInformation newServer;
    struct clientInformation clients[SOMAXCONN];
    int                      numClients;
    // sigaction --> close
    // server setup
    newServer.ip   = passedServerInfo[0];
    newServer.port = passedServerInfo[1];
    // socket
    newServer.fd = socket_create();
    if(set_nonblocking(newServer.fd) == -1)
    {
        perror("set_nonblocking failed");
        close(newServer.fd);
        return 0;
    }
    // bind
    if(socket_bind(newServer))
    {
        perror("SOCKET BIND");
        return -1;
    }

    start_listen(newServer.fd);

    // Initialize the main server socket in the pollfd array
    fds[0].fd     = newServer.fd;
    fds[0].events = POLLIN;
    numClients    = 1;    // Number of clients (starting with the server itself)

    handle_connection(newServer.fd, clients, &numClients);

    server_close(newServer);
    return 0;
}

// to set up server:

int socket_create(void)
{
    int serverSocket;
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1)
    {
        perror("Socket create failed");
        exit(EXIT_FAILURE);
    }
    return serverSocket;
}

int socket_bind(struct serverInformation activeServer)
{
    struct sockaddr_in server_address;
    int                port;
    char              *endptr;
    const int          decimalBase = 10;

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;

    if(inet_pton(AF_INET, activeServer.ip, &server_address.sin_addr) <= 0)
    {
        perror("Invalid IP address");
        return -1;
    }

    port = (int)strtol(activeServer.port, &endptr, decimalBase);

    if(*endptr != '\0' && *endptr != '\n')
    {
        perror("Invalid port number");
        return -1;
    }

    server_address.sin_port = htons((uint16_t)port);

    if(bind(activeServer.fd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Socket bound successfully to %s:%s.\n", activeServer.ip, activeServer.port);

    return 0;
}

void start_listen(int server_fd)
{
    if(listen(server_fd, SOMAXCONN) == -1)
    {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Listening for incoming connections...\n");
}

/**
 * Function handles main logic loop for ready client sockets using IO
 * multiplexing
 * @param server_fd server socket
 * @param clients array of client socket fds
 * @param numClients number of clients
 * @return 1 on success
 */
int handle_connection(int server_fd, struct clientInformation clients[], int *numClients)
{
    TokenAndStr        requestFirstLine;
    const HTTPRequest *httpRequest;

    // To silence the errors :/
    httpRequest = NULL;
    printHTTPRequestStruct(httpRequest);

    while(!exit_flag)
    {
        int activity;
        for(int i = 1; i < *numClients; i++)
        {
            fds[i].fd     = clients[i].fd;
            fds[i].events = POLLIN;
        }

        // poll used for IO multiplexing
        activity = poll(fds, (nfds_t)*numClients, -1);    // -1 means wait indefinitely

        if(activity == -1)
        {
            perror("poll failed");
            exit(EXIT_FAILURE);
        }

        // check for new connection on the main server socket
        if(fds[0].revents & POLLIN)
        {
            int client_fd = accept(server_fd, NULL, NULL);
            if(client_fd == -1)
            {
                perror("accept failed");
                // Handle error if needed
            }
            else
            {
                // Set client socket to non-blocking mode
                if(set_nonblocking(client_fd) == -1)
                {
                    perror("set_nonblocking failed");
                    // Handle error if needed
                    close(client_fd);
                }
                else
                {
                    printf("New client: adding to array\n");
                    clients[*numClients].fd = client_fd;
                    // TODO add to client struct here when ready using
                    // clients[*numClients]
                    (*numClients)++;

                    fds[*numClients - 1].fd = client_fd;

                    // waits for next input from any client
                    fds[*numClients - 1].events = POLLIN;
                }
            }
        }

        // check if any client is ready to send a response to
        for(int i = 1; i < *numClients; ++i)
        {
            if(fds[i].revents & POLLIN)
            {
                char    buffer[MAX_BUFFER_SIZE];
                ssize_t bytesRead;
                printf("Client ready; working..\n");
                // read in req
                bytesRead = recv(clients[i].fd, buffer, sizeof(buffer), 0);
                if(bytesRead == 0)
                {
                    printf("Client disconnected: %d\n", clients[i].fd);
                    close(clients[i].fd);
                    for(int j = i; j < *numClients - 1; ++j)
                    {
                        clients[j] = clients[j + 1];
                        fds[j]     = fds[j + 1];
                    }
                    (*numClients)--;
                }
                else if(bytesRead < 0)
                {
                    perror("recv failed");
                }
                else
                {
                    // todo implement read fully (wait until \r\n\r\n)
                    printf("\n\n"
                           "----- START CLIENT REQUEST ----- "
                           "\n\n"
                           "%s\n"
                           "----- END CLIENT REQUEST ----- \n"
                           "\n\n",
                           buffer);

                    // Get only the first line.
                    requestFirstLine = getFirstToken(buffer, "\n");

                    printf("Request first line: %s\n", requestFirstLine.token);

                    httpRequest = initializeHTTPRequestFromString(requestFirstLine.token);
                    printHTTPRequestStruct(httpRequest);

                    // handle request
                    if(strcmp(httpRequest->method, "GET") == 0)
                    {
                        get_req_response(clients[i].fd, httpRequest->path);
                    }
                    else if(strcmp(httpRequest->method, "HEAD") == 0)
                    {
                        head_req_response(clients[i].fd, httpRequest->path);
                    }
                    else
                    {
                        // default err handling
                        perror("Unknown method type");
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    return 1;
}

/**
 * Function to close the given server socket
 * @param activeServer server socket fd
 * @return 0 if success
 */
int server_close(struct serverInformation activeServer)
{
    if(close(activeServer.fd) == -1)
    {
        perror("close failed");
        exit(EXIT_FAILURE);
    }
    printf("Closing the server.\n");
    return 0;
}

/**
 * Function to close a client socket
 * @param activeClient client socket fd
 * @return 0 if success
 */
int client_close(int activeClient)
{
    if(close(activeClient) == -1)
    {
        perror("close failed");
        exit(EXIT_FAILURE);
    }
    printf("Closing the client.\n");
    return 0;
}

/**
 * Function to check if a filePath is the root. If it is the root, then it will
 * change the verified_path to a default value
 * @param filePath path of the resource
 * @param verified_path the file path of either ./html/index.html or a valid
 * request
 * @return 0 if default, 1 if request
 */
// todo move to server.h, remove static
static int checkIfRoot(const char *filePath, char *verified_path)
{
    if(strcmp(filePath, "../data/") == 0)
    {
        printf("Requesting default file path...");
        strcpy(verified_path, "index.html");
    }
    else
    {
        strcpy(verified_path, filePath);
        return 1;
    }
    return 0;
}

/**
 * Function to construct the response and send to client socket from a given
 * resource Only called when a resource is confirmed to exist
 * @param client_socket client to send the response to
 * @param content content of the resource requested
 * @return 0 if success
 */
// todo add status codes and handle each situation based on that
int send_response_resource(int client_socket, const char *content, size_t content_length)
{
    char   response[MAX_BUFFER_SIZE];
    size_t total_sent = 0;
    snprintf(response, MAX_BUFFER_SIZE, "HTTP/1.1 200 OK\r\nContent-Length: %zu\r\n\r\n", content_length);

    // send the response header
    if(send(client_socket, response, strlen(response), 0) == -1)
    {
        perror("Error sending response header");
        return -1;
    }

    // send content
    while(total_sent < content_length)
    {
        ssize_t sent = send(client_socket, content + total_sent, content_length - total_sent, 0);
        if(sent == -1)
        {
            perror("Error sending content");
            return -1;
        }
        total_sent += (size_t)sent;
    }
    return 0;
}

/**
 * Function to construct and send the head request response
 * @param client_socket the client that will get the response
 * @param content_length the length of the content in the requested resource
 * @return 0 if success
 */ // todo <-- additional status codes
int send_response_head(int client_socket, size_t content_length)
{
    char response[MAX_BUFFER_SIZE];
    snprintf(response, MAX_BUFFER_SIZE, "HTTP/1.1 200 OK\r\nContent-Length: %zu\r\n\r\n", content_length);

    // Send the response header
    if(send(client_socket, response, strlen(response), 0) == -1)
    {
        perror("Error sending response header");
        return -1;
    }

    return 0;
}

/**
 * Function to read the resource, save the resource into a malloc, send the
 * resource back to the client
 * @param client_socket client socket that sends the req
 * @return 0 if success
 */
// todo break down into smaller functions
int get_req_response(int client_socket, const char *filePath)
{
    char  *filePathWithDot;
    FILE  *resource_file;
    long   totalBytesRead;
    char  *file_content;
    char   verified_path[MAX_BUFFER_SIZE];
    size_t bytesRead;

    // todo shift all get/head functions into a single function to port to both
    // check if filePath is root
    checkIfRoot(filePath, verified_path);

    // append "./" to filePathWithDot
    filePathWithDot = addCharacterToStart(verified_path, "../data");
    if(filePathWithDot == NULL)
    {
        perror(". character not added");
        return -1;
    }

    // open file
    resource_file = fopen(filePathWithDot, "rbe");
    if(resource_file == NULL)
    {
        fprintf(stderr, "Error opening resource file: %s\n", filePathWithDot);
        free(filePathWithDot);
        return -1;
    }
    free(filePathWithDot);

    // move cursor to the end of the file, read the position in bytes, reset
    // cursor
    fseek(resource_file, 0, SEEK_END);
    totalBytesRead = ftell(resource_file);
    fseek(resource_file, 0, SEEK_SET);

    // allocate memory
    file_content = (char *)malloc((unsigned long)(totalBytesRead + 1));
    if(file_content == NULL)
    {
        perror("Error allocating memory");
        fclose(resource_file);
        return -1;
    }

    // read into buffer
    bytesRead = fread(file_content, 1, (unsigned long)totalBytesRead, resource_file);
    if((long)bytesRead != totalBytesRead)
    {
        perror("Error reading HTML file");
        free(file_content);
        fclose(resource_file);
        return -1;
    }
    fclose(resource_file);

    // create response and send
    if(send_response_resource(client_socket, file_content, bytesRead) == -1)
    {
        free(file_content);
        return -1;
    }

    // free resources on success
    free(file_content);
    return 0;
}

/**
 * Function to handle a HEAD request and send back the header
 * @return 0 if success
 */
int head_req_response(int client_socket, const char *filePath)
{
    /*
     * Steps:
     * Check if root
     * filePathWithDot
     * open file (check if exists)
     * close file
     * send response based on this
     */
    char *filePathWithDot;
    FILE *resource_file;
    char  verified_path[MAX_BUFFER_SIZE];
    long  totalBytesRead;

    // check if filePath is root
    checkIfRoot(filePath, verified_path);

    // append "." to filePathWithDot
    filePathWithDot = addCharacterToStart(verified_path, "../data");
    if(filePathWithDot == NULL)
    {
        perror(". character not added");
        return -1;
    }

    // open file
    resource_file = fopen(filePathWithDot, "rbe");
    free(filePathWithDot);
    if(resource_file == NULL)
    {
        perror("Error opening resource file");
        return -1;
    }

    fseek(resource_file, 0, SEEK_END);
    totalBytesRead = ftell(resource_file);
    fseek(resource_file, 0, SEEK_SET);

    // send header
    send_response_head(client_socket, (size_t)totalBytesRead);

    // close file
    fclose(resource_file);

    return 0;
}
