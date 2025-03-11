#include "../include/message.h"
#include "../include/account.h"
#include "../include/user_db.h"
#include "../include/utils.h"
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static ssize_t     read_packet(int fd, uint8_t **buf, request_t *request, response_t *response);
static ssize_t     create_response(const request_t *request, response_t *response, uint8_t **buf, size_t *response_len);
static ssize_t     sent_response(int fd, const uint8_t *buf, const size_t *response_len);
static ssize_t     decode_header(header_t *header, response_t *response, const uint8_t *buf, ssize_t nread);
static ssize_t     decode_payload(request_t *request, response_t *response, const uint8_t *buf, ssize_t nread);
static ssize_t     encode_header(const response_t *response, uint8_t *buf);
static ssize_t     encode_body(const response_t *response, uint8_t *buf);
static const char *error_code_to_string(const error_code_t *code);

static const error_code_map code_map[] = {
    {EC_GOOD,          ""                      },
    {EC_INV_USER_ID,   "Invalid User ID"       },
    {EC_INV_AUTH_INFO, "Invalid Authentication"},
    {EC_USER_EXISTS,   "User Already Exist"    },
    {EC_SERVER,        "Server Error"          },
    {EC_INV_REQ,       "Invalid Request"       },
    {EC_REQ_TIMEOUT,   "Request Timeout"       }
};

void handle_clients(int server_fd)
{
    struct pollfd fds[MAX_FDS];
    int           current_size;
    int           nfds;
    int           server_running = 1;

    if(init_user_list() < 0)
    {
        goto exit;
    }

    memset(fds, 0, sizeof(fds));
    fds[0].fd     = server_fd;
    fds[0].events = POLLIN;

    while(server_running)
    {
        int poll_count = poll(fds, nfds, -1);
        if(poll_count < 0)
        {
            perror("poll error");
            break;
        }

        current_size = nfds;
        for(int i = 0; i < current_size; i++)
        {
            if(fds[i].revents == 0)
                continue;

            if(fds[i].revents != POLLIN)
            {
                printf("Error! revents = %d\n", fds[i].revents);
                server_running = 0;
                break;
            }

            if(fds[i].fd == server_fd)
            {
                int                     client_fd;
                struct sockaddr_storage client_addr;
                socklen_t               client_addr_len = sizeof(client_addr);

                client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
                if(client_fd < 0)
                {
                    perror("accept error");
                    server_running = 0;
                    break;
                }

                printf("New incoming connection on fd %d\n", client_fd);
                fds[nfds].fd     = client_fd;
                fds[nfds].events = POLLIN;
                nfds++;
            }
            else
            {
                int client_fd = fds[i].fd;
                if(process_req(client_fd) < 0)
                {
                    close(client_fd);
                    fds[i].fd = -1;
                }
            }
        }

        for(int i = 0; i < nfds; i++)
        {
            if(fds[i].fd == -1)
            {
                for(int j = i; j < nfds - 1; j++)
                {
                    fds[j].fd = fds[j + 1].fd;
                }
                nfds--;
                i--;
            }
        }
    }

exit:
    dbm_close(user_db);
}

int process_req(int client_fd)
{
    int          retval;
    request_t    request;
    header_t     req_header;
    response_t   response;
    header_t     res_header;
    uint8_t     *res_buf;
    size_t       response_len;
    uint8_t     *buf;
    error_code_t code;

    retval                       = EXIT_SUCCESS;
    request.header               = &req_header;
    request.header_len           = sizeof(header_t);
    response.header              = &res_header;
    response.header->payload_len = 3;
    response.code                = &code;
    response_len                 = sizeof(header_t);
    code                         = EC_GOOD;

    request.body = (body_t *)malloc(sizeof(body_t));
    if(!request.body)
    {
        perror("Failed to allocate request body");
        retval = -1;
        goto exit;
    }

    response.body = (res_body_t *)malloc(sizeof(res_body_t));
    if(!response.body)
    {
        perror("Failed to allocate response body");
        retval = -1;
        goto exit;
    }

    buf = (uint8_t *)calloc(request.header_len, sizeof(uint8_t));
    if(!buf)
    {
        perror("Failed to allocate buf");
        retval = -1;
        goto exit;
    }

    if(read_packet(client_fd, &buf, &request, &response) < 0)
    {
        perror("Failed to read packet");
        retval = -1;
        goto exit;
    }

    account_handler(&request, &response);

    if(create_response(&request, &response, &res_buf, &response_len) < 0)
    {
        perror("Failed to create response");
        retval = -1;
        goto exit;
    }

    send_response(client_fd, res_buf, &response_len);
    retval = EXIT_SUCCESS;

exit:
    if(res_buf != NULL)
    {
        free(res_buf);
    }
    if(buf != NULL)
    {
        free(buf);
    }
    if(response.body != NULL)
    {
        free(response.body);
    }
    if(request.body != NULL)
    {
        free(request.body);
    }
    return retval;
}

