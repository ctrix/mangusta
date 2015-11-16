
#ifndef _MANGUSTA_INCLUDE_
#define _MANGUSTA_INCLUDE_

#include "apr.h"
#include "apr_errno.h"          /* Needed for apr_status_t */
#include "apr_pools.h"
#include "apr_strings.h"
#include "apr_network_io.h"

#define APR_ERROR       APR_EGENERAL
typedef short int apr_bool_t;
#define APR_ASSERT( data ) { assert(data); }

#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#else
#define UNUSED(x) x
#endif

#ifdef WIN32
#define WARN_UNUSED
#else
#if (__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#define WARN_UNUSED __attribute__((warn_unused_result))
#else
#define WARN_UNUSED __attribute__((warn_unused_result))
#endif
#endif

typedef enum http_resp_codes_e http_resp_codes_t;
enum http_resp_codes_e {
    HTTP_CONTINUE = 100,
    HTTP_SWITCHING_PROTOCOLS = 101,
    HTTP_OK = 200,
    HTTP_CREATED = 201,
    HTTP_ACCEPTED = 202,
    HTTP_NON_AUTH = 203,
    HTTP_NO_CONTENT = 204,
    HTTP_RESET_CONTENT = 205,
    HTTP_PARTIAL_CONTENT = 206,
    HTTP_MULTIPLE_CH = 300,
    HTTP_MOVED_PERM = 301,
    HTTP_MOVED_TEMP = 302,
    HTTP_SEE_OTHER = 303,
    HTTP_NOT_MODIFIED = 304,
    HTTP_USE_PROXY = 305,
    HTTP_USE_TEMP_REDIR = 307,
    HTTP_BAD_REQUEST = 400,
    HTTP_UNAUTHORIZED = 401,
    HTTP_PAYMENT_REQ = 402,
    HTTP_FORBIDDEN = 403,
    HTTP_NOT_FOUND = 404,
    HTTP_NOT_ALLOWED = 405,
    HTTP_NOT_ACCEPTABLE = 406,
    HTTP_PROXY_AUTH_REQ = 407,
    HTTP_TIMEOUT = 408,
    HTTP_CONFLICT = 409,
    HTTP_GONE = 410,
    HTTP_LENGTH_REQUIRED = 411,
    HTTP_PAYLOAD_TOO_LARGE = 413,
    HTTP_URI_TOO_LONG = 414,
    HTTP_UNSUPPORTED_MEDIA = 415,
    HTTP_UPGRADE_REQ = 426,
    HTTP_INTERNAL_SERVER_ERROR = 500,
    HTTP_NOT_IMPLEMENTED = 501,
    HTTP_BAD_GATEWAY = 502,
    HTTP_SERVICE_UNAVALAIBLE = 503,
    HTTP_GATEWAY_TIMEOUT = 504,
    HTTP_VERSION_NOT_SUPPORTED = 505,
};

/*
struct http_error_codes_s {
    http_resp_codes_t code;
    int number;
    char *string;
} ec[] = {
    {
    HTTP_CONTINUE, 100, "Continue"}, {
    HTTP_OK, 200, "Ok"}, {
    HTTP_CREATED, 201, "Created"}, {
    HTTP_ACCEPTED, 202, "Accepted"}, {
    HTTP_NO_CONTENT, 204, "No Content"}, {
    HTTP_RESET_CONTENT, 205, "Reset Content"}, {
    HTTP_PARTIAL_CONTENT, 206, "Partial Content"}, {
    HTTP_MULTIPLE_CH, 300, "Multiple Choices"}, {
    HTTP_MOVED_PERM, 301, "Moved Permanently"}, {
    HTTP_MOVED_TEMP, 302, "Moved Temporarily"}, {
    HTTP_NOT_MODIFIED, 304, "Not Modified"}, {
    HTTP_USE_PROXY, 305, "Use Proxy"}, {
    HTTP_BAD_REQUEST, 400, "Bad Request"}, {
    HTTP_UNAUTHORIZED, 401, "Unauthorized"}, {
    HTTP_FORBIDDEN, 403, "Forbidden"}, {
    HTTP_NOT_FOUND, 404, "Not Found"}, {
    HTTP_NOT_ALLOWED, 405, "Method Not Allowed"}, {
    HTTP_NOT_ACCEPTABLE, 406, "Not Acceptable"}, {
    HTTP_PROXY_AUTH_REQ, 407, "Proxy Authentication Required"}, {
    HTTP_TIMEOUT, 408, "Request Time-out"}, {
    HTTP_CONFLICT, 409, "Conflict"}, {
    HTTP_GONE, 410, "Gone"}, {
    HTTP_INTERNAL_SERVER_ERROR, 500, "Internal Server Error"}, {
    HTTP_NOT_IMPLEMENTED, 501, "Not Implemented"}, {
    HTTP_BAD_GATEWAY, 502, "Bad Gateway"}, {
    HTTP_SERVICE_UNAVALAIBLE, 503, "Service Unavailable"}, {
HTTP_VERSION_NOT_SUPPORTED, 505, "HTTP Version not supported"},};
*/

