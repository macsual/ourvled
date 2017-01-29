#ifndef OVLE_CONFIG_H
#define OVLE_CONFIG_H

#include <limits.h> /* HOST_NAME_MAX */

#include "ovle_http.h"
#include "ovle_moodle.h"

/* use smallest signed 64 bit long because of the minus sign */
#define INT64_LEN   (sizeof "-9223372036854775808" - 1)
#define UINT16_LEN  (sizeof "65535" - 1)

/*
 * Maximum length of a HTTP URL (without path data) not including the
 * terminating null.
 *
 * The host part of a URL's scheme specific part can either be a FQDN or an
 * IP address (RFC 1738 Section 3.1 page 5).  Among the maximum lengths of
 * FQDN, IPv4, and IPv6 (including IPv4-mapped) addresses with port numbers in
 * a URL, FDQN is longest.
 */
#define OVLE_HTTP_URL_MAX   HOST_NAME_MAX + (sizeof "https://:65535/" - 1)

struct ovle_buf {
    unsigned char *start;
    unsigned char *pos;
    unsigned char *end;
};

extern int ip_addr_set;
extern int ovle_daemon_flag;
extern char url[OVLE_HTTP_URL_MAX + 1];
extern struct ovle_http_url u;
extern char username[MDL_USERNAME_MAX + 1];
extern char password[MDL_PASSWORD_MAX + 1];
extern char token[OVLE_MD5_HASH_LEN + 1];
extern char service[MDL_SERVICE_SHORTNAME_MAX + 1];

extern int ovle_read_config(void);

#endif  /* OVLE_CONFIG_H */
