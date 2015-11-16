
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

int mg_url_decode(const char *src, int src_len, char *dst, int dst_len, int is_form_url_encoded) {
    int i, j, a, b;
#define HEXTOI(x) (isdigit(x) ? x - '0' : x - 'W')

    for (i = j = 0; i < src_len && j < dst_len - 1; i++, j++) {
        if (src[i] == '%' && i < src_len - 2 && isxdigit(*(const unsigned char *) (src + i + 1)) && isxdigit(*(const unsigned char *) (src + i + 2))) {
            a = tolower(*(const unsigned char *) (src + i + 1));
            b = tolower(*(const unsigned char *) (src + i + 2));
            dst[j] = (char) ((HEXTOI(a) << 4) | HEXTOI(b));
            i += 2;
        } else if (is_form_url_encoded && src[i] == '+') {
            dst[j] = ' ';
        } else {
            dst[j] = src[i];
        }
    }

    dst[j] = '\0';              // Null-terminate the destination

    return i >= src_len ? j : -1;
}

// Protect against directory disclosure attack by removing '..',
// excessive '/' and '\' characters
void remove_double_dots_and_double_slashes(char *s) {
    char *p = s;

    while (*s != '\0') {
        *p++ = *s++;
        if (s[-1] == '/' || s[-1] == '\\') {
            // Skip all following slashes, backslashes and double-dots
            while (s[0] != '\0') {
                if (s[0] == '/' || s[0] == '\\') {
                    s++;
                } else if (s[0] == '.' && s[1] == '.') {
                    s += 2;
                } else {
                    break;
                }
            }
        }
    }
    *p = '\0';
}