// BER Decoder function
static ssize_t read_packet(int fd, uint8_t **buf, request_t *request, response_t *response)
{
    size_t   header_len = request->header_len;
    uint16_t payload_len;
    ssize_t  nread;

    // Read 6 bytes from fd (header should be 6 bytes)
    nread = read(fd, *buf, header_len);
    if(nread < 0)
    {
        perror("Fail to read header");
        *(response->code) = EC_SERVER;
        return ERROR_READ_HEADER;
    }
    // Decode and check header completeness
    if(decode_header(request->header, response, *buf, nread) < 0)
    {
        perror("Failed to decode header");
        return ERROR_DECODE_HEADER;
    }

    payload_len = request->header->payload_len;
    if(!payload_len)
    {
        return 0;
    }

    // Reallocate buffer to fit the payload
    *buf = (uint8_t *)realloc(*buf, HEADERLEN + payload_len);
    if(*buf == NULL)
    {
        perror("Failed to reallocate buffer for payload");
        *(response->code) = EC_SERVER;
        return ERROR_REALLOCATE;
    }

    // Read payload_length into buffer
    nread = read(fd, *buf + header_len, payload_len);
    if(nread < 0)
    {
        perror("Failed to read payload");
        *(response->code) = EC_SERVER;
        return ERROR_READ_PAYLOAD;
    }

    if(decode_payload(request, response, *buf + header_len, nread) < 0)
    {
        perror("Failed to decode payload");
        return ERROR_DECODE_PAYLOAD;
    }

    return 0;
}

static ssize_t create_response(const request_t *request, response_t *response, uint8_t **buf, size_t *response_len)
{
    uint8_t    *temp;
    const char *msg;

    *response_len = (size_t)(request->header_len + response->header->payload_len);
    printf("response_len: %d\n", (int)*response_len);

    *buf = (uint8_t *)malloc(*response_len);
    if(*buf == NULL)
    {
        perror("Failed to allocate response buffer");
        *(response->code) = EC_SERVER;
        return -1;
    }

    msg = error_code_to_string(response->code);

    // tag
    response->body->msg_tag = (uint8_t)BER_STR;
    // len
    response->body->msg_len = (uint8_t)(strlen(msg));
    printf("msg_len: %d\n", (int)response->body->msg_len);
    // msg

    *response_len = *response_len + response->body->msg_len + 2;
    printf("response_len final: %d\n", (int)*response_len);

    if(*response->code == EC_GOOD)
    {
        response->header->type      = ACC_LOGIN_SUCCESS;
        response->header->version   = VERSION_NUM;
        response->header->sender_id = 0x00;
        // logout has no response body
        if(request->header->type == ACC_LOGOUT)
        {
            response->header->payload_len = 0;
            *response_len                 = *response_len - 2 - 3;
        }
        else
        {
            // no msg tag & msg len
            response->header->payload_len = (uint16_t)3;
            *response_len -= 2;
        }
    }
    else if(*response->code == EC_INV_AUTH_INFO)
    {
        printf("*response->code == INVALID_AUTH\n");

        response->header->type        = SYS_ERROR;
        response->header->version     = VERSION_NUM;
        response->header->sender_id   = 0x00;
        response->header->payload_len = (uint16_t)(3 + response->body->msg_len + 2);

        temp = (uint8_t *)realloc(*buf, *response_len);
        if(temp == NULL)
        {
            perror("read_packet::realloc");
            *(response->code) = EC_SERVER;
            return -4;
        }
        *buf = temp;
    }
    else
    {
        printf("*response->code == else \n");
        response->header->type        = SYS_ERROR;
        response->header->version     = VERSION_NUM;
        response->header->sender_id   = 0x00;
        response->header->payload_len = (uint16_t)(3 + response->body->msg_len + 2);

        temp = (uint8_t *)realloc(*buf, *response_len);
        if(temp == NULL)
        {
            perror("read_packet::realloc");
            *(response->code) = EC_SERVER;
            return -4;
        }
        *buf = temp;
    }

    encode_header(response, *buf);
    encode_body(response, *buf + sizeof(header_t));
    return 0;
}

ssize_t decode_header(header_t *header, response_t *response, const uint8_t *buf, ssize_t nread)
{
    size_t pos;

    pos = 0;
    if(nread < (ssize_t)sizeof(header_t))
    {
        perror("Payload length mismatch");
        *(response->code) = EC_INV_REQ;
        return -1;
    }

    memcpy(&header->type, buf + pos, sizeof(header->type));
    pos += sizeof(header->type);

    memcpy(&header->version, buf + pos, sizeof(header->version));
    pos += sizeof(header->version);

    memcpy(&header->sender_id, buf + pos, sizeof(header->sender_id));
    pos += sizeof(header->sender_id);
    header->sender_id = ntohs(header->sender_id);

    memcpy(&header->payload_len, buf + pos, sizeof(header->payload_len));
    header->payload_len = ntohs(header->payload_len);

    // Just for debugging
    printf("Header type: %d\n", (int)header->type);
    printf("Header version: %d\n", (int)header->version);
    printf("Header sender_id: %d\n", (int)header->sender_id);
    printf("Header payload_len: %d\n", (int)header->payload_len);

    return 0;
}

