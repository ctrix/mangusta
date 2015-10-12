
#ifndef _MANGUSTA_INCLUDE_
#define _MANGUSTA_INCLUDE_




#include "apr.h"
#include "apr_errno.h"          /* Needed for apr_status_t */
#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_network_io.h"

#define APR_ERROR       APR_EGENERAL
typedef short int       apr_bool_t;
#define APR_ASSERT( data ) { assert(data); }

#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif


#ifdef WIN32
#define WARN_UNUSED
#else
    #if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
	#define WARN_UNUSED __attribute__((warn_unused_result))
    #else
	#define WARN_UNUSED __attribute__((warn_unused_result))
    #endif
#endif



typedef struct mangusta_ctx_s mangusta_ctx_t;
typedef struct mangusta_connection_s mangusta_connection_t;
typedef apr_status_t (*mangusta_ctx_connect_cb_f) (mangusta_ctx_t *ctx, apr_socket_t *sock, apr_pool_t *pool);

APR_DECLARE(apr_status_t) mangusta_init(void);
APR_DECLARE(apr_status_t) mangusta_shutdown(void);

APR_DECLARE(mangusta_ctx_t *) mangusta_context_new(void);

APR_DECLARE(apr_pool_t *) mangusta_context_get_pool(mangusta_ctx_t *ctx);
APR_DECLARE(apr_status_t) mangusta_context_set_host(mangusta_ctx_t *ctx, const char *host);
APR_DECLARE(apr_status_t) mangusta_context_set_port(mangusta_ctx_t *ctx, int port);
APR_DECLARE(apr_status_t) mangusta_context_set_max_connections(mangusta_ctx_t *ctx, apr_size_t max);
APR_DECLARE(apr_status_t) mangusta_context_set_max_idle(mangusta_ctx_t *ctx, apr_size_t max);
APR_DECLARE(apr_status_t) mangusta_context_set_connect_cb(mangusta_ctx_t *ctx, mangusta_ctx_connect_cb_f cb);

APR_DECLARE(apr_status_t) mangusta_context_start(mangusta_ctx_t *ctx);
APR_DECLARE(apr_status_t) mangusta_context_wait(mangusta_ctx_t *ctx);
APR_DECLARE(apr_status_t) mangusta_context_background(mangusta_ctx_t *ctx);
APR_DECLARE(apr_status_t) mangusta_context_running(mangusta_ctx_t *ctx);
APR_DECLARE(apr_status_t) mangusta_context_stop(mangusta_ctx_t *ctx);
APR_DECLARE(apr_status_t) mangusta_context_free(mangusta_ctx_t *ctx);

#endif
