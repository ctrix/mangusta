
#include "_test_framework.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <curl/curl.h>

#include <mangusta.h>

static void test_perform(void **UNUSED(foo)) {

    MANGUSTA_TEST_SETUP;
    MANGUSTA_TEST_START;
    MANGUSTA_TEST_DISPOSE;

    return;
}

int main(void) {
    const struct UnitTest tests[] = {
        unit_test(test_perform),
    };

    return run_tests(tests);
}
