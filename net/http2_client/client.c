/*
 * nghttp2 - HTTP/2 C Library
 *
 * Copyright (c) 2013 Tatsuhiro Tsujikawa
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/*
 * This program is written to show how to use nghttp2 API in C and
 * intentionally made simple.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <inttypes.h>
#include <stdlib.h>
#include "mico.h"
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <nghttp2/nghttp2.h>
//#include "nghttp2_session.h"

#define NGHTTP2_NORETURN

enum { IO_NONE, WANT_READ, WANT_WRITE };

#define MAKE_NV(NAME, VALUE)                                                   \
  {                                                                            \
    (uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, sizeof(VALUE) - 1,    \
        NGHTTP2_NV_FLAG_NONE                                                   \
  }

#define MAKE_NV_CS(NAME, VALUE)                                                \
  {                                                                            \
    (uint8_t *)NAME, (uint8_t *)VALUE, sizeof(NAME) - 1, strlen(VALUE),        \
        NGHTTP2_NV_FLAG_NONE                                                   \
  }

#define WXLIB_MAKE_NV(NAME, VALUE)                                    \
  {                                                                    \
    (uint8_t *)NAME, (uint8_t *)VALUE, strlen(NAME), strlen(VALUE),    \
        NGHTTP2_NV_FLAG_NONE                                           \
  }

/** Flag indicating receive stream is reset */
#define DATA_RECV_RST_STREAM      1
/** Flag indicating frame is completely received  */
#define DATA_RECV_FRAME_COMPLETE  2

struct Connection {
  int  fd;
  
  mico_ssl_t ssl;
  nghttp2_session *session;
  /* WANT_READ if SSL/TLS connection needs more input; or WANT_WRITE
     if it needs more output; or IO_NONE. This is necessary because
     SSL/TLS re-negotiation is possible at any time. nghttp2 API
     offers similar functions like nghttp2_session_want_read() and
     nghttp2_session_want_write() but they do not take into account
     SSL/TSL connection. */
  int want_io;
};

struct Request {
  char *host;
  /* In this program, path contains query component as well. */
  char *path;
  /* This is the concatenation of host and port with ":" in
     between. */
  char *hostport;
  /* Stream ID for this request. */
  int32_t stream_id;
  uint16_t port;
};

struct URI {
  const char *host;
  /* In this program, path contains query component as well. */
  const char *path;
  size_t pathlen;
  const char *hostport;
  size_t hostlen;
  size_t hostportlen;
  uint16_t port;
};

/*
 * Returns copy of string |s| with the length |len|. The returned
 * string is NULL-terminated.
 */
static char *strcopy(const char *s, size_t len) {
  char *dst;
  dst = malloc(len + 1);
  memcpy(dst, s, len);
  dst[len] = '\0';
  return dst;
}

/*
 * Prints error message |msg| and exit.
 */
NGHTTP2_NORETURN
static void die(const char *msg) {
  fprintf(stderr, "FATAL: %s\n", msg);
  exit(EXIT_FAILURE);
}

/*
 * Prints error containing the function name |func| and message |msg|
 * and exit.
 */
NGHTTP2_NORETURN
static void dief(const char *func, const char *msg) {
  fprintf(stderr, "FATAL: %s: %s\n", func, msg);
  exit(EXIT_FAILURE);
}

/*
 * Prints error containing the function name |func| and error code
 * |error_code| and exit.
 */
NGHTTP2_NORETURN
static void diec(const char *func, int error_code) {
  fprintf(stderr, "FATAL: %s: error_code=%d, msg=%s\n", func, error_code,
          nghttp2_strerror(error_code));
  exit(EXIT_FAILURE);
}

char *WXlib_frame_type_str(int type)
{
    switch (type) {
    case NGHTTP2_HEADERS:
        return "HEADERS";
        break;
    case NGHTTP2_RST_STREAM:
        return "RST_STREAM";
        break;
    case NGHTTP2_GOAWAY:
        return "GOAWAY";
        break;
    case NGHTTP2_DATA:
        return "DATA";
        break;
    case NGHTTP2_SETTINGS:
        return "SETTINGS";
        break;
    case NGHTTP2_PUSH_PROMISE:
        return "PUSH_PROMISE";
        break;
    case NGHTTP2_PING:
        return "PING";
        break;
    default:
        return "other";
        break;
    }
}


