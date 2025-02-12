#ifndef message_h
#define message_h

#include <netdb.h>
#include <netinet/in.h>

struct MessageHeader
{
    uint8_t  packetType;
    uint8_t  version;
    uint16_t senderId;
    uint16_t payloadLength;
};

void handle_connection(int client_sockfd, struct sockaddr_storage *client_addr);

#endif
