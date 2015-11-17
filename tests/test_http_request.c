
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
#define URL3 "http://" MANGUSTA_TEST_BINDTO ":" MANGUSTA_TEST_BINDPORT R_URL3 "?"
#define URL4 "http://" MANGUSTA_TEST_BINDTO ":" MANGUSTA_TEST_BINDPORT R_URL4 "?"

/* Never writes anything, just returns the size presented */
size_t curl_dummy_write(char *UNUSED(ptr), size_t size, size_t nmemb, void *UNUSED(userdata)) {
    return size * nmemb;
}

static void curl_perform(mangusta_ctx_t * ctx, const char *url, long ver) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl != NULL) {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Test suite");
        /*curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_NONE); */
        curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, ver);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curl_dummy_write);

        res = curl_easy_perform(curl);

        if (CURLE_OK == res) {
            char *ct;
            long rc;

            res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rc);
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            printf("STATUS: %ld\n", rc);

        } else {
            assert_int_equal(res, CURLE_OK);
            mangusta_context_stop(ctx);
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }

    return;
}

static apr_status_t on_request_ready(mangusta_ctx_t * ctx, mangusta_request_t * req) {
    char *url;
    char *method;
    char *version;

    (void) ctx;

    url = mangusta_request_url_get(req);
    method = mangusta_request_method_get(req);
    version = mangusta_request_protoversion_get(req);

    assert(url && method && version);

    mangusta_response_status_set(req, 200, "OK Pass");
    mangusta_response_header_set(req, "Content-Type", "text/plain; charset=UTF-8");
    mangusta_response_body_appendf(req, "Test passed - %s\n", url);

    if (strstr(url, "asd") != NULL) {
        mangusta_response_status_set(req, 200, "OK Pass");
        return APR_ERROR;
    }

    if (strstr(url, "test1") != NULL) {
        assert_string_equal(version, "HTTP/1.0");
        assert_string_equal(url, R_URL1);
        mangusta_response_status_set(req, 200, "OK Pass");
    }

    if (strstr(url, "test2") != NULL) {
        assert_string_equal(version, "HTTP/1.1");
        assert_string_equal(url, R_URL2);
        mangusta_response_status_set(req, 200, "OK Pass");
    }

    if (strstr(url, "test3") != NULL) {
        assert_string_equal(version, "HTTP/1.1");
        assert_string_equal(url, R_URL3);
        mangusta_response_status_set(req, 500, "Test must fail");
    }

    /* Stop the test when we have received a response on our last request */
    if (strstr(url, "test3") != NULL) {
        mangusta_context_stop(ctx);
    }

    return APR_SUCCESS;
}

static void test_perform(void **UNUSED(foo)) {
    char *params;

    MANGUSTA_TEST_SETUP;

    assert_int_equal(mangusta_context_set_request_ready_cb(ctx, on_request_ready), APR_SUCCESS);

    curl_perform(ctx, URL1, CURL_HTTP_VERSION_1_0);
    curl_perform(ctx, URL2, CURL_HTTP_VERSION_1_1);

    params = malloc(PARAMSIZE_BIG1);
    memset(params, 'A', PARAMSIZE_BIG1);
    *(params + PARAMSIZE_BIG1 - 1) = '\0';
    memcpy(params, URL3, sizeof(URL3) - 1);
    curl_perform(ctx, params, CURL_HTTP_VERSION_1_1);

    while (mangusta_context_running(ctx) == APR_SUCCESS) {
        apr_sleep(APR_USEC_PER_SEC / 20);
    }

    free(params);

    MANGUSTA_TEST_DISPOSE;

    return;
}

int main(void) {
    const struct UnitTest tests[] = {
        unit_test(test_perform),
    };

    return run_tests(tests);
}
