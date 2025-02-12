//
// Created by eckotic on 2/4/25.
//

#include "../include/user_db.h"
#include <stdlib.h>

user_obj *new_user(void)
{
    return (user_obj *)malloc(sizeof(user_obj));
}
