#include "../include/message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BER_MAX_PAYLOAD_SIZE 1024

static int parse_ber_message(int client_sockfd, struct MessageHeader *msg_header, uint8_t *payload);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

// Modify the handle_connection function to process incoming BER messages
void handle_connection(int client_sockfd, struct sockaddr_storage *client_addr)
{
    struct MessageHeader msg_header;
    uint8_t              payload[BER_MAX_PAYLOAD_SIZE];    // Assuming payload doesn't exceed 1024 bytes

    // Parse the BER message
    if(parse_ber_message(client_sockfd, &msg_header, payload) == 0)
    {
        // Successfully parsed the BER message, print the information
        printf("Received BER message:\n");
        printf("Packet Type: %u\n", msg_header.packetType);
        printf("Version: %u\n", msg_header.version);
        printf("Sender ID: %u\n", msg_header.senderId);
        printf("Payload Length: %u\n", msg_header.payloadLength);

        // Print the payload if it exists
        if(msg_header.payloadLength > 0)
        {
            printf("Payload: ");
            for(uint16_t i = 0; i < msg_header.payloadLength; i++)
            {
                printf("%02x ", payload[i]);
            }
            printf("\n");
        }
    }
    else
    {
        fprintf(stderr, "Failed to parse BER message\n");
    }
}

#pragma GCC diagnostic pop

// BER Decoder function
static int parse_ber_message(int client_sockfd, struct MessageHeader *msg_header, uint8_t *payload)
{
    ssize_t bytes_received;
    uint8_t buffer[sizeof(struct MessageHeader) + BER_MAX_PAYLOAD_SIZE];

    // Read the message from the client
    bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0);
    if(bytes_received < 0)
    {
        perror("Error reading from socket");
        return -1;
    }

    // Assuming the message contains a header followed by the payload
    if(sizeof(bytes_received) < sizeof(struct MessageHeader))
    {
        fprintf(stderr, "Invalid message size\n");
        return -1;
    }

    // Parse the message header
    memcpy(msg_header, buffer, sizeof(struct MessageHeader));

    // Parse the payload (if any)
    if(msg_header->payloadLength > 0 && msg_header->payloadLength <= BER_MAX_PAYLOAD_SIZE)
    {
        memcpy(payload, buffer + sizeof(struct MessageHeader), msg_header->payloadLength);
    }
    else
    {
        fprintf(stderr, "Invalid payload length\n");
        return -1;
    }

    return 0;
}
