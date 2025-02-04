#include <arpa/inet.h>    // Provides functions for manipulating IP addresses
#include <errno.h>        // Provides error handling macros
#include <limits.h>       // Defines limits for integer types
#include <stdint.h>       // Provides fixed-width integer types
#include <stdio.h>        // Standard I/O functions
#include <stdlib.h>       // Provides memory allocation and conversion functions
#include <string.h>       // Provides string manipulation functions
#include <unistd.h>       // Provides close() function

// Define constants for message properties and network limits
#define PAYLOAD_SIZE 1024    // Maximum payload size
#define SENDER_ID 1234       // Unique identifier for sender
#define PAYLOAD_LENGTH 5     // Length of the payload being sent
#define BYTE_1 0xDE          // Sample payload data
#define BYTE_2 0xAD
#define BYTE_3 0xBE
#define BYTE_4 0xEF
#define BYTE_5 0x00
#define MAX_PORT 65535    // Maximum valid port number
#define THREE 3           // Expected argument count (program name, IP, and port)
#define BASE_TEN 10       // Base for number conversion
#define TWO 2             // Index for port argument in argv

// Structure defining the message header format
struct MessageHeader
{
    uint8_t  packetType;       // Type of packet
    uint8_t  version;          // Protocol version
    uint16_t senderId;         // ID of the sender
    uint16_t payloadLength;    // Length of the payload in bytes
};

// Function declarations
int            send_ber_message(int sockfd, const struct MessageHeader *msg_header, const uint8_t *payload);
void           parse_arguments(int argc, char *argv[], char **server_ip, int *server_port);
_Noreturn void usage(const char *program_name, int exit_code);

// Function to send a BER message over the network
int send_ber_message(int sockfd, const struct MessageHeader *msg_header, const uint8_t *payload)
{
    ssize_t result;

    // Send the message header first
    result = send(sockfd, msg_header, sizeof(struct MessageHeader), 0);
    if(result == -1)
    {
        perror("send header failed");
        return -1;
    }

    // Send the actual payload data
    result = send(sockfd, payload, msg_header->payloadLength, 0);
    if(result == -1)
    {
        perror("send payload failed");
        return -1;
    }

    return 0;    // Success
}

// Main function: Handles client setup and communication with the server
int main(int argc, char *argv[])
{
    char                *server_ip;                // Stores the server's IP address
    int                  server_port;              // Stores the server's port number
    int                  sockfd;                   // Socket file descriptor
    struct sockaddr_in   server_addr;              // Struct to hold server address details
    struct MessageHeader msg_header;               // Message header struct
    uint8_t              payload[PAYLOAD_SIZE];    // Buffer for payload data
    int                  result;

    // Parse command-line arguments to extract server IP and port
    parse_arguments(argc, argv, &server_ip, &server_port);

    // Create a TCP socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        perror("socket creation failed");
        return -1;
    }

    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;                         // Set address family to IPv4
    server_addr.sin_port   = htons((uint16_t)server_port);    // Convert port number to network byte order

    // Convert and validate server IP address
    result = inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    if(result <= 0)
    {
        perror("Invalid address or address not supported");
        close(sockfd);
        return -1;
    }

    // Establish connection to the server
    result = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if(result == -1)
    {
        perror("connection failed");
        close(sockfd);
        return -1;
    }

    // Prepare the message header
    msg_header.packetType    = 0x01;    // Example packet type
    msg_header.version       = 0x01;    // Protocol version
    msg_header.senderId      = SENDER_ID;
    msg_header.payloadLength = PAYLOAD_LENGTH;

    // Prepare the payload data
    payload[0] = BYTE_1;
    payload[1] = BYTE_2;
    payload[2] = BYTE_3;
    payload[3] = BYTE_4;
    payload[4] = BYTE_5;

    // Send the BER message
    result = send_ber_message(sockfd, &msg_header, payload);
    if(result == -1)
    {
        close(sockfd);
        return -1;
    }

    // Display confirmation message
    printf("Test BER message sent to server at %s:%d\n", server_ip, server_port);

    // Close the socket connection
    close(sockfd);
    return 0;
}

// Function to parse command-line arguments and extract server IP and port
void parse_arguments(int argc, char *argv[], char **server_ip, int *server_port)
{
    char *endptr;
    long  port;

    // Check if the correct number of arguments is provided
    if(argc != THREE)
    {
        usage(argv[0], EXIT_FAILURE);
    }

    *server_ip = argv[1];    // Extract server IP address from command-line arguments

    // Convert port argument from string to integer safely
    errno = 0;
    port  = strtol(argv[TWO], &endptr, BASE_TEN);

    // Validate the port number
    if(errno != 0 || *endptr != '\0' || port < 1 || port > MAX_PORT)
    {
        fprintf(stderr, "Invalid port number: %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    *server_port = (int)port;    // Store the valid port number
}

// Function to display correct usage instructions and exit
_Noreturn void usage(const char *program_name, int exit_code)
{
    fprintf(stderr, "Usage: %s <server_ip> <port>\n", program_name);
    exit(exit_code);
}
