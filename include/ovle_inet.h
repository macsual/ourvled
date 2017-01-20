#ifndef OVLE_INET_H
#define OVLE_INET_H

#include <netinet/in.h>     /* in_port_t */

#include "ovle_http.h"

extern int ovle_open_connection(const char *url);

#endif  /* OVLE_INET_H */
