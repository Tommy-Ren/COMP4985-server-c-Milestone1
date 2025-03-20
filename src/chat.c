#include "../include/chat.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ssize_t chat_handler(message_t *message)
{
    const char *timestamp;
    const char *content;
    const char *username;
    uint8_t     timestamp_len;
    uint8_t     content_len;
    uint8_t     user_len;
    char       *ptr;

    uint16_t sender_id = SYSID;

    ptr = (char *)message->res_buf;
    // tag
    *ptr++ = SYS_SUCCESS;
    // version
    *ptr++ = VERSION_NUM;
    // sender_id
    sender_id = htons(sender_id);
    memcpy(ptr, &sender_id, sizeof(sender_id));
    ptr += sizeof(sender_id);
    // payload_len
    message->response_len = htons(message->response_len);
    memcpy(ptr, &message->response_len, sizeof(message->response_len));
    ptr += sizeof(message->response_len);
    // payload CHOICE
    *ptr++ = BER_ENUM;
    *ptr++ = sizeof(uint8_t);
    *ptr++ = CHT_SEND;

    message->response_len = (uint16_t)(HEADERLEN + ntohs(message->response_len));
    printf("response_len: %d\n", (message->response_len));

    // ACK
    write(message->client_fd, message->res_buf, message->response_len);
    // Timestamp
    ptr = (char *)message->req_buf + HEADERLEN + 1;
    memcpy(&timestamp_len, ptr, sizeof(timestamp_len));
    ptr += sizeof(timestamp_len);
    timestamp = ptr;
    // Content len
    ptr += timestamp_len + 1;
    memcpy(&content_len, ptr, sizeof(content_len));
    // Content
    ptr += sizeof(content_len);
    content = ptr;
    // Username len
    ptr += content_len + 1;
    memcpy(&user_len, ptr, sizeof(user_len));
    // Username
    ptr += sizeof(user_len);
    username = ptr;
    // Response
    message->response_len = (uint16_t)(HEADERLEN + message->payload_len);
    memcpy(message->res_buf, message->req_buf, message->response_len);

    // DEBUG
    printf("Timestamp: %.*s\n", (int)timestamp_len, timestamp);
    printf("Chat message: %.*s\n", (int)content_len, content);
    printf("Username: %.*s\n", (int)user_len, username);
    printf("Response message: %.*s\n", (int)(message->response_len - HEADERLEN), message->res_buf + HEADERLEN);

    for(int i = 1; i < MAX_FDS; i++)
    {
        if(message->fds[i].fd != -1)
        {
            write(message->fds[i].fd, message->res_buf, message->response_len);
        }
    }

    message->response_len = 0;

    return 0;
}
