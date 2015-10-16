
static void curl_get_method(char *url, const char *method) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl != NULL) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        res = curl_easy_perform(curl);

        if (CURLE_OK == res) {
            char *ct;
            /* ask for the content-type */
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            if ((CURLE_OK == res) && ct) {
                printf("We received Content-Type: %s\n", ct);
            }
        } else {
            //printf("CURL received an error\n");
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
}

void *test_http_methods(void **foo) {
    apr_status_t status;
    mangusta_ctx_t *ctx;
    apr_pool_t *pool;

    status = mangusta_init();
    assert_int_equal(status, APR_SUCCESS);

    ctx = mangusta_context_new();
    assert_non_null(ctx);

    pool = mangusta_context_get_pool(ctx);
    assert_non_null(pool);

    assert_int_equal(mangusta_context_set_host(ctx, "127.0.0.1"), APR_SUCCESS);
    assert_int_equal(mangusta_context_set_port(ctx, 8090), APR_SUCCESS);
    assert_int_equal(mangusta_context_set_max_connections(ctx, 1024), APR_SUCCESS);
    assert_int_equal(mangusta_context_set_max_idle(ctx, 1024), APR_SUCCESS);

    assert_int_equal(mangusta_context_start(ctx), APR_SUCCESS);

    assert_int_equal(mangusta_context_background(ctx), APR_SUCCESS);

    curl_get_method("http://127.0.0.1:8090/test", "GET");

    while (mangusta_context_running(ctx) == APR_SUCCESS) {
        apr_sleep(100000);
    }

    assert_int_equal(mangusta_context_free(ctx), APR_SUCCESS);
    mangusta_shutdown();

    return NULL;
}
