
INCLUDE_DIRECTORIES(
    ${MANGUSTA_SOURCE_DIR}
    ${MANGUSTA_SOURCE_DIR}/include
    ${APR_INCLUDE_DIR}
    ${APRUTIL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
)

SET(USE_VALGRIND 0)

PROJECT_ADD_TEST(test_setup SOURCE test_setup.c LIBS mangusta cmocka ${CURL_LIBRARY} VALGRIND ${USE_VALGRIND})
PROJECT_ADD_TEST(test_hooks SOURCE test_hooks.c LIBS mangusta cmocka ${CURL_LIBRARY} VALGRIND ${USE_VALGRIND})
PROJECT_ADD_TEST(test_http_request SOURCE test_http_request.c LIBS mangusta cmocka ${CURL_LIBRARY} VALGRIND ${USE_VALGRIND})
PROJECT_ADD_TEST(test_http_payload SOURCE test_http_payload.c LIBS mangusta cmocka ${CURL_LIBRARY} VALGRIND ${USE_VALGRIND})
PROJECT_ADD_TEST(test_http_urlencode SOURCE test_http_urlencode.c LIBS mangusta cmocka ${CURL_LIBRARY} VALGRIND ${USE_VALGRIND})

#PROJECT_ADD_TEST(test_http_headers SOURCE test_http_headers.c LIBS mangusta cmocka ${CURL_LIBRARY} VALGRIND ${USE_VALGRIND})
#PROJECT_ADD_TEST(test_http_keepalive SOURCE test_http_keepalive.c LIBS mangusta cmocka ${CURL_LIBRARY} VALGRIND ${USE_VALGRIND})
#PROJECT_ADD_TEST(wiptest SOURCE wiptest.c LIBS mangusta ${CURL_LIBRARY} VALGRIND ${USE_VALGRIND})

