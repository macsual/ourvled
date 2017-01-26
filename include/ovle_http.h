#ifndef OVLE_HTTP_H
#define OVLE_HTTP_H

#include "ovle_inet.h"

#define CR      '\r'
#define LF      '\n'
#define CRLF    "\r\n"

struct ovle_http_parse_header {
    char *field_start;
    char *field_end;
    char *value_start;
    char *value_end;
};

/* RFC 1738 */
extern int ovle_http_parse_url(const char *url, struct ovle_http_url *u);
extern int ovle_http_parse_status_line(char *buf, char **p, int *statuscode);
extern int ovle_http_parse_header_line(char *buf, struct ovle_http_parse_header *h);

#endif  /* OVLE_HTTP_H */
