
#define MANGUSTA_TEST_BINDTO   "127.0.0.1"
#define MANGUSTA_TEST_BINDPORT 8090

#define MANGUSTA_TEST_INCLUDES \
    #include <assert.h> \
    #include <stdarg.h> \
    #include <stddef.h> \
    #include <setjmp.h> \
    #include <cmocka.h> \
    #include <curl/curl.h> \
    #include <mangusta.h>


#define MANGUSTA_TEST_SETUP \
    apr_status_t status; \
    mangusta_ctx_t *ctx; \
    apr_pool_t *pool; \
    \
    status = mangusta_init(); \
    assert_int_equal(status, APR_SUCCESS); \
    \
    ctx = mangusta_context_new(); \
    assert_non_null(ctx); \
    \
    pool = mangusta_context_get_pool(ctx); \
    assert_non_null(pool); \
    \
    assert_int_equal(mangusta_context_set_host(ctx, MANGUSTA_TEST_BINDTO), APR_SUCCESS); \
    assert_int_equal(mangusta_context_set_port(ctx, MANGUSTA_TEST_BINDPORT), APR_SUCCESS); \
    assert_int_equal(mangusta_context_set_max_connections(ctx, 1024), APR_SUCCESS); \
    assert_int_equal(mangusta_context_set_max_idle(ctx, 1024), APR_SUCCESS); \
    \
    assert_int_equal(mangusta_context_start(ctx), APR_SUCCESS); \
    assert_int_equal(mangusta_context_background(ctx), APR_SUCCESS);



#define MANGUSTA_TEST_DISPOSE \
    assert_int_equal(mangusta_context_free(ctx), APR_SUCCESS); \
    mangusta_shutdown();


