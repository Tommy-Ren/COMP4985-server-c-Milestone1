#include "../include/account.h"
#include "../include/user_db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ssize_t account_login(const request_t *request, response_t *response);
static ssize_t account_create(const request_t *request, response_t *response);
static ssize_t account_logout(void);

ssize_t account_handler(const request_t *request, response_t *response)
{
    ssize_t          result;
    const account_t *acc = (account_t *)request->body;

    result = -1;

    if(request->header->type == ACC_LOGIN)
    {
        result = account_login(request, response);
        free(acc->username);
        free(acc->password);
    }
    else if(request->header->type == ACC_CREATE)
    {
        result = account_create(request, response);
        free(acc->username);
        free(acc->password);
    }
    else if(request->header->type == ACC_LOGOUT)
    {
        result = account_logout();
    }

    return result;
}

ssize_t account_login(const request_t *request, response_t *response)
{
    char            *password;
    char             db_name[] = "mydb";
    ssize_t          result;
    DBO              dbo;
    datum            output;
    const account_t *acc = (account_t *)request->body;

    memset(&output, 0, sizeof(datum));
    dbo.name = db_name;

    printf("account_login\n");

    printf("username: %s\n", acc->username);
    printf("password: %s\n", acc->password);

    result = database_open(&dbo);

    if(result == -1)
    {
        perror("database error");
        *(response->code) = EC_SERVER;
        goto error;
    }

    // check user exists
    password = retrieve_string(dbo.db, (char *)acc->username);
    if(!password)
    {
        perror("Username not found");
        *(response->code) = EC_INV_USER_ID;
        goto error;
    }
    printf("Retrieved password: %s\n", password);

    if(strcmp(password, (char *)acc->password) != 0)
    {
        free((void *)password);
        *(response->code) = EC_INV_AUTH_INFO;
        goto error;
    }

    response->body->tag = BER_INT;
    response->body->len = 0x01;

    // user Id
    response->body->value = 0x01;
    free((void *)password);
    dbm_close(dbo.db);
    return 0;

error:
    response->body->tag   = BER_INT;
    response->body->len   = 0x01;
    response->body->value = (uint8_t)*response->code;
    dbm_close(dbo.db);
    return -1;
}

ssize_t account_create(const request_t *request, response_t *response)
{
    char    db_name[] = "mydb";
    ssize_t result;
    DBO     dbo;
    char   *existing;

    const account_t *acc = (account_t *)request->body;

    printf("username: %s\n", acc->username);
    printf("password: %s\n", acc->password);

    dbo.name = db_name;

    result = database_open(&dbo);
    if(result == -1)
    {
        perror("database error");
        *response->code = EC_SERVER;
        goto error;
    }

    // check user exists
    existing = retrieve_string(dbo.db, (char *)acc->username);

    if(existing)
    {
        printf("Retrieved username: %s\n", existing);
        *response->code = EC_USER_EXISTS;
        free((void *)existing);
        goto error;
    }

    // Store user
    if(store_string(dbo.db, (char *)acc->username, (char *)acc->password) != 0)
    {
        perror("user");
        *response->code = EC_SERVER;
        goto error;
    }

    response->body->tag   = ACC_LOGIN;
    response->body->len   = 0x01;
    response->body->value = ACC_CREATE;
    dbm_close(dbo.db);
    return 0;

error:
    response->body->tag   = BER_INT;
    response->body->len   = 0x01;
    response->body->value = (uint8_t)*response->code;
    dbm_close(dbo.db);
    return -1;
}

ssize_t account_logout(void)
{
    // need session design

    return 0;
}
