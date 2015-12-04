/*
    This tests methods with a Payload.
    Basically we're interested in testing
    - POST multipart *
    - POST chunked *
    - PUT raw *
    - PUT chunked
    - DELETE (only to document curl with a custom method, but without payload) *
*/

#include "_test_framework.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <curl/curl.h>

#include <mangusta.h>

#define URL1 "/test1"
#define URL2 "/test2"
#define URL3 "/test3"
#define URL4 "/test4"

#define URL_BASE "http://" MANGUSTA_TEST_BINDTO ":" MANGUSTA_TEST_BINDPORT

static int must_shutdown = 0;
static apr_size_t read_total = 0;

/* Never writes anything, just returns the size presented */
size_t curl_dummy_write(char *UNUSED(ptr), size_t size, size_t nmemb, void *UNUSED(userdata)) {
    return size * nmemb;
}

static size_t curl_dummy_read(void *ptr, size_t size, size_t nmemb, void *UNUSED(stream)) {
    apr_size_t max = size * nmemb;

    if (max > 1024) {
        max = 1001;
    }

    if (read_total < 25000) {
        memset(ptr, 'A', max);
        read_total += max;
        return max;
    } else {
        return 0;
    }
}

static void curl_perform(mangusta_ctx_t * ctx, const char *url, long ver, const char *method) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl != NULL) {
        struct curl_slist *chunk = NULL;
        struct curl_httppost *formpost = NULL;
        struct curl_httppost *lastptr = NULL;

        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Test suite");
        /*curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_NONE); */
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, ver);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_dummy_write);

        if (!strcmp(method, "GET")) {
            /* Nothing to do */
        } else if (!strcmp(method, "POST")) {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "sendfile", CURLFORM_FILE, __FILE__, CURLFORM_END);
            curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "filename", CURLFORM_COPYCONTENTS, __FILE__, CURLFORM_END);
            curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit", CURLFORM_COPYCONTENTS, "send", CURLFORM_END);
            curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
        } else if (!strcmp(method, "PUT")) {
            curl_easy_setopt(curl, CURLOPT_PUT, 1L);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_dummy_read);
        } else if (!strcmp(method, "DELETE")) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, curl_dummy_read);
        } else {
            assert_non_null(1);
        }

        if (strstr(url, "test2") != NULL) {
            chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
        }

        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        res = curl_easy_perform(curl);

        if (CURLE_OK == res) {
            char *ct;
            long rc;

            res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rc);
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            printf("CLIENT STATUS: %ld\n", rc);
            assert_int_equal(rc, 200);

        } else {
            assert_int_equal(res, CURLE_OK);
        }

        if (must_shutdown) {
            mangusta_context_stop(ctx);
        }

        curl_slist_free_all(chunk);

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    return;
}

static apr_status_t on_headers_ready(mangusta_ctx_t * UNUSED(ctx), mangusta_request_t * req) {
    apr_status_t status = APR_SUCCESS;

    (void) req;
/*
    char *url;
    url = mangusta_request_url_get(req);
*/
    return status;
}

static apr_status_t on_request_ready(mangusta_ctx_t * ctx, mangusta_request_t * req) {
    char *url;
    apr_status_t status = APR_SUCCESS;

    (void) ctx;
    url = mangusta_request_url_get(req);

    mangusta_response_status_set(req, 200, "OK Pass");

    printf("Done: %s\n", url);

    return status;
}

static void test_perform(void **UNUSED(foo)) {
    MANGUSTA_TEST_SETUP;
    MANGUSTA_TEST_START;

    assert_int_equal(mangusta_context_set_request_header_cb(ctx, on_headers_ready), APR_SUCCESS);
    assert_int_equal(mangusta_context_set_request_ready_cb(ctx, on_request_ready), APR_SUCCESS);

    curl_perform(ctx, URL_BASE URL1 "a", CURL_HTTP_VERSION_1_0, "POST");
    curl_perform(ctx, URL_BASE URL1 "b", CURL_HTTP_VERSION_1_1, "POST");
    curl_perform(ctx, URL_BASE URL2 "a", CURL_HTTP_VERSION_1_0, "POST");
    curl_perform(ctx, URL_BASE URL2 "b", CURL_HTTP_VERSION_1_1, "POST");
    /* curl_perform(ctx, URL_BASE URL3 "a", CURL_HTTP_VERSION_1_0, "PUT"); // Chunk upload not supported  */
    curl_perform(ctx, URL_BASE URL3 "b", CURL_HTTP_VERSION_1_1, "PUT");
    must_shutdown = 1;
    curl_perform(ctx, URL_BASE URL4, CURL_HTTP_VERSION_1_1, "DELETE");

    while (mangusta_context_running(ctx) == APR_SUCCESS) {
        apr_sleep(APR_USEC_PER_SEC / 20);
    }

    MANGUSTA_TEST_DISPOSE;

    return;
}

int main(void) {
    const struct UnitTest tests[] = {
        unit_test(test_perform),
    };

    return run_tests(tests);
}
