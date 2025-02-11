#ifndef ARGS_H
#define ARGS_H

#include <arpa/inet.h>

// struct to hold the values for
typedef struct Arguments
{
    const char *ip;
    in_port_t   port;
} Arguments;

// prints usage message and exits
_Noreturn void usage(const char *app_name, int exit_code, const char *message);

// checks arguments
void parse_args(Arguments *args, int argc, char *argv[]);

// starts server or connects client to server
void check_args(const char *program, const Arguments *args);

#endif
