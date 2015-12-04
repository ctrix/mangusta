
/*
    This tests the hooks (headers and request ready) and checks that the return behaviour is correct
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

#define PARAMSIZE_BIG1 MANGUSTA_REQUEST_HEADER_MAX_LEN + 100
#define PARAMSIZE_BIG2 MANGUSTA_REQUEST_HEADERS_MAX_SIZE + 100

#define R_URL1 "/test1"
#define R_URL2 "/test2"
#define R_URL3 "/test3"
#define R_URL4 "/test4"

#define URL1 "http://" MANGUSTA_TEST_BINDTO ":" MANGUSTA_TEST_BINDPORT R_URL1
#define URL2 "http://" MANGUSTA_TEST_BINDTO ":" MANGUSTA_TEST_BINDPORT R_URL2
#define URL3 "http://" MANGUSTA_TEST_BINDTO ":" MANGUSTA_TEST_BINDPORT R_URL3
#define URL4 "http://" MANGUSTA_TEST_BINDTO ":" MANGUSTA_TEST_BINDPORT R_URL4

/* Never writes anything, just returns the size presented */
size_t curl_dummy_write(char *UNUSED(ptr), size_t size, size_t nmemb, void *UNUSED(userdata)) {
    return size * nmemb;
}

static void curl_perform(mangusta_ctx_t * ctx, const char *url, long ver) {
    CURL *curl;
    CURLcode res;

/*
TODO DEL
http://curl.haxx.se/libcurl/c/post-callback.html
*/

    curl = curl_easy_init();
    if (curl != NULL) {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Test suite");
        /*curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_NONE); */
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, ver);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_dummy_write);

        res = curl_easy_perform(curl);

        if (CURLE_OK == res) {
            char *ct;
            long rc;

            res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rc);
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            printf("STATUS: %ld\n", rc);

            if (strstr(url, "test1") != NULL) {
                assert_int_equal(rc, 200);
            } else if (strstr(url, "test2") != NULL) {
                assert_int_equal(rc, 500);
            } else if (strstr(url, "test3") != NULL) {
                assert_int_equal(rc, 200);
            } else if (strstr(url, "test4") != NULL) {
                assert_int_equal(rc, 500);
            } else {
                assert_int_equal(rc, 200);
            }

        } else {
            assert_int_equal(res, CURLE_OK);
            mangusta_context_stop(ctx);
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    return;
}

static apr_status_t on_headers_ready(mangusta_ctx_t * UNUSED(ctx), mangusta_request_t * req) {
    apr_status_t status = APR_SUCCESS;
    char *url;

    url = mangusta_request_url_get(req);

    if (strstr(url, "test1") != NULL) {
        status = APR_SUCCESS;
    } else if (strstr(url, "test2") != NULL) {
        status = APR_ERROR;
    } else if (strstr(url, "test3") != NULL) {
        status = APR_SUCCESS;
    } else if (strstr(url, "test4") != NULL) {
        status = APR_SUCCESS;
    }

    return status;
}

static apr_status_t on_request_ready(mangusta_ctx_t * ctx, mangusta_request_t * req) {
    apr_status_t status = APR_ERROR;
    char *url;
    char *method;
    char *version;

    url = mangusta_request_url_get(req);
    method = mangusta_request_method_get(req);
    version = mangusta_request_protoversion_get(req);

    assert(url && method && version);

    //mangusta_response_status_set(req, 200, "OK Pass");
    mangusta_response_header_set(req, "Content-Type", "text/plain; charset=UTF-8");
    mangusta_response_body_appendf(req, "Testing...\n");

    if (strstr(url, "test1") != NULL) {
        mangusta_response_status_set(req, 200, "OK Pass");
        status = APR_SUCCESS;
    } else if (strstr(url, "test2") != NULL) {
        mangusta_response_status_set(req, 200, "OK Pass");
        status = APR_SUCCESS;
    } else if (strstr(url, "test3") != NULL) {
        mangusta_response_status_set(req, 200, "OK Pass");
        status = APR_SUCCESS;
    } else if (strstr(url, "test4") != NULL) {
        mangusta_response_status_set(req, 200, "OK Pass");
        status = APR_ERROR;
    }

    /* Stop the test when we have received a response on our last request */
    if (strstr(url, "test4") != NULL) {
        mangusta_context_stop(ctx);
    }

    return status;
}

static void test_perform(void **UNUSED(foo)) {
    MANGUSTA_TEST_SETUP;
    MANGUSTA_TEST_START;

    assert_int_equal(mangusta_context_set_request_header_cb(ctx, on_headers_ready), APR_SUCCESS);
    assert_int_equal(mangusta_context_set_request_ready_cb(ctx, on_request_ready), APR_SUCCESS);

    curl_perform(ctx, URL1, CURL_HTTP_VERSION_1_0);
    curl_perform(ctx, URL2, CURL_HTTP_VERSION_1_0);
    curl_perform(ctx, URL3, CURL_HTTP_VERSION_1_0);
    curl_perform(ctx, URL4, CURL_HTTP_VERSION_1_0);

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
