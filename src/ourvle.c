#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>      /* ssize_t */

#include "config.h"

#include "ovle_config.h"
#include "ovle_daemon.h"
#include "ovle_json.h"
#include "ovle_log.h"
#include "ovle_moodle.h"
#include "ovle_http.h"
#include "ovle_string.h"

pid_t ovle_pid;

static int ovle_show_help;
static int ovle_show_version;

static int ovle_get_options(int argc, char *argv[]);
static int ovle_create_pidfile(const char *name);
static void ovle_show_help_text(void);
static void ovle_show_version_info(void);

static int
ovle_get_options(int argc, char *argv[])
{
    int                     c, opt_index;
    static struct option    long_options[] = {
        {"version", no_argument, &ovle_show_version, 1},
        {"help", no_argument, &ovle_show_help, 1},
        {NULL, 0, NULL, 0}
    };

    for (;;) {
        opt_index = 0;

        c = getopt_long(argc, argv, "hvV", long_options, &opt_index);
        if (c == -1)
            break;

        switch (c) {
            case 0:
                if (long_options[opt_index].flag
                        && (ovle_show_help || ovle_show_version))
                {
                    return OVLE_OK;
                }

                break;

            case 'h':
                ovle_show_help = 1;
                return OVLE_OK;

            case 'v':   /* fall through */
            case 'V':
                ovle_show_version = 1;
                return OVLE_OK;

            case '?':
                return OVLE_ERROR;

            default:
                abort();
        }
    }

    return OVLE_OK;
}

static void
ovle_show_help_text(void)
{
#define USAGE   "Usage: " PACKAGE_NAME " [-hvV]\n"                            \
                "\n"                                                          \
                "Options:\n"                                                  \
                "  -h,  --help                 show this help and exit\n"     \
                "  -v,  --version              show version and exit\n"

    (void) write(STDERR_FILENO, USAGE, sizeof USAGE - 1);

#undef USAGE
}

static void
ovle_show_version_info(void)
{
#define VER_INFO    PACKAGE_STRING "\n"                                       \
                    "Copyright (C) 2017.\n\n"                                 \
                    "Written by Romario Maxwell.\n"

    (void) write(STDERR_FILENO, VER_INFO, sizeof VER_INFO - 1);

#undef VER_INFO
}


static int
ovle_create_pidfile(const char *name)
{
    int     fd, len;
    char    pid[INT64_LEN + 2];    /* 2 is for newline & null-character */

    fd = open(name, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        fprintf(stderr, "open() of \"%s\" failed\n", name);
        return OVLE_ERROR;
    }

    /*
     * FHS says, "The file must consist of the process identifier in
     * ASCII-encoded decimal, followed by a newline character."
     */
    len = snprintf(pid, sizeof (pid_t), "%ld\n", (long) ovle_pid);
    if (write(fd, pid, len) != len) {
        fprintf(stderr, "write() to \"%s\" failed\n", name);

        if (close(fd) == -1)
            fprintf(stderr, "close() on \"%s\" failed\n", name);

        return OVLE_ERROR;
    }

    if (close(fd) == -1)
        fprintf(stderr, "close() on \"%s\" failed\n", name);

    return OVLE_OK;
}

int
main(int argc, char *argv[])
{
    int ourvle_fd;
    char userid[OVLE_INT32_LEN + 1];

    if (ovle_get_options(argc, argv) == OVLE_ERROR)
        return 0;

    if (ovle_show_version) {
        ovle_show_version_info();
        return 0;
    }

    if (ovle_show_help) {
        ovle_show_help_text();
        return 0;
    }

    ovle_pid = getpid();

    if (ovle_read_config() == OVLE_ERROR)
        return 1;

    if (ovle_daemon_flag) {
        if (ovle_daemon() == OVLE_ERROR)
            return 1;
    }

    /*
     * we do not create the pid file until after daemonising because we need to
     * write the demonised process pid
     */
    if (ovle_create_pidfile("ourvle.pid") == OVLE_ERROR)
        return 1;

    for (;;) {
        ourvle_fd = ovle_http_open_connection(&u);
        if (ourvle_fd == OVLE_ERROR) {
            fprintf(stderr, "failed to open HTTP connection\n");
            return 1;
        }

        if (ovle_mdl_get_token(ourvle_fd, token) == OVLE_ERROR) {
            fprintf(stderr, "failed to get Moodle user token\n");
            return 1;
        }

        if (ovle_mdl_get_userid(ourvle_fd, token, userid) == OVLE_ERROR) {
            fprintf(stderr, "failed to get Moodle userid\n");
            return 1;
        }

        if (ovle_mdl_sync_course_content(ourvle_fd, token, userid) == OVLE_ERROR) {
            fprintf(stderr, "failed to sync Moodle course content\n");
            return 1;
        }

        if (close(ourvle_fd) == -1)
            fprintf(stderr, "close() failed\n");
    }

    return 0;
}
