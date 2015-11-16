
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <curl/curl.h>

#include <mangusta.h>

#include "test_http_methods.c"

int main(void) {
    const struct UnitTest tests[] = {
        unit_test(test_http_methods),
//    cmocka_unit_test(test_http_status)
    };

    return run_tests(tests);
}
