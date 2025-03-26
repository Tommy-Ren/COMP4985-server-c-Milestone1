#include "../include/message.h"
#include "../include/account.h"
#include "../include/chat.h"
#include "../include/network.h"
#include "../include/user_db.h"
#include "../include/utils.h"
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int user_count = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors)
static int msg_count  = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors)
static int user_index = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors)

static void handle_sm_diagnostic(char *msg);
/* Declaration for static functions */
static ssize_t handle_request(int client_fd, message_t *message);
static ssize_t handle_package(message_t *message);
static ssize_t handle_header(message_t *message, ssize_t nread);
static ssize_t handle_payload(message_t *message, ssize_t nread);
// static ssize_t     send_response(message_t *message);
static void        send_sm_response(int sm_fd, const char *msg);
static ssize_t     send_error_response(message_t *message);
static const char *error_code_to_string(const error_code_t *code);
static void        count_user(const int *client_id);

/* Error code map */
static const error_code_map code_map[] = {
    {EC_GOOD,          ""                      },
    {EC_INV_USER_ID,   "Invalid User ID"       },
    {EC_INV_AUTH_INFO, "Invalid Authentication"},
    {EC_USER_EXISTS,   "User Already Exist"    },
    {EC_SERVER,        "Server Error"          },
    {EC_INV_REQ,       "Invalid message"       },
    {EC_REQ_TIMEOUT,   "message Timeout"       }
};

void handle_connections(int server_fd, int sm_fd)
{
    /* Use the global server_running variable declared in utils.h */
    struct pollfd fds[MAX_FDS];
    int           client_id[MAX_FDS];
    char          db_name[] = "meta_db";
    DBO           meta_db;
    int           poll_count;
    message_t     message;
    char          sm_msg[MESSAGE_NUM];
    int           i;

    // Initialize fds
    fds[0].fd     = server_fd;
    fds[0].events = POLLIN;
    for(i = 1; i < MAX_FDS; i++)
    {
        fds[i].fd    = -1;
        client_id[i] = -1;
    }

    // Initialize meta database
    meta_db.name = db_name;
    if(init_pk(&meta_db, "USER_PK", &user_count) < 0)
    {
        perror("Failed to initialize meta_db\n");
        goto exit;
    }
    if(database_open(&meta_db) < 0)
    {
        perror("Failed to open meta_db");
        goto exit;
    }

    handle_sm_diagnostic(sm_msg);

    /* Global server_running will be used here */
    while(server_running)
    {
        errno      = 0;
        poll_count = poll(fds, MAX_FDS, TIMEOUT);

        // Poll for events on the file descriptors
        if(poll_count < 0)
        {
            if(errno == EINTR)
            {
                goto exit;
            }
            perror("poll error");
            goto exit;
        }
        // On poll timeout, update user count and send diagnostics if connected to server manager.
        if(poll_count == 0)
        {
            printf("poll timeout\n");
            if(store_int(meta_db.db, "USER_PK", user_index) != 0)
            {
                perror("update user_index");
                goto exit;
            }
            count_user(client_id);
            if(sm_fd >= 0)    // Only send diagnostic update if connected to the server manager.
            {
                send_sm_response(sm_fd, sm_msg);
            }
            continue;
        }
        // Check for new client connections
        if(fds[0].revents & POLLIN)
        {
            int                     client_added;
            int                     client_fd;
            struct sockaddr_storage client_addr;
            socklen_t               client_addr_len = sizeof(client_addr);

            client_added = 0;

            client_fd = socket_accept(server_fd, &client_addr, &client_addr_len);
            if(client_fd < 0)
            {
                if(errno == EINTR)
                {
                    server_running = 0;
                    goto exit;
                }
                perror("accept error");
                continue;
            }

            for(i = 1; i < MAX_FDS; i++)
            {
                if(fds[i].fd == -1)
                {
                    fds[i].fd     = client_fd;
                    fds[i].events = POLLIN;
                    client_added  = 1;
                    break;
                }
            }

            if(!client_added)
            {
                printf("Too many clients connected. Rejecting connection.\n");
                close(client_fd);
                continue;
            }
        }

        // Handle incoming data on existing client connections.
        for(i = 1; i < MAX_FDS; i++)
        {
            if(fds[i].fd == -1)
            {
                continue;
            }
            if(fds[i].revents & POLLIN)
            {
                message.fds        = fds;
                message.client_id  = &client_id[i];
                message.user_count = &user_count;
                if(handle_request(fds[i].fd, &message) < 0)
                {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                }
            }
        }
    }

    // Sync the user database
    if(store_int(meta_db.db, "USER_PK", user_index) != 0)
    {
        perror("Failed to sync user database");
        goto exit;
    }
    dbm_close(meta_db.db);

exit:
    store_int(meta_db.db, "USER_PK", user_index);
    dbm_close(meta_db.db);
}

