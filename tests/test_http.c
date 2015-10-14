
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <curl/curl.h>

#include <mangusta.h>

#include "test_http_methods.c"

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_http_methods),
//    cmocka_unit_test(test_http_status)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
