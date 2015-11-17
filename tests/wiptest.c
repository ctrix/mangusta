
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <curl/curl.h>

#include <mangusta.h>

/*
static void curl_get(char *url) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl != NULL) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        res = curl_easy_perform(curl);

        if (CURLE_OK == res) {
            char *ct;
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            if ((CURLE_OK == res) && ct) {
                printf("We received Content-Type: %s\n", ct);
            }
        } else {
            //printf("CURL received an error\n");
        }

        curl_easy_cleanup(curl);
    }
}
*/

int main(void) {
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

//    assert_int_equal(mangusta_context_set_request_header_cb(ctx, on_request_headers), APR_SUCCESS);
//    assert_int_equal(mangusta_context_set_request_ready_cb(ctx, on_request_ready), APR_SUCCESS);

    assert_int_equal(mangusta_context_start(ctx), APR_SUCCESS);

    assert_int_equal(mangusta_context_background(ctx), APR_SUCCESS);

    while (mangusta_context_running(ctx) == APR_SUCCESS) {
        apr_sleep(APR_USEC_PER_SEC / 100);
    }

    mangusta_context_stop(ctx);

    assert_int_equal(mangusta_context_free(ctx), APR_SUCCESS);
    mangusta_shutdown();

    return 0;
}