static ssize_t handle_request(int client_fd, message_t *message)
{
    ssize_t retval;
    message->client_fd    = client_fd;
    message->payload_len  = HEADERLEN;
    message->response_len = U8ENCODELEN;
    message->code         = EC_GOOD;

    /* Allocate the request buffer */
    message->req_buf = malloc(HEADERLEN);
    if(!message->req_buf)
    {
        perror("Failed to allocate message request buffer");
        retval = -1;
        goto exit;
    }

    /* Allocate the response buffer */
    message->res_buf = malloc(RESPONSELEN);
    if(!message->res_buf)
    {
        perror("Failed to allocate message response buffer");
        retval = -1;
        goto exit;
    }
    memset(message->res_buf, 0, RESPONSELEN);

    if(handle_package(message) < 0)
    {
        perror("Failed to read packet");
        retval = -1;
        goto exit;
    }

    retval = EXIT_SUCCESS;
exit:
    sfree(&message->req_buf);
    sfree(&message->res_buf);
    return retval;
}

/* BER Decoder function */
static ssize_t handle_package(message_t *message)
{
    ssize_t retval;
    ssize_t nread;
    void   *tmp;

    nread = read(message->client_fd, (char *)message->req_buf, message->payload_len);
    if(nread < 0)
    {
        perror("Fail to read header");
        message->code = EC_SERVER;
        return -1;
    }

    if(handle_header(message, nread) < 0)
    {
        perror("Failed to decode header");
        return -1;
    }

    // Reallocate buffer to fit the payload
    tmp = realloc(message->req_buf, message->payload_len + HEADERLEN);
    if(!tmp)
    {
        perror("Failed to reallocate message body\n");
        free(message->req_buf);
        message->code = EC_SERVER;
        return -1;
    }
    message->req_buf = tmp;

    // Read payload_length into buffer
    nread = read(message->client_fd, ((char *)message->req_buf) + HEADERLEN, message->payload_len);
    if(nread < 0)
    {
        perror("Failed to read message body\n");
        message->code = EC_SERVER;
        return -1;
    }

    retval = handle_payload(message, nread);
    if(retval == ACCOUNT_ERROR)
    {
        perror("Failed to identify account package\n");
        return ACCOUNT_ERROR;
    }
    if(retval == ACCOUNT_LOGIN_ERROR)
    {
        perror("Failed to login\n");
        return ACCOUNT_LOGIN_ERROR;
    }
    if(retval == ACCOUNT_CREATE_ERROR)
    {
        perror("Failed to create account\n");
        return ACCOUNT_CREATE_ERROR;
    }
    if(retval == ACCOUNT_EDIT_ERROR)
    {
        perror("Failed to edit account\n");
        return ACCOUNT_EDIT_ERROR;
    }
    if(retval == CHAT_ERROR)
    {
        perror("Chat error\n");
        return CHAT_ERROR;
    }

    return 0;
}

static ssize_t handle_header(message_t *message, ssize_t nread)
{
    char *buf;

    buf = (char *)message->req_buf;
    if(nread < (ssize_t)sizeof(message->payload_len))
    {
        perror("Payload length mismatch\n");
        message->code = EC_INV_REQ;
        return -1;
    }

    memcpy(&message->type, buf, sizeof(message->type));
    buf += sizeof(message->type);

    memcpy(&message->version, buf, sizeof(message->version));
    buf += sizeof(message->version);

    memcpy(&message->sender_id, buf, sizeof(message->sender_id));
    buf += sizeof(message->sender_id);
    message->sender_id = ntohs(message->sender_id);

    memcpy(&message->payload_len, buf, sizeof(message->payload_len));
    message->payload_len = ntohs(message->payload_len);

    // DEBUG
    printf("Header type: %d\n", (int)message->type);
    printf("Header version: %d\n", (int)message->version);
    printf("Header sender_id: %d\n", (int)message->sender_id);
    printf("Header payload_len: %d\n", (int)message->payload_len);

    return 0;
}

