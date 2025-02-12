//
// Created by eckotic on 2/4/25.
//

#ifndef USER_DB_H
#define USER_DB_H

#include <stddef.h>

#define MAX_USERS 100    // Define a reasonable max number of users

typedef struct
{
    int id;
} user_obj;

extern user_obj *user_arr[MAX_USERS];    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

user_obj *new_user(void);
void      init_user_list(void);    // Function to initialize user list
int       add_user(user_obj *user);
void      remove_user(int user_id);
user_obj *find_user(int user_id);

#endif    // USER_DB_H