static ssize_t decode_payload(request_t *request, response_t *response, const uint8_t *buf, ssize_t nread)
{
    if(nread < (ssize_t)request->header->payload_len)
    {
        printf("Payload length mismatch");
        *(response->code) = EC_INV_REQ;
        return -1;
    }

    if(request->header->type == ACC_CREATE || request->header->type == ACC_LOGIN)
    {
        size_t     pos;
        account_t *acc;

        acc = (account_t *)malloc(sizeof(account_t));
        if(!acc)
        {
            perror("Failed to allocate account_t");
            *(response->code) = EC_SERVER;
            return -1;
        }
        memset(acc, 0, sizeof(account_t));

        pos = 0;

        memcpy(&acc->username_tag, buf + pos, sizeof(acc->username_tag));
        pos += sizeof(acc->username_tag);

        printf("username tag: %d\n", (int)acc->username_tag);

        // Decode username length
        memcpy(&acc->username_len, buf + pos, sizeof(acc->username_len));
        pos += sizeof(acc->username_len);

        printf("username len: %d\n", (int)acc->username_len);

        // Decode username
        acc->username = (uint8_t *)malloc((size_t)acc->username_len + 1);
        if(!acc->username)
        {
            perror("Failed to allocate username");
            free(acc);
            *(response->code) = EC_SERVER;
            return -1;
        }
        memcpy(acc->username, buf + pos, acc->username_len);
        acc->username[acc->username_len] = '\0';
        pos += acc->username_len;

        printf("username: %s\n", acc->username);

        // Decode password tag
        memcpy(&acc->password_tag, buf + pos, sizeof(acc->password_tag));
        pos += sizeof(acc->password_tag);

        printf("password tag: %d\n", (int)acc->password_tag);

        // Decode password length
        memcpy(&acc->password_len, buf + pos, sizeof(acc->password_len));
        pos += sizeof(acc->password_len);

        printf("password len: %d\n", (int)acc->password_len);

        // Decode password
        acc->password = (uint8_t *)malloc((size_t)acc->password_len + 1);
        if(!acc->password)
        {
            perror("Failed to allocate password");
            free(acc->username);
            free(acc);
            *(response->code) = EC_SERVER;
            return -1;
        }
        memcpy(acc->password, buf + pos, acc->password_len);
        acc->password[acc->password_len] = '\0';

        free(request->body);
        request->body = (body_t *)acc;

        printf("password: %s\n", acc->password);

        return 0;
    }

    printf("Invalid type of request: 0x%X\n", request->header->type);
    *(response->code) = EC_INV_REQ;
    return -1;
}

static ssize_t encode_header(const response_t *response, uint8_t *buf)
{
    size_t pos;

    pos = 0;

    memcpy(buf + pos, &response->header->type, sizeof(response->header->type));
    pos += sizeof(response->header->type);

    memcpy(buf + pos, &response->header->version, sizeof(response->header->version));
    pos += sizeof(response->header->version);

    response->header->sender_id = htons(response->header->sender_id);
    memcpy(buf + pos, &response->header->sender_id, sizeof(response->header->sender_id));
    pos += sizeof(response->header->sender_id);

    response->header->payload_len = htons(response->header->payload_len);
    memcpy(buf + pos, &response->header->payload_len, sizeof(response->header->payload_len));

    return 0;
}

static ssize_t encode_body(const response_t *response, uint8_t *buf)
{
    size_t pos;

    pos = 0;

    memcpy(buf + pos, &response->body->tag, sizeof(response->body->tag));
    pos += sizeof(response->body->tag);

    memcpy(buf + pos, &response->body->len, sizeof(response->body->len));
    pos += sizeof(response->body->len);

    memcpy(buf + pos, &response->body->value, sizeof(response->body->value));
    pos += sizeof(response->body->value);

    response->header->payload_len = htons(response->header->payload_len);

    if(response->header->payload_len > pos)
    {
        memcpy(buf + pos, &response->body->msg_tag, sizeof(response->body->msg_tag));
        pos += sizeof(response->body->msg_tag);

        memcpy(buf + pos, &response->body->msg_len, sizeof(response->body->msg_len));
        pos += sizeof(response->body->msg_len);

        memcpy(buf + pos, error_code_to_string(response->code), response->header->payload_len - pos);
    }

    return 0;
}

static ssize_t sent_response(int fd, const uint8_t *buf, const size_t *response_len)
{
    ssize_t result;

    result = write(fd, buf, *response_len);
    if(result < 0)
    {
        perror("Failed to send response");
        return -1;
    }
    printf("write content: %s\n", buf);
    return 0;
}

static const char *error_code_to_string(const error_code_t *code)
{
    for(size_t i = 0; i < sizeof(code_map) / sizeof(code_map[0]); i++)
    {
        if(code_map[i].code == *code)
        {
            return code_map[i].msg;
        }
    }
    return UNKNOWNTYPE;
}
