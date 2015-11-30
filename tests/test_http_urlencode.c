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
#define UPARAMS "a=1&B=2&c=3&name=1%202&d=" /* URL Params */
#define PPARAMS "e=5&F=6&g=7&birth=may%201st&h="    /* Payload params */

#define URL_BASE "http://" MANGUSTA_TEST_BINDTO ":" MANGUSTA_TEST_BINDPORT

static int must_shutdown = 0;

/* Never writes anything, just returns the size presented */
size_t curl_dummy_write(char *UNUSED(ptr), size_t size, size_t nmemb, void *UNUSED(userdata)) {
    return size * nmemb;
}

static void curl_perform(mangusta_ctx_t * ctx, const char *url, long ver, const char *method, short multipart) {
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
            if (multipart != 0) {
                curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "sendfile", CURLFORM_FILE, __FILE__, CURLFORM_END);
                curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "filename", CURLFORM_COPYCONTENTS, __FILE__, CURLFORM_CONTENTTYPE, "text-foo", CURLFORM_END);
                curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit", CURLFORM_COPYCONTENTS, "send it!", CURLFORM_END);
                curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "libmangusta", CURLFORM_COPYCONTENTS, "r0x!! €", CURLFORM_END);
                curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
            } else {
                curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long) strlen(PPARAMS));
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, PPARAMS);
            }

        } else if (!strcmp(method, "DELETE")) {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
        } else {
            assert_non_null(1);
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
/*
    char *method;
    method = mangusta_request_method_get(req);
*/
    assert_string_equal(mangusta_request_getvar(req, "a"), "1");
    assert_null(mangusta_request_getvar(req, "b"));
    assert_string_equal(mangusta_request_getvar(req, "B"), "2");
    assert_string_equal(mangusta_request_getvar(req, "c"), "3");
    assert_string_equal(mangusta_request_getvar(req, "name"), "1 2");
    assert_string_equal(mangusta_request_getvar(req, "d"), "");

    return status;
}

static apr_status_t on_request_ready(mangusta_ctx_t * ctx, mangusta_request_t * req) {
    char *url;
    char *ctype;
    char *method;
    apr_status_t status = APR_SUCCESS;

    (void) ctx;
    url = mangusta_request_url_get(req);

    ctype = mangusta_request_header_get(req, "Content-Type");

    method = mangusta_request_method_get(req);
    mangusta_response_status_set(req, 200, "OK Pass");

    printf("Done: %s %s\n", method, url);

    if (ctype != NULL && strncmp(ctype, "application/x-www-form-urlencoded", sizeof("application/x-www-form-urlencoded")) == 0) {
        assert_string_equal(mangusta_request_postvar(req, "e"), "5");
        assert_null(mangusta_request_postvar(req, "f"));
        assert_string_equal(mangusta_request_postvar(req, "F"), "6");
        assert_string_equal(mangusta_request_postvar(req, "g"), "7");
        assert_string_equal(mangusta_request_postvar(req, "h"), "");
    }

    return status;
}

static void test_perform(void **UNUSED(foo)) {
    MANGUSTA_TEST_SETUP;

    assert_int_equal(mangusta_context_set_request_header_cb(ctx, on_headers_ready), APR_SUCCESS);
    assert_int_equal(mangusta_context_set_request_ready_cb(ctx, on_request_ready), APR_SUCCESS);

/*
    curl_perform(ctx, URL_BASE URL1 "?" UPARAMS, CURL_HTTP_VERSION_1_0, "GET", 0);
    curl_perform(ctx, URL_BASE URL1 "?" UPARAMS, CURL_HTTP_VERSION_1_1, "GET", 0);
    curl_perform(ctx, URL_BASE URL2 "?" UPARAMS, CURL_HTTP_VERSION_1_0, "POST", 0);
    curl_perform(ctx, URL_BASE URL2 "?" UPARAMS, CURL_HTTP_VERSION_1_1, "POST", 0);
    curl_perform(ctx, URL_BASE URL3 "?" UPARAMS, CURL_HTTP_VERSION_1_1, "DELETE", 0);
*/

    must_shutdown = 1;
    curl_perform(ctx, URL_BASE URL2 "?" UPARAMS, CURL_HTTP_VERSION_1_1, "POST", 1);

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