#define HTTP2_ALPN_LIST "h2,http/1.1"
/* SSL connection on the TCP socket that is already connected */
static int do_ssl_connect(struct Connection *hd, int sockfd, const char *hostname)
{
    //  nghttp2_session_callbacks *callbacks;
    int rv;
    int fd;
    mico_ssl_t ssl;

    ssl_set_alpn_list(HTTP2_ALPN_LIST);
    ssl = ssl_connect_sni(sockfd, 0, NULL, hostname, &rv);
    if (ssl == NULL) {
    dief("SSL_new", "error");
    }
    hd->ssl = ssl;
    hd->want_io = IO_NONE;

    printf("[INFO] SSL/TLS handshake completed\n");


//    SSL_CTX *ssl_ctx = SSL_CTX_new(TLSv1_2_client_method());
//    if (!ssl_ctx) {
//        return -1;
//    }
//
//    unsigned char vector[] = "\x02h2";
//    SSL_CTX_set_alpn_protos(ssl_ctx, vector, strlen((char *)vector));
//    SSL *ssl = SSL_new(ssl_ctx);
//    if (!ssl) {
//        SSL_CTX_free(ssl_ctx);
//        return -1;
//    }
//
//    SSL_set_tlsext_host_name(ssl, hostname);
//    SSL_set_fd(ssl, sockfd);
//    int ret = SSL_connect(ssl);
//    if (ret < 1) {
//        int err = SSL_get_error(ssl, ret);
//        ESP_LOGE(TAG, "[ssl-connect] Failed SSL handshake ret=%d error=%d", ret, err);
//        SSL_CTX_free(ssl_ctx);
//        SSL_free(ssl);
//        return -1;
//    }
//    hd->ssl_ctx = ssl_ctx;
//    hd->ssl = ssl;
//    hd->sockfd = sockfd;
//
//    int flags = fcntl(hd->sockfd, F_GETFL, 0);
//    fcntl(hd->sockfd, F_SETFL, flags | O_NONBLOCK);
//
//    return 0;
}

/*
 * The implementation of nghttp2_send_callback type. Here we write
 * |data| with size |length| to the network and return the number of
 * bytes actually written. See the documentation of
 * nghttp2_send_callback for the details.
 */
static ssize_t callback_send_inner(struct Connection *hd, const uint8_t *data,
                                   size_t length)
{
    int rv = ssl_send(hd->ssl, data, (int)length);
    if (rv <= 0) {
        int err = ssl_get_error(hd->ssl, rv);
        if (err == 2 || err == 6) {
            rv = NGHTTP2_ERR_WOULDBLOCK;
        } else {
            rv = NGHTTP2_ERR_CALLBACK_FAILURE;
        }
    }
    return rv;
}

/*
 * The implementation of nghttp2_send_callback type. Here we write
 * |data| with size |length| to the network and return the number of
 * bytes actually written. See the documentation of
 * nghttp2_send_callback for the details.
 */
static ssize_t send_callback(nghttp2_session *session, const uint8_t *data,
                             size_t length, int flags, void *user_data) {
    int rv = 0;
    struct Connection *hd = user_data;

    int copy_offset = 0;
    int pending_data = length;

    printf("[send_callback] %d\r\n", length);
    /* Send data in 1000 byte chunks */
    while (pending_data > 0) {
        int chunk_len = pending_data > 1000 ? 1000 : pending_data;
        int subrv = callback_send_inner(hd, data + copy_offset, chunk_len);
        if (subrv <= 0) {
            if (copy_offset) {
                /* If some data was xferred, send the number of bytes
                 * xferred */
                rv = copy_offset;
            } else {
                /* If not, send the error code */
                rv = subrv;
            }
            break;
        } else {
            rv = subrv;
        }
        copy_offset += chunk_len;
        pending_data -= chunk_len;
    }

    return rv;
}

/*
 * The implementation of nghttp2_recv_callback type. Here we read data
 * from the network and write them in |buf|. The capacity of |buf| is
 * |length| bytes. Returns the number of bytes stored in |buf|. See
 * the documentation of nghttp2_recv_callback for the details.
 */
