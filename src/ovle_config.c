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

int ip_addr_set;

int ovle_daemon_flag;

char url[OVLE_HTTP_URL_MAX + 1];
struct ovle_http_url u;
char username[MDL_USERNAME_MAX + 1];
char password[MDL_PASSWORD_MAX + 1];
char token[OVLE_MD5_HASH_LEN + 1];
char service[MDL_SERVICE_SHORTNAME_MAX + 1];

struct ovle_parse {
    unsigned char *field_start;
    unsigned char *field_end;
    unsigned char *value_start;
    unsigned char *value_end;
};

static int ovle_parse_config_token(struct ovle_buf *buf, struct ovle_parse *c);

static int
ovle_parse_config_token(struct ovle_buf *buf, struct ovle_parse *c)
{
    unsigned char ch;
    unsigned char *p;
    enum {
        sw_start,
        sw_field,
        sw_whitespace_after_field,
        sw_whitespace_before_value,
        sw_value,
        sw_whitespace_after_value
    } state;

    state = sw_start;

    for (p = buf->pos; /* void */; p++) {
        ch = *p;

        /* config file finished */
        if (p >= buf->end)
            return 3;

        switch (state) {
            /* first char */
            case sw_start:
                if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'
                    || ch == ';')
                {
                    break;
                }

                c->field_start = p;
                state = sw_field;

                /* fall through */

            case sw_field:
                switch (ch) {
                    case ' ':   /* fall through */
                    case '\t':  /* fall through */
                    case '\r':  /* fall through */
                    case '\n':  /* fall through */
                        c->field_end = p;
                        state = sw_whitespace_after_field;
                        break;

                    case '=':
                        c->field_end = p;
                        state = sw_whitespace_before_value;
                        break;
                }

                break;

            case sw_whitespace_after_field:
                switch (ch) {
                    case ' ':   /* fall through */
                    case '\t':  /* fall through */
                    case '\r':  /* fall through */
                    case '\n':  /* fall through */
                        break;

                    case '=':
                        state = sw_whitespace_before_value;
                        break;

                    default:
                        return -1;
                }

                break;

            case sw_whitespace_before_value:
                switch (ch) {
                    case ' ':   /* fall through */
                    case '\t':  /* fall through */
                    case '\r':  /* fall through */
                    case '\n':  /* fall through */
                        break;

                    case ';':
                        return -1;

                    default:
                        c->value_start = p;
                        state = sw_value;
                        break;
                }

                break;

            case sw_value:
                switch (ch) {
                    case ' ':   /* fall through */
                    case '\t':  /* fall through */
                    case '\r':  /* fall through */
                    case '\n':  /* fall through */
                        c->value_end = p;
                        state = sw_whitespace_after_value;
                        break;

                    case ';':
                        c->value_end = p;
                        goto done;
                }

                break;

            case sw_whitespace_after_value:
                switch (ch) {
                    case ' ':   /* fall through */
                    case '\t':  /* fall through */
                    case '\r':  /* fall through */
                    case '\n':  /* fall through */
                        break;

                    case ';':
                        goto done;

                    default:
                        return -1;
                }

                break;
        }
    }

done:

    buf->pos = p + 1;

    return 0;
}

int
ovle_read_config(void)
{
    int fd;
    int rv;
    off_t file_size;
    ssize_t bytes;
    size_t field_len, val_len;
    char *field, *value;
    char buf[BUFSIZ];
    char *home_path;
    char conf_file_path[PATH_MAX];
    struct stat file_info;
    struct ovle_buf b;
    struct ovle_parse c;

    home_path = getenv("HOME");
    if (home_path == NULL)
        return -1;

    (void) snprintf(conf_file_path, sizeof conf_file_path, "%s/." PACKAGE_NAME, home_path);

    fd = open(conf_file_path, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "open() \"%s\" failed\n", conf_file_path);
        return OVLE_ERROR;
    }

    if (fstat(fd, &file_info) == -1) {
        fprintf(stderr, "fstat() \"%s\" failed\n", conf_file_path);
        goto failed;
    }

    file_size = file_info.st_size;

    if (file_size > sizeof buf) {
        fprintf(stderr, "config file too large\n");
        goto failed;
    }

    bytes = read(fd, buf, sizeof buf);

    if (bytes == -1) {
        fprintf(stderr, "read() failed");
        goto failed;
    }

    if (bytes != file_size) {
        fprintf(stderr, "read() returned only %ld of %ld bytes\n", bytes, file_size);
        goto failed;
    }

    b.start = buf;
    b.pos = buf;
    b.end = buf + file_size;

    for (;;) {
        rv = ovle_parse_config_token(&b, &c);

        if (rv == OVLE_OK) {
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
            else if (strncasecmp(field, "URL", sizeof "URL" - 1) == 0) {
                (void) ovle_strlcpy(url, value, sizeof url);

                if (ovle_http_parse_url(url, &u) == -1)
                    goto failed;
            } else if (strcmp(field, "ip_address") == 0) {
                if (inet_pton(AF_INET, value, &u.host_address) != 1)
                    goto failed;

                ip_addr_set = 1;
            }
        }

        if (rv == 3)
            break;

        if (rv == OVLE_ERROR)
            goto failed;
    }

    ovle_daemon_flag = 0;

    if (close(fd) == -1)
        fprintf(stderr, "close() failed\n");

    return OVLE_OK;

failed:

    if (close(fd) == -1)
        fprintf(stderr, "close() failed\n");

    return OVLE_ERROR;
}
