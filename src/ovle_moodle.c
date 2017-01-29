#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "ovle_config.h"
#include "ovle_log.h"
#include "ovle_moodle.h"
#include "ovle_http.h"
#include "ovle_json.h"
#include "ovle_string.h"

int
ovle_moodle_get_token(int fd, char *token)
{
    int rv;
    int statuscode;
    ssize_t bytes;
    int http_request_len, http_body_len, host_len;
    char http_request[BUFSIZ], http_request_body[128];
    char http_response[BUFSIZ];
    struct ovle_buf buf;
    struct json_parse j;
    struct ovle_http_parse_header h;
    char host[HOST_NAME_MAX + 1];

    host_len = u.host_end - u.host_start;
    (void) memcpy(host, u.host_start, host_len);
    host[host_len] = '\0';

    http_body_len = snprintf(http_request_body, sizeof http_request_body, "username=%s&password=%s&service=%s",
                            username, password, service);

    /* TODO: uri encode HTTP body */

    http_request_len = snprintf(http_request, sizeof http_request,
                                "POST /login/token.php HTTP/1.1" CRLF
                                "Host: %s" CRLF
                                "Content-Type: application/x-www-form-urlencoded" CRLF
                                "Content-Length: %d" CRLF
                                CRLF
                                "%s",
                                host, http_body_len, http_request_body);

    /*
     * TODO:
     *      TCP_CORK or scatter gather I/O to consolidate HTTP header and body
     *      send() all bytes
     */

    send(fd, http_request, http_request_len, MSG_NOSIGNAL);

    /* TODO: recv() all bytes */

    bytes = recv(fd, http_response, sizeof http_response, 0);

    if (bytes == -1)
        return -1;

    if (bytes == 0)
        return -1;

    if (bytes > sizeof http_response) {
        fprintf(stderr, "HTTP response too large\n");
        return -1;
    }

    buf.start = http_response;
    buf.pos = http_response;
    buf.end = http_response + bytes;

    if (ovle_http_parse_status_line(&buf, &statuscode) == -1)
        return -1;

    if (statuscode != 200)
        return -1;

    for (;;) {
        rv = ovle_http_parse_header_line(&buf, &h);

        if (rv == -1)
            return -1;

        if (rv == 3)
            break;
    }

    if (ovle_json_parse_moodle_token(&buf, &j) == -1)
        return -1;

    if (j.error)
        return -1;

    (void) memcpy(token, j.value_start, OVLE_MD5_HASH_LEN);
    token[OVLE_MD5_HASH_LEN] = '\0';

    return 0;
}

int
ovle_get_moodle_userid(int fd, const char *token, char *userid)
{
    int rv;
    int statuscode;
    ssize_t bytes;
    int http_request_len;
    size_t name_len, val_len, host_len;
    char *name, *value;
    char http_request[BUFSIZ];
    char http_response[BUFSIZ];
    struct ovle_buf buf;
    struct json_parse j;
    struct ovle_http_parse_header h;
    char host[HOST_NAME_MAX + 1];

    host_len = u.host_end - u.host_start;
    (void) memcpy(host, u.host_start, host_len);
    host[host_len] = '\0';

    http_request_len = snprintf(http_request, sizeof http_request,
                                "GET /webservice/rest/server.php?wstoken=%s&wsfunction=core_webservice_get_site_info&moodlewsrestformat=json HTTP/1.1" CRLF
                                "Host: %s" CRLF
                                CRLF,
                                token, host);

    /* TODO: send() all bytes */

    send(fd, http_request, http_request_len, MSG_NOSIGNAL);

    /* TODO: recv() all bytes */

    bytes = recv(fd, http_response, sizeof http_response, 0);

    if (bytes == -1)
        return -1;

    if (bytes == 0)
        return -1;

    if (bytes > sizeof http_response) {
        fprintf(stderr, "HTTP response too large\n");
        return -1;
    }

    buf.start = http_response;
    buf.pos = http_response;
    buf.end = http_response + bytes;

    if (ovle_http_parse_status_line(&buf, &statuscode) == -1)
        return -1;

    if (statuscode != 200)
        return -1;

    for (;;) {
        rv = ovle_http_parse_header_line(&buf, &h);

        if (rv == -1)
            return -1;

        if (rv == 3)
            break;
    }

    j.state = 0;

    for (;;) {
        rv = ovle_json_parse_object_member(&buf, &j);

        if (rv == -1)
            return -1;

        if (rv == 3)
            break;

        if (rv == 0) {
            name_len = j.name_end - j.name_start;
            name = j.name_start;
            name[name_len] = '\0';

            val_len = j.value_end - j.value_start;
            value = j.value_start;
            value[val_len] = '\0';

            ovle_log_debug2("JSON object member \"%s\" : %s", name, value);

            /*
             * The Moodle userid value is the only information needed from the
             * site info so the rest of the JSON object is discarded as it is
             * assumed to be valid since the host is trusted, it is machine
             * generated and inconsequential to the program flow.
             */
            if (name_len == 6 && ovle_str6cmp(name, 'u', 's', 'e', 'r', 'i', 'd')) {
                (void) ovle_strlcpy(userid, value, INT64_LEN + 1);
                break;
            }
        }
    }

    return 0;
}

