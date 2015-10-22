#ifndef MANGUSTA_CONNECTION_H

#include "mangusta_private.h"

static void on_disconnect(mangusta_connection_t * conn) {
    assert(conn);

#if MANGUSTA_DEBUG > 5
    mangusta_log(MANGUSTA_LOG_DEBUG, "On client Disconnect");
#endif

    conn->terminated = 1;

    if (conn->sock) {
        apr_pollset_remove(conn->pollset, &conn->pfd);
        while (apr_socket_close(conn->sock) != APR_SUCCESS) {
#if MANGUSTA_DEBUG >= 1
            mangusta_log(MANGUSTA_LOG_DEBUG, "Closing client side socket");
#endif
            apr_sleep(APR_USEC_PER_SEC / 10);
        };
        conn->sock = NULL;
    }

    return;
}

static void on_read(mangusta_connection_t * conn) {
    apr_size_t tot;
    apr_status_t rv;
    char b[1024];

    tot = sizeof(b) - 2;

    rv = apr_socket_recv(conn->sock, b, &tot);
    if (rv == APR_SUCCESS) {
        mangusta_log(MANGUSTA_LOG_DEBUG, "[Read %zu bytes]", tot);
/*
        b[tot] = '\0';
        printf("************** %s\n", b);
*/
        mangusta_buffer_append(conn->buffer_r, b, tot);
        return;
    }
    /* APR_EOF Means the stream has been closed */
    else {
        on_disconnect(conn);
    }

    return;
}

static apr_status_t buffer_contains_headers(mangusta_connection_t * conn) {
    apr_uint32_t blen;
    char *bdata;
    char *end;

    assert(conn);

    blen = mangusta_buffer_get_char(conn->buffer_r, &bdata);
    if ((blen > 0) && (end = strnstr(bdata, HEADERS_END_MARKER, blen)) != NULL) {
        return APR_SUCCESS;
    }
    return APR_ERROR;
}

static void *APR_THREAD_FUNC conn_thread_run(apr_thread_t * UNUSED(thread), void *data) {
    char *ip;
    apr_status_t rv;
    apr_sockaddr_t *sa = NULL;
    mangusta_connection_t *conn;
    mangusta_request_t *req = NULL;

    assert(data);
    conn = (mangusta_connection_t *) data;

    apr_socket_addr_get(&sa, APR_REMOTE, conn->sock);
    apr_sockaddr_ip_get(&ip, sa);

#if MANGUSTA_DEBUG >= 1
    printf("New connection from IP: %s PORT: %d\n", ip, sa->port);
#endif

    rv = apr_pollset_create(&conn->pollset, DEFAULT_POLLSET_NUM, conn->pool, APR_POLLSET_WAKEABLE);
    assert(rv == APR_SUCCESS);

    conn->pfd.p = conn->pool;
    conn->pfd.desc_type = APR_POLL_SOCKET;
    conn->pfd.reqevents = APR_POLLIN;
    conn->pfd.rtnevents = 0;
    conn->pfd.desc.f = NULL;
    conn->pfd.desc.s = conn->sock;
    conn->pfd.client_data = NULL;

    apr_pollset_add(conn->pollset, &conn->pfd);

    conn->buffer_r = mangusta_buffer_init(conn->pool, 0, 0);
    assert(conn->buffer_r != NULL);

    conn->terminated = 0;

    while (!conn->terminated) {
        apr_int32_t num;
        const apr_pollfd_t *ret_pfd;

        rv = apr_pollset_poll(conn->pollset, DEFAULT_POLL_TIMEOUT, &num, &ret_pfd);

        if ((rv == APR_SUCCESS) || (rv == APR_TIMEUP)) {

            assert(num <= 1);
            conn->last_io = apr_time_now();

            /* Read only when at least one poll descriptor is ready */
            if (num > 0) {
                on_read(conn);
            }

            switch (conn->state) {
                case MANGUSTA_CONNECTION_NEW:
                case MANGUSTA_CONNECTION_REQUEST:

                    if (conn->current == NULL) {
                        mangusta_log(MANGUSTA_LOG_DEBUG, "Creating Request");
                        if (mangusta_request_create(conn, &req) == APR_SUCCESS) {
                            // TODO ASD
                            conn->current = req;
                        }
                    }

                    if ((conn->current->state == MANGUSTA_REQUEST_HEADERS) && buffer_contains_headers(conn) == APR_SUCCESS) {
                        mangusta_log(MANGUSTA_LOG_DEBUG, "Read buffer contains headers");
                        if (mangusta_request_parse_headers(req) != APR_SUCCESS) {
                            // TODO
                            assert(0);
                        } else {
                            if (conn->ctx->on_request_h != NULL) {
                                conn->ctx->on_request_h(conn->ctx, req);
                            }
                        }

                        if (mangusta_request_has_payload(req) == APR_SUCCESS) {
                            mangusta_log(MANGUSTA_LOG_DEBUG, "Mangusta request contains payload");
                            assert(mangusta_request_state_change(req, MANGUSTA_REQUEST_PAYLOAD) == APR_SUCCESS);
                            //assert(0);
                        } else {
                            mangusta_log(MANGUSTA_LOG_DEBUG, "Mangusta request without payload");
                            assert(mangusta_request_state_change(req, MANGUSTA_RESPONSE_HEADERS) == APR_SUCCESS);
                            conn->state = MANGUSTA_CONNECTION_RESPONSE;
                            //assert(0);
                        }
                    } else if (conn->current->state == MANGUSTA_REQUEST_PAYLOAD) {
                        // TODO ASD
                    }
                    break;
                case MANGUSTA_CONNECTION_RESPONSE:
                    /*
                       Here the application should wait for a response to be sent
                       The control is passed to the user-defined function where the user of
                       the library sets up the headers and the eventual payload of the response.

                       If no callback is set, then the default is to reply with a 500 Internal Server Error
                     */
                    if (conn->ctx->on_request_r != NULL) {
                        if (conn->ctx->on_request_r(conn->ctx, req) != APR_SUCCESS) {
                            mangusta_response_status_set(req, 500, NULL);
                        }
                    } else {
                        /* TODO 500  */
                        mangusta_response_status_set(req, 500, NULL);
                    }

                    if (mangusta_response_write(req) != APR_SUCCESS) {
                        /* TODO Disconnect the connection */
                    }

                    mangusta_request_destroy(conn->current);
                    conn->current = NULL;
                    conn->state = MANGUSTA_CONNECTION_NEW;

                    break;
                case MANGUSTA_CONNECTION_WEBSOCKET:
                    assert(0);
                    break;
            }
        } else if ((rv == APR_TIMEUP) || (rv == APR_EOF) || (rv == APR_EINTR)
            ) {
            /* Not such a bug problem TODO Close connection after too much time */
            mangusta_log(MANGUSTA_LOG_DEBUG, "+* %d", conn->state);
        } else {
            printf("************** %d\n", rv);
            assert(0);
            conn->terminated = 1;
            apr_socket_close(conn->sock);
        }

    }

#if MANGUSTA_DEBUG >= 1
    mangusta_log(MANGUSTA_LOG_DEBUG, "Terminating connection thread");
#endif
    mangusta_connection_destroy(conn);

    return NULL;
}

