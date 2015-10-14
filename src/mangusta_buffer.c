
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "apr.h"
#include "apr_pools.h"
#include "apr_strings.h"

#include "mangusta_buffer.h"

#define APR_ERROR       APR_EGENERAL

#define DEFAULT_BUFFER_SIZE 8192

#define BPTR(buf)  ((buf)->data + (buf)->offset)
#define BEND(buf)  (BPTR(buf) + (buf)->len)
#define BLEN(buf)  ((buf)->len)
#define BCAP(buf)  (buf_forward_capacity (buf))

struct mangusta_buffer_s {
    apr_pool_t *pool;
    apr_uint16_t flags;
    apr_uint32_t max_grow_size;
    apr_uint32_t capacity;

    apr_uint32_t offset;
    apr_uint32_t len;
    apr_byte_t *data;
    apr_uint32_t old_offset;
    apr_uint32_t old_len;
};

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

static void * apr_pool_resize(apr_pool_t * pool, void *old_addr, const unsigned long old_byte_size, const unsigned long new_byte_size) {
    void *new;

    /* If the size is shrinking, then ignore ... */
    if (new_byte_size <= old_byte_size) {
        return old_addr;
    }

    new = apr_palloc(pool, new_byte_size);
    if (new != NULL) {
        memcpy(new, old_addr, old_byte_size);
        return new;
    } else {
        return NULL;
    }
}


static apr_uint32_t buf_forward_capacity(mangusta_buffer_t * buf) {
    int ret = buf->capacity - (buf->offset + buf->len);
    if (ret < 0)
        ret = 0;
    return ret;
}

static int buf_safe(mangusta_buffer_t * buf, int len) {
    return len >= 0 && buf->offset + buf->len + len <= buf->capacity;
}

static apr_byte_t *buf_write_alloc(mangusta_buffer_t * buf, apr_uint32_t size) {
    apr_byte_t *ret;

    if (!buf_safe(buf, size))
        return NULL;

    buf->old_len = buf->len;
    buf->old_offset = buf->offset;

    ret = BPTR(buf) + buf->len;
    buf->len += size;
    return ret;
}

static apr_byte_t *buf_read_alloc(mangusta_buffer_t * buf, apr_uint32_t size) {
    apr_byte_t *ret;

    if (buf->len < size) {
        return NULL;
    }

    ret = BPTR(buf);
    buf->offset += size;
    buf->len -= size;
    return ret;
}

/* ************************************************************************* */
/* ************************************************************************* */
/* ************************************************************************* */

APR_DECLARE(mangusta_buffer_t *) mangusta_buffer_init(apr_pool_t * main_pool, apr_uint16_t flags, apr_uint32_t start_size) {
    apr_pool_t *pool;
    mangusta_buffer_t *buf;

    assert(main_pool);

    apr_pool_create(&pool, main_pool);
    assert(pool);

    if (start_size == 0) {
        start_size = DEFAULT_BUFFER_SIZE;
    }

    buf = apr_palloc(pool, sizeof(mangusta_buffer_t));

    if (buf) {
        buf->pool = pool;
        buf->flags = flags;
        buf->capacity = start_size;
        buf->len = 0;
        buf->offset = 0;
        buf->old_len = 0;
        buf->old_offset = 0;
        buf->max_grow_size = start_size;
        buf->data = apr_palloc(pool, buf->capacity);
        if (buf->data) {
            *(buf->data) = '\0';
        } else {
            buf = NULL;
        }
    }

    return buf;
}

APR_DECLARE(void) mangusta_buffer_grow_size(mangusta_buffer_t * buf, apr_uint32_t size) {
    assert(buf);
    buf->max_grow_size = size;
}

