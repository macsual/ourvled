#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <netdb.h>
#include <unistd.h>

#include <arpa/inet.h>      /* struct sockaddr_in, htons(), inet_pton() */

#include <netinet/in.h>     /* IPPROTO_TCP */
#include <netinet/tcp.h>    /* TCP_NODELAY */

#include <sys/socket.h>     /* AF_INET */
#include <sys/types.h>

#include "ovle_config.h"
#include "ovle_http.h"
#include "ovle_log.h"
#include "ovle_string.h"

int
ovle_http_open_connection(struct ovle_http_url *u)
{
    int fd;
    const int tcp_nodelay = 1;
    size_t host_len;
    char host[HOST_NAME_MAX];
    struct addrinfo hints;
    struct addrinfo *p, *servinfo;
    struct sockaddr_in addr;

    if (1 /* TODO */) {
        addr.sin_family = AF_INET;  /* OurVLE doesn't support IPv6 */
        addr.sin_addr = u->host_address;
        addr.sin_port = htons(u->port);

#if defined linux && defined SOCK_CLOEXEC
        fd = socket(AF_INET, SOCK_CLOEXEC | SOCK_STREAM, IPPROTO_TCP);
#else
        fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif

        if (fd == -1) {
            fprintf(stderr, "socket() failed\n");
            return -1;
        }

        if (connect(fd, (struct sockaddr *) &addr, sizeof addr) == -1) {
            if (close(fd) == -1)
                fprintf(stderr, "close() failed\n");

            fprintf(stderr, "connect() failed\n");

            return -1;
        }
    } else {
        (void) memset(&hints, 0, sizeof (struct addrinfo));

        /*
         * prefer AF_INET over AF_UNSPEC to speed up DNS resolution by avoiding
         * IPv6 resolution as OurVLE doesn't support IPv6
         */
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        host_len = u->host_end - u->host_start;
        (void) memcpy(host, u->host_start, host_len);
        host[host_len] = '\0';

        /* pass NULL to avoid service name resolution */
        if (getaddrinfo(host, NULL, &hints, &servinfo) != 0) {
            fprintf(stderr, "getaddrinfo() failed\n");
            return -1;
        }

        for (p = servinfo; p; p = p->ai_next) {
            /*
             * prefer macros for socket() params to avoid struct addrinfo
             * pointer dereferences since OurVLE doesn't support IPv6 and the
             * connection is TCP
             */
#if defined linux && defined SOCK_CLOEXEC
            fd = socket(AF_INET, SOCK_CLOEXEC | SOCK_STREAM, IPPROTO_TCP);
#else
            fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif

            if (fd == -1) {
                fprintf(stderr, "socket() failed\n");
                continue;
            }

            ((struct sockaddr_in *)p->ai_addr)->sin_port = htons(u->port);

            if (connect(fd, p->ai_addr, sizeof (struct sockaddr_in)) == -1) {
                if (close(fd) == -1)
                    fprintf(stderr, "close() failed\n");

                fprintf(stderr, "connect() failed\n");
                continue;
            }

            break;
        }

        if (p == NULL) {
            fprintf(stderr, "failed to connect\n");
            if (close(fd) == -1)
                fprintf(stderr, "close() failed\n");

            freeaddrinfo(servinfo);
            return -1;
        }

        freeaddrinfo(servinfo);
    }

    /*
     * Disable Nagle's algorithm.
     *
     * Each HTTP request is constructed in its entirety in a single buffer
     * before writing to the socket of the connection.
     */
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (const void *) &tcp_nodelay, sizeof (int)) == -1) {
        fprintf(stderr, "setsockopt() TCP_NODELAY failed\n");
        if (close(fd) == -1)
            fprintf(stderr, "close() failed\n");

        return -1;
    }

    return fd;
}