static ssize_t recv_callback(nghttp2_session *session, uint8_t *buf,
                             size_t length, int flags, void *user_data) {
  struct Connection *connection;
  int rv, err=0, ret;
//  (void)session;
//  (void)flags;

  connection = (struct Connection *)user_data;
  connection->want_io = IO_NONE;
  ret = ssl_recv(connection->ssl, buf, (int)length);
  rv = ret;
  if (ret <= 0) {
    err = ssl_get_error(connection->ssl, ret);
    if (err == 2 || err == 6) {
        rv = NGHTTP2_ERR_WOULDBLOCK;
    } else {
        rv = NGHTTP2_ERR_CALLBACK_FAILURE;
    }
  } else if (rv == 0) {
      rv = NGHTTP2_ERR_EOF;
  }
  printf("[recv_callback] lenght=%d rv=%d, err %d, ret %d\r\n",  length, rv, err, ret);
  printf("[recv_callback] buffer here\r\n %s\r\n", buf);
  return rv;
}

static int on_frame_send_callback(nghttp2_session *session,
                                  const nghttp2_frame *frame, void *user_data) {
  size_t i;
//  (void)user_data;
  printf("[frame-send] frame type %s stream_id %d\r\n",
         WXlib_frame_type_str(frame->hd.type),
         frame->hd.stream_id);
  switch (frame->hd.type) {
  case NGHTTP2_HEADERS:
    if (nghttp2_session_get_stream_user_data(session, frame->hd.stream_id)) {
      const nghttp2_nv *nva = frame->headers.nva;
      printf("[INFO] C ----------------------------> S (HEADERS)\n");
      for (i = 0; i < frame->headers.nvlen; ++i) {
        fwrite(nva[i].name, 1, nva[i].namelen, stdout);
        printf(": ");
        fwrite(nva[i].value, 1, nva[i].valuelen, stdout);
        printf("\n");
      }
    }
    break;
  case NGHTTP2_RST_STREAM:
    printf("[INFO] C ----------------------------> S (RST_STREAM)\n");
    break;
  case NGHTTP2_GOAWAY:
    printf("[INFO] C ----------------------------> S (GOAWAY)\n");
    break;
  }
  return 0;
}

typedef int (*WXlib_putpost_data_cb_t)(struct Connection *handle, char *data, size_t len, uint32_t *data_flags);

typedef int (*WXlib_frame_data_recv_cb_t)(struct Connection *handle, const char *data, size_t len, int flags);

static int on_frame_recv_callback(nghttp2_session *session,
                                  const nghttp2_frame *frame, void *user_data) {
  size_t i;
//  (void)user_data;
  printf("[frame-recv][sid: %d] frame type  %s\r\n", frame->hd.stream_id, WXlib_frame_type_str(frame->hd.type));

  switch (frame->hd.type) {
  case NGHTTP2_HEADERS:
    if (frame->headers.cat == NGHTTP2_HCAT_RESPONSE) {
      const nghttp2_nv *nva = frame->headers.nva;
      struct Request *req;
      req = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
      if (req) {
        printf("[INFO] C <---------------------------- S (HEADERS)\n");
        for (i = 0; i < frame->headers.nvlen; ++i) {
          fwrite(nva[i].name, 1, nva[i].namelen, stdout);
          printf(": ");
          fwrite(nva[i].value, 1, nva[i].valuelen, stdout);
          printf("\n");
        }
      }
    }
    break;
  case NGHTTP2_RST_STREAM:
    printf("[INFO] C <---------------------------- S (RST_STREAM)\n");
    break;
  case NGHTTP2_GOAWAY:
    printf("[INFO] C <---------------------------- S (GOAWAY)\n");
    break;
  case NGHTTP2_DATA:
    printf("[INFO] C <---------------------------- S (DATA)\n");
    WXlib_frame_data_recv_cb_t data_recv_cb = nghttp2_session_get_stream_user_data(session, frame->hd.stream_id);
    if (data_recv_cb) {
        struct Connection *h2 = user_data;
        (*data_recv_cb)(h2, NULL, 0, DATA_RECV_FRAME_COMPLETE);
    }
  }
  return 0;
}

/*
 * The implementation of nghttp2_on_stream_close_callback type. We use
 * this function to know the response is fully received. Since we just
 * fetch 1 resource in this program, after reception of the response,
 * we submit GOAWAY and close the session.
 */
