
#include "mangusta_private.h"

#ifndef MBEDTLS_THREADING_C
#error MBEDTLS_THREADING_C MUST BE SET !
#endif

/*
void mbedtls_ssl_set_bio( mbedtls_ssl_context *ssl,
        void *p_bio,
        int (*f_send)(void *, const unsigned char *, size_t),
        int (*f_recv)(void *, unsigned char *, size_t),
        int (*f_recv_timeout)(void *, unsigned char *, size_t, uint32_t) );

*/

static int mangusta_tls_send(void *data, const unsigned char *buf, size_t blen) {
    mangusta_connection_t *conn = data;
    apr_size_t size = blen;
    apr_status_t status;

    assert(data);

#if MANGUSTA_DEBUG >= 1
    mangusta_log(MANGUSTA_LOG_DEBUG, "*TLS* SEND %zu", blen);
#endif

    status = apr_socket_send(conn->sock, (char *) buf, &size);
    if (status == APR_EOF) {
        return 0;
    } else if (status == APR_SUCCESS) {
        return size;
    }

    return -1;
}

static int mangusta_tls_recv(void *data, unsigned char *buf, size_t blen) {
    mangusta_connection_t *conn = data;
    apr_size_t size = blen;
    apr_status_t status;

    assert(data);

#if MANGUSTA_DEBUG >= 1
    mangusta_log(MANGUSTA_LOG_DEBUG, "*TLS* RECV %zu", blen);
#endif

    status = apr_socket_recv(conn->sock, (char *) buf, &size);
    if (status == APR_SUCCESS) {
        return size;
    }

    return -1;
}

/*
static void my_debug(void *ctx, int level, const char *file, int line, const char *str) {
    ((void) level);

    mbedtls_fprintf((FILE *) ctx, "%s:%04d: %s", file, line, str);
    fflush((FILE *) ctx);
}
*/

apr_status_t mangusta_context_tls_enable(mangusta_ctx_t * ctx) {
    int ret;
    const char *pers = "ssl_server";
#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    unsigned char alloc_buf[100000];
#endif

    /*
       mbedtls_debug_set_threshold(9);
     */

    mangusta_log(MANGUSTA_LOG_DEBUG, "Enabling TLS");

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
    mbedtls_memory_buffer_alloc_init(alloc_buf, sizeof(alloc_buf));
#endif

    mbedtls_x509_crt_init(&ctx->tls.srvcert);
    mbedtls_ssl_cache_init(&ctx->tls.cache);
    mbedtls_ssl_config_init(&ctx->tls.conf);
    mbedtls_ctr_drbg_init(&ctx->tls.ctr_drbg);
    mbedtls_pk_init(&ctx->tls.pkey);

    /*
     * We use only a single entropy source that is used in all the threads.
     */
    mbedtls_entropy_init(&ctx->tls.entropy);
/*
    mbedtls_debug_set_threshold(MANGUSTA_DEBUG);
*/

    /*
     * 1. Load the certificates and private RSA key
     */
    ret = mbedtls_x509_crt_parse(&ctx->tls.srvcert, (const unsigned char *) mbedtls_test_srv_crt, mbedtls_test_srv_crt_len);
    if (ret != 0) {
        mangusta_log(MANGUSTA_LOG_ERROR, "%s in srv crt_parse", __FUNCTION__);
        return APR_ERROR;
    }

    ret = mbedtls_x509_crt_parse(&ctx->tls.cacert, (const unsigned char *) mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
    if (ret != 0) {
        mangusta_log(MANGUSTA_LOG_ERROR, "%s in cas crt_parse", __FUNCTION__);
        return APR_ERROR;
    }

    ret = mbedtls_pk_parse_key(&ctx->tls.pkey, (const unsigned char *) mbedtls_test_srv_key, mbedtls_test_srv_key_len, NULL, 0);
    if (ret != 0) {
        mangusta_log(MANGUSTA_LOG_ERROR, "%s in parse_key", __FUNCTION__);
        return APR_ERROR;
    }

    /*
     * 1b. Seed the random number generator
     */
    ret = mbedtls_ctr_drbg_seed(&ctx->tls.ctr_drbg, mbedtls_entropy_func, &ctx->tls.entropy, (const unsigned char *) pers, strlen(pers));
    if (ret != 0) {
        mangusta_log(MANGUSTA_LOG_ERROR, "%s in drbd_seed", __FUNCTION__);
        return APR_ERROR;
    }

    /*
     * 1c. Prepare SSL configuration
     */
    ret = mbedtls_ssl_config_defaults(&ctx->tls.conf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        mangusta_log(MANGUSTA_LOG_ERROR, "%s in config defaults", __FUNCTION__);
        return APR_ERROR;
    }

    mbedtls_ssl_conf_rng(&ctx->tls.conf, mbedtls_ctr_drbg_random, &ctx->tls.ctr_drbg);
    /*
       mbedtls_ssl_conf_dbg(&ctx->tls.conf, my_debug, stdout);
     */

    mbedtls_ssl_conf_session_cache(&ctx->tls.conf, &ctx->tls.cache, mbedtls_ssl_cache_get, mbedtls_ssl_cache_set);
    mbedtls_ssl_conf_ca_chain(&ctx->tls.conf, ctx->tls.cacert.next, NULL);

    ret = mbedtls_ssl_conf_own_cert(&ctx->tls.conf, &ctx->tls.srvcert, &ctx->tls.pkey);
    if (ret != 0) {
        mangusta_log(MANGUSTA_LOG_ERROR, "%s in conf_own_cert", __FUNCTION__);
        return APR_ERROR;
    }

    return APR_SUCCESS;
}

apr_status_t mangusta_context_tls_disable(mangusta_ctx_t * ctx) {
    assert(ctx);

    mbedtls_x509_crt_free(&ctx->tls.cacert);
    mbedtls_x509_crt_free(&ctx->tls.srvcert);
    mbedtls_pk_free(&ctx->tls.pkey);
    mbedtls_ssl_config_free(&ctx->tls.conf);
    mbedtls_ssl_cache_free(&ctx->tls.cache);
    mbedtls_ctr_drbg_free(&ctx->tls.ctr_drbg);
    mbedtls_entropy_free(&ctx->tls.entropy);

    return APR_SUCCESS;
}

apr_status_t mangusta_connection_tls_handshake(mangusta_connection_t * conn) {
    int ret;
    apr_status_t status = APR_ERROR;

    mbedtls_ssl_init(&conn->ssl);

    if ((ret = mbedtls_ssl_setup(&conn->ssl, &conn->ctx->tls.conf)) != 0) {
        mangusta_log(MANGUSTA_LOG_DEBUG, "TLS Connection: mbedtls_ssl_setup for connection\n");
        goto done;
    }

    mbedtls_ssl_set_bio(&conn->ssl, conn, mangusta_tls_send, mangusta_tls_recv, NULL);

    while ((ret = mbedtls_ssl_handshake(&conn->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mangusta_log(MANGUSTA_LOG_ERROR, "failed: mbedtls_ssl_handshake returned -0x%04x\n", -ret);
            goto done;
        }
    }

    status = APR_SUCCESS;
    return status;
 done:
    mbedtls_ssl_free(&conn->ssl);

    return status;
}

apr_status_t mangusta_connection_tls_bye(mangusta_connection_t * conn) {
    mbedtls_ssl_free(&conn->ssl);
    return APR_SUCCESS;
}
