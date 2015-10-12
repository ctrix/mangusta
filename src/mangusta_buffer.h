
#ifndef NN_BUFFER_H
#define NN_BUFFER_H

#include <apr_pools.h>

enum mangusta_buffer_flags_e {
    NN_BUF_FLAG_STRING = (1 << 0),
    NN_BUF_FLAG_NO_PREPEND = (1 << 1),
    NN_BUF_FLAG_DOUBLE = (1 << 2),
};

typedef enum mangusta_buffer_flags_e mangusta_buffer_flags_t;

typedef struct mangusta_buffer_s mangusta_buffer_t;

APR_DECLARE(mangusta_buffer_t *) mangusta_buffer_init(apr_pool_t * pool, apr_uint16_t flags, apr_uint32_t start_size);
APR_DECLARE(void) mangusta_buffer_grow_size(mangusta_buffer_t * buf, apr_uint32_t size);
APR_DECLARE(apr_status_t) mangusta_buffer_reset(mangusta_buffer_t * buf);
APR_DECLARE(apr_status_t) mangusta_buffer_autoshrink(mangusta_buffer_t * buf);
APR_DECLARE(apr_status_t) mangusta_buffer_enlarge(mangusta_buffer_t * buf, apr_uint32_t size);

APR_DECLARE(apr_status_t) mangusta_buffer_printf(mangusta_buffer_t * buf, const char *format, ...) __attribute__ ((format(printf, 2, 3)));
APR_DECLARE(apr_status_t) mangusta_buffer_append(mangusta_buffer_t * buf, const void *src, apr_uint32_t size);
APR_DECLARE(apr_status_t) mangusta_buffer_appendc(mangusta_buffer_t * buf, const char c);
APR_DECLARE(apr_status_t) mangusta_buffer_prepend(mangusta_buffer_t * buf, const void *src, apr_uint32_t size);

APR_DECLARE(apr_status_t) mangusta_buffer_read(mangusta_buffer_t * buf, char *dest, apr_uint32_t size);
APR_DECLARE(apr_status_t) mangusta_buffer_extract(mangusta_buffer_t * buf, char *dest, apr_uint32_t size, int sep);

APR_DECLARE(apr_uint32_t) mangusta_buffer_get_char(mangusta_buffer_t * buf, char **dest);
APR_DECLARE(apr_uint32_t) mangusta_buffer_capacity(mangusta_buffer_t * buf);
APR_DECLARE(apr_uint32_t) mangusta_buffer_size(mangusta_buffer_t * buf);
APR_DECLARE(apr_uint32_t) mangusta_buffer_size_left(mangusta_buffer_t * buf);
APR_DECLARE(apr_status_t) mangusta_buffer_rollback(mangusta_buffer_t * buf);
APR_DECLARE(apr_status_t) mangusta_buffer_destroy(mangusta_buffer_t * buf);

#endif
