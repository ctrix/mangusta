
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <curl/curl.h>

#include <mangusta.h>

static void curl_get(char *url) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if ( curl != NULL ) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        res = curl_easy_perform(curl);

        if( CURLE_OK == res ) {
            char *ct;
            /* ask for the content-type */
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);

            if( (CURLE_OK == res) && ct ) {
                printf("We received Content-Type: %s\n", ct);
            }
        }
        else {
            //printf("CURL received an error\n");
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
}

#include "test_suite_initialization.c"
#include "test_suite_context.c"
#include "test_suite_connection.c"

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_library_initialization),
    cmocka_unit_test(test_context_setup),
    cmocka_unit_test(test_connection)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
