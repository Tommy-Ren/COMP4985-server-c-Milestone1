//
// Created by eckotic on 2/4/25.
//

#ifndef USER_DB_H
#define USER_DB_H

#include <gdbm.h>
#include <stddef.h>
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

#define MAX_USERS 100    // Define a reasonable max number of users

typedef struct
{
    int id;
} user_obj;

extern user_obj *user_arr[MAX_USERS];    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

user_obj *new_user(void);
int       init_user_list(void);    // Function to initialize user list
int       add_user(user_obj *user);
void      remove_user(int user_id);
user_obj *find_user(int user_id);

#endif    // USER_DB_H
