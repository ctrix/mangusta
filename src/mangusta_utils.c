
#include "mangusta_private.h"

APR_DECLARE(char *) apr_lowercase(apr_pool_t * pool, const char *in) {
    char *out, *t;

    if (zstr(in)) {
        return NULL;
    }

    out = apr_pstrdup(pool, in);

    for (t = out; *t; ++t) {
        *t = apr_tolower(*t);
    }

    return out;
}

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

/*
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
*/

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

APR_DECLARE(char *) mangusta_html_specialchars(apr_pool_t * pool, const char *in) {
    char *p;
    char *ret = NULL, *tmp;
    mangusta_buffer_t *buf;

    if (in == NULL || pool == NULL) {
        return (char *) in;
    }

    buf = mangusta_buffer_init(pool, 0, strlen(in) * 2);
    if (buf == NULL) {
        return NULL;
    }

    /*
       " &quot
       & &amp
       ' &#039;
       < &lt;
       > &gt;
     */

    p = (char *) in;
    while (p && *p) {
        if (*p == '\'') {
            mangusta_buffer_append(buf, "&#039;", 6);
        } else if (*p == '"') {
            mangusta_buffer_append(buf, "&quot;", 6);
        } else if (*p == '&') {
            mangusta_buffer_append(buf, "&amp;", 5);
        } else if (*p == '<') {
            mangusta_buffer_append(buf, "&lt;", 4);
        } else if (*p == '>') {
            mangusta_buffer_append(buf, "&gt;", 4);
        } else {
            mangusta_buffer_appendc(buf, *p);
        }
        p++;
    }

    mangusta_buffer_appendc(buf, '\0');

    if (mangusta_buffer_get_char(buf, &tmp) > 0) {
        ret = apr_pstrdup(pool, tmp);
    }

    mangusta_buffer_destroy(buf);

    return ret;
}

APR_DECLARE(char *) mangusta_urldecode(apr_pool_t * pool, const char *intext) {
    char *outtext;
    char *cp, *xp;

    if (intext == NULL) {
        return NULL;
    }

    outtext = apr_pstrdup(pool, intext);

    for (cp = outtext, xp = outtext; *cp; cp++) {

        if (*cp == '%') {
            if (strchr("0123456789ABCDEFabcdef", *(cp + 1))
                && strchr("0123456789ABCDEFabcdef", *(cp + 2))
                ) {
                if (apr_islower((int) *(cp + 1)))
                    *(cp + 1) = apr_toupper((int) *(cp + 1));
                if (apr_islower((int) *(cp + 2)))
                    *(cp + 2) = apr_toupper((int) *(cp + 2));
                *(xp) = (*(cp + 1) >= 'A' ? *(cp + 1) - 'A' + 10 : *(cp + 1) - '0') * 16 + (*(cp + 2) >= 'A' ? *(cp + 2) - 'A' + 10 : *(cp + 2) - '0');
                xp++;
                cp += 2;
            }
        } else {
            *xp = *cp;
            xp++;
        }

    }

    memset(xp, 0, cp - xp);

    return outtext;
}

APR_DECLARE(char *) mangusta_urlencode(apr_pool_t * pool, const char *var) {
    int c;
    char *source;
    char *dest;
    char *ret;
    int dest_len = 0;
    char *hex = "0123456789abcdef";

    assert(var);

    source = (char *) var;
    dest_len = strlen(source) + 1;
    while (source && *source) {
        c = *source;

        if (('a' <= c && c <= 'z')
            || ('A' <= c && c <= 'Z')
            || ('0' <= c && c <= '9')
            ) {
            /* OK */
        } else {
            dest_len += 2;
        }
        source++;
    }

    source = (char *) var;
    ret = dest = apr_pcalloc(pool, dest_len + 1);
    if (!dest) {
        return NULL;
    }

    while (source && *source) {
        c = *source;

        if (('a' <= c && c <= 'z')
            || ('A' <= c && c <= 'Z')
            || ('0' <= c && c <= '9')
            ) {
            *dest++ = *source;
        } else {
            *dest++ = '%';
            *dest++ = hex[c >> 4];
            *dest++ = hex[c & 15];
        }
        source++;
    }
    *dest++ = '\0';

    return ret;
}

/* This function kindly inspired from anthm - Freeswitch */
APR_DECLARE(apr_size_t) apr_separate_string(char *buf, char delim, char **array, apr_size_t arraylen) {
//APR_DECLARE(unsigned int) apr_separate_string(char *buf, char delim, char **array, unsigned int arraylen) {
    apr_size_t argc;
    char *ptr;
    int quot = 0;
    char qc = '"';
    char *e;
    unsigned int x;

    if (!buf || !array || !arraylen) {
        return 0;
    }

    memset(array, 0, arraylen * sizeof(*array));

    ptr = buf;

    for (argc = 0; *ptr && (argc < arraylen - 1); argc++) {
        array[argc] = ptr;
        for (; *ptr; ptr++) {
            if (*ptr == qc) {
                if (quot) {
                    quot--;
                } else {
                    quot++;
                }
            } else if ((*ptr == delim) && !quot) {
                *ptr++ = '\0';
                break;
            }
        }
    }

    if (*ptr) {
        array[argc++] = ptr;
    }

    /* strip quotes */
    for (x = 0; x < argc; x++) {
        if (*(array[x]) == qc) {
            (array[x])++;
            if ((e = strchr(array[x], qc))) {
                *e = '\0';
            }
        }
    }

    return argc;
}
