#include <ctype.h>
#include <errno.h>
#include <stdint.h>     /* UINT16_MAX */
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#include "ovle_config.h"
#include "ovle_http.h"
#include "ovle_log.h"

#define CR      '\r'
#define LF      '\n'
#define CRLF    "\r\n"

int
ovle_http_process_status_line(int fd, struct ovle_buf *b, int *statuscode)
{
    int rv;
    ssize_t bytes;

    rv = OVLE_AGAIN;

    for (;;) {
        if (rv == OVLE_AGAIN) {
            bytes = recv(fd, b->last, b->end - b->last, MSG_DONTWAIT);

            if (bytes == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;
                else
                    return OVLE_ERROR;
            }

            if (bytes == 0) {
                ovle_log_debug0("server prematurely closed connection");
                return OVLE_ERROR;
            }

            b->last += bytes;
        }

        rv = ovle_http_parse_status_line(b, statuscode);

        if (rv == OVLE_AGAIN) {
            if (b->last == b->end) {
                ovle_log_debug0("server sent a header that is too large");
                return OVLE_ERROR;
            }

            continue;
        }

        if (rv == OVLE_OK) {
            ovle_log_debug1("http status %d", *statuscode);

            return OVLE_OK;
        }

        if (rv == OVLE_ERROR)
            return OVLE_ERROR;
    }
}

int
ovle_http_process_response_headers(int fd, struct ovle_buf *b, int *content_length)
{
    int rv;
    char *field, *value;
    ssize_t bytes;
    size_t field_len, val_len;
    struct ovle_http_parse_header h;

    for (;;) {
        if (rv == OVLE_AGAIN) {
            bytes = recv(fd, b->last, b->end - b->last, MSG_DONTWAIT);

            if (bytes == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;
                else
                    return OVLE_ERROR;
            }

            if (bytes == 0) {
                ovle_log_debug0("server prematurely closed connection");
                return OVLE_ERROR;
            }

            b->last += bytes;
        }

        rv = ovle_http_parse_header_line(b, &h);

        if (rv == OVLE_AGAIN) {
             if (b->last == b->end) {
                ovle_log_debug0("server sent a header that is too large");
                return OVLE_ERROR;
            }

            continue;
        }

        if (rv == OVLE_OK) {
            field_len = h.field_end - h.field_start;
            field = h.field_start;
            field[field_len] = '\0';

            val_len = h.value_end - h.value_start;
            value = h.value_start;
            value[val_len] = '\0';

            if (strcmp(field, "Content-Length") == 0)
                *content_length = atoi(value);

            ovle_log_debug2("http header: \"%s: %s\"", field, value);

            continue;
        }

        if (rv == 3)
            return OVLE_OK;

        /* an error occured while parsing header line */

        ovle_log_debug0("server sent invalid header");

        return OVLE_ERROR;
    }
}