static int on_stream_close_callback(nghttp2_session *session, int32_t stream_id,
                                    uint32_t error_code, void *user_data) {
    printf("[stream-close][sid %d]\r\n", stream_id);
    WXlib_frame_data_recv_cb_t data_recv_cb = nghttp2_session_get_stream_user_data(session, stream_id);
    if (data_recv_cb) {
        struct Connection *h2 = user_data;
        (*data_recv_cb)(h2, NULL, 0, DATA_RECV_RST_STREAM);
    }
    return 0;
}

/*
 * The implementation of nghttp2_on_data_chunk_recv_callback type. We
 * use this function to print the received response body.
 */
static int on_data_chunk_recv_callback(nghttp2_session *session, uint8_t flags,
                                       int32_t stream_id, const uint8_t *data,
                                       size_t len, void *user_data) {
    WXlib_frame_data_recv_cb_t data_recv_cb;
    printf("[data-chunk][sid:%d]\r\n", stream_id);
    data_recv_cb = nghttp2_session_get_stream_user_data(session, stream_id);
    if (data_recv_cb) {
        printf("[data-chunk] C <---------------------------- S (DATA chunk)"
                "%lu bytes",
                (unsigned long int)len);
        struct Connection *h2 = user_data;
        (*data_recv_cb)(h2, (char *)data, len, 0);
        /* TODO: What to do with the return value: look for pause/abort */
    }
    return 0;
}

static int on_header_callback(nghttp2_session *session, const nghttp2_frame *frame,
                              const uint8_t *name, size_t namelen, const uint8_t *value,
                              size_t valuelen, uint8_t flags, void *user_data)
{
    printf("[hdr-recv][sid:%d] %s : %s\r\n", frame->hd.stream_id, name, value);
    return 0;
}

/*
 * Setup callback functions. nghttp2 API offers many callback
 * functions, but most of them are optional. The send_callback is
 * always required. Since we use nghttp2_session_recv(), the
 * recv_callback is also required.
 */
static void setup_nghttp2_callbacks(nghttp2_session_callbacks *callbacks) {
  nghttp2_session_callbacks_set_send_callback(callbacks, send_callback);

  nghttp2_session_callbacks_set_recv_callback(callbacks, recv_callback);

  nghttp2_session_callbacks_set_on_frame_send_callback(callbacks,
                                                       on_frame_send_callback);

  nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks,
                                                       on_frame_recv_callback);

  nghttp2_session_callbacks_set_on_stream_close_callback(
      callbacks, on_stream_close_callback);

  nghttp2_session_callbacks_set_on_data_chunk_recv_callback(
      callbacks, on_data_chunk_recv_callback);
}

/*
 * Callback function for TLS NPN. Since this program only supports
 * HTTP/2 protocol, if server does not offer HTTP/2 the nghttp2
 * library supports, we terminate program.
 */
static int select_next_proto_cb(mico_ssl_t ssl, unsigned char **out,
                                unsigned char *outlen, const unsigned char *in,
                                unsigned int inlen, void *arg) {
  int rv;
  (void)ssl;
  (void)arg;

  /* nghttp2_select_next_protocol() selects HTTP/2 protocol the
     nghttp2 library supports. */
  rv = nghttp2_select_next_protocol(out, outlen, in, inlen);
  if (rv <= 0) {
    die("Server did not advertise HTTP/2 protocol");
  }
  return 0;
}


/*
 * Connects to the host |host| and port |port|.  This function returns
 * the file descriptor of the client socket.
 */
static int connect_to(const char *host, uint16_t port) {
  struct addrinfo hints;
  int fd = -1;
  int rv;
  char service[64];
  struct addrinfo *res, *rp;
  snprintf(service, sizeof(service), "%u", port);
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  rv = getaddrinfo(host, service, &hints, &res);
  if (rv != 0) {
    dief("getaddrinfo", "error");
  }
  for (rp = res; rp; rp = rp->ai_next) {
    fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (fd == -1) {
      continue;
    }
    while ((rv = connect(fd, rp->ai_addr, rp->ai_addrlen)) == -1 &&
           errno == EINTR)
      ;
    if (rv == 0) {
      break;
    }
    close(fd);
    fd = -1;
  }
  freeaddrinfo(res);
  return fd;
}

