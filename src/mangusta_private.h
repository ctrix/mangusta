
#include <assert.h>
#include <string.h>
#include <stddef.h>

#include "mangusta.h"
#include "mangusta_buffer.h"

#include "apr_poll.h"
#include "apr_hash.h"
#include "apr_thread_pool.h"

#define zstr(x)  ( ((x==NULL) || (*x == '\0')) ? 1 : 0)

#define DEFAULT_POLLSET_NUM 8
#define DEFAULT_POLL_TIMEOUT (APR_USEC_PER_SEC * 0.1)

#define DEFAULT_SOCKET_BACKLOG SOMAXCONN
#define DEFAULT_THREAD_STACKSIZE 250 * 1024
#define DEFAULT_BUFFER_SIZE 4096
#define REQUEST_HEADERS_MAX_SIZE 8192
#define HEADERS_END_MARKER "\r\n\r\n"

enum mangusta_request_state_e {
    MANGUSTA_REQUEST_INIT,
    MANGUSTA_REQUEST_HEADERS,
    MANGUSTA_REQUEST_PAYLOAD,
    MANGUSTA_RESPONSE_HEADERS,
    MANGUSTA_RESPONSE_PAYLOAD,
    MANGUSTA_REQUEST_WAIT,
    MANGUSTA_REQUEST_CLOSE
};

enum mangusta_http_version_e {
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

enum mangusta_payload_type_e {
    MANGUSTA_PAYLOAD_RAW,
    MANGUSTA_PAYLOAD_MULTIPART,
    MANGUSTA_PAYLOAD_BOH,
};

//struct mangusta_http_methods
//struct mangusta_http_responses

struct mangusta_ctx_s {
    const char                  *host;
    int	                        port;
    apr_size_t                  maxconn;
    apr_size_t                  maxidle;
    mangusta_ctx_connect_cb_f   on_connect;
    char                        buffer[DEFAULT_BUFFER_SIZE];
    apr_size_t                  buffer_len;

    /* RUNTIME VALUES */
    apr_pool_t                  *pool;
    apr_socket_t                *sock;
    apr_pollset_t               *pollset;
    apr_thread_pool_t           *tp;
    apr_thread_t                *thread;
    apr_bool_t                  stopped;
};

struct mangusta_connection_s {
    mangusta_ctx_t                      *ctx;
    apr_pool_t                          *pool;
    apr_socket_t                        *sock;
    apr_pollset_t                       *pollset;
    char                                *method_string;
    enum mangusta_request_state_e       state;
    enum mangusta_request_method_e      method;
    enum mangusta_http_version_e        httpv;
    char                                *url;
    apr_hash_t                          *headers;

    short                               terminated;
    apr_pollfd_t                        pfd;
    apr_time_t                          last_io;
    mangusta_buffer_t                   *buffer_r;
};

mangusta_connection_t *mangusta_connection_create(mangusta_ctx_t * ctx, apr_socket_t * sock);
void mangusta_connection_destroy(mangusta_connection_t * conn);
apr_status_t mangusta_connection_play(mangusta_connection_t * conn);

#ifndef strnstr
char *strnstr(const char *haystack, const char *needle, size_t len);
#endif

apr_status_t chomp(char *buffer, size_t length);

