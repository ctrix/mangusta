
static apr_status_t connect_cb(mangusta_ctx_t *ctx, apr_socket_t *sock, apr_pool_t *pool) {
    assert(ctx && sock && pool);

    assert_int_equal(           mangusta_context_stop(ctx), APR_SUCCESS);

    return APR_SUCCESS;
}

void test_connection() {
    apr_status_t status;
    mangusta_ctx_t *ctx;
    apr_pool_t *pool;

    status = mangusta_init();
    assert_int_equal(status, APR_SUCCESS);

    ctx = mangusta_context_new();
    assert_non_null(ctx);

    pool = mangusta_context_get_pool(ctx);
    assert_non_null(pool);

    assert_int_equal(           mangusta_context_set_host(ctx, "127.0.0.1"), APR_SUCCESS);
    assert_int_equal(           mangusta_context_set_port(ctx, 8090), APR_SUCCESS);
    assert_int_equal(           mangusta_context_set_max_connections(ctx, 1024), APR_SUCCESS);
    assert_int_equal(           mangusta_context_set_max_idle(ctx, 1024), APR_SUCCESS);

    assert_int_equal(           mangusta_context_start(ctx), APR_SUCCESS);

    assert_int_equal(           mangusta_context_set_connect_cb(ctx, connect_cb), APR_SUCCESS);

    assert_int_equal(           mangusta_context_background(ctx), APR_SUCCESS);

    curl_get("http://127.0.0.1:8090/test");

    while ( mangusta_context_running(ctx) == APR_SUCCESS ) {
        apr_sleep(100000);
    }

    assert_int_equal(           mangusta_context_free(ctx), APR_SUCCESS);
    mangusta_shutdown();

    return;
}
