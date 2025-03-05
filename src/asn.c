#include "../include/asn.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int  decode_uint8(const uint8_t buf[], int pos);
static int  decode_uint16(const uint8_t buf[], int pos);
static int  decode_int(const uint8_t buf[], int pos);
static int  decode_enum(const uint8_t buf[], int pos);
static int  decode_str(const uint8_t buf[], int pos);
static int  decode_time(const uint8_t buf[], int pos);
static int  check_header(const header_t *header);
static void print_header(const header_t *header);
static int  decode_field(const uint8_t buf[], int pos);
static int  encode_uint8(uint8_t buf[], uint8_t i, int pos, int asntype);
static int  encode_uint16(uint8_t buf[], uint16_t i, int pos, int asntype);
static int  encode_str(uint8_t buf[], const char *str, int pos, int asntype);
static void encode_header(uint8_t buf[], const header_t *header);

/*
 * Errors
 * -1 Unrecognized Tag Type
 * -2 Invalid Integer Length
 * -3 Field Length of Zero
 * -4 Unrecognized Packet Type
 * -5 Unsupported Version
 */

static int decode_uint8(const uint8_t buf[], int pos)
{
    printf("uint8: %u\n", buf[pos]);
    return (int)sizeof(uint8_t);
}

static int decode_uint16(const uint8_t buf[], int pos)
{
    uint16_t copy;
    memcpy(&copy, buf + pos, sizeof(uint16_t));
    printf("uint16: %u\n", ntohs(copy));
    return (int)sizeof(uint16_t);
}

/* following a ASN_INT tag, pos would land on len */
static int decode_int(const uint8_t buf[], int pos)
{
    int len = buf[pos];
    int res = 0;
    if(len < 1)
    {
        fprintf(stderr, "Field length of Zero\n");
        return FIELDLENGTHOFZERO;
    }
    switch(len)
    {
        case 1:
            res = decode_uint8(buf, pos + 1);
            break;
        case 2:
            res = decode_uint16(buf, pos + 1);
            break;
        default:
            fprintf(stderr, "Invalid Integer Length: %d\n", len);
            return INVALIDINTEGERLENGTH;
    }
    return (res > 0) ? res + 1 : res;
}

static int decode_enum(const uint8_t buf[], int pos)
{
    printf("Enum: %u\n", buf[pos + 1]);
    return 1 + 1;
}

/* following ASN_STR tag, pos would land on len */
static int decode_str(const uint8_t buf[], int pos)
{
    int   len = buf[pos];
    char *msg;
    if(len < 1)
    {
        fprintf(stderr, "Field length of Zero\n");
        return FIELDLENGTHOFZERO;
    }
    msg = (char *)malloc((size_t)len + 1);
    if(msg == NULL)
    {
        perror("malloc\n");
        exit(EXIT_FAILURE);
    }
    memcpy(msg, buf + pos + 1, (size_t)len);
    msg[len] = '\0';
    printf("string: %s\n", msg);
    free(msg);
    return len + 1;
}

static int decode_time(const uint8_t buf[], int pos)
{
    int   len = buf[pos];
    char *time;
    if(len != TIMESTRLEN)
    {
        fprintf(stderr, "Invalid Integer Length: %d\n", len);
        return INVALIDINTEGERLENGTH;
    }
    time = (char *)malloc((size_t)len + 1);
    if(time == NULL)
    {
        perror("malloc\n");
        exit(EXIT_FAILURE);
    }
    memcpy(time, buf + pos + 1, (size_t)len);
    time[len] = '\0';
    printf("time: %s\n", time);
    free(time);
    return len + 1;
}

void decode_header(const uint8_t buf[], header_t *header)
{
    int      pos = 0;
    uint16_t sender_id;
    uint16_t payload_len;

    memcpy(&header->packet_type, buf, sizeof(uint8_t));
    pos += (int)sizeof(uint8_t);

    memcpy(&header->version, buf + pos, sizeof(uint8_t));
    pos += (int)sizeof(uint8_t);

    memcpy(&sender_id, buf + pos, sizeof(uint16_t));
    pos += (int)sizeof(uint16_t);
    header->sender_id = ntohs(sender_id);

    memcpy(&payload_len, buf + pos, sizeof(uint16_t));
    header->payload_len = ntohs(payload_len);
}

static int check_header(const header_t *header)
{
    uint8_t  h = header->packet_type;
    uint16_t plen;

    if(h != SYS_SUCCESS && h != SYS_ERROR && h != ACC_LOGIN && h != ACC_LOGIN_SUCCESS && h != ACC_LOGOUT && h != ACC_CREATE && h != ACC_EDIT && h != CHT_SEND && h != LST_GET && h != LST_RESPONSE)
    {
        fprintf(stderr, "Unrecognized Packet Type: %u\n", h);
        return UNRECOGNIZEDPACKETTYPE;
    }

    h = header->version;

    if(h > CURRVER)
    {
        fprintf(stderr, "Unsupported Version: %u\n", h);
        return UNSUPPORTEDVERSION;
    }

    plen = header->payload_len;
    if(plen > MAXPAYLOADLEN)
    {
        fprintf(stderr, "Exceeded Max Payload Length\n");
        return EXCEEDMAXPAYLOAD;
    }

    return 0;
}

static void print_header(const header_t *header)
{
    printf("***HEADER***\nPacket Type: %u\nVersion: %u\nSender ID: %u\nPayload Length: %u\n***PAYLOAD***\n", header->packet_type, header->version, header->sender_id, header->payload_len);
}

