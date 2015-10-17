
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>
#include "stdarg.h"

#include "mangusta.h"
#include "mangusta_buffer.h"

#include "apr_poll.h"
#include "apr_hash.h"
#include "apr_queue.h"
#include "apr_thread_pool.h"

#define MANGUSTA_DEBUG 9

#define zstr(x)  ( ((x==NULL) || (*x == '\0')) ? 1 : 0)

#define DEFAULT_POLLSET_NUM 8
#define DEFAULT_POLL_TIMEOUT (APR_USEC_PER_SEC * 0.1)

#define DEFAULT_SOCKET_BACKLOG SOMAXCONN
#define DEFAULT_THREAD_STACKSIZE 250 * 1024
#define DEFAULT_BUFFER_SIZE 4096
#define REQUEST_HEADERS_MAX_SIZE 8192
#define HEADERS_END_MARKER "\r\n\r\n"
#define DEFAULT_REQUESTS_PER_CONNECTION_QUEUE 20

enum mangusta_read_state_e {
    MANGUSTA_CONNECTION_NEW = 1,
    MANGUSTA_CONNECTION_REQUEST,
    MANGUSTA_CONNECTION_RESPONSE,
    MANGUSTA_CONNECTION_WEBSOCKET
};

enum mangusta_request_state_e {
    MANGUSTA_REQUEST_INIT = 1,
    MANGUSTA_REQUEST_HEADERS,
    MANGUSTA_REQUEST_PAYLOAD,
    MANGUSTA_RESPONSE_HEADERS,
    MANGUSTA_RESPONSE_PAYLOAD,
    MANGUSTA_REQUEST_CLOSE
};

enum mangusta_http_version_e {
    MANGUSTA_HTTP_INVALID = 0,
    MANGUSTA_HTTP_10,
    MANGUSTA_HTTP_11,
    MANGUSTA_HTTP_20
};

enum mangusta_request_method_e {
    MANGUSTA_METHOD_GET,
    MANGUSTA_METHOD_HEAD,
    MANGUSTA_METHOD_CONNECT,
    MANGUSTA_METHOD_POST,
    MANGUSTA_METHOD_PUT,
    MANGUSTA_METHOD_DELETE,
    MANGUSTA_METHOD_OPTIONS,
    MANGUSTA_METHOD_PROPFIND,
    MANGUSTA_METHOD_MKCOL,
    MANGUSTA_METHOD_OTHER
};

static struct {
    enum mangusta_request_method_e method;
    char *name;
} mangusta_request_names[] = {
    {
    MANGUSTA_METHOD_GET, "GET"}, {
    MANGUSTA_METHOD_HEAD, "HEAD"}, {
    MANGUSTA_METHOD_CONNECT, "CONNECT"}, {
    MANGUSTA_METHOD_POST, "POST"}, {
    MANGUSTA_METHOD_PUT, "PUT"}, {
    MANGUSTA_METHOD_DELETE, "DELETE"}, {
    MANGUSTA_METHOD_OPTIONS, "OPTIONS"}, {
    MANGUSTA_METHOD_PROPFIND, "PROPFIND"}, {
    MANGUSTA_METHOD_MKCOL, "MKCOL"}, {
    MANGUSTA_METHOD_OTHER, NULL}
};

enum mangusta_payload_type_e {
    MANGUSTA_PAYLOAD_RAW,
    MANGUSTA_PAYLOAD_MULTIPART,
    MANGUSTA_PAYLOAD_BOH,
};

//struct mangusta_http_methods
//struct mangusta_http_responses

struct mangusta_ctx_s {
    const char *host;
    int port;
    apr_size_t maxconn;
    apr_size_t maxidle;
    mangusta_ctx_connect_cb_f on_connect;
    mangusta_ctx_request_header_cb_f on_request_h;
    mangusta_ctx_request_ready_cb_f on_request_r;
    char buffer[DEFAULT_BUFFER_SIZE];
    apr_size_t buffer_len;

    /* RUNTIME VALUES */
    apr_pool_t *pool;
    apr_socket_t *sock;
    apr_pollset_t *pollset;
    apr_thread_pool_t *tp;
    apr_thread_t *thread;
    apr_bool_t stopped;
};

struct mangusta_connection_s {
    mangusta_ctx_t *ctx;
    apr_pool_t *pool;
    apr_socket_t *sock;
    apr_pollset_t *pollset;
    char *method_string;
    char *url;
    apr_hash_t *headers;
    apr_uint32_t request_count;
    apr_queue_t *requests;
    mangusta_request_t *current;

    enum mangusta_read_state_e state;
    short terminated;
    apr_pollfd_t pfd;
    apr_time_t last_io;
    mangusta_buffer_t *buffer_r;
    short must_close;
};

struct mangusta_request_s {
    mangusta_connection_t *conn;
    apr_pool_t *pool;
    enum mangusta_request_state_e state;

    char *c_method;
    enum mangusta_request_method_e method;
    char *url;
    char *c_http_version;
    enum mangusta_http_version_e http_version;

    apr_hash_t *headers;
};

mangusta_connection_t *mangusta_connection_create(mangusta_ctx_t * ctx, apr_socket_t * sock);
void mangusta_connection_destroy(mangusta_connection_t * conn);
apr_status_t mangusta_connection_play(mangusta_connection_t * conn);

apr_status_t mangusta_request_create(mangusta_connection_t * conn, mangusta_request_t ** req);
apr_status_t mangusta_request_state_change(mangusta_request_t * req, enum mangusta_request_state_e newstate);
apr_status_t mangusta_request_parse_headers(mangusta_request_t * req);
apr_status_t mangusta_request_has_payload(mangusta_request_t * req);
apr_status_t mangusta_request_header_set(mangusta_request_t * req, const char *name, const char *value);

#ifndef strnstr
char *strnstr(const char *haystack, const char *needle, size_t len);
#endif

apr_status_t chomp(char *buffer, size_t length);

typedef enum {
    MANGUSTA_LOG_VERBOSE = 0,
    MANGUSTA_LOG_CRIT = 1,
    MANGUSTA_LOG_ERROR = 2,
    MANGUSTA_LOG_WARNING = 3,
    MANGUSTA_LOG_NOTICE = 4,
    MANGUSTA_LOG_DEBUG = 5
} mangusta_log_types;

typedef apr_status_t mangusta_log_func_t(mangusta_log_types level, const char *fmt, va_list argp);

APR_DECLARE(void) mangusta_log_set_function(mangusta_log_func_t * func);
APR_DECLARE(void) mangusta_log(mangusta_log_types level, const char *fmt, ...) __attribute__ ((format(printf, 2, 3)));
APR_DECLARE(const char *) mangusta_log_level_string(mangusta_log_types level);
