/*
 * curl_shim.c - Minimal libcurl implementation using POSIX socket WASM shims
 *
 * Implements curl_easy_* functions by parsing HTTP URLs and using our
 * posix_sockets.h imports for actual networking.
 */

#include "curl_shim.h"

extern int printf(const char *fmt, ...);
extern int snprintf(char *buf, unsigned long size, const char *fmt, ...);
extern unsigned long strlen(const char *s);
extern void *malloc(unsigned long size);
extern void free(void *ptr);

/* Simple string helpers since we can't use string.h easily */
static int str_starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++;
        prefix++;
    }
    return 1;
}

static void str_copy(char *dst, const char *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int str_len(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

/* Parse a URL like "http://1.2.3.4:80/path" into host, port, path */
static CURLcode parse_url(const char *url, char *host, int host_max,
                          int *port, char *path, int path_max) {
    if (!str_starts_with(url, "http://"))
        return CURLE_UNSUPPORTED_PROTOCOL;

    const char *p = url + 7; /* skip "http://" */
    int i = 0;

    /* Parse host (or IP) */
    while (p[i] && p[i] != ':' && p[i] != '/' && i < host_max - 1) {
        host[i] = p[i];
        i++;
    }
    host[i] = '\0';
    p += i;

    /* Parse optional port */
    *port = 80; /* default */
    if (*p == ':') {
        p++;
        *port = 0;
        while (*p >= '0' && *p <= '9') {
            *port = *port * 10 + (*p - '0');
            p++;
        }
    }

    /* Parse path */
    if (*p == '/') {
        str_copy(path, p, path_max);
    } else {
        path[0] = '/';
        path[1] = '\0';
    }

    return CURLE_OK;
}

CURL *curl_easy_init(void) {
    CURL *c = (CURL *)malloc(sizeof(CURL));
    if (!c) return (CURL *)0;

    /* Zero out */
    char *p = (char *)c;
    for (int i = 0; i < (int)sizeof(CURL); i++) p[i] = 0;

    c->port = 80;
    c->path[0] = '/';
    return c;
}

void curl_easy_cleanup(CURL *handle) {
    if (handle) free(handle);
}

CURLcode curl_easy_setopt_str(CURL *handle, CURLoption option, const char *value) {
    if (!handle) return CURLE_URL_MALFORMAT;

    switch (option) {
    case CURLOPT_URL:
        str_copy(handle->url, value, sizeof(handle->url));
        return parse_url(value, handle->host, sizeof(handle->host),
                        &handle->port, handle->path, sizeof(handle->path));
    default:
        return CURLE_OK;
    }
}

CURLcode curl_easy_setopt_long(CURL *handle, CURLoption option, long value) {
    if (!handle) return CURLE_URL_MALFORMAT;

    switch (option) {
    case CURLOPT_PORT:
        handle->port = (int)value;
        return CURLE_OK;
    default:
        return CURLE_OK;
    }
}

CURLcode curl_easy_setopt_ptr(CURL *handle, CURLoption option, void *value) {
    if (!handle) return CURLE_URL_MALFORMAT;

    switch (option) {
    case CURLOPT_WRITEFUNCTION:
        handle->write_fn = (curl_write_callback)value;
        return CURLE_OK;
    case CURLOPT_WRITEDATA:
        handle->write_data = value;
        return CURLE_OK;
    default:
        return CURLE_OK;
    }
}

CURLcode curl_easy_perform(CURL *handle) {
    if (!handle) return CURLE_URL_MALFORMAT;

    /* The host field must be an IP address (we can't do DNS in WASM).
     * The user should pass URLs like http://93.184.215.14/path */
    const char *ip = handle->host;
    int port = handle->port;

    /* Create TCP socket */
    int fd = wasm_sock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) return CURLE_COULDNT_CONNECT;

    /* Build sockaddr */
    struct sockaddr_in addr;
    for (int i = 0; i < (int)sizeof(addr); i++) ((char *)&addr)[i] = 0;
    addr.sin_family = AF_INET;
    addr.sin_port = htons_wasm((unsigned short)port);
    addr.sin_addr = inet_addr_wasm(ip);

    /* Connect */
    if (wasm_sock_connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        wasm_sock_close(fd);
        return CURLE_COULDNT_CONNECT;
    }

    /* Build and send HTTP request */
    char request[1024];
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "User-Agent: curl-wasm-shim/1.0\r\n"
        "Accept: */*\r\n"
        "Connection: close\r\n"
        "\r\n",
        handle->path, handle->host);

    int req_len = str_len(request);
    if (wasm_sock_send(fd, request, req_len, 0) < 0) {
        wasm_sock_close(fd);
        return CURLE_SEND_ERROR;
    }

    /* Read response */
    char buf[4096];
    int total_recv = 0;
    int header_done = 0;
    int body_start = 0;

    while (1) {
        int n = wasm_sock_recv(fd, buf + body_start, (int)sizeof(buf) - 1 - body_start, 0);
        if (n <= 0) break;
        n += body_start;
        buf[n] = '\0';
        body_start = 0;

        if (!header_done) {
            /* Scan for end of headers: \r\n\r\n */
            for (int i = 0; i < n - 3; i++) {
                if (buf[i] == '\r' && buf[i+1] == '\n' &&
                    buf[i+2] == '\r' && buf[i+3] == '\n') {
                    header_done = 1;
                    int body_offset = i + 4;
                    int body_len = n - body_offset;
                    if (body_len > 0 && handle->write_fn) {
                        handle->write_fn(buf + body_offset, 1, body_len, handle->write_data);
                    }
                    total_recv += body_len;
                    break;
                }
            }
        } else {
            /* Already past headers, deliver body data */
            if (handle->write_fn) {
                handle->write_fn(buf, 1, n, handle->write_data);
            }
            total_recv += n;
        }
    }

    wasm_sock_close(fd);
    return CURLE_OK;
}

const char *curl_easy_strerror(CURLcode code) {
    switch (code) {
    case CURLE_OK: return "No error";
    case CURLE_UNSUPPORTED_PROTOCOL: return "Unsupported protocol";
    case CURLE_URL_MALFORMAT: return "URL malformat";
    case CURLE_COULDNT_RESOLVE_HOST: return "Couldn't resolve host";
    case CURLE_COULDNT_CONNECT: return "Couldn't connect";
    case CURLE_SEND_ERROR: return "Send error";
    case CURLE_RECV_ERROR: return "Receive error";
    default: return "Unknown error";
    }
}
