//
// Created by eckotic on 2/4/25.
//

#include "../include/user_db.h"
#include <stdio.h>    // For printf debugging
#include <stdlib.h>
#include <string.h>

user_obj *user_arr[MAX_USERS] = {NULL};    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void init_user_list(void)
{
    for(int i = 0; i < MAX_USERS; i++)
    {
        user_arr[i] = NULL;    // Ensure list is empty on startup
    }
}

user_obj *new_user(void)
{
    user_obj *user = (user_obj *)malloc(sizeof(user_obj));
    if(!user)
    {
        perror("Failed to allocate memory for new user");
        return NULL;
    }

    memset(user, 0, sizeof(user_obj));    // Initialize struct
    return user;
}

int add_user(user_obj *user)
{
    for(int i = 0; i < MAX_USERS; i++)
    {
        if(user_arr[i] == NULL)    // Find an empty slot
        {
            user_arr[i] = user;
            printf("User added with ID: %d at index %d\n", user->id, i);
            return i;    // Return index where user was added
        }
    }

    printf("User list full. Cannot add more users.\n");
    return -1;    // No space available
}

void remove_user(int user_id)
{
    for(int i = 0; i < MAX_USERS; i++)
    {
        if(user_arr[i] != NULL && user_arr[i]->id == user_id)
        {
            printf("Removing user with ID: %d\n", user_id);
            free(user_arr[i]);     // Free memory
            user_arr[i] = NULL;    // Mark slot as empty
            return;
        }
    }

    printf("User with ID %d not found\n", user_id);
}

user_obj *find_user(int user_id)
{
    for(int i = 0; i < MAX_USERS; i++)
    {
        if(user_arr[i] != NULL && user_arr[i]->id == user_id)
        {
            return user_arr[i];
        }
    }
    return NULL;    // Not found
}
