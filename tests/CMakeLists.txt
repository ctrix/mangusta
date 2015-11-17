
INCLUDE_DIRECTORIES(
    ${MANGUSTA_SOURCE_DIR}
    ${MANGUSTA_SOURCE_DIR}/include
    ${APR_INCLUDE_DIR}
    ${APRUTIL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
)

PROJECT_ADD_TEST(test_setup SOURCE test_setup.c LIBS mangusta cmocka curl VALGRIND 0)
PROJECT_ADD_TEST(test_http_keepalive SOURCE test_http_keepalive.c LIBS mangusta cmocka curl)
