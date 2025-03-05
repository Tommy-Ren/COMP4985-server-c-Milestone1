#include "logging.h"
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

// current server_log calls have stdout as the fd until proper implementation with the server manager is done
void server_log(int fd, char const *msg, int lvl)
{
    switch(lvl)
    {
        case LOG_INFO:
            syslog(LOG_INFO, "INFO: %s", msg);
            break;
        case LOG_NOTICE:
            syslog(LOG_NOTICE, "NOTICE: %s", msg);
            break;
        case LOG_ERR:
            syslog(LOG_ERR, "ERROR: %s", msg);
            break;
        default:
            syslog(LOG_INFO, "[UNIMPLEMENTED]: %s", msg);
            printf("Unhandled log type, attempting to send to server manager at FD: %d?", fd);
    }
    //    write(fd, msg, strlen(msg)); this needs to be ASN and following the protocol rules
}
