#ifndef OVLE_MOODLE_H
#define OVLE_MOODLE_H

/*
 * A Moodle user token for web services is generated in PHP by calling md5()
 * without the raw_output parameter which returns the hash formatted as a
 * hexadecimal string.
 *
 * A md5 hash value has a precision of 128 bits.  A single hexadecimal symbol
 * can represent a maximum value of 2^4 - 1 (15) which can be represented by 4
 * binary digits.  Therefore the length of a md5 hash hexadecimal string is
 * 128 bits / 4 bits which equals 32 symbols.
 */
#define OVLE_MD5_HASH_LEN               32

/* Moodle Mobile web service shortname */
#define OVLE_MOODLE_SERVICE_NAME_LEN    (sizeof "moodle_mobile_app" - 1)

/* new UWI student ID # scheme */
#define OVLE_UWI_STU_ID_LEN             (sizeof "6200xxxxx" - 1)


/*
 * Get Moodle user token
 *
 * sockfd is a file descriptor for an open connection to a Moodle server.
 * token points to a buffer which must be large enough to store the hexadecimal
 * string representation of a md5 hash, which is 32 bytes.  The Moodle user
 * token value will be copied to the buffer token points to upon success.
 */
extern int ovle_moodle_get_token(int sockfd, char *token);

/*
 * Get Moodle userid ASCII representation
 *
 * sockfd is a file descriptor for an open connection to a Moodle server.
 * token points to a buffer that stores a C string that represents the Moodle
 * user token which is used as an argument to a call to
 * core_webservice_get_site_info() of the Moodle web service.  The Moodle
 * userid is retrieved from the JSON object returned by the web service
 * function call and its network ASCII representation is copied to the buffer
 * that userid points to.
 *
 * The ASCII representation is stored to avoid integer conversion at runtime
 * when the Moodle userid is used as an argument to another web service
 * function call.
 */
extern int ovle_get_moodle_userid(int fd, const char *token, char *userid);


/*
 * sockfd is a file descriptor for an open connection to a Moodle server.
 * token points to a buffer that stores a C string that represents a Moodle
 * user token which is used as an argument to an internal Moodle web service
 * function call.  userid points to a buffer that stores a C string that
 * represents the ASCII representation of a Moodle userid.
 */
extern int ovle_sync_moodle_content(int fd, const char *token, const char *userid);

#endif  /* OVLE_MOODLE_H */
