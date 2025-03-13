#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__linux__) && defined(__clang__)
_Pragma("clang diagnostic ignored \"-Wdisabled-macro-expansion\"")
#endif

#define SIG_SIZE 64

    volatile sig_atomic_t server_running = 1;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors)

static void sigint_handler(int sig);

void sfree(void **ptr)
{
    if(ptr != NULL && *ptr != NULL)
    {
        free(*ptr);
        *ptr = NULL;
    }
}

void setup_signal_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = sigint_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void sigint_handler(int sig)
{
    char message[SIG_SIZE];

    snprintf(message, sizeof(message), "Caught signal: %d (%s)\n", sig, strsignal(sig));
    write(STDOUT_FILENO, message, strlen(message));

    if(sig == SIGINT)
    {
        server_running = 0;
        snprintf(message, sizeof(message), "%s", "Shutting down...\n");
    }
    write(STDOUT_FILENO, message, strlen(message));
}

#pragma GCC diagnostic pop
