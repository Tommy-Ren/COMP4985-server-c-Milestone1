#include "../include/account.h"
#include "../include/user_db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ssize_t account_login(message_t *message);
static ssize_t account_create(message_t *message);
static ssize_t account_edit(message_t *message);
static ssize_t account_logout(message_t *message);

ssize_t account_handler(const message_t *message)
{
    ssize_t result;

    result = ACCOUNT_ERROR;

    if(message->type == ACC_CREATE)
    {
        result = account_create(message);
    }
    else if(message->type == ACC_LOGIN)
    {
        result = account_login(message);
    }
    else if(message->type == ACC_EDIT)
    {
        result = account_edit(message);
    }
    else if(message->type == ACC_LOGOUT)
    {
        result = account_logout(message);
    }

    return result;
}

static ssize_t account_create(message_t *message)
{
    char db_name[]    = "user_db";
    char index_name[] = "index_db";
    DBO  userDB;
    DBO  index_userDB;

    const char *username;
    const char *password;
    uint8_t     user_len;
    uint8_t     pass_len;

    int   user_id;
    char *key;
    char *ptr;

    uint16_t sender_id = SYSID;

    userDB.name       = db_name;
    userDB.db         = NULL;
    index_userDB.name = index_name;
    index_userDB.db   = NULL;
    key               = NULL;

    if(database_open(&userDB) < 0)
    {
        perror("Failed to open user_db\n");
        message->code = EC_SERVER;
        goto error;
    }

    if(database_open(&index_userDB) < 0)
    {
        perror("Failed to open index_userDB");
        message->code = EC_SERVER;
        goto error;
    }

    // username len
    ptr = (char *)message->req_buf + HEADERLEN + 1;
    memcpy(&user_len, ptr, sizeof(user_len));
    // username
    ptr += sizeof(user_len);
    username = ptr;

    // password len
    ptr += user_len + 1;
    memcpy(&pass_len, ptr, sizeof(pass_len));
    // password
    ptr += sizeof(pass_len);
    password = ptr;

    printf("Username: %s\n", (int)user_len, username);
    printf("Username length: %d\n", (int)user_len);
    printf("Password: %s\n", (int)pass_len, password);
    printf("Password length: %d\n", (int)pass_len);

    // check user exists
    if(retrieve_byte(userDB.db, username, user_len))
    {
        message->code = EC_USER_EXISTS;
        goto error;
    }

    (*message->user_count)++;
    *message->client_id = *message->user_count;

    // Store user
    if(store_byte(userDB.db, username, user_len, password, pass_len) != 0)
    {
        perror("Failed to store username and password\n");
        message->code = EC_SERVER;
        goto error;
    }
    key = strndup(username, user_len);
    if(key == NULL)
    {
        perror("Failed to allocate memory\n");
        message->code = EC_SERVER;
        goto error;
    }

    // Store user index
    if(store_int(index_userDB.db, key, *message->client_id) < 0)
    {
        perror("Failed to store user index\n");
        message->code = EC_SERVER;
        goto error;
    }

    // Retrieve existing user id
    if(retrieve_int(index_userDB.db, key, &user_id) < 0)
    {
        printf("Failed to retrieve user info\n");
        message->code = EC_SERVER;
        goto error;
    }
    printf("User %.*d created\n", (int)sizeof(*message->client_id), user_id);

    ptr = (char *)message->res_buf;
    // tag
    *ptr++ = SYS_SUCCESS;
    // version
    *ptr++ = VERSION_NUM;
    // sender_id
    sender_id = htons(sender_id);
    memcpy(ptr, &sender_id, sizeof(sender_id));
    ptr += sizeof(sender_id);
    // payload len
    message->response_len = htons(message->response_len);
    memcpy(ptr, &message->response_len, sizeof(message->response_len));
    ptr += sizeof(message->response_len);
    // payload CHOICE
    *ptr++ = BER_ENUM;
    *ptr++ = sizeof(uint8_t);
    *ptr++ = ACC_CREATE;

    dbm_close(userDB.db);
    dbm_close(index_userDB.db);
    sfree(key);
    return 0;

error:
    dbm_close(userDB.db);
    dbm_close(index_userDB.db);
    sfree(key);
    return ACCOUNT_CREATE_ERROR;
}

