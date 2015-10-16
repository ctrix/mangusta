
#include "mangusta_private.h"

#ifndef strnstr

char *strnstr(const char *haystack, const char *needle, size_t len) {
    int i;
    size_t needle_len;

    /* segfault here if needle is not NULL terminated */
    if (0 == (needle_len = strlen(needle)))
        return (char *) haystack;

    for (i = 0; i <= (int) (len - needle_len); i++) {
        if ((haystack[0] == needle[0]) && (0 == strncmp(haystack, needle, needle_len)))
            return (char *) haystack;

        haystack++;
    }
    return NULL;
}

#endif

apr_status_t chomp(char *buffer, size_t length) {
    size_t chars;

    if (!length) {
        length = strlen(buffer);
    }

    if (!length) {
        return APR_SUCCESS;
    }

    if (buffer == NULL) {
        return APR_ERROR;
    }
    if (length < 1) {
        return APR_ERROR;
    }

    chars = 0;

    --length;
    while (buffer[length] == '\r' || buffer[length] == '\n') {
        buffer[length] = '\0';
        chars++;

        /* Stop once we get to zero to prevent wrap-around */
        if (length-- == 0) {
            break;
        }
    }

    return APR_SUCCESS;
}