int
ovle_http_parse_status_line(struct ovle_buf *b, int *statuscode)
{
    unsigned char ch;
    unsigned char *p;
    enum {
        sw_start,
        sw_H,
        sw_HT,
        sw_HTT,
        sw_HTTP,
        sw_first_major_digit,
        sw_major_digit,
        sw_first_minor_digit,
        sw_minor_digit,
        sw_status_first_digit,
        sw_status_second_digit,
        sw_status_third_digit,
        sw_space_after_status,
        sw_status_text,
        sw_almost_done
    } state;

    state = b->state;

    for (p = b->pos; p < b->last; p++) {
        ch = *p;

        switch (state) {

             /* "HTTP/" */
            case sw_start:
                switch (ch) {
                    case 'H':
                        state = sw_H;
                        break;

                    default:
                        return OVLE_ERROR;
                }
                break;

            case sw_H:
                switch (ch) {
                    case 'T':
                        state = sw_HT;
                        break;

                    default:
                        return OVLE_ERROR;
                }
                break;

            case sw_HT:
                switch (ch) {
                    case 'T':
                        state = sw_HTT;
                        break;
                    
                    default:
                        return OVLE_ERROR;
                }
                break;

            case sw_HTT:
                switch (ch) {
                    case 'P':
                        state = sw_HTTP;
                        break;

                    default:
                        return OVLE_ERROR;
                }
                break;

            case sw_HTTP:
                switch (ch) {
                    case '/':
                        state = sw_first_major_digit;
                        break;

                    default:
                        return OVLE_ERROR;
                }
                break;

            /* the first digit of major HTTP version */
            case sw_first_major_digit:
                if (ch < '1' || ch > '9')
                    return OVLE_ERROR;

                state = sw_major_digit;
                break;

            /* the major HTTP version or dot */
            case sw_major_digit:
                if (ch == '.') {
                    state = sw_first_minor_digit;
                    break;
                }

                if (ch < '0' || ch > '9')
                    return OVLE_ERROR;

                break;

            /* the first digit of minor HTTP version */
            case sw_first_minor_digit:
                if (ch < '0' || ch > '9')
                    return OVLE_ERROR;

                state = sw_minor_digit;
                break;

            /* the minor HTTP version or the end of the request line */
            case sw_minor_digit:
                if (ch == ' ') {
                    state = sw_status_first_digit;
                    break;
                }

                if (ch < '0' || ch > '9')
                    return OVLE_ERROR;

                break;

            /* HTTP status code */

            case sw_status_first_digit:
                if (ch < '0' || ch > '9')
                    return OVLE_ERROR;

                *statuscode = ch - '0';

                state = sw_status_second_digit;
                break;

            case sw_status_second_digit:
                if (ch < '0' || ch > '9')
                    return OVLE_ERROR;

                *statuscode = *statuscode * 10 + ch - '0';

                state = sw_status_third_digit;
                break;

            case sw_status_third_digit:
                if (ch < '0' || ch > '9')
                    return OVLE_ERROR;

                *statuscode = *statuscode * 10 + ch - '0';

                state = sw_space_after_status;
                break;

            /* space or end of line */
            case sw_space_after_status:
                switch (ch) {
                    case ' ':
                        state = sw_status_text;
                        break;

                    case CR:
                        state = sw_almost_done;
                        break;

                    case LF:
                        goto done;

                    default:
                        return OVLE_ERROR;
                }
                break;

            /* any text until end of line */
            case sw_status_text:
                switch (ch) {
                    case CR:
                        state = sw_almost_done;
                        break;

                    case LF:
                        goto done;
                }
                break;

            /* end of status line */
            case sw_almost_done:
                switch (ch) {
                    case LF:
                        goto done;

                    default:
                        return OVLE_ERROR;
                }
        }
    }

    b->pos = p;
    b->state = state;

    return OVLE_AGAIN;

done:

    b->pos = p + 1;
    b->state = sw_start;

    return OVLE_OK;
}

