/*
 * curl_demo.c - Demo program using the libcurl-compatible API in WASM
 *
 * Shows that a program written against the libcurl easy API can work
 * in WASM with our socket shim layer + curl_shim implementation.
 *
 * Usage: python3 ../run_wasm.py curl_demo.wasm <url>
 *   e.g. python3 ../run_wasm.py curl_demo.wasm http://127.0.0.1:8080/index.html
 *
 * NOTE: URLs must use IP addresses, not hostnames (no DNS in WASM).
 */

#include "curl_shim.h"

extern int printf(const char *fmt, ...);
extern unsigned long strlen(const char *s);

/* Write callback - receives body data from curl */
static unsigned long write_callback(char *ptr, unsigned long size,
                                    unsigned long nmemb, void *userdata) {
    unsigned long total = size * nmemb;
    /* Print the received data */
    /* We need to null-terminate for printf - data might not be */
    for (unsigned long i = 0; i < total; i++) {
        printf("%c", ptr[i]);
    }
    /* Track total bytes */
    unsigned long *counter = (unsigned long *)userdata;
    *counter += total;
    return total;
}

int main(int argc, char **argv) {
    const char *url;

    if (argc < 2) {
        printf("Usage: curl_demo <url>\n");
        printf("Example: curl_demo http://93.184.215.14/\n");
        printf("NOTE: Use IP addresses, not hostnames (no DNS in WASM)\n");
        return 1;
    }

    url = argv[1];

    printf("=== WASM curl demo ===\n");
    printf("URL: %s\n\n", url);

    /* Initialize curl - just like real libcurl! */
    CURL *curl = curl_easy_init();
    if (!curl) {
        printf("curl_easy_init() failed\n");
        return 1;
    }

    unsigned long bytes_received = 0;

    /* Set options - same API as real libcurl */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt_ptr(curl, CURLOPT_WRITEFUNCTION, (void *)write_callback);
    curl_easy_setopt_ptr(curl, CURLOPT_WRITEDATA, &bytes_received);

    /* Perform the request */
    CURLcode res = curl_easy_perform(curl);

    printf("\n\n");
    if (res != CURLE_OK) {
        printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } else {
        printf("Transfer complete: %lu bytes received\n", bytes_received);
    }

    /* Cleanup */
    curl_easy_cleanup(curl);

    return (res != CURLE_OK) ? 1 : 0;
}
