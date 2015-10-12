#ifndef MANGUSTA_CONNECTION_H

#include "mangusta_private.h"

static void on_disconnect(mangusta_connection_t * conn) {

    assert(conn);

#if DEBUG_CONNECTION > 5
    nn_log(NN_LOG_DEBUG, "On client Disconnect");
#endif

    conn->terminated = 1;

    if (conn->sock) {
        apr_pollset_remove(conn->pollset, &conn->pfd);
        while (apr_socket_close(conn->sock) != APR_SUCCESS) {
#if DEBUG_CONNECTION >= 1
            nn_log(NN_LOG_DEBUG, "Closing client side socket");
#endif
            apr_sleep(10000);
        };
        conn->sock = NULL;
    }

    return;
}

static void on_read(mangusta_connection_t *conn) {
    apr_size_t tot;
    apr_status_t rv;
    char b[1024];

    tot = sizeof(b);

    rv = apr_socket_recv(conn->sock, b, &tot);
    if ( rv == APR_SUCCESS ) {

        mangusta_buffer_append(conn->buffer_r, b, tot);
        printf("%zu %u<-----------\n", tot, mangusta_buffer_size(conn->buffer_r) );
        printf("%s\n", b);
/*
*/
    }
    /* APR_EOF MEans the stream has been closed */
    else {
        on_disconnect(conn);
    }

    return;
}

static int eloops = 0; // TODO Remove this dev hack

static void *APR_THREAD_FUNC conn_thread_run(apr_thread_t * UNUSED(thread), void *data) {
    char *ip;
    apr_status_t rv;
    apr_sockaddr_t *sa = NULL;
    mangusta_connection_t *conn;

    assert(data);
    conn = (mangusta_connection_t *) data;

    apr_socket_addr_get(&sa, APR_REMOTE, conn->sock);
    apr_sockaddr_ip_get(&ip, sa);

#if DEBUG_CONNECTION >= 1
    nn_log(NN_LOG_DEBUG, "New connection from IP: %s PORT: %d for profile %s", ip, sa->port, conn->profile->name);
#endif

    rv = apr_pollset_create(&conn->pollset, DEFAULT_POLLSET_NUM, conn->pool, APR_POLLSET_WAKEABLE);
    assert( rv == APR_SUCCESS );

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

    eloops = 0;

    while (!conn->terminated) {
        int i;
        apr_int32_t num;
        apr_socket_t *s;
        const apr_pollfd_t *ret_pfd;

        rv = apr_pollset_poll(conn->pollset, DEFAULT_POLL_TIMEOUT, &num, &ret_pfd);

        if (rv == APR_SUCCESS) {
            assert(num == 1);
            conn->last_io = apr_time_now();

            for (i = 0; i < num; i++) {
                s = ret_pfd[i].desc.s;

                on_read(conn);
                apr_socket_close(conn->sock);
            }
        }
        else if (rv == APR_TIMEUP) {
                //printf("************** TIMEOUT %d\n", rv);
                eloops++;
                if ( eloops >= 2) {
                    on_disconnect(conn);
                }
        }
        else if (rv == APR_EOF) {
                printf("************** EOF %d\n", rv);
                apr_pollset_remove(conn->pollset, &conn->pfd);
                apr_socket_close(conn->sock);
                break;
        }
        else if (rv == APR_EINTR) {
                printf("************** EINTR %d\n", rv);
                apr_socket_close(conn->sock);
                break;
        }
        else {
                printf("************** %d *****\n", rv);
//                assert( rv == APR_SUCCESS);
//            client_timer_inactivity_check(conn);
        }
    }

#if DEBUG_CONNECTION >= 1
    nn_log(NN_LOG_DEBUG, "Terminating connection thread");
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

//        nn_log(NN_LOG_ERROR, "Cannot start thread on incoming connection from IP: %s PORT: %d for ctx %s", ip, sa->port, conn->ctx->name);
        return APR_ERROR;
    }

    return APR_SUCCESS;
}

mangusta_connection_t *mangusta_connection_create(mangusta_ctx_t * ctx, apr_socket_t * sock, apr_pool_t * pool) {
    mangusta_connection_t *c = NULL;

    assert(pool);

    c = apr_pcalloc(pool, sizeof(mangusta_connection_t));
    if (c != NULL) {
        c->ctx = ctx;
        c->sock = sock;
        c->pool = pool;
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

#ifdef NP_POOL_DEBUG
    {
        apr_pool_t *pool;
        pool = conn->pool;

        apr_size_t s = apr_pool_num_bytes(pool, 1);
        nn_log(NN_LOG_DEBUG, "Connection pool size is %zu", (size_t) s);
    }
#endif

    apr_pool_destroy(conn->pool);

    return;
}

#endif
