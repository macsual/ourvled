#include <stddef.h> /* size_t */

#include "ovle_string.h"

size_t
ovle_strlcpy(char *dst, const char *src, size_t siz)
{
    register char        *d = dst;
    register const char  *s = src;
    register size_t      n = siz;

    /* Copy as many bytes as will fit */
    if (n != 0 && --n != 0) {
        do {
            if ((*d++ = *s++) == 0)
                break;
        } while (--n != 0);
    }

    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0)
            *d = '\0';      /* NUL-terminate dst */
        while (*s++)
            /* empty */;
    }

    return (s - src - 1);    /* count does not include NUL */
}