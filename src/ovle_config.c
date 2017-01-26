#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <limits.h>         /* HOST_NAME_MAX */
#include <strings.h>        /* strncasecmp() */
#include <unistd.h>

#include <arpa/inet.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"

#include "ovle_config.h"
#include "ovle_http.h"
#include "ovle_log.h"
#include "ovle_string.h"    /* ovle_strlcpy() */

int ovle_daemon_flag;

char url[OVLE_HTTP_URL_MAX];
struct ovle_http_url u;
char username[OVLE_UWI_STU_ID_LEN + 1];
char password[100];
char token[OVLE_MD5_HASH_LEN + 1];
char service[OVLE_MOODLE_SERVICE_NAME_LEN + 1];

struct ovle_parse {
    char *field_start;
    char *field_end;
    char *value_start;
    char *value_end;
};

static int ovle_parse_config_token(const char *buf, const char *buf_end, struct ovle_parse *c);

static int
ovle_parse_config_token(const char *buf, const char *buf_end, struct ovle_parse *c)
{
    unsigned char ch;
    const unsigned char *p;
    enum {
        sw_start,
        sw_field,
        sw_space_after_field,
        sw_space_before_value,
        sw_value
    } state;

    state = sw_start;

    for (p = buf; ; p++) {
        ch = *p;

        if (p >= (unsigned char *) buf_end)
            return 3;

        switch (state) {
            /* first char */
            case sw_start:
                c->field_start = (char *) p;
                state = sw_field;
                break;

            case sw_field:
                switch (ch) {
                    case ' ':
                        c->field_end = (char *) p;
                        state = sw_space_after_field;
                        break;

                    case '=':
                        c->field_end = (char *) p;
                        state = sw_space_before_value;
                        break;

                    case ';':
                        return -1;
                }

                break;

            case sw_space_after_field:
                switch (ch) {
                    case ' ':
                        break;

                    case ';':
                        return -1;

                    default:
                        c->value_start = (char *) p;
                        state = sw_value;
                        break;
                }

                break;

            case sw_space_before_value:
                switch (ch) {
                    case ' ':
                        break;

                    case ';':
                        return -1;

                    default:
                        c->value_start = (char *) p;
                        state = sw_value;
                        break;
                }

                break;

            case sw_value:
                switch (ch) {
                    case ';':
                        c->value_end = (char *) p;
                        goto done;
                }

                break;
        }
    }

done:

    return 0;
}

int
ovle_read_config(void)
{
    int fd;
    int rv;
    char *p;
    off_t file_size;
    ssize_t bytes;
    size_t field_len, val_len;
    char *field, *value;
    char buf[BUFSIZ];
    struct ovle_parse c;
    char *home_path;
    char conf_file_path[PATH_MAX];
    struct stat file_info;

    home_path = getenv("HOME");
    if (home_path == NULL)
        return -1;

    (void) snprintf(conf_file_path, sizeof conf_file_path, "%s/." PACKAGE_NAME, home_path);

    fd = open(conf_file_path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "open() \"%s\" failed\n", conf_file_path);
        return -1;
    }

    if (fstat(fd, &file_info) == -1) {
        fprintf(stderr, "fstat() \"%s\" failed\n", conf_file_path);
        return -1;
    }

    file_size = file_info.st_size;

    if (file_size > sizeof buf) {
        fprintf(stderr, "config file too large\n");
        return -1;
    }

    bytes = read(fd, buf, sizeof buf);

    if (bytes == -1) {
        fprintf(stderr, "read() failed");

        if (close(fd) == -1)
            fprintf(stderr, "close() failed");

        return -1;
    }

    if (bytes != file_size) {
        fprintf(stderr, "read() returned only %ld of %ld bytes\n", bytes, file_size);
        return -1;
    }

    p = buf;

    for (;;) {
        rv = ovle_parse_config_token(p, buf + file_size, &c);

        if (rv == 3)
            break;

        if (rv == -1)
            return -1;

        if (rv == 0) {
            field_len = c.field_end - c.field_start;
            field = c.field_start;
            field[field_len] = '\0';

            val_len = c.value_end - c.value_start;
            value = c.value_start;
            value[val_len] = '\0';

            ovle_log_debug2("config line: %s = %s", field, value);

            if (strcmp(field, "username") == 0)
                (void) ovle_strlcpy(username, value, sizeof username);
            else if (strcmp(field, "password") == 0)
                (void) ovle_strlcpy(password, value, sizeof password);
            else if (strcmp(field, "service_shortname") == 0)
                (void) ovle_strlcpy(service, value, sizeof service);
            else if (strcmp(field, "token") == 0)
                (void) ovle_strlcpy(token, value, sizeof token);
            else if (strncasecmp(field, "URL", sizeof "URL" - 1) == 0) {
                (void) ovle_strlcpy(url, value, sizeof url);

                if (ovle_http_parse_url(url, &u) == -1)
                    return -1;
            } else if (strcmp(field, "ip_address") == 0) {
                if (inet_pton(AF_INET, value, &u.host_address) != 1)
                    return -1;
            }
        }

        p = c.value_end + 2;
    }

    ovle_daemon_flag = 0;

    if (close(fd) == -1)
        fprintf(stderr, "close() failed\n");

    return 0;
}