#define ACCESS_TOKEN "Bearer 26.f7acfc53a494b599b2280a60188273b3.2592000.1516349054.890415133-10317421"

/*
 * Submits the request |req| to the connection |connection|.  This
 * function does not send packets; just append the request to the
 * internal queue in |connection->session|.
 */
static void submit_request(struct Connection *connection, struct Request *req) {
  int32_t stream_id;
  /* Make sure that the last item is NULL */
  const nghttp2_nv nva[] = {
                            WXLIB_MAKE_NV(":method", "GET"),
                            WXLIB_MAKE_NV(":path", "/dcs/v1/directives"),
                            WXLIB_MAKE_NV(":scheme", "https"),
                            WXLIB_MAKE_NV("host", "dueros-h2.baidu.com"),
                            WXLIB_MAKE_NV("authorization", ACCESS_TOKEN),
                            WXLIB_MAKE_NV("dueros-device-id", "123"),
                           };

  stream_id = nghttp2_submit_request(connection->session, NULL, nva,
                                     sizeof(nva) / sizeof(nva[0]), NULL, req);

  if (stream_id < 0) {
    diec("nghttp2_submit_request", stream_id);
  }

  req->stream_id = stream_id;
  printf("[INFO] %d Stream ID = %d\n", __LINE__, stream_id);
}

int handle_get_response(struct Connection *handle, const char *data, size_t len, int flags)
{
    if (len) {
        printf("[get-response] %.*s\n", len, data);
    }
    if (flags == DATA_RECV_FRAME_COMPLETE) {
        printf("[get-response] Frame fully received\n");
    }
    if (flags == DATA_RECV_RST_STREAM) {
        printf("[get-response] Stream Closed\n");
    }
    return 0;
}

int handle_echo_response(struct Connection *handle, const char *data, size_t len, int flags)
{
    printf("enter echo response\r\n");
    if (len) {
        printf("[echo-response] %.*s\n", len, data);
    }
    if (flags == DATA_RECV_FRAME_COMPLETE) {
        printf("[echo-response] Frame fully received\n");
    }
    if (flags == DATA_RECV_RST_STREAM) {
        printf("[echo-response] Stream Closed\n");
    }
    return 0;
}

int send_put_data(struct Connection *handle, char *buf, size_t length, uint32_t *data_flags)
{
    char * DATA_TO_SEND =
        "--this-is-a-boundary\r\n" \
        "Content-Disposition: form-data; name=\"metadata\"\r\n" \
        "Content-Type: application/json; charset=UTF-8\r\n" \
        "\r\n" \
        "{\r\n"
          "\"clientContext\" : [\r\n" \
            "0,\r\n" \
            "0,\r\n" \
            "30,\r\n" \
            "0\r\n" \
          "],\r\n" \
          "\"event\" : {\r\n" \
            "\"payload\" : {\r\n" \
              "\"format\" : \"AUDIO_L16_RATE_16000_CHANNELS_1\"\r\n" \
            "},\r\n" \
            "\"header\" : {\r\n" \
              "\"namespace\" : \"ai.dueros.device_interface.voice_input\",\r\n" \
              "\"name\" : \"ListenStarted\",\r\n" \
              "\"messageId\" : \"123\",\r\n" \
              "\"dialogRequestId\" : \"666\"\r\n" \
            "}\r\n" \
          "}\r\n" \
        "}\r\n" \
        "\r\n" \
        "--this-is-a-boundary\r\n" \
        "Content-Disposition: form-data; name=\"audio\"\r\n" \
        "Content-Type: application/octet-stream\r\n\r\n" \
        "\r\n--this-is-a-boundary--\r\n";

        int copylen = strlen(DATA_TO_SEND);

        if (copylen < length) {
            printf("[data-prvd] Sending %d bytes\n", copylen);
            memcpy(buf, DATA_TO_SEND, copylen);
        } else {
            copylen = 0;
        }

        (*data_flags) |= NGHTTP2_DATA_FLAG_EOF;
        return copylen;
}



ssize_t WXlib_data_provider_cb(nghttp2_session *session, int32_t stream_id, uint8_t *buf,
                                size_t length, uint32_t *data_flags,
                                nghttp2_data_source *source, void *user_data)
{
    struct Connection *h2 = user_data;
    WXlib_putpost_data_cb_t data_cb = source->ptr;
    return (*data_cb)(h2, (char *)buf, length, data_flags);
}

