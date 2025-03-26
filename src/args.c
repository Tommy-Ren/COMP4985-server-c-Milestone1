#include "../include/args.h"
#include "../include/network.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define OPTION_MESSAGE_LEN 50
#define IP_ADDRESS "127.0.0.1"
#define SERVER_MANAGER_IP "192.168.0.66"
#define PORT "8000"
#define SERVER_MANAGER_PORT "9000"

/* Initialize the socket and bind it to the specified port */
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
    fputs("  -A <address>, --address <address>  IP Address of the server manager.\n", stderr);
    fputs("  -P <port>,    --port <port>        PORT number of the server manager.\n", stderr);
    exit(exit_code);
}

/*
 * Parse command line arguments and populate the Arguments structure.
 *
 * argc: Number of command line arguments.
 * argv: Array of command line arguments.
 * args: Pointer to the Arguments structure to populate.
 */
void parse_args(int argc, char *argv[], Arguments *args)
{
    int opt;

    static struct option long_options[] = {
        {"address",                required_argument, NULL, 'a'},
        {"port",                   required_argument, NULL, 'p'},
        {"server manager address", required_argument, NULL, 'A'},
        {"server manager port",    required_argument, NULL, 'P'},
        {"help",                   no_argument,       NULL, 'h'},
        {NULL,                     0,                 NULL, 0  }
    };

    while((opt = getopt_long(argc, argv, "ha:p:A:P:", long_options, NULL)) != -1)
    {
        switch(opt)
        {
            case 'a':
                args->ip = optarg;
                break;
            case 'p':
                args->port = convert_port(argv[0], optarg);
                break;
            case 'A':
                args->sm_ip = optarg;
                break;
            case 'P':
                args->sm_port = convert_port(argv[0], optarg);
                break;
            case 'h':
                usage(argv[0], EXIT_SUCCESS, NULL);
            case '?':
                if(optopt != 'a' && optopt != 'p' && optopt != 'A' && optopt != 'P')
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

    // Apply defaults if no arguments were given
    if(args->ip == NULL)
    {
        args->ip = IP_ADDRESS;    // Default to 127.0.0.1
    }
    if(args->port == 0)
    {
        args->port = convert_port(argv[0], PORT);
    }
    if(args->sm_ip == NULL)
    {
        args->sm_ip = SERVER_MANAGER_IP;    // Default to 192.168.122.1 (DEMO only)
    }
    if(args->sm_port == 0)
    {
        args->sm_port = convert_port(argv[0], SERVER_MANAGER_PORT);
    }
}
