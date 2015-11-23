
#include "mangusta_private.h"

/* The following header has been created with bin2c */
#include "mangusta_request_error.h"

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
        r->rheaders = apr_hash_make(r->pool);

        r->response = mangusta_buffer_init(r->pool, 0, 0);

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

static void mangusta_request_destroy(mangusta_request_t * req) {
    apr_pool_destroy(req->pool);
	return;
}

apr_status_t mangusta_request_state_change(mangusta_request_t * req, enum mangusta_request_state_e newstate) {
    switch (req->state) {
        case MANGUSTA_REQUEST_INIT:
            if (newstate == MANGUSTA_REQUEST_HEADERS || newstate == MANGUSTA_RESPONSE_HEADERS) {
                req->state = newstate;
                return APR_SUCCESS;
            }
            break;
        case MANGUSTA_REQUEST_HEADERS:
            if (newstate == MANGUSTA_REQUEST_PAYLOAD || newstate == MANGUSTA_RESPONSE_HEADERS) {
                req->state = newstate;
                return APR_SUCCESS;
            }
            break;
        case MANGUSTA_REQUEST_PAYLOAD:
            if (newstate == MANGUSTA_RESPONSE_HEADERS) {
                req->state = newstate;
                return APR_SUCCESS;
            }
            break;
        case MANGUSTA_RESPONSE_HEADERS:
            if (newstate == MANGUSTA_RESPONSE_PAYLOAD || newstate == MANGUSTA_REQUEST_CLOSE) {
                req->state = newstate;
                return APR_SUCCESS;
            }
            break;
        case MANGUSTA_RESPONSE_PAYLOAD:
            if (newstate == MANGUSTA_REQUEST_CLOSE) {
                req->state = newstate;
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
    char line[REQUEST_HEADER_MAX_LEN - 1];

    assert(req);
    conn = req->conn;
    assert(conn);
    assert(req->state == MANGUSTA_REQUEST_INIT || req->state == MANGUSTA_REQUEST_HEADERS);

    /* If in doubt, close the connection as default */
    req->conn->must_close = 1;

    /* Can we find the emd-header marker ? */
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
            req->c_method = apr_pstrdup(req->pool, line);
            req->url = apr_pcalloc(req->pool, strlen(line) + 1);
            req->c_http_version = apr_pcalloc(req->pool, strlen(line) + 1);

            ret = sscanf(line, "%[^ ] %[^ ] %[^ ]", req->c_method, req->url, req->c_http_version);
            /* Don't consider ret = 2 for GET requests which indicate HTTP/0.9 protocol. It's obsoleted. */

            if (ret == 3 && !strncasecmp(req->c_http_version, "HTTP/", strlen("HTTP/"))) {
                unsigned int major, minor;
                /* Break apart the protocol and update the connection structure. */
                ret = sscanf(req->c_http_version + strlen("HTTP/"), "%u.%u", &major, &minor);

// TODO Request too large (URL) REJECT
// TODO VALIDATE URI

                // Conform to http://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html#sec5.1.2
                // URI can be an asterisk (*) or should start with slash.
                //return uri[0] == '/' || (uri[0] == '*' && uri[1] == '\0');

// SPLIT URL AND QUERY STRING AFTER ?

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
            /*
               Error extracting request line!
               This happens when the header is too long
             */
            mangusta_response_status_set(req, 400, "Bad Request");
            //assert(0);          // TODO BAD REQUEST
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
        char *t;
        char *lowername = strdup(name);
        if (lowername != NULL) {
            for (t = lowername; *t; ++t) {
                *t = tolower(*t);
            }
            return apr_hash_get(req->headers, lowername, APR_HASH_KEY_STRING);
        }
    }
    return NULL;
}

APR_DECLARE(char *) mangusta_request_method_get(mangusta_request_t * req) {
    assert(req);
    return req->c_method;
}

APR_DECLARE(char *) mangusta_request_url_get(mangusta_request_t * req) {
    assert(req);
    return req->url;
}

APR_DECLARE(char *) mangusta_request_protoversion_get(mangusta_request_t * req) {
    assert(req);
    return req->c_http_version;
}

apr_status_t mangusta_request_header_set(mangusta_request_t * req, const char *name, const char *value) {
    assert(req);
    if (!zstr(name)) {
        apr_hash_set(req->headers, apr_pstrndup(req->pool, name, strlen(name)), APR_HASH_KEY_STRING, apr_pstrndup(req->pool, value, strlen(value)));
        return APR_SUCCESS;
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

apr_status_t mangusta_request_payload_received(mangusta_request_t * req) {
    char *te = NULL;
    apr_int64_t cl = 0;
    apr_uint32_t blen;
    char *bdata;

    blen = mangusta_buffer_get_char(req->conn->buffer_r, &bdata);

    te = mangusta_request_header_get(req, "Transfer-Encoding");

    if (mangusta_request_header_get(req, "Content-Length")) {
        cl = apr_strtoi64(mangusta_request_header_get(req, "Content-Length"), NULL, 0);
    }

    if ((te == NULL) && (cl <= blen)) {
        return APR_SUCCESS;
    }

    return APR_ERROR;
}

APR_DECLARE(apr_status_t) mangusta_response_status_set(mangusta_request_t * req, short status, const char *message) {
    assert(req);
    req->status = status;
    req->message = apr_pstrdup(req->pool, message);

    return APR_SUCCESS;
}

APR_DECLARE(apr_status_t) mangusta_response_header_del(mangusta_request_t * req, const char *name) {
    assert(req);

    if (!zstr(name)) {
        apr_hash_set(req->rheaders, name, APR_HASH_KEY_STRING, NULL);
        return APR_SUCCESS;
    }

    return APR_ERROR;
}

APR_DECLARE(apr_status_t) mangusta_response_header_set(mangusta_request_t * req, const char *name, const char *value) {
    assert(req);

    if (!zstr(name)) {
        apr_hash_set(req->rheaders, apr_pstrndup(req->pool, name, strlen(name)), APR_HASH_KEY_STRING, apr_pstrndup(req->pool, value, strlen(value)));
        return APR_SUCCESS;
    }

    return APR_ERROR;
}

APR_DECLARE(apr_status_t) mangusta_response_header_exists(mangusta_request_t * req, const char *name) {
    assert(req);

    if (!zstr(name)) {
        if (apr_hash_get(req->rheaders, name, APR_HASH_KEY_STRING) != NULL) {
            return APR_SUCCESS;
        }
    }

    return APR_ERROR;
}

APR_DECLARE(apr_status_t) mangusta_response_body_clear(mangusta_request_t * req) {
    assert(req);

    return mangusta_buffer_reset(req->response);
}

APR_DECLARE(apr_status_t) mangusta_response_body_append(mangusta_request_t * req, const char *value, apr_uint32_t size) {
    assert(req);

    if (!zstr(value)) {
        return mangusta_buffer_append(req->response, value, size);
    }

    return APR_ERROR;
}

APR_DECLARE(apr_status_t) mangusta_response_body_appendf(mangusta_request_t * req, const char *fmt, ...) {
    char *s;
    va_list ap;

    va_start(ap, fmt);
    s = apr_pvsprintf(req->pool, fmt, ap);
    va_end(ap);

    if (!zstr(s)) {
        return mangusta_buffer_append(req->response, s, 0);
    }

    return APR_ERROR;
}

APR_DECLARE(apr_status_t) mangusta_response_write(mangusta_request_t * req) {
    char *out;
    apr_uint32_t osize;
    apr_size_t blen;
    char buf[DEFAULT_BUFFER_SIZE];
    apr_hash_index_t *hi;
    char timebuf[APR_RFC822_DATE_LEN];

    if (req->status <= 0) {
        mangusta_log(MANGUSTA_LOG_DEBUG, "Status cannot be ZERO. Please set one.");
        mangusta_response_status_set(req, 500, "Ooops! Something weird happened.");
        req->status = 500;
    }

    /*
       TODO Add default headers:
       Content-Type
       Server
       Connection
     */

    apr_rfc822_date(timebuf, apr_time_now());
    mangusta_response_header_set(req, "Date", timebuf);

    //mangusta_response_header_set(req, "Connection", "Close"); // TODO Only if keep alive is disabled and header is not set

    osize = mangusta_buffer_get_char(req->response, &out);
    snprintf(buf, sizeof(buf) - 1, "%d", osize);
    mangusta_response_header_set(req, "Content-Length", buf);

    /* Start the output */
    blen = snprintf(buf, sizeof(buf) - 1, "%s %d %s\r\n", (req->c_http_version != NULL) ? req->c_http_version : "HTTP/1.0", req->status, req->message ? req->message : "Unset");
    buf[blen] = '\0';
    apr_socket_send(req->conn->sock, buf, &blen);

    for (hi = apr_hash_first(req->pool, req->rheaders); hi; hi = apr_hash_next(hi)) {
        char *key;
        char *val;

        apr_hash_this(hi, (const void **) &key, NULL, (void **) &val);
        if (!zstr(val)) {
            blen = snprintf(buf, sizeof(buf) - 1, "%s: %s\r\n", key, val);
            buf[blen] = '\0';
            apr_socket_send(req->conn->sock, buf, &blen);
        }
    }

    blen = 2;
    apr_socket_send(req->conn->sock, "\r\n", &blen);

    osize = mangusta_buffer_get_char(req->response, &out);
    if (osize > 0) {
        blen = osize;
        apr_socket_send(req->conn->sock, out, &blen);
        mangusta_buffer_reset(req->response);
    }

    return APR_SUCCESS;
}

static void do_subst(const char *in, mangusta_buffer_t * buff, const char *s1, const char *s2) {
    const char *p1 = NULL;
    char *p2 = NULL;

    if (!in || !s1 || !s2) {
        return;
    }

    p1 = in;

    while ((p1 <= in + strlen(in)) && (p2 = strstr(p1, s1))) {
        int len = p2 - p1;
        if (len) {
            mangusta_buffer_append(buff, p1, len);
        }
        mangusta_buffer_append(buff, s2, strlen(s2));
        p1 = p2 + strlen(s1);
    }

    if (p1) {
        mangusta_buffer_append(buff, p1, strlen(p1));
    }

    return;
}

APR_DECLARE(apr_status_t) mangusta_error_write(mangusta_request_t * req) {
    mangusta_buffer_t *b;
    char *bdata;
    char cstatus[8];

    /* TODO If the headers have already been writte, then SKIP */

    if (req->status < 400 || req->status > 599) {
        mangusta_response_status_set(req, 500, "Internal server error");
    }

    mangusta_response_body_clear(req);

    req->must_close = 1;
    mangusta_response_header_set(req, "Connection", "close");
    mangusta_response_header_set(req, "Content-type", "text/html; charset=utf-8");

    b = mangusta_buffer_init(req->pool, 0, 0);
    do_subst(error_page, b, "[MESSAGE]", req->message);
    mangusta_buffer_get_char(b, &bdata);
    snprintf(cstatus, sizeof(cstatus) - 1, "%d", req->status);
    do_subst(bdata, req->response, "[STATUS]", cstatus);

    return mangusta_response_write(req);
}
