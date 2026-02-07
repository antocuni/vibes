/*
 * posix_sockets.h - POSIX socket shim for WASM
 *
 * Declares socket functions as imports from the "posix_sockets" WASM module.
 * The host (Python/wasmtime) provides the actual implementations.
 *
 * C-side functions use wasm_sock_ prefix to avoid collisions with wasi-libc
 * symbols. The import_name attribute maps them to clean names in the WASM
 * import table.
 */

#ifndef POSIX_SOCKETS_H
#define POSIX_SOCKETS_H

/* Address families */
#define AF_INET 2

/* Socket types */
#define SOCK_STREAM 1
#define SOCK_DGRAM 2

/* Protocols */
#define IPPROTO_TCP 6

/* Shutdown modes */
#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

/* sockaddr_in for IPv4 - matches standard layout */
struct sockaddr_in {
    unsigned short sin_family;
    unsigned short sin_port;       /* network byte order */
    unsigned int   sin_addr;       /* network byte order */
    char           sin_zero[8];
};

/* Generic sockaddr */
struct sockaddr {
    unsigned short sa_family;
    char           sa_data[14];
};

typedef unsigned int socklen_t;

/*
 * These functions are imported from the "posix_sockets" WASM module.
 * The __attribute__((import_module(...))) tells the WASM linker to
 * look for these in the specified import module.
 */

__attribute__((import_module("posix_sockets"), import_name("socket")))
int wasm_sock_socket(int domain, int type, int protocol);

__attribute__((import_module("posix_sockets"), import_name("connect")))
int wasm_sock_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

__attribute__((import_module("posix_sockets"), import_name("send")))
int wasm_sock_send(int sockfd, const void *buf, int len, int flags);

__attribute__((import_module("posix_sockets"), import_name("recv")))
int wasm_sock_recv(int sockfd, void *buf, int len, int flags);

__attribute__((import_module("posix_sockets"), import_name("close")))
int wasm_sock_close(int sockfd);

__attribute__((import_module("posix_sockets"), import_name("bind")))
int wasm_sock_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

__attribute__((import_module("posix_sockets"), import_name("listen")))
int wasm_sock_listen(int sockfd, int backlog);

__attribute__((import_module("posix_sockets"), import_name("accept")))
int wasm_sock_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

__attribute__((import_module("posix_sockets"), import_name("shutdown")))
int wasm_sock_shutdown(int sockfd, int how);

/* Helper: convert host byte order to network byte order (big endian) */
static inline unsigned short htons_wasm(unsigned short hostshort) {
    return (unsigned short)(((hostshort & 0xFF) << 8) | ((hostshort >> 8) & 0xFF));
}

/* Helper: parse dotted-quad IP into network byte order uint32 */
static inline unsigned int inet_addr_wasm(const char *cp) {
    unsigned int result = 0;
    int i = 0, num = 0, dots = 0;
    unsigned int parts[4];

    while (1) {
        if (cp[i] == '.' || cp[i] == '\0') {
            parts[dots] = num;
            num = 0;
            dots++;
            if (cp[i] == '\0') break;
        } else {
            num = num * 10 + (cp[i] - '0');
        }
        i++;
    }
    if (dots != 4) return 0;
    result = parts[0] | (parts[1] << 8) | (parts[2] << 16) | (parts[3] << 24);
    return result;
}

#endif /* POSIX_SOCKETS_H */
