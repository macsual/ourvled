#ifndef OVLE_HTTP_H
#define OVLE_HTTP_H

#include "ovle_inet.h"

#define CR      '\r'
#define LF      '\n'
#define CRLF    "\r\n"

#define OVLE_HTTP_STATUS_OK     200

struct ovle_buf;

struct ovle_http_parse_header {
    unsigned char *field_start;
    unsigned char *field_end;
    unsigned char *value_start;
    unsigned char *value_end;
};

extern int ovle_http_process_status_line(int fd, struct ovle_buf *b, int *statuscode);
extern int ovle_http_process_response_headers(int fd, struct ovle_buf *b, int *content_length);

/* RFC 1738 */
extern int ovle_http_parse_url(const char *buf, struct ovle_http_url *u);
extern int ovle_http_parse_status_line(struct ovle_buf *b, int *statuscode);
extern int ovle_http_parse_header_line(struct ovle_buf *b, struct ovle_http_parse_header *h);

#endif  /* OVLE_HTTP_H */
