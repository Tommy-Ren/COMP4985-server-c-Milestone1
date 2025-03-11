#ifndef message_h
#define message_h

#include "../include/user_db.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>

#define SYSID (0)
#define VERSION_NUM (2)    // Updated to Protocol Version 2

#define MAX_FDS (5)
#define TIMEOUT (3000)

#define HEADERLEN (6)
#define TIMESTRLEN (15)
#define PACKETLEN (777)
#define MAXPAYLOADLEN (771)
#define U8ENCODELEN (3)
#define SYS_SUCCESS_LEN (3)
#define ACC_LOGIN_SUCCESS_LEN (4)

#define ERROR_READ_HEADER (-1)
#define ERROR_DECODE_HEADER (-2)
#define ERROR_REALLOCATE (-3)
#define ERROR_READ_PAYLOAD (-4)
#define ERROR_DECODE_PAYLOAD (-5)
#define EXCEEDMAXPAYLOAD (-6)

#define UNKNOWNTYPE "Unknown Type"

typedef enum
{
    BER_BOOL     = 0x01,
    BER_INT      = 0x02,
    BER_NULL     = 0x05,
    BER_ENUM     = 0x0A,
    BER_STR      = 0x0C,
    BER_SEQ      = 0x10,
    BER_SEQ_OF   = 0x30,
    BER_PRINTSTR = 0x13,
    BER_UTC_TIME = 0x17,
    BER_TIME     = 0x18,
} tag_t;

typedef enum
{
    // System
    SYS_SUCCESS = 0x00,
    SYS_ERROR   = 0x01,

    // Account
    ACC_LOGIN         = 0x0A,
    ACC_LOGIN_SUCCESS = 0x0B,
    ACC_LOGOUT        = 0x0C,
    ACC_CREATE        = 0x0D,
    ACC_EDIT          = 0x0E,

    // Chat
    CHT_SEND = 0x14,

    // List Users
    LST_GET      = 0x1E,
    LST_RESPONSE = 0x1F,

    // Group Chat
    GRP_JOIN   = 0x28,
    GRP_EXIT   = 0x29,
    GRP_CREATE = 0x2A,
} packet_t;

typedef enum
{
    // Success
    EC_GOOD = 0x00,

    // Client Errors
    EC_INV_USER_ID   = 0x0B,
    EC_INV_AUTH_INFO = 0x0C,
    EC_USER_EXISTS   = 0x0D,

    // Server Errors
    EC_SERVER = 0x15,

    // Validity Errors
    EC_INV_REQ     = 0x1F,
    EC_REQ_TIMEOUT = 0x20
} error_code_t;

/* Message Header */
typedef struct header_t
{
    uint8_t  type;
    uint8_t  version;
    uint16_t sender_id;
    uint16_t payload_len;
} header_t;

typedef struct body_t
{
    uint8_t *msg;
} body_t;

typedef struct res_body_t
{
    uint8_t  tag;
    uint8_t  len;
    uint8_t  value;
    uint8_t  msg_tag;
    uint8_t  msg_len;
    uint8_t *msg;
} res_body_t;

typedef struct account_t
{
    uint8_t  username_tag;
    uint8_t  username_len;
    uint8_t *username;
    uint8_t  password_tag;
    uint8_t  password_len;
    uint8_t *password;
} account_t;

typedef struct request_t
{
    header_t *header;
    uint8_t   header_len;
    body_t   *body;
} request_t;

typedef struct response_t
{
    header_t     *header;
    error_code_t *code;
    res_body_t   *body;
} response_t;

typedef struct
{
    error_code_t code;
    const char  *msg;
} error_code_map;

typedef struct user_count_t
{
    uint8_t  type;
    uint8_t  version;
    uint16_t payload_len;
    uint8_t  tag;
    uint8_t  len;
    uint16_t value;
} user_count_t;

void handle_clients(int server_fd);
void process_req(int client_fd);

#endif
