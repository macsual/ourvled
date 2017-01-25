#include <ctype.h>
#include <stdint.h>     /* UINT16_MAX */

#include "ovle_http.h"

#define CR      '\r'
#define LF      '\n'
#define CRLF    "\r\n"

int
ovle_http_parse_status_line(char *buf, char **ptr)
{
    int count = 0;
    int ch;
    char *p;
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
        sw_status,
        sw_space_after_status,
        sw_status_text,
        sw_almost_done
    } state;

    state = sw_start;

    for (p = buf; p; p++) {
        ch = *p;

        switch (state) {

             /* "HTTP/" */
            case sw_start:
                switch (ch) {
                    case 'H':
                        state = sw_H;
                        break;

                    default:
                        return -1;
                }
                break;

            case sw_H:
                switch (ch) {
                    case 'T':
                        state = sw_HT;
                        break;

                    default:
                        return -1;
                }
                break;

            case sw_HT:
                switch (ch) {
                    case 'T':
                        state = sw_HTT;
                        break;
                    
                    default:
                        return -1;
                }
                break;

            case sw_HTT:
                switch (ch) {
                    case 'P':
                        state = sw_HTTP;
                        break;

                    default:
                        return -1;
                }
                break;

            case sw_HTTP:
                switch (ch) {
                    case '/':
                        state = sw_first_major_digit;
                        break;

                    default:
                        return -1;
                }
                break;

            /* the first digit of major HTTP version */
            case sw_first_major_digit:
                if (ch < '1' || ch > '9')
                    return -1;

                state = sw_major_digit;
                break;

            /* the major HTTP version or dot */
            case sw_major_digit:
                if (ch == '.') {
                    state = sw_first_minor_digit;
                    break;
                }

                if (ch < '0' || ch > '9')
                    return -1;

                break;

            /* the first digit of minor HTTP version */
            case sw_first_minor_digit:
                if (ch < '0' || ch > '9')
                    return -1;

                state = sw_minor_digit;
                break;

            /* the minor HTTP version or the end of the request line */
            case sw_minor_digit:
                if (ch == ' ') {
                    state = sw_status;
                    break;
                }

                if (ch < '0' || ch > '9')
                    return -1;

                break;

            /* HTTP status code */
            case sw_status:
                if (ch == ' ')
                    break;

                if (ch < '0' || ch > '9')
                    return -1;

                if (++count == 3)
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
                        return -1;
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
                        return -1;
                }
        }
    }

done:

    *ptr = p + 1;

    return 0;
}

int
ovle_http_parse_header_line(char *buf, struct ovle_http_parse_header *h)
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

    state = sw_start;

    for (p = buf; p; p++) {
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

                        if (ch == '\0')
                            return -1;

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

                if (ch == '\0')
                    return -1;

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

                    case '\0':
                        return -1;

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

                    case '\0':
                        return -1;
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

                    case '\0':
                        return -1;

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
                        return -1;
                }
                break;

            /* end of header */
            case sw_header_almost_done:
                switch (ch) {
                    case LF:
                        goto header_done;

                    default:
                        return -1;
                }
        }
    }

done:
    return 0;

header_done:

    return 3;
}

int
ovle_http_parse_url(const char *url, struct ovle_http_url *u)
{
    unsigned char c, ch;
    const unsigned char *p;
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

    for (p = url; 1/* TODO */; p++) {
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
                        return -1;
                }

                break;

            case sw_scheme_h:
                switch (ch) {
                    case 'T':   /* fall through */
                    case 't':
                        state = sw_scheme_ht;
                        break;

                    default:
                        return -1;
                }

                break;

            case sw_scheme_ht:
                switch (ch) {
                    case 'T':   /* fall through */
                    case 't':
                        state = sw_scheme_htt;
                        break;

                    default:
                        return -1;
                }

                break;

            case sw_scheme_htt:
                switch (ch) {
                    case 'P':   /* fall through */
                    case 'p':
                        state = sw_scheme_http;
                        break;

                    default:
                        return -1;
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
                        return -1;
                }

                break;

            case sw_scheme_https:
                switch (ch) {
                    case ':':
                        state = sw_scheme_slash;
                        break;

                    default:
                        return -1;
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
                        return -1;
                }

                break;

            case sw_scheme_slash_slash:
                switch (ch) {
                    case '/':
                        state = sw_host_start;
                        break;

                    default:
                        return -1;
                }

                break;

            case sw_host_start:
                u->host_start = (char *)p;
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
                u->host_end = (char *)p;

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
                        u->port_start = (char *)p;
                        u->port_end = (char *)p;
                        u->port = u->https ? 443 : 80; /* http port defaults */
                        goto done;

                    default:
                        return -1;
                }

                break;

            case sw_port_start:
                u->port_start = (char *)p;
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
                    return -1;

                switch (ch) {
                    case '\0':  /* fall through */
                    case '/':
                        u->port_end = (char *)p;
                        goto done;

                    default:
                        return -1;
                }

                break;
        }
    }

done:

    return 0;
}
