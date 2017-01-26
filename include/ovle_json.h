#ifndef OVLE_JSON_H
#define OVLE_JSON_H

#include <stddef.h>  /* size_t */

struct ovle_buf;

struct json_parse {
    char *name_start;
    char *name_end;
    char *value_start;
    char *value_end;

    int state;
    int error;
};

extern int ovle_json_parse_array_element(struct ovle_buf *b, struct json_parse *j);
extern int ovle_json_parse_object_member(struct ovle_buf *b, struct json_parse *j);
extern int ovle_json_parse_moodle_token(struct ovle_buf *b, struct json_parse *j);

#endif  /* OVLE_JSON_H */
