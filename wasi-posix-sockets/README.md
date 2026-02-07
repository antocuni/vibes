# WASI POSIX Sockets Demo

POSIX socket functions (socket, connect, send, recv, bind, listen, accept, close)
exposed as WASM imports, with a Python/wasmtime host providing the real implementations.

## What was explored

WASI doesn't include socket support in its standard preview1 API. This project demonstrates
how to bridge that gap: C programs compiled to WASM declare socket functions as imports
(using `__attribute__((import_module(...)))`) and a Python host using the `wasmtime` library
fulfills those imports by delegating to real Python `socket` operations.

This approach lets C code use a familiar POSIX-like socket API while running inside a WASM
sandbox, with the host controlling all network access.

## Architecture

```
┌─────────────────────────────────────────────────┐
│  C program (compiled to WASM)                   │
│  ┌───────────────────────────────────────────┐  │
│  │  http_client.c / echo_server.c            │  │
│  │  calls: wasm_sock_socket(), connect(), ...│  │
│  └──────────────────┬────────────────────────┘  │
│                     │ WASM imports               │
│                     │ (posix_sockets module)     │
├─────────────────────┼───────────────────────────┤
│  Python host        │                           │
│  ┌──────────────────▼────────────────────────┐  │
│  │  run_wasm.py                              │  │
│  │  SocketShim class                         │  │
│  │  - manages fd -> socket.socket table      │  │
│  │  - reads/writes WASM linear memory        │  │
│  │  - translates sockaddr structs            │  │
│  └──────────────────┬────────────────────────┘  │
│                     │ real Python sockets        │
│                     ▼                            │
│               Operating System                   │
└─────────────────────────────────────────────────┘
```

## Files

| File | Description |
|------|-------------|
| `posix_sockets.h` | C header declaring socket functions as WASM imports |
| `http_client.c` | HTTP client demo - fetches a URL via raw sockets |
| `echo_server.c` | TCP echo server demo - accepts one connection, echoes data |
| `run_wasm.py` | Python host that loads WASM and provides socket imports |
| `test_demo.py` | End-to-end tests for all demos |
| `libcurl-wasm/` | libcurl-compatible API implemented on top of the socket shims |

## Implementation

### The shim header (`posix_sockets.h`)

Socket functions are declared with GCC/Clang attributes that control WASM import generation:

```c
__attribute__((import_module("posix_sockets"), import_name("socket")))
int wasm_sock_socket(int domain, int type, int protocol);
```

This tells the WASM linker: "this function is imported from module `posix_sockets`, name `socket`".
The C-side function name (`wasm_sock_socket`) avoids collisions with wasi-libc symbols.

### The Python host (`run_wasm.py`)

The `SocketShim` class maintains a table mapping integer file descriptors to Python `socket.socket`
objects. Each imported function:

1. Receives WASM i32 arguments (fd numbers, memory pointers, lengths)
2. Reads data from WASM linear memory when needed (e.g., sockaddr structs, send buffers)
3. Calls the corresponding Python socket method
4. Writes results back to WASM memory when needed (e.g., recv buffers)
5. Returns an i32 result code

The host uses `wasmtime.Linker.define_func()` with `access_caller=True` to get a `Caller`
object that provides access to the WASM module's exported memory.

### libcurl-compatible demo (`libcurl-wasm/`)

A minimal implementation of the libcurl easy API (`curl_easy_init`, `curl_easy_setopt`,
`curl_easy_perform`, `curl_easy_cleanup`) built on top of the socket shims. Demonstrates
that higher-level networking APIs can work in WASM using this approach.

## Key findings

- WASM `import_module`/`import_name` attributes work well for creating clean import boundaries
- The host needs to handle byte order carefully: `sin_port` is stored in network (big-endian)
  order by the C code, but WASM is little-endian for struct field access
- wasi-libc provides stdio (printf, etc.) which works alongside custom socket imports
- The `Caller` API in wasmtime-py provides convenient access to WASM linear memory
- Function names in C must avoid colliding with wasi-libc symbols (e.g., `posix_close` exists
  in wasi-libc, so we use `wasm_sock_close` instead)

## Usage

### Prerequisites

```bash
apt install wasi-libc libclang-rt-18-dev-wasm32 clang lld
pip install wasmtime
```

### Build

```bash
make                          # builds http_client.wasm and echo_server.wasm
cd libcurl-wasm && make       # builds curl_demo.wasm
```

### Run demos

```bash
# HTTP client (needs a server at the given IP:port)
python3 run_wasm.py http_client.wasm 93.184.215.14 80 /

# Echo server (listens on port 8080)
python3 run_wasm.py echo_server.wasm 8080

# curl demo (needs a server at the given URL - IP addresses only, no DNS)
python3 run_wasm.py libcurl-wasm/curl_demo.wasm http://127.0.0.1:8080/
```

### Run tests

```bash
python3 test_demo.py
```

The tests start local TCP servers and verify the WASM modules work end-to-end.

## Limitations

- **No DNS**: WASM modules must use IP addresses directly (the host could add a
  `gethostbyname` import to fix this)
- **IPv4 only**: Only `AF_INET` is implemented
- **HTTP/1.0 only**: The curl shim uses `Connection: close` semantics
- **No TLS**: Would need an additional shim layer for SSL/TLS
- **Single-threaded**: The echo server handles one connection at a time