int WXlib_do_get_with_nv(struct Connection *hd, const nghttp2_nv *nva, size_t nvlen,
                              WXlib_frame_data_recv_cb_t recv_cb,
                              struct Request *req)
{
    int stream_id = nghttp2_submit_request(hd->session, NULL, nva, nvlen, NULL, recv_cb);

    if (stream_id < 0) {
        diec("nghttp2_submit_request", stream_id);
      }

      req->stream_id = stream_id;
      printf("[INFO] %d Stream ID = %d\n", __LINE__, stream_id);
}

int WXlib_do_putpost_with_nv(struct Connection *hd, const nghttp2_nv *nva, size_t nvlen,
                              WXlib_putpost_data_cb_t send_cb,
                              WXlib_frame_data_recv_cb_t recv_cb,
                              struct Request *req)
{

    nghttp2_data_provider WXlib_data_provider;
    WXlib_data_provider.read_callback = WXlib_data_provider_cb;
    WXlib_data_provider.source.ptr = send_cb;
    int stream_id = nghttp2_submit_request(hd->session, NULL, nva, nvlen, &WXlib_data_provider, recv_cb);

    if (stream_id < 0) {
        diec("nghttp2_submit_request", stream_id);
      }

      req->stream_id = stream_id;
      printf("[INFO] %d Stream ID = %d\n", __LINE__, stream_id);
}

/*
 * Performs the network I/O.
 */
static void exec_io(struct Connection *connection) {
  int rv;
  
  rv = nghttp2_session_send(connection->session);
  if (rv != 0) {
    diec("nghttp2_session_send", rv);
  }
  rv = nghttp2_session_recv(connection->session);
  if (rv != 0) {
    diec("nghttp2_session_recv", rv);
  }
}

static void request_init(struct Request *req, const struct URI *uri) {
  req->host = strcopy(uri->host, uri->hostlen);
  req->port = uri->port;
  req->path = strcopy(uri->path, uri->pathlen);
  req->hostport = strcopy(uri->hostport, uri->hostportlen);
  req->stream_id = -1;
}

static void request_free(struct Request *req) {
  free(req->host);
  free(req->path);
  free(req->hostport);
}

int connect_to_host(const char *host, size_t hostlen, uint16_t port)
{
    int ret;
    struct addrinfo hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
    };

    char service[6];
    snprintf(service, sizeof(service), "%u", port);

    char *use_host = calloc(1, hostlen + 1);
    if (!use_host) {
         return -1;
    }
    strncpy(use_host, host, hostlen);

    struct addrinfo *res;
    ret = getaddrinfo(use_host, service, &hints, &res);
    if (ret) {
        free(use_host);
        return -1;
    }
    free(use_host);

    ret = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (ret < 0) {
        goto err_freeaddr;
    }

    int fd = ret;
    ret = connect(fd, res->ai_addr, res->ai_addrlen);
    if (ret < 0) {
        goto err_freesocket;
    }

    return fd;

err_freesocket:
    close(fd);
err_freeaddr:
    freeaddrinfo(res);
    return -1;
}

