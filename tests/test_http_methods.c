
#define URL1 "http://127.0.0.1:8090/test"
#define URL2 "http://127.0.0.1:8090/stop"

static apr_status_t on_request_ready(mangusta_ctx_t * ctx, mangusta_request_t * req) {
    char *location;
    char *method;
    char *version;

    (void) ctx;

    location = mangusta_request_header_get(req, "host");
    method = mangusta_request_method_get(req);
    version = mangusta_request_protoversion_get(req);

    mangusta_response_status_set(req, 200, "OK boss");
    mangusta_response_header_set(req, "X-test", "foobar");
    mangusta_response_header_set(req, "Content-Type", "text/plain; charset=UTF-8");

    mangusta_response_body_appendf(req, "Test di accodamento\n");
    mangusta_response_body_appendf(req, "Test n° %d per capire\n", 2);

    printf("** %s => %s %s %s\n", __FUNCTION__, method, location, version);

    mangusta_context_stop(ctx);

    return APR_SUCCESS;
}

static apr_status_t on_request_headers(mangusta_ctx_t * ctx, mangusta_request_t * req) {
    char *host = mangusta_request_header_get(req, "host");

    (void) ctx;

    printf("** %s - Host: %s\n", __FUNCTION__, host);
    return APR_SUCCESS;
}

static void curl_perform(mangusta_ctx_t * ctx) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl != NULL) {
        struct curl_slist *chunk1 = NULL;
        struct curl_slist *chunk2 = NULL;

        chunk1 = curl_slist_append(chunk1, "Connection: keep-alive");
        chunk1 = curl_slist_append(chunk1, "X-TestSuite: true");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk1);

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Test suite");
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        //curl_easy_setopt(curl, CURLOPT_PATH_AS_IS, 1L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Test suite");
        //curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, URL1);
        res = curl_easy_perform(curl);

        if (CURLE_OK == res) {
            char *ct;
            long rc;
            /* ask for the content-type */
            res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rc);
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            if ((CURLE_OK == res) && ct) {
                printf("%ld - We received Content-Type: %s\n", rc, ct);
            }
        } else {
            printf("CURL received an error\n");
        }

        /* free the custom headers */
        curl_slist_free_all(chunk1);

        chunk2 = curl_slist_append(chunk2, "Connection: close");
        chunk2 = curl_slist_append(chunk2, "X-TestSuite: true");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk2);

        curl_easy_setopt(curl, CURLOPT_URL, URL2);
        res = curl_easy_perform(curl);

        if (CURLE_OK == res) {
            char *ct;
            long rc;
            /* ask for the content-type */
            res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rc);
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            if ((CURLE_OK == res) && ct) {
                printf("%ld - We received Content-Type: %s\n", rc, ct);
            }
        } else {
            printf("CURL received an error\n");
        }

        /* free the custom headers */
        curl_slist_free_all(chunk2);

        /* always cleanup */
        curl_easy_cleanup(curl);

        mangusta_context_stop(ctx);
    }
}

static void test_http_methods(void **UNUSED(foo)) {
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

    assert_int_equal(mangusta_context_set_request_header_cb(ctx, on_request_headers), APR_SUCCESS);
    assert_int_equal(mangusta_context_set_request_ready_cb(ctx, on_request_ready), APR_SUCCESS);

    assert_int_equal(mangusta_context_start(ctx), APR_SUCCESS);

    assert_int_equal(mangusta_context_background(ctx), APR_SUCCESS);

    curl_perform(ctx);
    while (mangusta_context_running(ctx) == APR_SUCCESS) {
        apr_sleep(APR_USEC_PER_SEC / 20);
    }

    /* mangusta_context_stop(ctx); */

    assert_int_equal(mangusta_context_free(ctx), APR_SUCCESS);
    mangusta_shutdown();

    return;
}
