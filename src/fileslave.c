#include "config.h"
#include "fileslave.h"
#include "alloc.h"

#include <sys/poll.h>
#include <errno.h>
#include <signal.h>

static char logname[FILESLAVE_MAX_FNAME + 1];
static char realname[FILESLAVE_MAX_FNAME + 1];
static char logdata[FILESLAVE_MAX_STRING + 1];
static char response[FILESLAVE_MAX_STRING + 1];
static dbref player;

pid_t parent_pid;

void do_logging()
{
    FILE *logfile;
    static int len;

    memset(logname, '\0', FILESLAVE_MAX_FNAME + 1);
    memset(realname, '\0', FILESLAVE_MAX_FNAME + 1);
    memset(logdata, '\0', FILESLAVE_MAX_STRING + 1);
    memset(response, '\0', FILESLAVE_MAX_STRING + 1);

    if (read(STDIN_FILENO, &len, sizeof(int)) <= 0)
        return;
    
    if (len > 0)
        if (read(STDIN_FILENO, logname, len) <= 0)
            return;

    if (read(STDIN_FILENO, &len, sizeof(int)) <= 0)
        return;
    
    if (len > 0)
        if (read(STDIN_FILENO, logdata, len) <= 0)
            return;

    if (read(STDIN_FILENO, &player, sizeof(dbref)) <= 0)
        return;

    snprintf(realname, FILESLAVE_MAX_FNAME, "logs/%s", logname);
    strcat(logdata, "\n");
    
    if ((logfile = fopen(realname, "a")) == NULL)
        snprintf(response, FILESLAVE_MAX_STRING, "Error opening file in append mode.");
    else
        if (fwrite(logdata, sizeof(char), strlen(logdata), logfile) < strlen(logdata))
            snprintf(response, FILESLAVE_MAX_STRING, "Error writing data to file. Not all data may be written.");
        else
            snprintf(response, FILESLAVE_MAX_STRING, "Log written.");
    if (logfile != NULL)
        fclose(logfile);

    if (write(STDOUT_FILENO, &player, sizeof(dbref)) <= 0)
        return;

    len = strlen(response);
    if (write(STDOUT_FILENO, &len, sizeof(int)) <= 0)
        return;
    if (len > 0)
        if (write(STDOUT_FILENO, response, len) <= 0)
            return;
    return;
}

int main(int argc, char *argv[]) {
    struct pollfd item;

    parent_pid = getppid();
    if (parent_pid == 1)
        exit(1);

    signal(SIGPIPE, SIG_DFL);

    item.fd = STDIN_FILENO;
    item.events = (POLLIN|POLLERR|POLLHUP|POLLNVAL);
    item.revents = 0;

    while (1) {
        if (parent_pid != getppid())
            exit(2);
        switch (poll(&item, 1, -1)) {
            case -1:
                if (errno == EINTR)
                    continue;
                exit(3);
            case 0:
                continue;
            default:
                if (item.revents & (POLLERR|POLLNVAL|POLLHUP))
                    exit(4);
                if (!(item.revents & POLLIN))
                    continue;
                do_logging();
                continue;
        }
    }
    exit(0);
}