static int do_http2_connect(struct Connection *hd)
{
    int ret;
    nghttp2_session_callbacks *callbacks;
    nghttp2_session_callbacks_new(&callbacks);
    nghttp2_session_callbacks_set_send_callback(callbacks, send_callback);
    nghttp2_session_callbacks_set_recv_callback(callbacks, recv_callback);
    nghttp2_session_callbacks_set_on_frame_send_callback(callbacks, on_frame_send_callback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback);
    nghttp2_session_callbacks_set_on_data_chunk_recv_callback(callbacks, on_data_chunk_recv_callback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
    ret = nghttp2_session_client_new(&hd->session, callbacks, hd);
    if (ret != 0) {
        printf("[sh2-connect] New http2 session failed");
        nghttp2_session_callbacks_del(callbacks);
        return -1;
    }
    nghttp2_session_callbacks_del(callbacks);

    /* Create the SETTINGS frame */
    ret = nghttp2_submit_settings(hd->session, NGHTTP2_FLAG_NONE, NULL, 0);
    if (ret != 0) {
        printf("[sh2-connect] Submit settings failed");
        return -1;
    }
    return 0;
}

void WXlib_free(struct Connection *hd)
{
    if (hd->session) {
        nghttp2_session_del(hd->session);
        hd->session = NULL;
    }
    if (hd->ssl) {
        ssl_free(hd->ssl);
        hd->ssl = NULL;
    }
//    if (hd->ssl_ctx) {
//        SSL_CTX_free(hd->ssl_ctx);
//        hd->ssl_ctx = NULL;
//    }
//    if (hd->sockfd) {
//        close(hd->sockfd);
//        hd->ssl_ctx = 0;
//    }
}

int WXlib_connect(struct Connection *hd, struct URI uri)
{
    memset(hd, 0, sizeof(*hd));

    /* TCP connection with the server */
    int sockfd = connect_to_host(uri.host, uri.hostlen, uri.port);
    if (sockfd < 0) {
        printf("[WX-connect] Failed to connect to %s", uri.host);
        return -1;
    }

    /* SSL Connection on the socket */
    if (do_ssl_connect(hd, sockfd, uri.host) != 0) {
        printf("[WX-connect] SSL Handshake failed with %s", uri.host);
        goto error;
    }

    /* HTTP/2 Connection */
    if (do_http2_connect(hd) != 0) {
        printf("[WX-connect] HTTP2 Connection failed with %s", uri.host);
        goto error;
    }
    hd->fd = sockfd;
    
    return 0;
error:
    request_free(hd);
    return -1;
}



/*
 * Fetches the resource denoted by |uri|.
 */
static void run_progress(const struct URI uri) {
//  nghttp2_session_callbacks *callbacks;
//  int fd;
//  mico_ssl_t ssl;
//  struct Request req;
//  struct Connection connection;
//  int rv;
//
//
//  request_init(&req, uri);
//
//  printf("req host %s, port %d\r\n", req.host, req.port);
//  /* Establish connection and setup SSL */
//  fd = connect_to(req.host, req.port);
//  if (fd == -1) {
//    die("Could not open file descriptor");
//  }
//  ssl_set_alpn_list(HTTP2_ALPN_LIST);
//  ssl = ssl_connect_sni(fd, 0, NULL, req.host,&rv);
//  if (ssl == NULL) {
//    dief("SSL_new", "error");
//  }
//  connection.ssl = ssl;
//  connection.want_io = IO_NONE;
//
//
//  printf("[INFO] SSL/TLS handshake completed\n");
//
//  rv = nghttp2_session_callbacks_new(&callbacks);
//
//  if (rv != 0) {
//    diec("nghttp2_session_callbacks_new", rv);
//  }
//
//  setup_nghttp2_callbacks(callbacks);
//
//  rv = nghttp2_session_client_new(&connection.session, callbacks, &connection);
//
//  nghttp2_session_callbacks_del(callbacks);
//
//  if (rv != 0) {
//    diec("nghttp2_session_client_new", rv);
//  }
//
//  rv = nghttp2_submit_settings(connection.session, NGHTTP2_FLAG_NONE, NULL, 0);
//
//  if (rv != 0) {
//    diec("nghttp2_submit_settings", rv);
//  }

    int rv;
    fd_set readfds, writefds;
    struct timeval t;
    
    /* HTTP2: one connection multiple requests. Do the TLS/TCP connection first */
    printf("Connecting to server\n");
    struct Connection connection;
    struct Request req;
    request_init(&req, &uri);
    if (WXlib_connect(&connection, uri) != 0) {
    printf("Failed to connect\n");
        return;
    }
    printf("Connection done\n");


  /* Submit the HTTP request to the outbound queue. */
//  submit_request(&connection, &req);

    const nghttp2_nv nva_get_connect[] = {
                                WXLIB_MAKE_NV(":method", "GET"),
                                WXLIB_MAKE_NV(":path", "/dcs/v1/directives"),
                                WXLIB_MAKE_NV(":scheme", "https"),
                                WXLIB_MAKE_NV("host", "dueros-h2.baidu.com"),
                                WXLIB_MAKE_NV("authorization", ACCESS_TOKEN),
                                WXLIB_MAKE_NV("dueros-device-id", "123"),
                            };

  WXlib_do_get_with_nv(&connection, nva_get_connect,
                sizeof(nva_get_connect)/sizeof(nva_get_connect[0]),
                handle_get_response,
                &req);


  const nghttp2_nv nva_post_voice[] = {
                              WXLIB_MAKE_NV(":method", "POST"),
                              WXLIB_MAKE_NV(":path", "/dcs/v1/events"),
                              WXLIB_MAKE_NV(":scheme", "https"),
                              WXLIB_MAKE_NV("host", "dueros-h2.baidu.com"),
                              WXLIB_MAKE_NV("authorization", ACCESS_TOKEN),
                              WXLIB_MAKE_NV("dueros-device-id", "123"),
                              WXLIB_MAKE_NV("content-type", "multipart/form-data; boundary=this-is-a-boundary"),
                           };

  uint32_t start_time = 0;

  start_time = mico_rtos_get_time();

  WXlib_do_putpost_with_nv(&connection, nva_post_voice,
          sizeof(nva_post_voice)/sizeof(nva_post_voice[0]),
          send_put_data,handle_echo_response,
          &req);

  while( 1 ) {
    t.tv_sec = 1;
    t.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(connection.fd, &readfds); 
    FD_SET(connection.fd, &writefds);
    select(connection.fd  + 1, &readfds, &writefds, NULL, &t);
    if (FD_ISSET(connection.fd, &writefds) && nghttp2_session_want_write(connection.session)) {
        rv = nghttp2_session_send(connection.session);
        if (rv != 0) {
          diec("nghttp2_session_send", rv);
        }
    }
    if (FD_ISSET(connection.fd, &readfds) && nghttp2_session_want_read(connection.session)) {
        rv = nghttp2_session_recv(connection.session);
        if (rv != 0) {
          diec("nghttp2_session_recv", rv);
        }
    }
    

  }
  
  /* Resource cleanup */
//  nghttp2_session_del(connection.session);
//  ssl_close(ssl);
//  shutdown(fd, SHUT_WR);
//  close(fd);
  request_free(&req);
}

static int parse_uri(struct URI *res, const char *uri) {
  /* We only interested in https */
  size_t len, i, offset;
  int ipv6addr = 0;
  memset(res, 0, sizeof(struct URI));
  len = strlen(uri);
  if (len < 9 || memcmp("https://", uri, 8) != 0) {
    return -1;
  }
  offset = 8;
  res->host = res->hostport = &uri[offset];
  res->hostlen = 0;
  if (uri[offset] == '[') {
    /* IPv6 literal address */
    ++offset;
    ++res->host;
    ipv6addr = 1;
    for (i = offset; i < len; ++i) {
      if (uri[i] == ']') {
        res->hostlen = i - offset;
        offset = i + 1;
        break;
      }
    }
  } else {
    const char delims[] = ":/?#";
    for (i = offset; i < len; ++i) {
      if (strchr(delims, uri[i]) != NULL) {
        break;
      }
    }
    res->hostlen = i - offset;
    offset = i;
  }
  if (res->hostlen == 0) {
    return -1;
  }
  /* Assuming https */
  res->port = 443;
  if (offset < len) {
    if (uri[offset] == ':') {
      /* port */
      const char delims[] = "/?#";
      int port = 0;
      ++offset;
      for (i = offset; i < len; ++i) {
        if (strchr(delims, uri[i]) != NULL) {
          break;
        }
        if ('0' <= uri[i] && uri[i] <= '9') {
          port *= 10;
          port += uri[i] - '0';
          if (port > 65535) {
            return -1;
          }
        } else {
          return -1;
        }
      }
      if (port == 0) {
        return -1;
      }
      offset = i;
      res->port = (uint16_t)port;
    }
  }
  res->hostportlen = (size_t)(uri + offset + ipv6addr - res->host);
  for (i = offset; i < len; ++i) {
    if (uri[i] == '#') {
      break;
    }
  }
  if (i - offset == 0) {
    res->path = "/";
    res->pathlen = 1;
  } else {
    res->path = &uri[offset];
    res->pathlen = i - offset;
  }
  return 0;
}

int http2_client_main(char *url) {
  struct URI uri;
  int rv;


  rv = parse_uri(&uri, url);
  if (rv != 0) {
    die("parse_uri failed");
  }

//  uri.hostport = ;
//  uri.hostportlen = strlen(uri.hostport);
//  fetch_uri(&uri);
  run_progress(uri);
  return EXIT_SUCCESS;
}

