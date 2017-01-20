#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <limits.h>         /* HOST_NAME_MAX */
#include <strings.h>        /* strncasecmp() */
#include <unistd.h>

#include <sys/stat.h>

#include "ovle_config.h"
#include "ovle_http.h"
#include "ovle_log.h"
#include "ovle_string.h"    /* ovle_strlcpy() */

int ovle_daemon_flag;

char url[OVLE_HTTP_URL_MAX];
char username[OVLE_UWI_STU_ID_LEN + 1];
char password[OVLE_DOB_LEN + 1];
char token[OVLE_MD5_HASH_LEN + 1];
char service[OVLE_MOODLE_SERVICE_NAME_LEN + 1];

struct ovle_parse {
    char *field_start;
    char *field_end;
    char *value_start;
    char *value_end;
};

static int ovle_parse_config_line(const char *buf, struct ovle_parse *c);

static int
ovle_parse_config_line(const char *buf, struct ovle_parse *c)
{
    unsigned char ch;
    const unsigned char *p;
    enum {
        sw_start,
        sw_field,
        sw_space_before_value,
        sw_value,
    } state;

    state = sw_start;

    for (p = buf; /* TODO */; p++) {
        ch = *p;

        switch (state) {
            /* first char */
            case sw_start:
                c->field_start = (char *)p;

                switch (ch) {
                    case ';':
                        c->field_end = (char *)p;
                        return 3;

                    default:
                        state = sw_field;

                        if (ch == '\0')
                            return -1;

                        break;
                }
                break;

            case sw_field:
                if (ch == '=') {
                    c->field_end = (char *)p;
                    state = sw_space_before_value;
                    break;
                }

                if (ch == ';') {
                    c->field_end = (char *)p;
                    c->value_start = (char *)p;
                    c->value_end = (char *)p;
                    goto done;
                }

                if (ch == '\0')
                    return -1;

                break;

            case sw_space_before_value:
                switch (ch) {
                    case ' ':
                        break;

                    case ';':
                        c->value_start = (char *)p;
                        c->value_end = (char *)p;
                        goto done;

                    case '\0':
                        return -1;

                    default:
                        c->value_start = (char *)p;
                        state = sw_value;
                        break;
                }
                break;

            case sw_value:
                switch (ch) {
                    case ' ':
                        break;

                    case ';':
                        c->value_end = (char *)p;
                        goto done;

                    case '\0':
                        return -1;
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
    ssize_t bytes;
    size_t field_len, val_len;
    char *field, *value;
    char buf[BUFSIZ];
    struct ovle_parse c;
    struct ovle_http_url u;

    fd = open("/etc/ourvle.conf", O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "failed to parse config file\n");
        return -1;
    }

    bytes = read(fd, buf, sizeof buf);

    if (bytes == -1) {
        fprintf(stderr, "read() failed");

        if (close(fd) == -1)
            fprintf(stderr, "close() failed");

        return -1;
    }

    p = buf;

    for (;;) {
        rv = ovle_parse_config_line(p, &c);

        if (rv == 3)
            break;

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
            else if (strcmp(field, "service") == 0)
                (void) ovle_strlcpy(service, value, sizeof service);
            else if (strcmp(field, "token") == 0)
                (void) ovle_strlcpy(token, value, sizeof token);
            else if (strncasecmp(field, "URL", sizeof "URL" - 1) == 0)
                (void) ovle_strlcpy(url, value, sizeof url);
        }

        p = c.value_end + 2;
    }

    ovle_daemon_flag = 0;

    if (close(fd) == -1)
        fprintf(stderr, "close() failed\n");

    return 0;
}
