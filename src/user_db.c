/*******************************************************************************
 * User Database Management System
 *
 * This module provides a persistent storage interface for user management using
 * DBM (Database Manager) for data storage. It supports both account credential
 * operations (for login/creation) and user list management. All user data is
 * stored persistently via DBM, so no in-memory global array is needed.
 ******************************************************************************/

#include "../include/user_db.h"
#include <fcntl.h>
#include <gdbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Named constants */
#define USER_DB_MODE 0666
#define DB_NAME_SIZE 32
#define KEY_STR_SIZE 16

/* Global pointer for the user list DB */
static DBM *user_db = NULL; /* NOLINT(cppcoreguidelines-avoid-non-const-global-variables) */

/* Prototypes for externally visible functions */
user_obj *new_user(void);
void      list_all_users(void);

/* --- Functions for account credential storage --- */

/* Opens the DBM database specified by dbo->name.
   Returns 0 on success, -1 on error. */
int database_open(DBO *dbo)
{
    if(dbo == NULL || dbo->name == NULL)
    {
        return -1;
    }
    dbo->db = dbm_open(dbo->name, O_RDWR | O_CREAT, USER_DB_MODE);
    if(dbo->db == NULL)
    {
        perror("database_open: dbm_open failed");
        return -1;
    }
    return 0;
}

/* Retrieves a stored string value for the given key from the database.
   Returns a newly allocated copy of the string on success, or NULL if not found. */
char *retrieve_string(DBM *db, char *key)
{
    datum key_datum;
    datum data;
    char *value;

    if(db == NULL || key == NULL)
    {
        return NULL;
    }
    key_datum.dptr  = key;
    key_datum.dsize = (datum_size)(strlen(key) + 1);
    data            = dbm_fetch(db, key_datum);
    if(data.dptr == NULL)
    {
        return NULL;
    }
    value = (char *)malloc((size_t)data.dsize);
    if(value == NULL)
    {
        perror("retrieve_string: malloc failed");
        return NULL;
    }
    memcpy(value, data.dptr, (size_t)data.dsize);
    return value;
}

/* Stores the string value under the given key in the database.
   Returns 0 on success, -1 on failure. */
int store_string(DBM *db, char *key, char *value)
{
    datum key_datum;
    datum data;

    if(db == NULL || key == NULL || value == NULL)
    {
        return -1;
    }
    key_datum.dptr  = key;
    key_datum.dsize = (datum_size)(strlen(key) + 1);
    data.dptr       = value;
    data.dsize      = (datum_size)(strlen(value) + 1);
    if(dbm_store(db, key_datum, data, DBM_REPLACE) != 0)
    {
        perror("store_string: dbm_store failed");
        return -1;
    }
    return 0;
}

/* --- Functions for user list management --- */

/* Initializes the user list database for storing user_obj structures.
   Returns 0 on success, -1 on failure. */
int init_user_list(void)
{
    const char *const_db_name = "user_db";
    // Create a mutable buffer to hold the database name.
    char db_name[DB_NAME_SIZE];
    strncpy(db_name, const_db_name, sizeof(db_name) - 1);
    db_name[sizeof(db_name) - 1] = '\0';

    user_db = dbm_open(db_name, O_RDWR | O_CREAT, USER_DB_MODE);
    if(user_db == NULL)
    {
        perror("Failed to open user database");
        return -1;
    }
    printf("DBM database '%s' opened successfully.\n", db_name);
    return 0;
}

/* Allocates and initializes a new user object.
   Returns a pointer to the new user_obj or NULL on failure. */
user_obj *new_user(void)
{
    user_obj *user;
    user = (user_obj *)malloc(sizeof(user_obj));
    if(user == NULL)
    {
        perror("new_user: Failed to allocate memory for new user");
        return NULL;
    }
    memset(user, 0, sizeof(user_obj));
    return user;
}

/* Adds a user object to the user list database.
   Returns 0 on success, -1 on failure. */
int add_user(user_obj *user)
{
    datum key;
    datum data;
    char  key_str[KEY_STR_SIZE];

    sprintf(key_str, "%d", user->id);
    key.dptr  = key_str;
    key.dsize = (datum_size)(strlen(key_str) + 1);

    data.dptr  = (char *)user;
    data.dsize = sizeof(user_obj);

    if(dbm_store(user_db, key, data, DBM_REPLACE) != 0)
    {
        perror("add_user: dbm_store failed");
        return -1;
    }
    printf("User added with ID: %d\n", user->id);
    return 0;
}

/* Removes a user from the user list database by user ID. */
void remove_user(int user_id)
{
    datum key;
    char  key_str[KEY_STR_SIZE];

    sprintf(key_str, "%d", user_id);
    key.dptr  = key_str;
    key.dsize = (datum_size)(strlen(key_str) + 1);

    if(dbm_delete(user_db, key) != 0)
    {
        printf("remove_user: User with ID %d not found or deletion failed\n", user_id);
    }
    else
    {
        printf("Removed user with ID: %d\n", user_id);
    }
}

/* Finds a user in the user list database by their user ID.
   Returns a dynamically allocated copy of the user_obj if found, or NULL if not found.
   The caller is responsible for freeing the returned memory. */
user_obj *find_user(int user_id)
{
    datum     key;
    char      key_str[KEY_STR_SIZE];
    user_obj *user;

    sprintf(key_str, "%d", user_id);
    key.dptr  = key_str;
    key.dsize = (datum_size)(strlen(key_str) + 1);

    {
        /* Limit the scope of 'data' */
        datum data = dbm_fetch(user_db, key);
        if(data.dptr == NULL)
        {
            return NULL;
        }
        user = (user_obj *)malloc(sizeof(user_obj));
        if(user == NULL)
        {
            perror("find_user: Failed to allocate memory for user fetch");
            return NULL;
        }
        memcpy(user, data.dptr, sizeof(user_obj));
    }
    return user;
}

/* Lists all users stored in the user list database. */
void list_all_users(void)
{
    datum    key;
    user_obj temp_user;

    printf("Listing all users in DBM:\n");
    key = dbm_firstkey(user_db);
    while(key.dptr != NULL)
    {
        datum data;
        data = dbm_fetch(user_db, key);
        if(data.dptr != NULL)
        {
            memcpy(&temp_user, data.dptr, sizeof(user_obj));
            printf("User ID: %d\n", temp_user.id);
        }
        key = dbm_nextkey(user_db);
    }
}

void close_user_list(void)
{
    if(user_db)
    {
        dbm_close(user_db);
        printf("DBM database closed.\n");
        user_db = NULL;
    }
}
