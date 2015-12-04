
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

static apr_status_t on_request_ready(mangusta_ctx_t * ctx, mangusta_request_t * req) {
    mangusta_response_status_set(req, 200, "OK");
    mangusta_response_header_set(req, "Connection", "keep-alive");
    mangusta_response_body_appendf(req, "Done!");
    return APR_SUCCESS;
}

int main(void) {
    mangusta_ctx_t *ctx;

    mangusta_init();

    ctx = mangusta_context_new();

    mangusta_context_get_pool(ctx);

    mangusta_context_set_host(ctx, "::");
    mangusta_context_set_port(ctx, 8090);
    mangusta_context_set_max_connections(ctx, 1024);
    mangusta_context_set_max_idle(ctx, 1024);

//    mangusta_context_set_request_header_cb(ctx, on_request_headers);
    mangusta_context_set_request_ready_cb(ctx, on_request_ready);

    mangusta_context_start(ctx);

    mangusta_context_background(ctx);

    while (mangusta_context_running(ctx) == APR_SUCCESS) {
        apr_sleep(APR_USEC_PER_SEC / 100);
    }

    mangusta_context_stop(ctx);

    mangusta_context_free(ctx);
    mangusta_shutdown();

    return 0;
}
