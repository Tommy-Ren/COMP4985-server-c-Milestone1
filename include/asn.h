//
// https://github.com/kvnbanunu/comp4985-test-server/blob/main/include/asn.h
//

#ifndef ASN_H
#define ASN_H

#include <stdint.h>

#define HEADERLEN 6
#define TIMESTRLEN 15
#define PACKETLEN 777
#define SYSID 0
#define CURRVER 1
#define U8ENCODELEN 3
#define SYS_SUCCESS_LEN 3
#define ACC_LOGIN_SUCCESS_LEN 4
#define UNRECOGNIZEDTAGTYPE -1
#define INVALIDINTEGERLENGTH -2
#define FIELDLENGTHOFZERO -3
#define UNRECOGNIZEDPACKETTYPE -4
#define UNSUPPORTEDVERSION -5

enum ASNTag
{
    ASN_BOOL = 1,
    ASN_INT  = 2,
    ASN_NULL = 5,
    ASN_ENUM = 10,
    ASN_STR  = 12,
    ASN_TIME = 24,
    ASN_SEQ  = 48
};

enum Packet_Type
{
    SYS_SUCCESS,
    SYS_ERROR,
    ACC_LOGIN = 10,
    ACC_LOGIN_SUCCESS,
    ACC_LOGOUT,
    ACC_CREATE,
    ACC_EDIT,
    CHT_SEND = 20,
    LST_GET  = 30,
    LST_RESPONSE,
    GRP_JOIN = 40,
    GRP_EXIT,
    GRP_CREATE,
    HST_GET = 50
};

enum Error_Code
{
    EC_INVUSERID = 11,
    EC_INVAUTHINFO,
    EC_USEREXISTS,
    EC_GENSERVER = 21,
    EC_INVREQ    = 31,
    EC_REQTO
};

typedef struct header_t
{
    uint8_t  packet_type;
    uint8_t  version;
    uint16_t sender_id;
    uint16_t payload_len;
} header_t;

void decode_header(const uint8_t buf[], header_t *header);
int  decode_packet(const uint8_t buf[], const header_t *header);
int  encode_sys_success_res(uint8_t buf[], uint8_t packet_type);
int  encode_sys_error_res(uint8_t buf[], int err);
int  encode_acc_login_success_res(uint8_t buf[], uint16_t user_id);

#endif    // ASN_H