#ifndef OVLE_STRING_H
#define OVLE_STRING_H

#include <stddef.h> /* size_t */

/* TODO: optimise strcmp on little endian systems when string is memory aligned */

#define ovle_str2cmp(s, c0, c1) s[0] == c0 && s[1] == c1

#define ovle_str6cmp(s, c0, c1, c2, c3, c4, c5)                               \
    s[0] == c0 && s[1] == c1 && s[2] == c2 && s[3] == c3 && s[4] == c4        \
        && s[5] == c5

#define ovle_str9cmp(s, c0, c1, c2, c3, c4, c5, c6, c7, c8)                   \
    s[0] == c0 && s[1] == c1 && s[2] == c2 && s[3] == c3                      \
        && s[4] == c4 && s[5] == c5 && s[6] == c6 && s[7] == c7 && s[8] == c8

extern size_t ovle_strlcpy(char *dst, const char *src, size_t siz);

#endif  /* OVLE_STRING_H */
