#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>  /* pid_t */

#include "ovle_config.h"
#include "ovle_daemon.h"

extern pid_t ovle_pid;

int
ovle_daemon(void)
{
    int fd;

    switch (fork()) {
        case -1:
            fprintf(stderr, "fork() failed");
            return OVLE_ERROR;

        case 0:
            break;

        default:
            exit(0);
    }

    ovle_pid = getpid();

    if (setsid() == -1) {
        fprintf(stderr, "setsid() failed\n");
        return OVLE_ERROR;
    }

    (void) umask(0);   /* clear any inherited file mode creation mask */

    /* redirection of standard input streams to /dev/null */
    fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "open(\"/dev/null\") failed\n");
        return OVLE_ERROR;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        fprintf(stderr, "dup2(STDIN) failed\n");
        return OVLE_ERROR;
    }
    
    if (dup2(fd, STDOUT_FILENO) == -1) {
        fprintf(stderr, "dup2(STDOUT) failed\n");
        return OVLE_ERROR;
    }

    if (dup2(fd, STDERR_FILENO) == -1) {
        fprintf(stderr, "dup2(STDERR) failed\n");
        return OVLE_ERROR;
    }

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            fprintf(stderr, "close(\"/dev/null\") failed\n");
            return OVLE_ERROR;
        }
    }

    return OVLE_OK;
}
