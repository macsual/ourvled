#ifndef OVLE_MOODLE_H
#define OVLE_MOODLE_H

/*
 * Moodle user tokens for web services are generated from PHP md5() without the
 * optional raw_output parameter which returns a 32 character hexadecimal
 * number
 */
#define OVLE_MD5_HASH_LEN               32

#define OVLE_MOODLE_SERVICE_NAME_LEN    (sizeof "moodle_mobile_app" - 1)

#define OVLE_DOB_LEN                    (sizeof "yyyymmdd" - 1)

#define OVLE_UWI_STU_ID_LEN             (sizeof "6200xxxxx" - 1)

extern int ovle_moodle_get_token(int sockfd, char *token);
extern int ovle_get_moodle_userid(int fd, const char *token, char *userid);
extern int ovle_sync_moodle_content(int fd, const char *token, const char *userid);

#endif  /* OVLE_MOODLE_H */