int
ovle_http_parse_header_line(struct ovle_buf *b, struct ovle_http_parse_header *h)
{
    unsigned char ch;
    unsigned char *p;
    enum {
        sw_start,
        sw_name,
        sw_space_before_value,
        sw_value,
        sw_space_after_value,
        sw_almost_done,
        sw_header_almost_done
    } state;

    state = b->state;

    for (p = b->pos; p < b->last; p++) {
        ch = *p;

        switch (state) {
            case sw_start:
                h->field_start = p;

                switch (ch) {
                    case CR:
                        h->field_end = p;
                        state = sw_header_almost_done;
                        break;

                    case LF:
                        h->field_end = p;
                        goto header_done;

                    default:
                        state = sw_name;
                        break;
                }

                break;

            case sw_name:
                if (ch == ':') {
                    h->field_end = p;
                    state = sw_space_before_value;
                    break;
                }

                if (ch == CR) {
                    h->field_end = p;
                    h->value_start = p;
                    h->value_end = p;
                    state = sw_almost_done;
                    break;
                }

                if (ch == LF) {
                    h->field_end = p;
                    h->value_start = p;
                    h->value_end = p;
                    goto done;
                }

                break;

            case sw_space_before_value:
                switch (ch) {
                    case ' ':
                        break;

                    case CR:
                        h->value_start = p;
                        h->value_end = p;
                        state = sw_almost_done;
                        break;

                    case LF:
                        h->value_start = p;
                        h->value_end = p;
                        goto done;

                    default:
                        h->value_start = p;
                        state = sw_value;
                        break;
                }

                break;

            case sw_value:
                switch (ch) {
                    case ' ':
                        h->value_end = p;
                        state = sw_space_after_value;
                        break;

                    case CR:
                        h->value_end = p;
                        state = sw_almost_done;
                        break;

                    case LF:
                        h->value_end = p;
                        goto done;
                }

                break;

            /* space* before end of header line */
            case sw_space_after_value:
                switch (ch) {
                    case ' ':
                        break;
                    
                    case CR:
                        state = sw_almost_done;
                        break;

                    case LF:
                        goto done;

                    default:
                        state = sw_value;
                        break;
                }

                break;

            /* end of header line */
            case sw_almost_done:
                switch (ch) {
                    case LF:
                        goto done;

                    case CR:
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            /* end of header */
            case sw_header_almost_done:
                switch (ch) {
                    case LF:
                        goto header_done;

                    default:
                        return OVLE_ERROR;
                }
        }
    }

    b->pos = p;
    b->state = state;

    return OVLE_AGAIN;

done:

    b->pos = p + 1;
    b->state = sw_start;

    return OVLE_OK;

header_done:

    b->pos = p + 1;
    b->state = sw_start;

    return 3;
}

int
ovle_http_parse_url(const char *url, struct ovle_http_url *u)
{
    unsigned char c, ch;
    unsigned char *p;
    enum {
        sw_start,
        sw_scheme_h,
        sw_scheme_ht,
        sw_scheme_htt,
        sw_scheme_http,
        sw_scheme_https,
        sw_scheme_slash,
        sw_scheme_slash_slash,
        sw_host_start,
        sw_host,
        sw_host_end,
        sw_port_start,
        sw_port
    } state;

    state = sw_start;

    for (p = (unsigned char * ) url; 1/* TODO */; p++) {
        ch = *p;

        switch (state) {
            case sw_start:
                switch (ch) {
                    /*
                     * RFC 1738 Section 2.1 page 1:
                     * "For resiliency, programs interpreting URLs should treat
                     * upper case letters as equivalent to lower case in scheme
                     * names"
                     */
                    case 'H':   /* fall through */
                    case 'h':
                        state = sw_scheme_h;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_scheme_h:
                switch (ch) {
                    case 'T':   /* fall through */
                    case 't':
                        state = sw_scheme_ht;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_scheme_ht:
                switch (ch) {
                    case 'T':   /* fall through */
                    case 't':
                        state = sw_scheme_htt;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_scheme_htt:
                switch (ch) {
                    case 'P':   /* fall through */
                    case 'p':
                        state = sw_scheme_http;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_scheme_http:
                switch (ch) {
                    case ':':
                        u->https = 0;
                        state = sw_scheme_slash;
                        break;

                    case 'S':   /* fall through */
                    case 's':
                        u->https = 1;
                        state = sw_scheme_https;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_scheme_https:
                switch (ch) {
                    case ':':
                        state = sw_scheme_slash;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            /*
             * RFC 1738 Section 3.1 page 4:
             * 'The scheme specific data start with a double slash "//"...'
             */

            case sw_scheme_slash:
                switch (ch) {
                    case '/':
                        state = sw_scheme_slash_slash;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_scheme_slash_slash:
                switch (ch) {
                    case '/':
                        state = sw_host_start;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_host_start:
                u->host_start = p;
                state = sw_host;

                /* fall through */

            case sw_host:
                c = (unsigned char) (ch | 0x20);
                if (c >= 'a' && c <= 'z')
                    break;

                if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-')
                    break;

                /*
                 * TODO:
                 * validate
                 * break after host == HOST_NAME_MAX - 1
                 */

                /* fall through */

            case sw_host_end:
                u->host_end = p;

                switch (ch) {
                    case ':':
                        state = sw_port_start;
                        break;

                    case '\0':
                        /*
                         * RFC 1738 Section 3.3 page 8:
                         * "If neither <path> nor <searchpart> is present, the
                         * "/" may also be omitted."
                         */

                        /* fall through */

                    case '/':
                        u->port_start = p;
                        u->port_end = p;
                        u->port = u->https ? 443 : 80; /* http port defaults */
                        goto done;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_port_start:
                u->port_start = p;
                u->port = ch - '0';
                state = sw_port;

                /* fall through */

            case sw_port:
                /*
                 * RFC 1738 Section 3.1 page 5:
                 * "Another port number may optionally be supplied, in
                 * decimal..."
                 */
                if (ch >= '0' && ch <= '9') {
                    u->port = u->port * 10 + ch - '0';
                    break;
                }

                if (u->port > UINT16_MAX)
                    return OVLE_ERROR;

                switch (ch) {
                    case '\0':  /* fall through */
                    case '/':
                        u->port_end = p;
                        goto done;

                    default:
                        return OVLE_ERROR;
                }

                break;
        }
    }

done:

    return OVLE_OK;
}
