#ifndef OVLE_CONFIG_H
#define OVLE_CONFIG_H

#include <limits.h> /* HOST_NAME_MAX */

#include "ovle_http.h"
#include "ovle_moodle.h"

#define OVLE_INT32_LEN  (sizeof "-2147483648" - 1)
#define OVLE_INT64_LEN  (sizeof "-9223372036854775808" - 1)

#define OVLE_OK      0
#define OVLE_ERROR  -1
#define OVLE_AGAIN  -2

struct ovle_buf {
    unsigned char *pos;
    unsigned char *last;

    unsigned char *start;
    unsigned char *end;

    int state;
};

extern int ip_addr_set;
extern int ovle_daemon_flag;
extern struct ovle_http_url u;
extern char username[MDL_USERNAME_MAX + 1];
extern char password[MDL_PASSWORD_MAX + 1];
extern char token[OVLE_MD5_HASH_LEN + 1];
extern char service[MDL_SERVICE_SHORTNAME_MAX + 1];

extern int ovle_read_config(void);

#endif  /* OVLE_CONFIG_H */
