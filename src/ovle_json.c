#include <ctype.h>      /* isxdigit() */

#include "ovle_config.h"
#include "ovle_json.h"

int
ovle_json_parse_moodle_token(struct ovle_buf *b, struct json_parse *j)
{
    unsigned char ch;
    unsigned char *p;
    enum {
        sw_start,
        sw_name_lquot,
        sw_name_c0,
        sw_name_c1,
        sw_name_c2,
        sw_name_c3,
        sw_name_c4,
        sw_name_rquot,
        sw_colon,
        sw_value_lquot,
        sw_value_token_first_symbol,
        sw_value_token,
        sw_value_rquot,
        sw_value_error_first_char,
        sw_value_error,
        sw_comma
    } state;

    state = sw_start;

    for (p = b->pos; p < b->end; p++) {
        ch = *p;

        switch (state) {
            case sw_start:
                switch (ch) {
                    case '{':
                        state = sw_name_lquot;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_name_lquot:
                switch (ch) {
                    case ' ':
                        break;

                    case '"':
                        state = sw_name_c0;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_name_c0:
                j->error = 0;

                switch (ch) {
                    case 'e':
                        /* fallthrough */
                    case 't':
                        state = sw_name_c1;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_name_c1:
                switch (ch) {
                    /* to */
                    case 'o':
                        /* fallthrough */

                    /* er */
                    case 'r':
                        state = sw_name_c2;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_name_c2:
                switch (ch) {
                    /* tok */
                    case 'k':
                        /* fallthrough */

                    /* err */
                    case 'r':
                        state = sw_name_c3;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_name_c3:
                switch (ch) {
                    /* toke */
                    case 'e':
                        /* fallthrough */

                    /* erro */
                    case 'o':
                        state = sw_name_c4;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_name_c4:
                switch (ch) {
                    /* token */
                    case 'n':
                        state = sw_name_rquot;
                        break;

                    /* error */
                    case 'r':
                        j->error = 1;
                        state = sw_name_rquot;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_name_rquot:
                switch (ch) {
                    case '"':
                        state = sw_colon;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_colon:
                switch (ch) {
                    case ' ':
                        break;

                    case ':':
                        state = sw_value_lquot;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_value_lquot:
                switch (ch) {
                    case ' ':
                        break;

                    case '"':
                        if (j->error) {
                            state = sw_value_error_first_char;
                            break;
                        }

                        state = sw_value_token_first_symbol;
                        break;

                    default:
                        return OVLE_ERROR;
                }

                break;

            /*
             * Moodle backend uses PHP md5() to generate token and md5() does
             * not return any of the uppercase symbols in the hexadecimal radix
             */

            case sw_value_token_first_symbol:
                /* TODO: check if hex alpha symbol is lowercase */
                if (!isxdigit(ch))
                    return OVLE_ERROR;

                j->value_start = p;
                state = sw_value_token;
                break;

            case sw_value_token:
                /* TODO: check if hex alpha symbol is lowercase */
                if (isxdigit(ch))
                    break;

                if (ch == '"') {
                    j->value_end = p;
                    state = sw_value_rquot;
                    break;
                }

                /* TODO: stop after 32 chars */

                break;

            case sw_value_rquot:
                switch (ch) {
                    case '}':
                        goto done;

                    default:
                        return OVLE_ERROR;
                }

                break;

            case sw_value_error_first_char:
                /* TODO: validate string first char*/
                j->value_start = p;
                state = sw_value_error;
                break;

            case sw_value_error:
                /* TODO: validate string */

                if (ch == '"') {
                    j->value_end = p;
                    state = sw_comma;
                    break;
                }

                break;

            /* TODO */

            case sw_comma:
                switch (ch) {
                    case ' ':
                        break;

                    case ',':
                        // state = sw_name_lquot;
                        goto done;
                }

                break;
        }
    }

done:

    b->pos = p + 1;

#if 0

    len = j->value_end - j->value_start;
    if (len != OVLE_MD5_HASH_LEN)
        return OVLE_ERROR;

#endif

    return OVLE_OK;
}

int
ovle_json_parse_object_member(struct ovle_buf *b, struct json_parse *j)
{
    int nest_level;
    unsigned char ch;
    unsigned char *p;
    enum {
        sw_start,
        sw_name_lquot,
        sw_name_string_value_start,
        sw_name,
        sw_name_rquot,
        sw_space_before_value,
        sw_value_string_lquot,
        sw_value_string,
        sw_value_string_rquot,
        sw_value_number_digits,
        sw_value_number_frac,
        sw_value_number_exp_sign,
        sw_value_number_exp_digits,
        sw_value_object,
        sw_value_array,
        sw_almost_done
    } state;

    nest_level = 0;

    state = j->state;

    for (p = b->pos; p < b->end; p++) {
        ch = *p;

        switch (state) {
            case sw_start:
                switch (ch) {
                    case '{':
                        state = sw_name_lquot;
                        break;

                    default:
                        return -1;
                }

                break;

            case sw_name_lquot:
                switch (ch) {
                    case ' ':
                        break;

                    case '"':
                        state = sw_name_string_value_start;
                        break;

                    default:
                        return -1;
                }

                break;

            case sw_name_string_value_start:
                j->name_start = p;
                state = sw_name;
                break;

            case sw_name:
                if (iscntrl(ch))
                    return -1;

                /* TODO: handle escape characters */
                if (ch == '\\')
                    return -1;

                if (ch == '"') {
                    j->name_end = p;
                    state = sw_name_rquot;
                    break;
                }

                break;

            case sw_name_rquot:
                switch (ch) {
                    case ':':
                        state = sw_space_before_value;
                        break;

                    default:
                        return -1;
                }

                break;

            case sw_space_before_value:
                switch (ch) {
                    case ' ':
                        break;

                    case '"':
                        state = sw_value_string_lquot;
                        break;

                    case '{':
                        nest_level++;
                        j->value_start = p;
                        state = sw_value_object;
                        break;

                    case '[':
                        nest_level++;
                        j->value_start = p;
                        state = sw_value_array;
                        break;

                    case '-':
                        j->value_start = p;
                        state = sw_value_number_digits;
                        break;

                    default:
                        if (isdigit(ch)) {
                            j->value_start = p;
                            state = sw_value_number_digits;
                            break;
                        }

                        return -1;
                }

                break;

            case sw_value_string_lquot:
                j->value_start = p;
                state = sw_value_string;
                break;

            case sw_value_string:
                if (ch == '"') {
                    j->value_end = p;
                    state = sw_value_string_rquot;
                    break;
                }

                break;

            case sw_value_string_rquot:
                switch (ch) {
                    case ' ':
                        break;

                    case ',':
                        goto done;

                    case '}':
                        goto member_done;

                    default:
                        return -1;
                }

                break;

            case sw_value_number_digits:
                if (ch >= '0' && ch <= '9')
                    break;

                if (ch == '.') {
                    state = sw_value_number_frac;
                    break;
                }

                if (ch == 'e' || ch == 'E') {
                    state = sw_value_number_exp_sign;
                    break;
                }

                if (ch == ',') {
                    j->value_end = p;
                    goto done;
                }

                if (ch == '}')
                    goto member_done;

                return -1;

                break;

            case sw_value_number_frac:
                if (ch >= '0' && ch <= '9')
                    break;

                if (ch == 'e' || ch == 'E') {
                    state = sw_value_number_exp_sign;
                    break;
                }

                if (ch == ',') {
                    j->value_end = p;
                    goto done;
                }

                if (ch == '}')
                    goto member_done;

                return -1;

                break;

            case sw_value_number_exp_sign:
                switch (ch) {
                    case ' ':
                        state = sw_value_number_exp_digits;
                        break;

                    case '-':
                        state = sw_value_number_exp_digits;
                        break;

                    case '+':
                        state = sw_value_number_exp_digits;
                        break;

                    default:
                        return -1;
                }

                break;

            case sw_value_number_exp_digits:
                if (ch >= '0' && ch <= '9')
                    break;

                if (ch = ',') {
                    j->value_end = p;
                    goto done;
                }

                if (ch == '}')
                    goto member_done;

                return -1;

                break;

            case sw_value_object:
                switch (ch) {
                    case '{':
                        nest_level++;
                        break;

                    case '}':
                        if (--nest_level == 0) {
                            j->value_end = p;
                            state = sw_almost_done;
                            break;
                        }

                        break;
                }

                break;

            case sw_value_array:
                switch (ch) {
                    case '[':
                        nest_level++;
                        break;

                    case ']':
                        if (--nest_level == 0) {
                            j->value_end = p;
                            state = sw_almost_done;
                            break;
                        }

                        break;
                }

                break;

            case sw_almost_done:
                switch (ch) {
                    case ' ':
                        break;

                    case ',':
                        goto done;

                    case '}':
                        goto member_done;

                    default:
                        return -1;
                }

                break;
        }
    }

done:

    b->pos = p + 1;
    j->state = sw_name_lquot;

    return 0;

member_done:

    b->pos = p + 1;

    return 3;
}

int
ovle_json_parse_array_element(struct ovle_buf *b, struct json_parse *j)
{
    int count;
    unsigned char ch;
    unsigned char *p;
    enum {
        sw_start,
        sw_space_before_value,
        sw_value,
        sw_value_object,
        sw_value_array,
        sw_almost_done
    } state;

    count = 0;

    state = j->state;

    for (p = b->pos; p < b->end; p++) {
        ch = *p;

        switch (state) {
            case sw_start:
                switch (ch) {
                    case '[':
                        state = sw_space_before_value;
                        break;

                    default:
                        return -1;
                }

                break;

            case sw_space_before_value:
                switch (ch) {
                    case ' ':
                        break;

                    case '{':
                        count++;
                        j->value_start = p;
                        state = sw_value_object;
                        break;

                    case '[':
                        count++;
                        j->value_start = p;
                        state = sw_value_array;
                        break;

                    default:
                        j->value_start = p;
                        state = sw_value;
                        break;
                }

                break;

            case sw_value:
                switch (ch) {
                    case ',':
                        j->value_end = p;
                        goto done;

                    case ']':
                        goto element_done;
                }

                break;

            case sw_value_object:
                switch (ch) {
                    case '{':
                        count++;
                        break;

                    case '}':
                        if (--count == 0) {
                            state = sw_almost_done;
                            break;
                        }

                        break;
                }

                break;

            case sw_value_array:
                switch (ch) {
                    case '[':
                        count++;
                        break;

                    case ']':
                        if (--count == 0) {
                            state = sw_almost_done;
                            break;
                        }

                        break;
                }

                break;

            case sw_almost_done:
                switch (ch) {
                    case ' ':
                        break;

                    case ',':
                        j->value_end = p;
                        goto done;

                    case ']':
                        goto element_done;

                    default:
                        return -1;
                }

                break;
        }
    }

done:

    b->pos = p + 1;
    j->state = sw_space_before_value;

    return 0;

element_done:

    b->pos = p + 1;

    return 3;
}
