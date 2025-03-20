#ifndef ARGS_H
#define ARGS_H

#include <arpa/inet.h>

// struct to hold the arguments
typedef struct Arguments
{
    const char *ip;
    in_port_t   port;
    const char *sm_ip;
    in_port_t   sm_port;
} Arguments;

// prints usage message and exits
_Noreturn void usage(const char *app_name, int exit_code, const char *message);

// checks arguments
void parse_args(int argc, char *argv[], Arguments *args);

#endif
