
void test_library_initialization(void) {
    apr_status_t status;

    status = mangusta_init();
    assert_int_equal(status, APR_SUCCESS);
    mangusta_shutdown();

    return;
}