static ssize_t handle_payload(message_t *message, ssize_t nread)
{
    ssize_t retval;

    if(nread < (ssize_t)message->payload_len)
    {
        printf("Payload length mismatch");
        message->code = EC_INV_REQ;
        return ACCOUNT_ERROR;
    }

    switch(message->type)
    {
        case ACC_LOGIN:
        case ACC_CREATE:
        case ACC_EDIT:
        case ACC_LOGOUT:
            retval = account_handler(message);
            if(retval < 0)
            {
                send_error_response(message);
                // For login failure, do not treat it as fatal so that the connection remains open.
                if(message->type == ACC_LOGIN)
                {
                    return 0;
                }
                return retval;
            }
            break;

        case CHT_SEND:
            retval = chat_handler(message);
            if(retval < 0)
            {
                send_error_response(message);
                return retval;
            }
            break;

        default:
            message->code = EC_INV_REQ;
            send_error_response(message);
            return ACCOUNT_ERROR;
    }

    return 0;
}

// static ssize_t send_response(message_t *message)
// {
//     if(message->type != CHT_SEND)
//     {
//         message->response_len = (uint16_t)(HEADERLEN + ntohs(message->response_len));
//         printf("response_len: %d\n", (message->response_len));
//         write(message->client_fd, message->res_buf, message->response_len);
//     }

//     sfree(message->req_buf);
//     close(message->client_fd);
//     message->client_fd = -1;

//     return 0;
// }

static void count_user(const int *client_id)
{
    user_count = 0;
    for(int i = 1; i < MAX_FDS; i++)
    {
        printf("user id: %d\n", client_id[i]);
        if(client_id[i] != -1)
        {
            user_count++;
        }
    }
    printf("Current number of users: %d\n", user_count);
}

static void handle_sm_diagnostic(char *msg)
{
    char    *ptr = msg;
    uint16_t net_user_count;
    uint32_t net_msg_count;
    uint16_t msg_payload_len = htons(DIAGNOSTIC_PAYLOAD_LEN);

    *ptr++ = SVR_DIAGNOSTIC;
    *ptr++ = VERSION_NUM;
    memcpy(ptr, &msg_payload_len, sizeof(msg_payload_len));
    ptr += sizeof(msg_payload_len);

    *ptr++         = BER_INT;
    *ptr++         = sizeof(user_count);    // This is the length field in the protocol (size in bytes).
    net_user_count = htons((uint16_t)user_count);
    memcpy(ptr, &net_user_count, sizeof(net_user_count));    // Use size of net_user_count (2 bytes).
    ptr += sizeof(net_user_count);

    *ptr++        = BER_INT;
    *ptr++        = sizeof(msg_count);    // Length field as per protocol.
    net_msg_count = htonl((uint32_t)msg_count);
    memcpy(ptr, &net_msg_count, sizeof(net_msg_count));    // Use size of net_msg_count (typically 4 bytes).
}

static void send_sm_response(int sm_fd, const char *msg)
{
    // Create a local copy of the message buffer.
    char       local_msg[MESSAGE_LEN];
    uint16_t   net_user_count;
    uint32_t   net_msg_count;
    char      *ptr;
    int        fd;
    const char log[] = "./text.txt";

    memcpy(local_msg, msg, MESSAGE_LEN);
    ptr = local_msg;
    // Advance pointer past header.
    ptr += HEADERLEN;
    // Convert user_count to network order (2 bytes).
    net_user_count = htons((uint16_t)user_count);
    memcpy(ptr, &net_user_count, sizeof(net_user_count));    // Use size of net_user_count (2 bytes).
    // Advance pointer by 2 bytes plus protocol padding (here +1+1 bytes).
    ptr += sizeof(net_user_count) + 1 + 1;

    // Convert message count to network order (4 bytes).
    net_msg_count = htonl((uint32_t)msg_count);
    memcpy(ptr, &net_msg_count, sizeof(net_msg_count));    // Use size of net_msg_count (4 bytes).

    printf("send_user_count\n");

    // Write to a log file.
    fd = open(log, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, S_IRUSR | S_IWUSR);
    if(fd == -1)
    {
        perror("Failed to open the file");
    }
    else
    {
        write(fd, local_msg, MESSAGE_LEN);
        close(fd);
    }

    // Send the constructed message to the server manager.
    if(write(sm_fd, local_msg, MESSAGE_LEN) < 0)
    {
        perror("Failed to send user count");
    }
}

