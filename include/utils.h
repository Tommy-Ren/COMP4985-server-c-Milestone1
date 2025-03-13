#ifndef UTILS_H
#define UTILS_H

#include <signal.h>

extern volatile sig_atomic_t server_running;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables,-warnings-as-errors)

void sfree(void **ptr);

void setup_signal_handler(void);

#endif
