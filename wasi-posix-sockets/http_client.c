/*
 * http_client.c - Simple HTTP client using POSIX socket shims over WASM
 *
 * This program makes an HTTP GET request to a server, reads the response,
 * and prints it. All socket operations are delegated to the host via
 * WASM imports from the "posix_sockets" module.
 *
 * Compile: see Makefile
 * Run: python3 run_wasm.py http_client.wasm <host> <port> <path>
 */

#include "posix_sockets.h"

/* We use WASI for stdio (printf, etc.) but sockets come from our shim */
extern int printf(const char *fmt, ...);
extern int snprintf(char *buf, unsigned long size, const char *fmt, ...);
extern unsigned long strlen(const char *s);

int main(int argc, char **argv) {
    const char *host;
    int port;
    const char *path;

    if (argc < 4) {
        printf("Usage: http_client <ip> <port> <path>\n");
        printf("Example: http_client 93.184.215.14 80 /\n");
        return 1;
    }

    host = argv[1];
    /* simple atoi */
    port = 0;
    for (int i = 0; argv[2][i]; i++) {
        port = port * 10 + (argv[2][i] - '0');
    }
    path = argv[3];

    printf("=== WASM HTTP Client ===\n");
    printf("Connecting to %s:%d%s\n", host, port, path);

    /* Create a TCP socket */
    int fd = wasm_sock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) {
        printf("socket() failed: %d\n", fd);
        return 1;
    }
    printf("Socket created: fd=%d\n", fd);

    /* Set up the server address */
    struct sockaddr_in addr;
    for (int i = 0; i < (int)sizeof(addr); i++)
        ((char *)&addr)[i] = 0;
    addr.sin_family = AF_INET;
    addr.sin_port = htons_wasm((unsigned short)port);
    addr.sin_addr = inet_addr_wasm(host);

    /* Connect */
    int ret = wasm_sock_connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        printf("connect() failed: %d\n", ret);
        wasm_sock_close(fd);
        return 1;
    }
    printf("Connected!\n");

    /* Build HTTP request */
    char request[1024];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.0\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host);

    printf("Sending request (%d bytes)...\n", (int)strlen(request));
    ret = wasm_sock_send(fd, request, (int)strlen(request), 0);
    if (ret < 0) {
        printf("send() failed: %d\n", ret);
        wasm_sock_close(fd);
        return 1;
    }
    printf("Sent %d bytes\n", ret);

    /* Read response */
    printf("\n--- Response ---\n");
    char buf[4096];
    int total = 0;
    while (1) {
        ret = wasm_sock_recv(fd, buf, (int)sizeof(buf) - 1, 0);
        if (ret <= 0) break;
        buf[ret] = '\0';
        printf("%s", buf);
        total += ret;
    }
    printf("\n--- End of response (%d bytes) ---\n", total);

    /* Clean up */
    wasm_sock_close(fd);
    printf("Connection closed.\n");

    return 0;
}