// static ssize_t send_error_response(message_t *message)
// {
//     char    *ptr;
//     uint16_t sender_id;
//     uint8_t  msg_len;

//     sender_id = SYSID;
//     ptr       = (char *)message->res_buf;

//     // If not logout, send error response
//     if(message->type != ACC_LOGOUT)
//     {
//         const char *msg;
//         // tag
//         *ptr++ = SYS_ERROR;
//         // version
//         *ptr++ = VERSION_NUM;
//         // sender_id
//         sender_id = htons(sender_id);
//         memcpy(ptr, &sender_id, sizeof(sender_id));
//         ptr += sizeof(sender_id);
//         msg                   = error_code_to_string(&message->code);
//         msg_len               = (uint8_t)strlen(msg);
//         message->response_len = (uint16_t)(message->response_len + (sizeof(uint8_t) + sizeof(uint8_t) + msg_len));
//         // payload len
//         message->response_len = htons(message->response_len);
//         memcpy(ptr, &message->response_len, sizeof(message->response_len));
//         ptr += sizeof(message->response_len);
//         // payload CHOICE
//         *ptr++ = BER_INT;
//         *ptr++ = sizeof(uint8_t);
//         memcpy(ptr, &message->code, sizeof(uint8_t));
//         ptr += sizeof(uint8_t);
//         *ptr++ = BER_STR;
//         memcpy(ptr, &msg_len, sizeof(msg_len));
//         ptr += sizeof(msg_len);
//         // response_len
//         memcpy(ptr, msg, msg_len);
//         message->response_len = (uint16_t)(HEADERLEN + ntohs(message->response_len));
//     }

//     if(write(message->client_fd, message->res_buf, message->response_len) < 0)
//     {
//         perror("Failed to send error response\n");
//         sfree((void **)message->req_buf);
//         close(message->client_fd);
//         message->client_fd = -1;
//         return -1;
//     }

//     printf("Response: %s\n", (char *)message->res_buf);
//     sfree((void **)message->req_buf);
//     close(message->client_fd);
//     message->client_fd = -1;
//     return 0;
// }
static ssize_t send_error_response(message_t *message)
{
    char    *ptr;
    uint16_t sender_id;
    uint8_t  msg_len;

    sender_id = SYSID;
    ptr       = (char *)message->res_buf;

    if(message->type != ACC_LOGOUT)
    {
        const char *msg;
        // Build error response header.
        *ptr++    = SYS_ERROR;
        *ptr++    = VERSION_NUM;
        sender_id = htons(sender_id);
        memcpy(ptr, &sender_id, sizeof(sender_id));
        ptr += sizeof(sender_id);
        msg     = error_code_to_string(&message->code);
        msg_len = (uint8_t)strlen(msg);
        // Update response_len to include tag fields and message.
        message->response_len = (uint16_t)(message->response_len + (sizeof(uint8_t) + sizeof(uint8_t) + msg_len));
        message->response_len = htons(message->response_len);
        memcpy(ptr, &message->response_len, sizeof(message->response_len));
        ptr += sizeof(message->response_len);
        // Encode error code and error string.
        *ptr++ = BER_INT;
        *ptr++ = sizeof(uint8_t);
        memcpy(ptr, &message->code, sizeof(uint8_t));
        ptr += sizeof(uint8_t);
        *ptr++ = BER_STR;
        memcpy(ptr, &msg_len, sizeof(msg_len));
        ptr += sizeof(msg_len);
        memcpy(ptr, msg, msg_len);
        // Calculate the final response length.
        message->response_len = (uint16_t)(HEADERLEN + ntohs(message->response_len));
    }

    if(write(message->client_fd, message->res_buf, message->response_len) < 0)
    {
        perror("Failed to send error response");
        close(message->client_fd);
        message->client_fd = -1;
        return -1;
    }

    printf("Response: %s\n", (char *)message->res_buf);
    if(message->type != ACC_LOGIN)
    {
        close(message->client_fd);
        message->client_fd = -1;
    }
    return 0;
}

static const char *error_code_to_string(const error_code_t *code)
{
    size_t i;
    for(i = 0; i < sizeof(code_map) / sizeof(code_map[0]); i++)
    {
        if(code_map[i].code == *code)
        {
            return code_map[i].msg;
        }
    }
    return UNKNOWNTYPE;
}
