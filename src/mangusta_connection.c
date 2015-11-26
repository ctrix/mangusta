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
#if MANGUSTA_DEBUG >= 1
        mangusta_log(MANGUSTA_LOG_DEBUG, "[Read %zu bytes]", tot);
#endif
        b[tot] = '\0';
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

static apr_status_t buffer_size_ok(mangusta_connection_t * conn) {
    apr_uint32_t blen;
    char *bdata;

    assert(conn);

    blen = mangusta_buffer_get_char(conn->buffer_r, &bdata);

    if (blen < REQUEST_HEADERS_MAX_SIZE) {
        return APR_SUCCESS;
    }

    return APR_ERROR;
}

static void *APR_THREAD_FUNC conn_thread_run(apr_thread_t * UNUSED(thread), void *data) {
    char *ip;
    char skip_poll = 0;
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
        char *ctype;
        apr_int32_t num;
        const apr_pollfd_t *ret_pfd;

        if (!skip_poll) {
            rv = apr_pollset_poll(conn->pollset, DEFAULT_POLL_TIMEOUT, &num, &ret_pfd);
        } else {
            skip_poll = 0;
            num = 0;
            rv = APR_SUCCESS;
        }

        if (rv == APR_SUCCESS) {

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
                            conn->current = req;
                        } else {
                            mangusta_response_status_set(req, 500, "Internal server error");
                            mangusta_log(MANGUSTA_LOG_ERROR, "Cannot create new request structure");
                            mangusta_error_write(req);
                            goto done;
                        }
                    }

                    if (buffer_size_ok(conn) != APR_SUCCESS) {
                        /* Buffer size is too large. */
                        mangusta_response_status_set(req, 400, "Bad Request");
                        mangusta_log(MANGUSTA_LOG_ERROR, "Request with headers too long");
                        mangusta_error_write(req);
                        goto done;
                    }

                    if ((conn->current->state == MANGUSTA_REQUEST_HEADERS) && (buffer_contains_headers(conn) == APR_SUCCESS)) {
                        mangusta_log(MANGUSTA_LOG_DEBUG, "Read buffer contains headers");
                        if (mangusta_request_parse_headers(req) != APR_SUCCESS) {
                            mangusta_response_status_set(req, 400, "Bad Request");
                            mangusta_log(MANGUSTA_LOG_ERROR, "Request with bad headers");
                            mangusta_error_write(req);
                            goto done;
                        } else {
                            mangusta_request_extract_querystring(req);

                            if (conn->ctx->on_request_h != NULL) {
                                if (conn->ctx->on_request_h(conn->ctx, req) != APR_SUCCESS) {
                                    /* We're asked to stop and disconnect with a 400 Bad Request TODO */
                                    // TODO TEST this condition
                                    mangusta_error_write(req);
                                    goto done;
                                } else {
                                    char *exp;
                                    char *cont = "HTTP/1.1 100 Continue\r\n\r\n";
                                    apr_size_t blen = strlen(cont);

                                    if (((exp = mangusta_request_header_get(conn->current, "Expect")) != NULL) && (strstr(exp, "continue"))) {
                                        mangusta_log(MANGUSTA_LOG_DEBUG, "Sending 100-continue");
                                        if (apr_socket_send(conn->sock, cont, &blen) != APR_SUCCESS) {
                                            goto done;
                                        }
                                    }
                                }
                            }
                        }

                        if (mangusta_request_has_payload(req) == APR_SUCCESS) {
                            mangusta_log(MANGUSTA_LOG_DEBUG, "Mangusta request contains payload");
                            assert(mangusta_request_state_change(req, MANGUSTA_REQUEST_PAYLOAD) == APR_SUCCESS);
                        } else {
                            mangusta_log(MANGUSTA_LOG_DEBUG, "Mangusta request without payload");
                            assert(mangusta_request_state_change(req, MANGUSTA_RESPONSE_HEADERS) == APR_SUCCESS);
                            conn->state = MANGUSTA_CONNECTION_RESPONSE;
                        }

                        skip_poll = 1;
                        continue;
                    } else if (conn->current->state == MANGUSTA_REQUEST_PAYLOAD) {
                        // TODO ASD + Add TEST
                        mangusta_request_feed(req, conn->buffer_r);

                        if (mangusta_request_payload_received(req) == APR_SUCCESS) {
                            mangusta_log(MANGUSTA_LOG_DEBUG, "Mangusta Request Payload received");
                            //assert(0);
                            assert(mangusta_request_state_change(req, MANGUSTA_RESPONSE_HEADERS) == APR_SUCCESS);
                            conn->state = MANGUSTA_CONNECTION_RESPONSE;
                            skip_poll = 1;
                            continue;
                        }
                    }
                    break;
                case MANGUSTA_CONNECTION_RESPONSE:
                    /*
                       Here the application should wait for a response to be sent
                       The control is passed to the user-defined function where the user of
                       the library sets up the headers and the eventual payload of the response.

                       If no callback is set, then the default is to reply with a 500 Internal Server Error
                     */
                    ctype = mangusta_request_header_get(req, "Content-Type");
                    if (ctype != NULL) {
                        if (strncmp(ctype, "application/x-www-form-urlencoded", sizeof("application/x-www-form-urlencoded")) == 0) {
                            mangusta_request_extract_form_urlencoded(req);
                        }

                        if (strncmp(ctype, "multipart/form-data", sizeof("multipart/form-data") - 1) == 0) {
                            mangusta_request_extract_multipart(req);
                        }
                    }

                    if (conn->ctx->on_request_r != NULL) {
                        if (conn->ctx->on_request_r(conn->ctx, req) != APR_SUCCESS) {
                            // TODO TEST this condition
                            mangusta_error_write(req);
                            goto done;
                        }
                    } else {
                        /* TODO 500 - Test auto-message */
                        mangusta_response_status_set(req, 500, NULL);
                    }

                    if (mangusta_response_write(req) != APR_SUCCESS) {
                        /* TODO Disconnect the connection */
                        mangusta_response_status_set(req, 500, NULL);
                        mangusta_error_write(req);
                        goto done;
                    }

                    mangusta_request_destroy(conn->current);
                    conn->current = NULL;
                    conn->state = MANGUSTA_CONNECTION_NEW;

                    break;
                case MANGUSTA_CONNECTION_WEBSOCKET:
                    assert(0);
                    break;
            }
        } else if (rv == APR_TIMEUP) {
            apr_time_t delta = apr_time_now() - conn->last_io;
            if ((float) delta / APR_USEC_PER_SEC >= conn->httpkeepalive) {
                mangusta_log(MANGUSTA_LOG_DEBUG, "Keepalive expired: (%.2fs). Terminating connection.", (float) delta / APR_USEC_PER_SEC);
                conn->terminated = 1;
                apr_socket_close(conn->sock);
            }
        } else if ((rv == APR_EOF) || (rv == APR_EINTR)) {
            /* Not such a big problem: Close connection */
            conn->terminated = 1;
            apr_socket_close(conn->sock);
        } else {
            mangusta_log(MANGUSTA_LOG_ERROR, "Poll result not managed ** %d ** Please report to the developer\n", rv);
#if MANGUSTA_DEBUG >= 1
            assert(0);          /* Should never happen. */
#endif
            conn->terminated = 1;
            apr_socket_close(conn->sock);
        }

    }

 done:
#if MANGUSTA_DEBUG >= 1
    mangusta_log(MANGUSTA_LOG_DEBUG, "Terminating connection thread");
#endif
    mangusta_connection_destroy(conn);

    return NULL;
}

apr_status_t mangusta_connection_play(mangusta_connection_t * conn) {

    conn_thread_run(NULL, conn);

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
        c->last_io = apr_time_now();
        c->httpkeepalive = ctx->httpkeepalive;

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

APR_DECLARE(apr_status_t) mangusta_connection_set_http_keepalive(mangusta_connection_t * conn, apr_int32_t sec) {
    assert(conn);
    conn->httpkeepalive = sec;
    return APR_SUCCESS;
}

#endif
