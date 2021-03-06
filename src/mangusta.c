
#ifndef MANGUSTA_H

#include "mangusta_private.h"

/* ******************************************************************************* */
/* ******************************************************************************* */
/* ******************************************************************************* */

APR_DECLARE(apr_status_t) mangusta_init(void) {
    apr_initialize();

    apr_signal(SIGPIPE, NULL);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_shutdown(void) {
    apr_terminate();
    return APR_SUCCESS;
}

APR_DECLARE(mangusta_ctx_t *) mangusta_context_new(void) {
    mangusta_ctx_t *ctx = NULL;
    apr_pool_t *pool;

    if (apr_pool_create_core(&pool) == APR_SUCCESS) {
        assert(pool);

        ctx = apr_pcalloc(pool, sizeof(mangusta_ctx_t));
        if (ctx != NULL) {
            ctx->pool = pool;
        }
    }

    return ctx;
}

APR_DECLARE(apr_pool_t *) mangusta_context_get_pool(mangusta_ctx_t * ctx) {
    if (ctx != NULL) {
        return ctx->pool;
    }

    return NULL;
}

APR_DECLARE(apr_status_t) mangusta_context_set_host(mangusta_ctx_t * ctx, const char *host) {
    assert(ctx && ctx->pool);

    if (!zstr(host)) {
        ctx->host = apr_pstrdup(ctx->pool, host);
        ctx->httpkeepalive = 5;
        return APR_SUCCESS;
    }

    return APR_ERROR;
}

APR_DECLARE(apr_status_t) mangusta_context_set_port(mangusta_ctx_t * ctx, int port) {
    assert(ctx && ctx->pool);

    if (port > 0) {
        ctx->port = port;
        return APR_SUCCESS;
    }

    return APR_ERROR;
}

APR_DECLARE(apr_status_t) mangusta_context_set_max_connections(mangusta_ctx_t * ctx, apr_size_t max) {
    assert(ctx && ctx->pool);

    ctx->maxconn = max;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_context_set_max_idle(mangusta_ctx_t * ctx, apr_size_t max) {
    assert(ctx && ctx->pool);

    ctx->maxidle = max;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_context_set_http_keepalive(mangusta_ctx_t * ctx, apr_int32_t sec) {
    assert(ctx && ctx->pool);

    ctx->httpkeepalive = sec;
    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_context_start(mangusta_ctx_t * ctx) {
    apr_status_t status;
    apr_sockaddr_t *sa;
    apr_socket_t *sock;
    apr_pollfd_t pfd = { ctx->pool, APR_POLL_SOCKET, APR_POLLIN, 0, {NULL}
    , NULL
    };

    assert(ctx);

#ifdef MANGUSTA_ENABLE_TLS
    if (mangusta_context_tls_enable(ctx) != APR_SUCCESS) {
        mangusta_log(MANGUSTA_LOG_ERROR, "Cannot enable SSL for this context.");
        return APR_ERROR;
    }
#endif

    status = apr_sockaddr_info_get(&sa, ctx->host, APR_UNSPEC, ctx->port, 0, ctx->pool);
    if (status != APR_SUCCESS) {
        mangusta_log(MANGUSTA_LOG_ERROR, "Cannot get sockaddr info for this context");
        return APR_ERROR;
    }

    status = apr_socket_create(&sock, sa->family, SOCK_STREAM, APR_PROTO_TCP, ctx->pool);
    status = apr_socket_opt_set(sock, APR_SO_REUSEADDR, 1);
    status = apr_socket_bind(sock, sa);
    status = apr_socket_listen(sock, 5);
    apr_socket_opt_set(sock, APR_TCP_DEFER_ACCEPT, 1);

    ctx->sock = sock;

    status = apr_pollset_create(&ctx->pollset, DEFAULT_POLLSET_NUM, ctx->pool, 0);
    assert(status == APR_SUCCESS);
#ifdef apr_pollset_method_name
    /*
       Older APR versions may not have this function.
       It's easier to conditionally remove this log line
       without testing for the correct APR version.
     */
    mangusta_log(MANGUSTA_LOG_DEBUG, "Context is using pollset method '%s'", apr_pollset_method_name(p->pollset));
#endif

    pfd.client_data = NULL;
    pfd.desc.s = ctx->sock;
    status = apr_pollset_add(ctx->pollset, &pfd);
    assert(status == APR_SUCCESS);

    status = apr_thread_pool_create(&ctx->tp, 1, ctx->maxconn, ctx->pool);
    assert(status == APR_SUCCESS);
    apr_thread_pool_idle_max_set(ctx->tp, ctx->maxidle);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_context_set_connect_cb(mangusta_ctx_t * ctx, mangusta_ctx_connect_cb_f cb) {
    assert(ctx);

    ctx->on_connect = cb;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_context_set_request_header_cb(mangusta_ctx_t * ctx, mangusta_ctx_request_header_cb_f cb) {
    assert(ctx);

    ctx->on_request_h = cb;

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_context_set_request_ready_cb(mangusta_ctx_t * ctx, mangusta_ctx_request_ready_cb_f cb) {
    assert(ctx);

    ctx->on_request_r = cb;

    return APR_SUCCESS;
}

struct tp_data {
    apr_pool_t *p;
    mangusta_ctx_t *c;
    apr_socket_t *s;
};

static void *on_client_connect(apr_thread_t * thread, void *data) {
    mangusta_ctx_t *ctx;
    mangusta_connection_t *conn;
    apr_pool_t *pool = NULL;
    apr_status_t rv;
    apr_socket_t *new_sock;
    struct tp_data *tpd = data;

    pool = tpd->p;
    ctx = tpd->c;
    new_sock = tpd->s;

    assert(thread && ctx && pool && new_sock);

    if (pool != NULL) {
        //apr_socket_opt_set(new_sock, APR_SO_NONBLOCK, 1);
        apr_socket_opt_set(new_sock, APR_SO_KEEPALIVE, 1);
        apr_socket_opt_set(new_sock, APR_SO_LINGER, 1);

        conn = mangusta_connection_create(ctx, new_sock);
        if (conn != NULL) {

            rv = APR_SUCCESS;
            if (ctx->on_connect != NULL) {
                rv = ctx->on_connect(ctx, new_sock, pool);
                // IF FAIL SET LINGER TO ZERO SO THAT THE SOCKET, ON SOME TCP STACK, CAN SEND A RST
                if (rv != APR_SUCCESS) {
                    apr_socket_opt_set(new_sock, APR_SO_LINGER, 0);
                }
            }

            if ((rv != APR_SUCCESS) || (mangusta_connection_play(conn) != APR_SUCCESS)) {
                mangusta_connection_destroy(conn);
            }
        } else {
            apr_socket_close(new_sock);
            apr_pool_destroy(pool);
        }
    }

    return NULL;
}

APR_DECLARE(apr_status_t) mangusta_context_wait(mangusta_ctx_t * ctx) {
    apr_status_t status, rv;
    apr_int32_t num;
    const apr_pollfd_t *ret_pfd = NULL;

    assert(ctx);

    status = APR_ERROR;

    rv = apr_pollset_poll(ctx->pollset, DEFAULT_POLL_TIMEOUT, &num, &ret_pfd);
    if (rv == APR_SUCCESS) {
        int i;
        assert(num > 0);
        for (i = 0; i < num; i++) {
            if ((ret_pfd[i].desc.s == ctx->sock) && (ret_pfd[i].rtnevents == APR_POLLIN)) {
                apr_pool_t *pool;
                apr_socket_t *sock;
                struct tp_data *tpd;

                apr_pool_create(&pool, ctx->pool);
                if (pool != NULL) {
                    rv = apr_socket_accept(&sock, ctx->sock, pool);
                    if (rv == APR_SUCCESS) {
                        tpd = apr_palloc(pool, sizeof(struct tp_data));
                        if (tpd != NULL) {
                            tpd->p = pool;
                            tpd->c = ctx;
                            tpd->s = sock;
                            status = apr_thread_pool_push(ctx->tp, (apr_thread_start_t) on_client_connect, tpd, APR_THREAD_TASK_PRIORITY_NORMAL, sock);
                        }
                    }
                }

                assert(status == APR_SUCCESS);
            } else {
                // This should never happen...
                assert(0);
            }
        }
    }

    return status;
}

static void *APR_THREAD_FUNC mangusta_ctx_loop(apr_thread_t * thd, void *data) {
    mangusta_ctx_t *ctx = data;

    while (!ctx->stopped) {
        mangusta_context_wait(ctx);
    }

    apr_thread_exit(thd, APR_SUCCESS);
    mangusta_log(MANGUSTA_LOG_DEBUG, "Context Thread terminated...");

    return NULL;
}

APR_DECLARE(apr_status_t) mangusta_context_background(mangusta_ctx_t * ctx) {
    apr_threadattr_t *th_attr = NULL;
    apr_status_t status;

    assert(ctx && ctx->pool);

    status = apr_threadattr_create(&th_attr, ctx->pool);
    assert(status == APR_SUCCESS);

    apr_threadattr_stacksize_set(th_attr, DEFAULT_THREAD_STACKSIZE);

    status = apr_thread_create(&ctx->thread, th_attr, mangusta_ctx_loop, ctx, ctx->pool);
    assert(status == APR_SUCCESS);

    return status;
}

APR_DECLARE(apr_status_t) mangusta_context_running(mangusta_ctx_t * ctx) {

    if (!ctx->stopped) {
        return APR_SUCCESS;
    }

    return APR_ERROR;
}

APR_DECLARE(apr_status_t) mangusta_context_stop(mangusta_ctx_t * ctx) {
    assert(ctx);

    ctx->stopped = 1;

    mangusta_log(MANGUSTA_LOG_DEBUG, "Context stopped");

    if (ctx->sock != NULL) {
        apr_socket_close(ctx->sock);
        ctx->sock = NULL;
    }

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_context_free(mangusta_ctx_t * ctx) {
    apr_status_t status = APR_SUCCESS;
    apr_pool_t *pool;
    assert(ctx);

    apr_thread_pool_destroy(ctx->tp);

    if (ctx->stopped != 1) {
        mangusta_context_stop(ctx);
    }

    mangusta_log(MANGUSTA_LOG_DEBUG, "Freeing Context");

    if (ctx->thread != NULL) {
        apr_thread_join(&status, ctx->thread);
    }

    pool = ctx->pool;

#ifdef MANGUSTA_ENABLE_TLS
    if (mangusta_context_tls_disable(ctx) != APR_SUCCESS) {
        mangusta_log(MANGUSTA_LOG_ERROR, "Cannot disable SSL for this context.");
    }
#endif

    // Improve cleanup: thread pool, pollset etcc...
    apr_pool_destroy(pool);

    return APR_SUCCESS;
}

#endif