static ssize_t account_login(message_t *message)
{
    char db_name[]    = "user_db";
    char index_name[] = "index_db";
    DBO  userDB;
    DBO  index_userDB;

    const char *username;
    const char *password;
    uint8_t     user_len;
    uint8_t     pass_len;

    int   user_id;
    datum output;
    char *existing;
    char *key;
    char *ptr;

    uint16_t sender_id = SYSID;

    userDB.name       = db_name;
    userDB.db         = NULL;
    index_userDB.name = index_name;
    index_userDB.db   = NULL;
    key               = NULL;

    memset(&output, 0, sizeof(datum));

    if(database_open(&userDB) < 0)
    {
        perror("Failed to open user_db\n");
        message->code = EC_SERVER;
        goto error;
    }

    if(database_open(&index_userDB) < 0)
    {
        perror("Failed to open index_userDB");
        message->code = EC_SERVER;
        goto error;
    }

    // username len
    ptr = (char *)message->req_buf + HEADERLEN + 1;
    memcpy(&user_len, ptr, sizeof(user_len));
    // username
    ptr += sizeof(user_len);
    username = ptr;

    // password len
    ptr += user_len + 1;
    memcpy(&pass_len, ptr, sizeof(pass_len));
    // password
    ptr += sizeof(pass_len);
    password = ptr;

    printf("Username: %s\n", (int)user_len, username);
    printf("Username length: %d\n", (int)user_len);
    printf("Password: %s\n", (int)pass_len, password);
    printf("Password length: %d\n", (int)pass_len);

    // Retrieve existing user
    existing = retrieve_byte(userDB.db, username, user_len);
    if(!existing)
    {
        perror("Failed to find user\n");
        message->code = EC_INV_USER_ID;
        goto error;
    }

    // Check if password is correct
    if(memcmp(existing, password, pass_len) != 0)
    {
        sfree(existing);
        perror("Failed to provide correct password\n");
        message->code = EC_INV_AUTH_INFO;
        goto error;
    }

    key = strndup(username, user_len);
    if(key == NULL)
    {
        perror("Failed to allocate memory");
        message->code = EC_SERVER;
        goto error;
    }

    if(retrieve_int(index_userDB.db, key, &user_id) < 0)
    {
        perror("Failed to provide correct password\n");
        message->code = EC_SERVER;
        goto error;
    }
    printf("User %.*d logged in\n", (int)sizeof(*message->client_id), user_id);

    ptr = (char *)message->res_buf;
    // tag
    *ptr++ = ACC_LOGIN_SUCCESS;
    // version
    *ptr++ = SYSID;
    // sender_id
    sender_id = htons(sender_id);
    memcpy(ptr, &sender_id, sizeof(sender_id));
    ptr += sizeof(sender_id);
    // payload len
    message->response_len = 4;
    message->response_len = htons(message->response_len);
    memcpy(ptr, &message->response_len, sizeof(message->response_len));
    ptr += sizeof(message->response_len);
    // payload CHOICE
    *ptr++ = BER_INT;
    *ptr++ = sizeof(uint16_t);
    // payload user_id
    user_id = htons((uint16_t)user_id);
    memcpy(ptr, &user_id, sizeof(user_id));
    message->client_id = ntohs((uint16_t)user_id);

    dbm_close(userDB.db);
    dbm_close(index_userDB.db);
    sfree(key);
    sfree(existing);
    return 0;

error:
    dbm_close(userDB.db);
    dbm_close(index_userDB.db);
    sfree(key);
    sfree(existing);
    return ACCOUNT_LOGIN_ERROR;
}

static ssize_t account_edit(message_t *message)
{
    {
        char db_name[] = "user_db";
        DBO  userDB;

        const char *username;
        const char *new_password;
        uint8_t     user_len;
        uint8_t     pass_len;

        char *existing;
        char *ptr;

        userDB.name = db_name;
        userDB.db   = NULL;

        if(database_open(&userDB) < 0)
        {
            perror("Failed to open user_db\n");
            message->code = EC_SERVER;
            goto error;
        }

        // username len
        ptr = (char *)message->req_buf + HEADERLEN + 1;
        memcpy(&user_len, ptr, sizeof(user_len));
        // username
        ptr += sizeof(user_len);
        username = ptr;

        // new password len
        ptr += user_len + 1;
        memcpy(&pass_len, ptr, sizeof(pass_len));
        // new password
        ptr += sizeof(pass_len);
        new_password = ptr;

        printf("Username: %s\n", (int)user_len, username);
        printf("Username length: %d\n", (int)user_len);
        printf("New Password: %s\n", (int)pass_len, new_password);
        printf("New Password length: %d\n", (int)pass_len);

        // Retrieve existing user
        existing = retrieve_byte(userDB.db, username, user_len);
        if(!existing)
        {
            perror("Failed to find user\n");
            message->code = EC_INV_USER_ID;
            goto error;
        }

        // Update password
        if(store_byte(userDB.db, username, user_len, new_password, pass_len) != 0)
        {
            perror("Failed to update password\n");
            message->code = EC_SERVER;
            goto error;
        }

        printf("User %.*s password updated\n", (int)user_len, username);

        ptr = (char *)message->res_buf;
        // tag
        *ptr++ = SYS_SUCCESS;
        // version
        *ptr++ = VERSION_NUM;
        // sender_id
        uint16_t sender_id = htons(SYSID);
        memcpy(ptr, &sender_id, sizeof(sender_id));
        ptr += sizeof(sender_id);
        // payload len
        message->response_len = 0;
        message->response_len = htons(message->response_len);
        memcpy(ptr, &message->response_len, sizeof(message->response_len));

        dbm_close(userDB.db);
        sfree(existing);
        return 0;

    error:
        dbm_close(userDB.db);
        sfree(existing);
        return ACCOUNT_EDIT_ERROR;
    }
}

static ssize_t account_logout(message_t *message)
{
    printf("User %d logged out\n", message->client_id);

    message->response_len = 0;
    return -1;
}
