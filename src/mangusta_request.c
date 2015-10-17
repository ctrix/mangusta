
#include "mangusta_private.h"

/* ********************************************************************************************* */

apr_status_t mangusta_request_create(mangusta_connection_t * conn, mangusta_request_t ** req) {
    apr_pool_t *pool;
    apr_status_t status;
    mangusta_request_t *r;

    assert(conn);
    assert(req);

    *req = NULL;

    status = apr_pool_create(&pool, conn->pool);
    if (status != APR_SUCCESS) {
        return status;
    }

    r = apr_pcalloc(pool, sizeof(mangusta_request_t));
    if (r != NULL) {
        r->pool = pool;
        r->conn = conn;
        r->state = MANGUSTA_REQUEST_HEADERS;
        r->headers = apr_hash_make(r->pool);

        status = apr_queue_push(conn->requests, r);
        if (status != APR_SUCCESS) {
            // TODO Is this correct ?
            return status;
        }

        *req = r;
        return status;
    }

    return APR_ERROR;
}

apr_status_t mangusta_request_state_change(mangusta_request_t * req, enum mangusta_request_state_e newstate) {
    switch (req->state) {
        case MANGUSTA_REQUEST_INIT:
            if (newstate == MANGUSTA_REQUEST_HEADERS || newstate == MANGUSTA_RESPONSE_HEADERS) {
                return APR_SUCCESS;
            }
            break;
        case MANGUSTA_REQUEST_HEADERS:
            if (newstate == MANGUSTA_REQUEST_PAYLOAD || newstate == MANGUSTA_RESPONSE_HEADERS) {
                return APR_SUCCESS;
            }
            break;
        case MANGUSTA_REQUEST_PAYLOAD:
            if (newstate == MANGUSTA_RESPONSE_HEADERS) {
                return APR_SUCCESS;
            }
            break;
        case MANGUSTA_RESPONSE_HEADERS:
            if (newstate == MANGUSTA_RESPONSE_PAYLOAD || newstate == MANGUSTA_REQUEST_CLOSE) {
                return APR_SUCCESS;
            }
            break;
        case MANGUSTA_RESPONSE_PAYLOAD:
            if (newstate == MANGUSTA_REQUEST_CLOSE) {
                return APR_SUCCESS;
            }
            break;
        case MANGUSTA_REQUEST_CLOSE:
            break;
    }
    return APR_ERROR;
}

/* ********************************************************************************************* */

apr_status_t mangusta_request_parse_headers(mangusta_request_t * req) {
    mangusta_connection_t *conn;
    apr_uint32_t blen;
    char *bdata;
    char *end;
    char line[DEFAULT_BUFFER_SIZE];

    assert(req);
    conn = req->conn;
    assert(conn);
    assert(req->state == MANGUSTA_REQUEST_INIT || req->state == MANGUSTA_REQUEST_HEADERS);

    blen = mangusta_buffer_get_char(conn->buffer_r, &bdata);
    if ((blen > 0) && (end = strnstr(bdata, HEADERS_END_MARKER, blen)) != NULL) {
        /* Headers found, so parse them */

        blen = (end + strlen(HEADERS_END_MARKER)) - bdata;
        /* Request line */
        if (mangusta_buffer_extract(conn->buffer_r, line, sizeof(line) - 1, '\n') == APR_SUCCESS) {
            size_t ret;

            // TODO Check that the final \r\n exists
            chomp(line, 0);

            req->c_method = apr_pcalloc(req->pool, strlen(line) + 1);
            req->url = apr_pcalloc(req->pool, strlen(line) + 1);
            req->c_http_version = apr_pcalloc(req->pool, strlen(line) + 1);

            ret = sscanf(line, "%[^ ] %[^ ] %[^ ]", req->c_method, req->url, req->c_http_version);
            /* Don't consider ret = 2 for GET requests which indicate HTTP/0.9 protocol. It's obsoleted. */

            if (ret == 3 && !strncasecmp(req->c_http_version, "HTTP/", strlen("HTTP/"))) {
                unsigned int major, minor;

                /* Break apart the protocol and update the connection structure. */
                ret = sscanf(req->c_http_version + strlen("HTTP/"), "%u.%u", &major, &minor);
                if (ret != 2) {
                    // TODO RETURN PROPER ERROR
                    return APR_ERROR;
                }
                if (major != 1) {
                    // TODO RETURN PROPER ERROR
                    return APR_ERROR;
                }
                if (minor == 0) {
                    req->http_version = MANGUSTA_HTTP_10;
                } else if (minor == 1) {
                    req->http_version = MANGUSTA_HTTP_11;
                } else {
                    return APR_ERROR;
                }
            }

            if (zstr(req->url)) {
                //TODO BAD REQUEST
                return APR_ERROR;
            }

            if (req->http_version == MANGUSTA_HTTP_10) {
                req->conn->must_close = 1;
            }

        } else {
            /* Error extracting request line! */
            assert(0);          // TODO BAD REQUEST
            return APR_ERROR;
        }

        while (mangusta_buffer_extract(conn->buffer_r, line, sizeof(line) - 1, '\n') == APR_SUCCESS) {
            char *h, *v, *t;
            // TODO Check that the final \r close to \n exists
            chomp(line, 0);

            /* Are we done parsing the header ? */
            if (zstr(line)) {
                break;
            }

            h = line;
            v = strchr(h, ':');
            if (v == NULL) {
                // TODO BAD REQUEST
                return APR_ERROR;
            }
            *v = '\0';
            v++;

            /* Remove initial spaces */
            while (*v == ' ') {
                v++;
            }

            /* To lowercase */
            for (t = h; *t; ++t) {
                *t = tolower(*t);
            }

            mangusta_request_header_set(req, h, v);
        }

        return APR_SUCCESS;
    }

    return APR_INCOMPLETE;
}

APR_DECLARE(char *) mangusta_request_header_get(mangusta_request_t * req, const char *name) {
    assert(req);
    if (!zstr(name)) {
        return apr_hash_get(req->headers, name, APR_HASH_KEY_STRING);
    }
    return NULL;
}

apr_status_t mangusta_request_header_set(mangusta_request_t * req, const char *name, const char *value) {
    assert(req);
    if (!zstr(name)) {
        apr_hash_set(req->headers, apr_pstrndup(req->pool, name, strlen(name)), APR_HASH_KEY_STRING, apr_pstrndup(req->pool, value, strlen(value)));
        return APR_ERROR;
    }
    return APR_ERROR;
}

apr_status_t mangusta_request_has_payload(mangusta_request_t * req) {
    /* https://tools.ietf.org/html/rfc7230#section-3.3 */
    if (mangusta_request_header_get(req, "Transfer-Encoding") || mangusta_request_header_get(req, "Content-Length")) {
        return APR_SUCCESS;
    }
    return APR_ERROR;
}

apr_status_t mangusta_request_write_response(mangusta_request_t * req) {

}
