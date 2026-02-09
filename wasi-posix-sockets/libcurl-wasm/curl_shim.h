/*
 * curl_shim.h - Minimal libcurl-compatible API implemented on top of
 *               POSIX socket WASM shims
 *
 * This provides a subset of the libcurl easy API (curl_easy_init,
 * curl_easy_setopt, curl_easy_perform, curl_easy_cleanup) that works
 * in WASM by delegating socket operations to the host.
 *
 * Supported options:
 *   CURLOPT_URL          - set the target URL (http:// only)
 *   CURLOPT_WRITEFUNCTION - set a callback for received data
 *   CURLOPT_WRITEDATA    - set userdata for the write callback
 *   CURLOPT_PORT         - override port number
 */

#ifndef CURL_SHIM_H
#define CURL_SHIM_H

#include "../posix_sockets.h"

/* CURLcode - simplified error codes */
typedef enum {
    CURLE_OK = 0,
    CURLE_UNSUPPORTED_PROTOCOL = 1,
    CURLE_URL_MALFORMAT = 3,
    CURLE_COULDNT_RESOLVE_HOST = 6,
    CURLE_COULDNT_CONNECT = 7,
    CURLE_SEND_ERROR = 55,
    CURLE_RECV_ERROR = 56,
} CURLcode;

/* CURLoption - subset of libcurl options */
typedef enum {
    CURLOPT_URL = 10002,
    CURLOPT_PORT = 3,
    CURLOPT_WRITEFUNCTION = 20011,
    CURLOPT_WRITEDATA = 10001,
} CURLoption;

/* Write callback: size_t callback(char *ptr, size_t size, size_t nmemb, void *userdata) */
typedef unsigned long (*curl_write_callback)(char *ptr, unsigned long size,
                                             unsigned long nmemb, void *userdata);

/* CURL handle */
typedef struct {
    char url[512];
    char host[256];
    char path[256];
    char ip[64];     /* resolved IP - must be passed as IP since we can't do DNS */
    int  port;
    curl_write_callback write_fn;
    void *write_data;
} CURL;

/* Forward declarations - implemented in curl_shim.c */
CURL *curl_easy_init(void);
void  curl_easy_cleanup(CURL *handle);
CURLcode curl_easy_setopt_str(CURL *handle, CURLoption option, const char *value);
CURLcode curl_easy_setopt_long(CURL *handle, CURLoption option, long value);
CURLcode curl_easy_setopt_ptr(CURL *handle, CURLoption option, void *value);
CURLcode curl_easy_perform(CURL *handle);
const char *curl_easy_strerror(CURLcode code);

/*
 * Convenience macros for curl_easy_setopt - select the right typed version
 * based on the option. In real libcurl this uses a variadic function.
 */
#define curl_easy_setopt(handle, option, value) \
    _Generic((value), \
        char*: curl_easy_setopt_str, \
        const char*: curl_easy_setopt_str, \
        long: curl_easy_setopt_long, \
        int: curl_easy_setopt_long, \
        default: curl_easy_setopt_ptr \
    )(handle, option, value)

#endif /* CURL_SHIM_H */
