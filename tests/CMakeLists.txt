
INCLUDE_DIRECTORIES(
    ${MANGUSTA_SOURCE_DIR}
    ${MANGUSTA_SOURCE_DIR}/include
    ${APR_INCLUDE_DIR}
    ${APRUTIL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
)

ADD_EXECUTABLE(test_suite test_suite.c)
ADD_DEPENDENCIES(test_suite test_suite)
#ADD_TEST(test_suite_valgrind
#	    valgrind
#	    --error-exitcode=1
#	    --read-var-info=yes
#	    --leak-check=full
#	    --show-leak-kinds=all
#	    ./test_state
#)
TARGET_LINK_LIBRARIES(
    test_suite
	cmocka
	mangusta
	${CURL_LIBRARIES}
)
#ADD_TEST(test_suite test_suite)


ADD_EXECUTABLE(test_http test_http.c)
ADD_DEPENDENCIES(test_http test_http)
TARGET_LINK_LIBRARIES(
    test_http
	cmocka
	mangusta
	${CURL_LIBRARIES}
)
#ADD_TEST(test_http test_http)

ADD_EXECUTABLE(wiptest wiptest.c)
ADD_DEPENDENCIES(wiptest wiptest)
TARGET_LINK_LIBRARIES(
    wiptest
	cmocka
	mangusta
	${CURL_LIBRARIES}
)
#ADD_TEST(wiptest_vg
#	    valgrind
#	    --error-exitcode=1
#	    --read-var-info=yes
#	    --leak-check=full
#	    --show-leak-kinds=all
#	    ./wiptest
#)
ADD_TEST(wiptest wiptest)
