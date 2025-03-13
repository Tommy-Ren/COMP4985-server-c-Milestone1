#ifndef USER_DB_H
#define USER_DB_H

#include <gdbm.h>
#include <stddef.h>
#include <string.h>
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

#define MAX_USERS 100 /* Define a reasonable max number of users */

/* A simple user object for the user list (currently only holds an id) */
typedef struct
{
    int id;
} user_obj;

/* ---------------------------------------------------------------------------
   New definitions to support account credential storage
   --------------------------------------------------------------------------- */

/* DBO: Database Object wrapper for account-related operations */
typedef struct
{
    DBM  *db;
    char *name; /* Filename of the database (e.g. "mydb") */
} DBO;

/* Opens the database specified in dbo->name in read/write mode (creating it if needed).
   Returns 0 on success, -1 on failure. */
int database_open(DBO *dbo);

/* Retrieves the stored string value associated with key from the given DBM.
   On success, returns a pointer to a newly allocated copy of the value (which must be freed by the caller);
   returns NULL if the key is not found or on error. */
char *retrieve_string(DBM *db, char *key);

/* Stores the string value under the key into the given DBM.
   Returns 0 on success, -1 on failure. */
int store_string(DBM *db, char *key, char *value);

/* ---------------------------------------------------------------------------
   Existing functions for user list management
   --------------------------------------------------------------------------- */
int       init_user_list(void);
void      close_user_list(void);
int       add_user(user_obj *user);
void      remove_user(int user_id);
user_obj *find_user(int user_id);

#endif /* USER_DB_H */
