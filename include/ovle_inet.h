#ifndef OVLE_INET_H
#define OVLE_INET_H

#include <limits.h>         /* HOST_NAME_MAX */

#include <netinet/in.h>     /* struct in_addr, in_port_t */

struct ovle_http_url {
    char host[HOST_NAME_MAX + 1];
    struct in_addr host_address;
    int https;
    in_port_t port;
};

extern int ovle_http_open_connection(struct ovle_http_url *u);

#endif  /* OVLE_INET_H */
