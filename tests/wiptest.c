
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <curl/curl.h>

#include <mangusta.h>

static apr_status_t on_request_ready(mangusta_ctx_t * ctx, mangusta_request_t * req) {
    char *url;
    assert(ctx);


    url = mangusta_request_url_get(req);

    if ( strstr(url, "stop") ) {
        mangusta_context_stop(ctx);
    }

    mangusta_response_status_set(req, 200, "OK");
    mangusta_response_header_set(req, "X-test", "Success");
    mangusta_response_header_set(req, "Connection", "close");
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