/* returns # of bytes decoded */
static int decode_field(const uint8_t buf[], int pos)
{
    int     res  = 0;
    uint8_t type = buf[pos];
    switch(type)
    {
        case ASN_INT:
            res = decode_int(buf, pos + 1);
            break;
        case ASN_ENUM:
            res = decode_enum(buf, pos + 1);
            break;
        case ASN_STR:
            res = decode_str(buf, pos + 1);
            break;
        case ASN_TIME:
            res = decode_time(buf, pos + 1);
            break;
        default:
            fprintf(stderr, "Unrecognized tag type: %u\n", type);
            res = UNRECOGNIZEDTAGTYPE;
    }
    return (res > 0) ? res + 1 : res;
}

int decode_packet(const uint8_t buf[], const header_t *header)
{
    int header_res;
    print_header(header);
    header_res = check_header(header);
    if(header_res < 0)
    {
        return header_res;
    }
    if(header->payload_len > 0)
    {
        int remaining = header->payload_len;
        int pos       = HEADERLEN;
        while(remaining)
        {
            int decoded = decode_field(buf, pos);
            if(decoded < 0)
            {
                return decoded;
            }
            remaining -= decoded;
            pos += decoded;
        }
    }
    return 0;
}

static int encode_uint8(uint8_t buf[], uint8_t i, int pos, int asntype)
{
    buf[pos++] = (uint8_t)asntype;
    buf[pos++] = 1;
    buf[pos++] = i;
    return pos;
}

static int encode_uint16(uint8_t buf[], uint16_t i, int pos, int asntype)
{
    uint16_t copy = htons(i);
    size_t   len  = sizeof(uint16_t);
    buf[pos++]    = (uint8_t)asntype;
    buf[pos++]    = (uint8_t)len;
    memcpy(buf + pos, &copy, len);
    return pos + (int)len;
}

/* returns new position */
static int encode_str(uint8_t buf[], const char *str, int pos, int asntype)
{
    size_t len = strlen(str);
    buf[pos++] = (uint8_t)asntype;
    buf[pos++] = (uint8_t)len;
    // NOLINTNEXTLINE(bugprone-not-null-terminated-result)
    memcpy(buf + pos, str, len);
    return pos + (int)len;
}

static void encode_header(uint8_t buf[], const header_t *header)
{
    uint16_t copy;
    int      pos   = 1;
    size_t   u16sz = sizeof(uint16_t);
    memcpy(buf, &header->packet_type, 1);
    memcpy(buf + pos, &header->version, 1);
    pos++;
    copy = htons(header->sender_id);
    memcpy(buf + pos, &copy, u16sz);
    pos += (int)u16sz;
    copy = htons(header->payload_len);
    memcpy(buf + pos, &copy, u16sz);
}

/* returns total packet length */
int encode_sys_success_res(uint8_t buf[], uint8_t packet_type)
{
    header_t header = {SYS_SUCCESS, CURRVER, SYSID, SYS_SUCCESS_LEN};
    encode_header(buf, &header);
    encode_uint8(buf, packet_type, HEADERLEN, ASN_ENUM);

    return HEADERLEN + SYS_SUCCESS_LEN;
}

int encode_sys_error_res(uint8_t buf[], int err)
{
    uint8_t     errcode;
    const char *msg;
    int         pos = HEADERLEN;
    int         len;
    header_t    header = {SYS_ERROR, CURRVER, SYSID, 0};
    switch(err)
    {
        /* more added later */
        case UNRECOGNIZEDTAGTYPE:
            errcode = EC_GENSERVER;
            msg     = "Unrecognized Tag Type";
            break;
        case INVALIDINTEGERLENGTH:
            errcode = EC_GENSERVER;
            msg     = "Invalid Integer Length";
            break;
        case FIELDLENGTHOFZERO:
            errcode = EC_GENSERVER;
            msg     = "Field Length of Zero";
            break;
        case UNRECOGNIZEDPACKETTYPE:
            errcode = EC_INVREQ;
            msg     = "Unrecognized Packet Type";
            break;
        case UNSUPPORTEDVERSION:
            errcode = EC_INVREQ;
            msg     = "Unsupported Version";
            break;
        case EXCEEDMAXPAYLOAD:
            errcode = EC_INVREQ;
            msg     = "Exceeded Max Payload Length";
            break;
        default:
            errcode = EC_GENSERVER;
            msg     = "Server Error";
    }
    len                = (int)strlen(msg) + 2; /* plus tag & len */
    header.payload_len = (uint16_t)(U8ENCODELEN + len);
    encode_header(buf, &header);
    pos = encode_uint8(buf, errcode, pos, ASN_ENUM);
    encode_str(buf, msg, pos, ASN_STR);
    return HEADERLEN + U8ENCODELEN + len;
}

int encode_acc_login_success_res(uint8_t buf[], uint16_t user_id)
{
    header_t header = {ACC_LOGIN_SUCCESS, CURRVER, SYSID, ACC_LOGIN_SUCCESS_LEN};
    encode_header(buf, &header);
    encode_uint16(buf, user_id, HEADERLEN, ASN_INT);
    return HEADERLEN + ACC_LOGIN_SUCCESS_LEN;
}

int encode_cht_send(uint8_t buf[])
{
    // hardcoded packet
    int            pos       = HEADERLEN;
    const uint16_t id        = (uint16_t)69420;
    const char    *timestamp = "20250304160000Z";
    const char    *content   = "Hello from the test server";
    const char    *username  = "Banunu";
    header_t       header    = {CHT_SEND, CURRVER, id, 0};

    pos = encode_str(buf, timestamp, pos, ASN_TIME);
    pos = encode_str(buf, content, pos, ASN_STR);
    pos = encode_str(buf, username, pos, ASN_STR);

    header.payload_len = (uint16_t)(pos - HEADERLEN);
    encode_header(buf, &header);
    return pos;
}