apr_status_t mangusta_connection_play(mangusta_connection_t * conn) {
    apr_thread_t *thread;
    apr_threadattr_t *thd_attr;

    assert(conn);

    apr_threadattr_create(&thd_attr, conn->pool);
    apr_threadattr_stacksize_set(thd_attr, DEFAULT_THREAD_STACKSIZE);
    apr_threadattr_detach_set(thd_attr, 1);

    if (apr_thread_create(&thread, thd_attr, conn_thread_run, conn, conn->pool) != APR_SUCCESS) {
        apr_sockaddr_t *sa;
        char *ip;

        apr_socket_addr_get(&sa, APR_REMOTE, conn->sock);
        apr_sockaddr_ip_get(&ip, sa);

        mangusta_log(MANGUSTA_LOG_ERROR, "Cannot start thread on incoming connection from IP: %s PORT: %d", ip, sa->port);
        return APR_ERROR;
    }

    return APR_SUCCESS;
}

mangusta_connection_t *mangusta_connection_create(mangusta_ctx_t * ctx, apr_socket_t * sock) {
    mangusta_connection_t *c = NULL;
    apr_pool_t *npool;

    //assert( apr_pool_create(&npool, pool) == APR_SUCCESS);
    assert(apr_pool_create_core(&npool) == APR_SUCCESS);

    c = apr_pcalloc(npool, sizeof(mangusta_connection_t));
    if (c != NULL) {
        c->ctx = ctx;
        c->sock = sock;
        c->pool = npool;
        c->state = MANGUSTA_CONNECTION_NEW;

        apr_queue_create(&c->requests, DEFAULT_REQUESTS_PER_CONNECTION_QUEUE, c->pool);
    }

    return c;

/*
 err:
    apr_pool_destroy(pool);
*/

    return NULL;
}

void mangusta_connection_destroy(mangusta_connection_t * conn) {

    assert(conn);
    apr_pool_destroy(conn->pool);

    return;
}

#endif
