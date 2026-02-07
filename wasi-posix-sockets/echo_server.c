/*
 * echo_server.c - Simple TCP echo server using POSIX socket shims over WASM
 *
 * Binds to a port, accepts one connection, echoes back everything received,
 * then exits. All socket operations are delegated to the host via WASM imports.
 */

#include "posix_sockets.h"

extern int printf(const char *fmt, ...);

int main(int argc, char **argv) {
    int port = 8080;

    if (argc >= 2) {
        port = 0;
        for (int i = 0; argv[1][i]; i++)
            port = port * 10 + (argv[1][i] - '0');
    }

    printf("=== WASM Echo Server ===\n");
    printf("Starting on port %d\n", port);

    /* Create socket */
    int server_fd = wasm_sock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
        printf("socket() failed: %d\n", server_fd);
        return 1;
    }
    printf("Server socket created: fd=%d\n", server_fd);

    /* Bind */
    struct sockaddr_in addr;
    for (int i = 0; i < (int)sizeof(addr); i++)
        ((char *)&addr)[i] = 0;
    addr.sin_family = AF_INET;
    addr.sin_port = htons_wasm((unsigned short)port);
    addr.sin_addr = 0; /* INADDR_ANY */

    int ret = wasm_sock_bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        printf("bind() failed: %d\n", ret);
        wasm_sock_close(server_fd);
        return 1;
    }
    printf("Bound to port %d\n", port);

    /* Listen */
    ret = wasm_sock_listen(server_fd, 1);
    if (ret < 0) {
        printf("listen() failed: %d\n", ret);
        wasm_sock_close(server_fd);
        return 1;
    }
    printf("Listening... (waiting for one connection)\n");

    /* Accept one connection */
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = wasm_sock_accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        printf("accept() failed: %d\n", client_fd);
        wasm_sock_close(server_fd);
        return 1;
    }
    printf("Client connected: fd=%d\n", client_fd);

    /* Echo loop */
    char buf[4096];
    int total = 0;
    while (1) {
        ret = wasm_sock_recv(client_fd, buf, (int)sizeof(buf), 0);
        if (ret <= 0) break;
        printf("Received %d bytes, echoing back...\n", ret);
        int sent = wasm_sock_send(client_fd, buf, ret, 0);
        if (sent < 0) {
            printf("send() failed: %d\n", sent);
            break;
        }
        total += ret;
    }

    printf("Client disconnected. Total echoed: %d bytes\n", total);

    /* Clean up */
    wasm_sock_close(client_fd);
    wasm_sock_close(server_fd);
    printf("Server shut down.\n");

    return 0;
}