typedef struct mangusta_ctx_s mangusta_ctx_t;
typedef struct mangusta_connection_s mangusta_connection_t;
typedef struct mangusta_request_s mangusta_request_t;
typedef apr_status_t(*mangusta_ctx_connect_cb_f) (mangusta_ctx_t * ctx, apr_socket_t * sock, apr_pool_t * pool);
typedef apr_status_t(*mangusta_ctx_request_header_cb_f) (mangusta_ctx_t * ctx, mangusta_request_t * req);
typedef apr_status_t(*mangusta_ctx_request_ready_cb_f) (mangusta_ctx_t * ctx, mangusta_request_t * req);

APR_DECLARE(apr_status_t) mangusta_init(void);
APR_DECLARE(apr_status_t) mangusta_shutdown(void);

APR_DECLARE(mangusta_ctx_t *) mangusta_context_new(void);

APR_DECLARE(apr_pool_t *) mangusta_context_get_pool(mangusta_ctx_t * ctx);
APR_DECLARE(apr_status_t) mangusta_context_set_host(mangusta_ctx_t * ctx, const char *host);
APR_DECLARE(apr_status_t) mangusta_context_set_port(mangusta_ctx_t * ctx, int port);
APR_DECLARE(apr_status_t) mangusta_context_set_max_connections(mangusta_ctx_t * ctx, apr_size_t max);
APR_DECLARE(apr_status_t) mangusta_context_set_max_idle(mangusta_ctx_t * ctx, apr_size_t max);
APR_DECLARE(apr_status_t) mangusta_context_set_http_keepalive(mangusta_ctx_t * ctx, apr_size_t sec);

APR_DECLARE(apr_status_t) mangusta_context_set_connect_cb(mangusta_ctx_t * ctx, mangusta_ctx_connect_cb_f cb);
APR_DECLARE(apr_status_t) mangusta_context_set_request_header_cb(mangusta_ctx_t * ctx, mangusta_ctx_request_header_cb_f cb);
APR_DECLARE(apr_status_t) mangusta_context_set_request_ready_cb(mangusta_ctx_t * ctx, mangusta_ctx_request_ready_cb_f cb);

APR_DECLARE(apr_status_t) mangusta_context_start(mangusta_ctx_t * ctx);
APR_DECLARE(apr_status_t) mangusta_context_wait(mangusta_ctx_t * ctx);
APR_DECLARE(apr_status_t) mangusta_context_background(mangusta_ctx_t * ctx);
APR_DECLARE(apr_status_t) mangusta_context_running(mangusta_ctx_t * ctx);
APR_DECLARE(apr_status_t) mangusta_context_stop(mangusta_ctx_t * ctx);
APR_DECLARE(apr_status_t) mangusta_context_free(mangusta_ctx_t * ctx);

APR_DECLARE(apr_status_t) mangusta_connection_set_http_keepalive(mangusta_connection_t * conn, apr_size_t sec);

APR_DECLARE(char *) mangusta_request_header_get(mangusta_request_t * req, const char *name);
APR_DECLARE(char *) mangusta_request_method_get(mangusta_request_t * req);
APR_DECLARE(char *) mangusta_request_protoversion_get(mangusta_request_t * req);

APR_DECLARE(apr_status_t) mangusta_response_status_set(mangusta_request_t * req, short status, const char *message);
APR_DECLARE(apr_status_t) mangusta_response_header_set(mangusta_request_t * req, const char *name, const char *value);
APR_DECLARE(apr_status_t) mangusta_response_body_append(mangusta_request_t * req, const char *value, apr_uint32_t size);
APR_DECLARE(apr_status_t) mangusta_response_body_appendf(mangusta_request_t * req, const char *fmt, ...) __attribute__ ((format(printf, 2, 3)));
APR_DECLARE(apr_status_t) mangusta_response_write(mangusta_request_t * req);

#endif
