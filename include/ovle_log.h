#ifndef OVLE_LOG_H
#define OVLE_LOG_H

#include <stdio.h>

#include "config.h"

#ifdef NDEBUG   /* production */

#define ovle_log_debug0(fmt)
#define ovle_log_debug1(fmt, arg1)
#define ovle_log_debug2(fmt, arg1, arg2)
#define ovle_log_debug3(fmt, arg1, arg2, arg3)
#define ovle_log_debug4(fmt, arg1, arg2, arg3, arg4)
#define ovle_log_debug5(fmt, arg1, arg2, arg3, arg4, arg5)

#else /* not NDEBUG */

#define ovle_log_debug0(fmt)                                                  \
        fprintf(stderr, "[D] " PACKAGE_NAME ":%s:%d: " fmt  "\n",             \
                __FILE__, __LINE__)

#define ovle_log_debug1(fmt, arg1)                                            \
        fprintf(stderr, "[D] " PACKAGE_NAME ":%s:%d: " fmt "\n",              \
                __FILE__, __LINE__, arg1)

#define ovle_log_debug2(fmt, arg1, arg2)                                      \
        fprintf(stderr, "[D] " PACKAGE_NAME ":%s:%d: " fmt "\n",              \
                __FILE__, __LINE__, arg1, arg2)

#define ovle_log_debug3(fmt, arg1, arg2, arg3)                                \
        fprintf(stderr, "[D] " PACKAGE_NAME ": %s:%d: " fmt "\n",             \
                __FILE__, __LINE__, arg1, arg2, arg3)

#define ovle_log_debug4(fmt, arg1, arg2, arg3, arg4)                          \
        fprintf(stderr, "[D] " PACKAGE_NAME ":%s:%d: " fmt "\n",              \
                __FILE__, __LINE__, arg1, arg2, arg3, arg4)

#define ovle_log_debug5(fmt, arg1, arg2, arg3, arg4, arg5)                    \
        fprintf(stderr, "[D] " PACKAGE_NAME ":%s:%d: " fmt "\n",              \
                __FILE__, __LINE__, arg1, arg2, arg3, arg4, arg5)

#endif /* NDEBUG */
#endif  /* OVLE_LOG_H */
