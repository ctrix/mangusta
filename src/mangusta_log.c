
#include "mangusta_private.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static mangusta_log_func_t *cbf = NULL;
static const char *log_text[] = {
    "VERBOSE ",
    "CRITICAL",
    "ERROR   ",
    "WARNING ",
    "NOTICE  ",
    "DEBUG   ",
    "[Woop..]",
    "[Woop..]",
    "[Woop..]",
    "[Woop..]"
};

APR_DECLARE(const char *) mangusta_log_level_string(mangusta_log_types level) {
    return log_text[level];
}

APR_DECLARE(void) mangusta_log_set_function(mangusta_log_func_t * func) {
    cbf = func;
    return;
}

APR_DECLARE(void) mangusta_log(mangusta_log_types level, const char *fmt, ...) {
    va_list ap;

    if (cbf != NULL) {
        va_start(ap, fmt);
        cbf(level, fmt, ap);
        va_end(ap);
    } else {
        size_t len;
        char buf[1024];

        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf) - 2, fmt, ap);
        va_end(ap);

        fprintf(stderr, "%s- ", log_text[level]);
        fprintf(stderr, "%s", buf);

        len = strlen(buf);
        if ((len > 0) && buf[len - 1] != '\n') {
            fprintf(stderr, "\n");
        }
    }

    return;
}
