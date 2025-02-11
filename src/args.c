#include "../include/args.h"
#include "../include/network.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define OPTION_MESSAGE_LEN 50

_Noreturn void usage(const char *app_name, int exit_code, const char *message)
{
    if(message)
    {
        fprintf(stderr, "%s\n", message);
    }

    fprintf(stderr, "Usage: %s [-h] -a <address> -p <port>\n", app_name);
    fputs("Options:\n", stderr);
    fputs("  -h, --help                         Display this help message\n", stderr);
    fputs("  -a <address>, --address <address>  IP Address of the server.\n", stderr);
    fputs("  -p <port>,    --port <port>        PORT number of the server.\n", stderr);
    exit(exit_code);
}

void parse_args(Arguments *args, int argc, char *argv[])
{
    int opt;
    int err;

    static struct option long_options[] = {
        {"address", required_argument, NULL, 'a'},
        {"port",    required_argument, NULL, 'p'},
        {"help",    no_argument,       NULL, 'h'},
        {NULL,      0,                 NULL, 0  }
    };

    // Parse arguments
    while((opt = getopt_long(argc, argv, "ha:p:", long_options, NULL)) != -1)
    {
        switch(opt)
        {
            case 'a':
                args->ip = optarg;
                break;
            case 'p':
                args->port = convert_port(optarg, &err);
                if(err != 0)
                {
                    usage(argv[0], EXIT_FAILURE, "Port must be between 1 and 65535");
                }
                break;
                case 'h':
                usage(argv[0], EXIT_SUCCESS, NULL);
            case '?':
                if(optopt != 'a' && optopt != 'p')
                {
                    char message[OPTION_MESSAGE_LEN];

                    snprintf(message, sizeof(message), "Unknown option '-%c'.", optopt);
                    usage(argv[0], EXIT_FAILURE, message);
                }
                break;
            default:
                usage(argv[0], EXIT_FAILURE, NULL);
        }
    }
}

int check_args(const char *binary_name, const Arguments *args)
{
    if (args->ip == NULL)
    {
        usage(binary_name, EXIT_FAILURE, "Missing ipv4 address.");
    }
    if (args->port == NULL)
    {
        usage(binary_name, EXIT_FAILURE, "Missing port number.");
    }
}
