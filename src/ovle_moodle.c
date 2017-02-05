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
ovle_mdl_get_token(int fd, char *token)
{
    int content_length;
    int statuscode;
    ssize_t bytes;
    int http_request_len, http_body_len, host_len;
    char http_request[BUFSIZ], http_request_body[128];
    char http_response[BUFSIZ];
    struct ovle_buf buf;
    struct json_parse j;
    char host[HOST_NAME_MAX + 1];

    host_len = u.host_end - u.host_start;
    (void) memcpy(host, u.host_start, host_len);
    host[host_len] = '\0';

    http_body_len = snprintf(http_request_body, sizeof http_request_body,
                            "username=%s&password=%s&service=%s",
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

    buf.start = http_response;
    buf.pos = http_response;
    buf.last = http_response;
    buf.end = http_response + sizeof http_response;

    buf.state = 0;

    if (ovle_http_process_status_line(fd, &buf, &statuscode) == -1)
        return OVLE_ERROR;

    if (statuscode != 200)
        return OVLE_ERROR;

    if (ovle_http_process_response_headers(fd, &buf, &content_length) == -1)
        return OVLE_ERROR;

    if (ovle_json_parse_moodle_token(&buf, &j) == -1)
        return OVLE_ERROR;

    if (j.error) {
        j.error_start[j.error_end - j.error_start] = '\0';

        fprintf(stderr, "error: %s\n", j.error_start);

        return OVLE_ERROR;
    }

    (void) memcpy(token, j.value_start, OVLE_MD5_HASH_LEN);
    token[OVLE_MD5_HASH_LEN] = '\0';

    ovle_log_debug1("Moodle user token %s", token);

    return OVLE_OK;
}

int
ovle_mdl_get_userid(int fd, const char *token, char *userid)
{
    int rv;
    int content_length;
    int statuscode;
    ssize_t bytes;
    int http_request_len;
    size_t name_len, val_len, host_len;
    char *name, *value;
    char http_request[BUFSIZ];
    char http_response[BUFSIZ];
    struct ovle_buf buf;
    struct json_parse j;
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

    buf.start = http_response;
    buf.pos = http_response;
    buf.last = http_response;
    buf.end = http_response + sizeof http_response;

    buf.state = 0;

    if (ovle_http_process_status_line(fd, &buf, &statuscode) == -1)
        return OVLE_ERROR;

    if (statuscode != 200)
        return OVLE_ERROR;

    if (ovle_http_process_response_headers(fd, &buf, &content_length) == -1)
        return OVLE_ERROR;

    j.state = 0;

    for (;;) {
        rv = ovle_json_parse_object_member(&buf, &j);

        if (rv == OVLE_OK) {
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
                (void) ovle_strlcpy(userid, value, OVLE_INT32_LEN + 1);
                break;
            }
        }

        if (rv == 3)
            break;

        if (rv == OVLE_ERROR)
            return OVLE_ERROR;
    }

    ovle_log_debug1("Moodle userid %s", userid);

    return OVLE_OK;
}

int
ovle_mdl_sync_course_content(int sockfd, const char *token, const char *userid)
{
    int rv;
    int statuscode;
    int file_fd;
    int content_length;
    int http_request_len;
    char http_request[BUFSIZ];
    char http_response[BUFSIZ];
    char response2[BUFSIZ];
    ssize_t bytes;
    size_t name_len, val_len, host_len;
    char *name, *value;
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

    buf.start = http_response;
    buf.pos = http_response;
    buf.last = http_response;
    buf.end = http_response + sizeof http_response;

    buf.state = 0;

    if (ovle_http_process_status_line(sockfd, &buf, &statuscode) == -1)
        return OVLE_ERROR;

    if (statuscode != 200)
        return OVLE_ERROR;

    if (ovle_http_process_response_headers(sockfd, &buf, &content_length) == -1)
        return OVLE_ERROR;

    j.state = 0;

    for (;;) {
        rv = ovle_json_parse_array_element(&buf, &j);

        if (rv == OVLE_OK) {
            k.state = 0;

            for (;;) {
                rv = ovle_json_parse_object_member(&buf, &k);

                if (rv == OVLE_OK) {
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

                if (rv == 3)
                    break;

                if (rv == OVLE_ERROR)
                    return OVLE_ERROR;
            }

            if (chdir(shortname) == -1) {
                if (errno == ENOENT) {
                    if (mkdir(shortname, 0777) == -1) {
                        fprintf(stderr, "mkdir(%s) failed\n", shortname);
                        return OVLE_ERROR;
                    }
                } else {
                    fprintf(stderr, "chdir(\"%s\") failed\n", shortname);
                    return OVLE_ERROR;
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
            //     return OVLE_ERROR;

            // bytes_received = 0;

            // for (;;) {
            //     bytes = recv(sockfd, buf, sizeof buf, 0);

            //     if (bytes == -1)
            //         return OVLE_ERROR;

            //     if (bytes == 0)
            //         break;

            //     bytes_received += bytes;

            //     (void) write(file_fd, buf, (size_t) bytes);
            // }

            // if (bytes_received == 0)
            //     return OVLE_ERROR;
        }

        if (rv == 3)
            break;

        if (rv == OVLE_ERROR)
            return OVLE_ERROR;
    }

    return OVLE_OK;
}
