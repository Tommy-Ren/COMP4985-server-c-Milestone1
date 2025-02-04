
#include "../include/server.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define USAGE "Usage: -i ip -p port\n"

// struct to hold the values for
struct arguments
{
    char *ip;
    char *port;
};

// checks that arguments
struct arguments parse_args(int argc, char *argv[]);

// starts server or connects client to server
int handle_args(struct arguments args);

// drives code
int main(int argc, char *argv[])
{
    // Set Ctrl-C override.
    signal(SIGINT, sigintHandler);

    // handle args --> set up server on ip:port
    handle_args(parse_args(argc, argv));

    return EXIT_SUCCESS;
}

struct arguments parse_args(int argc, char *argv[])
{
    int              opt;
    struct arguments newArgs;

    // Initialize struct
    newArgs.ip   = NULL;
    newArgs.port = NULL;

    // Parse arguments
    while((opt = getopt(argc, argv, "i:p:")) != -1)
    {
        switch(opt)
        {
            case 'i':
                newArgs.ip = optarg;
                break;
            case 'p':
                newArgs.port = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -i ip -p port\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Check if required options are provided
    if(newArgs.ip == NULL || newArgs.port == NULL)
    {
        fprintf(stderr, "Usage: %s -t type -i ip -p port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    return newArgs;
}

int handle_args(struct arguments passedArgs)
{
    char *serverInformation[2];
    serverInformation[0] = passedArgs.ip;
    serverInformation[1] = passedArgs.port;
    printf("%s\n", passedArgs.ip);
    if(passedArgs.ip == NULL || passedArgs.port == NULL)
    {
        return 1;
    }

    server_setup(serverInformation);
    return 0;
}