APR_DECLARE(apr_status_t) mangusta_buffer_reset(mangusta_buffer_t * buf) {
    assert(buf);
    buf->len = 0;
    buf->old_len = 0;
    buf->offset = 0;
    buf->old_offset = 0;
    //memset(buf->data, 0, buf->capacity);
    *(buf->data) = '\0';
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_buffer_autoshrink(mangusta_buffer_t * buf) {
    assert(buf);

    /* 
       if the offset is bigger than (about) 30% of the 
       total capacity, we're wasting space.
       So move the data back to the beginning of the buffer
     */
    if (buf->offset && buf->offset > buf->capacity / 3) {
        buf->old_len = buf->len;
        buf->old_offset = buf->offset;
        memmove(buf->data, buf->data + buf->offset, buf->len + 1);
        buf->offset = 0;
        memset(buf->data + buf->offset + buf->len, 0, 1);
        *(BEND(buf)) = '\0';
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_buffer_enlarge(mangusta_buffer_t * buf, apr_uint32_t size) {
    void *newdata = NULL;

    assert(buf);
    if (size == 0) {
        size = buf->capacity;
    } else if (buf->flags & NN_BUF_FLAG_DOUBLE) {
        size = buf->capacity;
    }

    if ((buf->max_grow_size > 0) && (size > buf->max_grow_size)) {
        size = buf->max_grow_size;
    }

    newdata = apr_pool_resize(buf->pool, buf->data, buf->capacity, buf->capacity + size);

    if (!newdata) {
        return APR_ERROR;
    }

    buf->data = newdata;
    buf->capacity += size;

    return APR_SUCCESS;
}

apr_status_t mangusta_buffer_printf(mangusta_buffer_t * buf, const char *format, ...) {
    char *str;
    va_list arglist;

    va_start(arglist, format);
    str = apr_pvsprintf(buf->pool, format, arglist);
    va_end(arglist);

    if (str == NULL) {
        return APR_ERROR;
    }

    return mangusta_buffer_append(buf, str, strlen(str));
}

APR_DECLARE(apr_status_t) mangusta_buffer_append(mangusta_buffer_t * buf, const void *src, apr_uint32_t size) {
    apr_byte_t *cp = NULL;

    if (!src) {
        return APR_ERROR;
    }

    if (!size) {
        size = strlen((char *) src);
    }

    if (size <= 0) {
        return APR_SUCCESS;
    }

    cp = buf_write_alloc(buf, size);
    if (cp == NULL) {
        if (mangusta_buffer_enlarge(buf, size) == APR_SUCCESS) {
            return mangusta_buffer_append(buf, src, size);
        } else {
            return APR_ERROR;
        }
    } else {
        memcpy(cp, src, size);
        //memmove (cp, src, size);
        *(BEND(buf)) = 0;
    }

    mangusta_buffer_autoshrink(buf);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_buffer_appendc(mangusta_buffer_t * buf, const char c) {
    apr_byte_t *cp;

    mangusta_buffer_autoshrink(buf);

    cp = buf_write_alloc(buf, 1);
    if (!cp) {
        /*
           Enlarge by 1024 bytes even if we need only one to
           prevent too many re-enlargements that would waste too
           much memory
         */
        mangusta_buffer_enlarge(buf, 1024);
        return mangusta_buffer_appendc(buf, c);
    } else {
        memcpy(cp, &c, 1);
        *(BEND(buf)) = 0;       // TOD This makes our life easier with strings. But is it safe ? (boundary check)
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_buffer_prepend(mangusta_buffer_t * buf, const void *src, apr_uint32_t size) {
    /* Cannot prepend, should move the data or resize our data pool */
    assert(buf);

    /* This buffer is not suitable to prepend any data. */
    if (buf->flags & NN_BUF_FLAG_NO_PREPEND) {
        return APR_ERROR;
    }

    if (!src) {
        return APR_ERROR;
    }

    if (!size)
        size = strlen(src);

    if (BCAP(buf) >= size) {
        /* We can move our data */
        buf->old_len = buf->len;
        buf->old_offset = size;

        memmove(BPTR(buf) + size, BPTR(buf), BLEN(buf));
        buf->len += size;
        memcpy(BPTR(buf), src, size);
        return APR_SUCCESS;
    }

    return APR_ERROR;
}

APR_DECLARE(apr_status_t) mangusta_buffer_read(mangusta_buffer_t * buf, char *dest, apr_uint32_t size) {
    apr_byte_t *cp = NULL;
    assert(buf);

    cp = buf_read_alloc(buf, size);
    if (!cp) {
        return APR_ERROR;
    } else {
        if (dest) {
            memcpy(dest, cp, size);
        }
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_buffer_extract(mangusta_buffer_t * buf, char *dest, apr_uint32_t size, int sep) {
    apr_status_t status;
    apr_uint32_t rlen;
    apr_byte_t *c;
    char *bufstr = (char *) BPTR(buf);

    assert(buf);

    /* If we have the separator */
    //if ((c = (apr_byte_t *) strchr(bufstr, sep)) != NULL) {
    if ((c = (apr_byte_t *) memchr(bufstr, sep, BLEN(buf) )) != NULL) {
        /* If the resulting string is short enough to fit into our destination */
        if (c < buf->data + buf->offset + buf->len) {
            rlen = c - (buf->data + buf->offset) + 1;
            if ( rlen <= size ) {
                memset(dest + size, 0, 1);
                status = mangusta_buffer_read(buf, dest, rlen);
                if ( rlen < size ) {
                    memset(dest + rlen, 0, 1);
                }
            }
            else {
                status = APR_ERROR;
            }
            return status;
        }
    }
    return APR_ERROR;
}

APR_DECLARE(apr_uint32_t) mangusta_buffer_get_char(mangusta_buffer_t * buf, char **dest) {
    apr_uint32_t len;

    if (dest) {
        *dest = (char *) BPTR(buf);
    }

    len = BLEN(buf);
    return len;
}

APR_DECLARE(apr_uint32_t) mangusta_buffer_size(mangusta_buffer_t * buf) {
    assert(buf);
    return BLEN(buf);
}

APR_DECLARE(apr_uint32_t) mangusta_buffer_capacity(mangusta_buffer_t * buf) {
    return buf->capacity;
}

APR_DECLARE(apr_uint32_t) mangusta_buffer_size_left(mangusta_buffer_t * buf) {
    assert(buf);
    return BCAP(buf);
}

APR_DECLARE(apr_status_t) mangusta_buffer_rollback(mangusta_buffer_t * buf) {
    assert(buf);
    buf->offset = buf->old_offset;
    buf->len = buf->old_len;
    *(BEND(buf)) = 0;           /* windows vsnprintf needs this */
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_buffer_destroy(mangusta_buffer_t * buf) {
    assert(buf);
    apr_pool_destroy(buf->pool);
    return APR_SUCCESS;
}
