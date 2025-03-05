/*******************************************************************************
 * User Database Management System
 *
 * This module provides a persistent storage interface for user management using
 * DBM (Database Manager) for data storage. It supports basic CRUD operations:
 * - Creating new users
 * - Retrieving user information
 * - Updating user records
 * - Deleting users
 * - Listing all users
 *
 * The implementation uses GDBM on Linux/FreeBSD and NDBM on macOS, providing
 * cross-platform compatibility through conditional compilation.
 *
 * Data Storage:
 * - Keys: User IDs stored as strings
 * - Values: Binary serialized user_obj structures
 *
 * Dependencies:
 * - GDBM/NDBM library
 * - user_db.h for structure definitions
 *
 * Usage:
 * Call init_user_list() before any operations and close_user_list() when done.
 ******************************************************************************/

#include "../include/user_db.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __APPLE__
    #include <ndbm.h>
typedef size_t datum_size;
#elif defined(__FreeBSD__)
    #include <gdbm.h>
typedef int datum_size;
#else
    /* Assume Linux/Ubuntu; using gdbm_compat which provides an ndbm-like interface */
    #include <ndbm.h>
typedef int datum_size;
#endif

/* Named constants */
#define USER_DB_MODE 0644
#define KEY_STR_SIZE 16

/* Prototypes for functions missing in header */
void        list_all_users(void);
void        close_user_list(void);
static void safe_dbm_fetch(DBM *db, datum key, datum *result);

/*
 * Global DBM pointer for the user database.
 * This global variable is mutable because it is updated at runtime.
 * NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
 */
static DBM *user_db = NULL;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

/* Define a constant character array for the DB filename */
static char user_db_filename[] = "user_db";    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

/* Function: init_user_list
   Description: Initializes the DBM database for user management.
                Opens the database for reading and writing; creates it if it does not exist.
   Returns: void */
void init_user_list(void)
{
    user_db = dbm_open((char *)user_db_filename, O_RDWR | O_CREAT, USER_DB_MODE);
    if(user_db == NULL)
    {
        perror("Failed to open DBM database");
        exit(EXIT_FAILURE);
    }
    printf("DBM database '%s' opened successfully.\n", user_db_filename);
}

/* Function: new_user
   Description: Allocates and initializes a new user object.
   Returns: Pointer to a new user_obj, or NULL on failure */
user_obj *new_user(void)
{
    user_obj *user;
    user = (user_obj *)malloc(sizeof(user_obj));
    if(user == NULL)
    {
        perror("Failed to allocate memory for new user");
        return NULL;
    }
    memset(user, 0, sizeof(user_obj));
    return user;
}

/* Function: add_user
   Description: Adds a user object to the DBM database.
                Uses the user's id as a key (converted to a string) and stores the binary data.
   Returns: 0 on success, -1 on failure */
int add_user(user_obj *user)
{
    datum key;
    datum data;
    char  key_str[KEY_STR_SIZE];

    /* Create a key string based on the user's ID */
    sprintf(key_str, "%d", user->id);
    key.dptr  = key_str;
    key.dsize = (datum_size)(strlen(key_str) + 1);

    /* Prepare the data as the binary representation of the user_obj */
    data.dptr  = (char *)user;
    data.dsize = sizeof(user_obj);

    if(dbm_store(user_db, key, data, DBM_REPLACE) != 0)
    {
        perror("dbm_store failed");
        return -1;
    }
    printf("User added with ID: %d\n", user->id);
    return 0;
}

/* Function: remove_user
   Description: Removes a user from the DBM database by user ID.
   Returns: void */
void remove_user(int user_id)
{
    datum key;
    char  key_str[KEY_STR_SIZE];

    sprintf(key_str, "%d", user_id);
    key.dptr  = key_str;
    key.dsize = (datum_size)(strlen(key_str) + 1);

    if(dbm_delete(user_db, key) != 0)
    {
        printf("User with ID %d not found or deletion failed\n", user_id);
    }
    else
    {
        printf("Removed user with ID: %d\n", user_id);
    }
}

/* Function: find_user
   Description: Finds a user in the DBM database by their user ID.
                Returns a dynamically allocated copy of the user_obj if found; otherwise, NULL.
                The caller is responsible for freeing the returned memory.
   Returns: user_obj * */
user_obj *find_user(int user_id)
{
    datum     key;
    datum     data;
    char      key_str[KEY_STR_SIZE];
    user_obj *user;

    sprintf(key_str, "%d", user_id);
    key.dptr  = key_str;
    key.dsize = (datum_size)(strlen(key_str) + 1);

    safe_dbm_fetch(user_db, key, &data);
    if(data.dptr == NULL)
    {
        return NULL;
    }
    user = (user_obj *)malloc(sizeof(user_obj));
    if(user == NULL)
    {
        perror("Failed to allocate memory for user fetch");
        return NULL;
    }
    memcpy(user, data.dptr, sizeof(user_obj));
    return user;
}

/* Function: list_all_users
   Description: Lists all users stored in the DBM database.
                Used to support user list queries and server feedback.
   Returns: void */
void list_all_users(void)
{
    datum key;
    datum data;
    /* Temporary user object */
    user_obj temp_user;

    printf("Listing all users in DBM:\n");
    key = dbm_firstkey(user_db);
    while(key.dptr != NULL)
    {
        safe_dbm_fetch(user_db, key, &data);
        if(data.dptr != NULL)
        {
            memcpy(&temp_user, data.dptr, sizeof(user_obj));
            /* Print only the user ID as user_obj currently lacks a 'username' field */
            printf("User ID: %d\n", temp_user.id);
        }
        key = dbm_nextkey(user_db);
    }
}

/* Function: close_user_list
   Description: Closes the DBM database.
   Returns: void */
void close_user_list(void)
{
    if(user_db != NULL)
    {
        dbm_close(user_db);
        user_db = NULL;
        printf("DBM database closed.\n");
    }
}

/* Helper function: safe_dbm_fetch
   Description: Wraps dbm_fetch to avoid aggregate return issues.
   Parameters:
       db     - the DBM pointer
       key    - the key datum to fetch
       result - pointer to a datum that will be populated with the result
   Returns: void */
static void safe_dbm_fetch(DBM *db, datum key, datum *result)
{
    datum temp;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waggregate-return"
    temp = dbm_fetch(db, key);
#pragma GCC diagnostic pop
    result->dptr  = temp.dptr;
    result->dsize = temp.dsize;
}