int
ovle_sync_moodle_content(int sockfd, const char *token, const char *userid)
{
    int rv;
    int statuscode;
    int file_fd;
    int http_request_len;
    char http_request[BUFSIZ];
    char http_response[BUFSIZ];
    char response2[BUFSIZ];
    ssize_t bytes;
    size_t name_len, field_len, val_len, host_len;
    char *name, *field, *value;
    char *id, *shortname;
    struct ovle_http_parse_header h;
    struct ovle_buf buf;
    struct json_parse j, k;
    char host[HOST_NAME_MAX + 1];

    host_len = u.host_end - u.host_start;
    (void) memcpy(host, u.host_start, host_len);
    host[host_len] = '\0';

    http_request_len = snprintf(http_request, sizeof http_request,
                                "GET /webservice/rest/server.php?wstoken=%s&wsfunction=core_enrol_get_users_courses&moodlewsrestformat=json&userid=%s HTTP/1.1" CRLF
                                "Host: %s" CRLF
                                CRLF,
                                token, userid, host);

    /* TODO: send() all bytes */

    send(sockfd, http_request, http_request_len, MSG_NOSIGNAL);

    /* TODO: recv() all bytes */

    bytes = recv(sockfd, http_response, sizeof http_response, 0);

    if (bytes == -1)
        return -1;

    if (bytes == 0)
        return -1;

    if (bytes > sizeof http_response) {
        fprintf(stderr, "HTTP response too large\n");
        return -1;
    }

    buf.start = http_response;
    buf.pos = http_response;
    buf.end = http_response + bytes;

    if (ovle_http_parse_status_line(&buf, &statuscode) == -1)
        return -1;

    if (statuscode != 200)
        return -1;

    for (;;) {
        rv = ovle_http_parse_header_line(&buf, &h);

        if (rv == 3)
            break;

        if (rv == 0) {
            field_len = h.field_end - h.field_start;
            field = h.field_start;
            field[field_len] = '\0';

            val_len = h.value_end - h.value_start;
            value = h.value_start;
            value[val_len] = '\0';

            ovle_log_debug2("header %s: %s", field, value);
        }
    }

    j.state = 0;

    for (;;) {
        rv = ovle_json_parse_array_element(&buf, &j);

        if (rv == -1)
            return -1;

        if (rv == 3)
            break;

        if (rv == 0) {
            k.state = 0;

            for (;;) {
                rv = ovle_json_parse_object_member(&buf, &k);

                if (rv == -1)
                    return -1;

                if (rv == 3)
                    break;

                if (rv == 0) {
                    name_len = k.name_end - k.name_start;
                    name = k.name_start;
                    name[name_len] = '\0';

                    val_len = k.value_end - k.value_start;
                    value = k.value_start;
                    value[val_len] = '\0';

                    ovle_log_debug2("JSON object member \"%s\" : %s", name, value);

                    if (name_len == 2 && ovle_str2cmp(name, 'i', 'd'))
                        id = value;

                    if (name_len == 9 && ovle_str9cmp(name, 's', 'h', 'o', 'r', 't', 'n', 'a', 'm', 'e'))
                        shortname = value;
                }
            }

            if (chdir(shortname) == -1) {
                if (errno == ENOENT) {
                    if (mkdir(shortname, 0777) == -1) {
                        fprintf(stderr, "mkdir(%s) failed\n", shortname);
                        return -1;
                    }
                } else {
                    fprintf(stderr, "chdir(\"%s\") failed\n", shortname);
                    return -1;
                }
            }

            http_request_len = snprintf(http_request, sizeof http_request,
                                        "GET /webservice/rest/server.php?wstoken=%s&wsfunction=core_course_get_contents&moodlewsrestformat=json&courseid=%s HTTP/1.1" CRLF
                                        "Host: %s" CRLF
                                        CRLF,
                                        token, id, host);

            /* TODO: send() all */

            send(sockfd, http_request, http_request_len, MSG_NOSIGNAL);

            /* TODO: recv() all */

            bytes = recv(sockfd, response2, sizeof response2, 0);

            if (bytes == -1)
                return -1;

            if (bytes == 0)
                return -1;

            if (bytes > sizeof response2) {
                fprintf(stderr, "HTTP response too large\n");
                return -1;
            }

            // send(sockfd, request, len, MSG_NOSIGNAL);

            // file_fd = open("test.pdf", O_RDWR | O_CREAT, 0644);
            // if (file_fd == -1)
            //     return 1;

            // bytes_received = 0;

            // for (;;) {
            //     bytes = recv(sockfd, buf, sizeof buf, 0);

            //     if (bytes == -1)
            //         return -1;

            //     if (bytes == 0)
            //         break;

            //     bytes_received += bytes;

            //     (void) write(file_fd, buf, (size_t) bytes);
            // }

            // if (bytes_received == 0)
            //     return -1;
        }
    }

    return 0;
}
