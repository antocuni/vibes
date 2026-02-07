# WASI HTTP Requests from C

Working examples of making HTTP requests from C, compiled to WebAssembly (WASI), and run with wasmtime.

## What was explored

Can we make HTTP requests from C compiled to WASI? Specifically:
- Can libcurl be used from WASI?
- Can other C HTTP libraries work with wasmtime's socket support?
- What actually works today?

## Key findings

- **libcurl cannot be compiled to WASI** -- wasi-libc does not yet provide POSIX socket APIs (`socket()`, `connect()`, `getaddrinfo()`). This is tracked in [wasi-libc#447](https://github.com/WebAssembly/wasi-libc/issues/447). No existing C networking library that uses BSD sockets can be compiled to WASI today.

- **wasi:sockets exists but has no C POSIX shim** -- wasmtime implements the `wasi:sockets` interfaces (TCP, UDP, DNS), but wasi-libc hasn't built the POSIX compatibility layer on top of them. You could use raw `wit-bindgen` bindings but would need to implement HTTP yourself.

- **wasi:http works and is the practical solution** -- The `wasi:http/outgoing-handler` interface provides HTTP-level abstractions in the WASI Component Model. Using `wit-bindgen` to generate C bindings, you can make full HTTP requests (GET, POST with body, custom headers) from C compiled to `wasm32-wasip2`.

- **The wit-bindgen C API has a non-obvious convention**: `bool`-returning functions return `true` (1) for success and `false` (0) for error. This is the opposite of the common C convention where 0 = success.

## How it works

The approach uses the **WASI Component Model** with `wasi:http@0.2.8`:

1. **WIT files** define the HTTP interfaces (`wasi:http/types`, `wasi:http/outgoing-handler`)
2. **wit-bindgen** generates C bindings (`.c`, `.h`, `_component_type.o`)
3. **wasi-sdk** compiles C code targeting `wasm32-wasip2`
4. The `wasm-component-ld` linker (bundled in wasi-sdk) creates a WASI component
5. **wasmtime** runs the component with `-S http -S inherit-network`

```
WIT files ──(wit-bindgen c)──> C bindings
     your_code.c + bindings ──(wasi-sdk clang)──> .wasm component
                                                       │
                                    ──(wasmtime run -S http)──> execution
```

## Examples

### HTTP GET (`http_get.c`)

Makes a GET request to `https://httpbin.org/get`, prints the status code, response headers, and body.

### HTTP POST (`http_post.c`)

Makes a POST request to `https://httpbin.org/post` with a JSON body, demonstrating how to write request headers and body.

## Prerequisites

- [wasi-sdk](https://github.com/WebAssembly/wasi-sdk) (v25+) -- C/C++ to WASI compiler
- [wasmtime](https://wasmtime.dev/) (v29+) -- WebAssembly runtime
- [wit-bindgen](https://github.com/bytecodealliance/wit-bindgen) -- generates C bindings from WIT
- [wasm-tools](https://github.com/bytecodealliance/wasm-tools) -- WebAssembly tooling

Install Rust tools:
```bash
cargo install wit-bindgen-cli wasm-tools
```

## Quick start

```bash
# Download wasi-sdk and WIT files
make setup

# Build both examples
make

# Run (against httpbin.org or any HTTP server)
make run-get
make run-post
```

## Local testing

A test script is included that starts a local HTTP server and runs both examples against it:

```bash
./test_local.sh
```

## Usage

```bash
# Run GET example
wasmtime run -S http -S inherit-network http_get.wasm

# Run POST example
wasmtime run -S http -S inherit-network http_post.wasm
```

The `-S http` flag enables the wasi:http imports, and `-S inherit-network` grants network access to the component.

## The wasi:http C API pattern

The generated C API follows this pattern for making outbound requests:

```c
#include "generated/imports.h"

// 1. Create headers
wasi_http_types_own_fields_t headers = wasi_http_types_constructor_fields();

// 2. Create request with headers
wasi_http_types_own_outgoing_request_t request =
    wasi_http_types_constructor_outgoing_request(headers);

// 3. Configure request (method, scheme, authority, path)
wasi_http_types_borrow_outgoing_request_t req = wasi_http_types_borrow_outgoing_request(request);
wasi_http_types_method_t method = { .tag = WASI_HTTP_TYPES_METHOD_GET };
wasi_http_types_method_outgoing_request_set_method(req, &method);
// ... set scheme, authority, path

// 4. Send request
wasi_http_outgoing_handler_own_future_incoming_response_t future;
wasi_http_outgoing_handler_error_code_t err;
wasi_http_outgoing_handler_handle(request, NULL, &future, &err);

// 5. Block on the future, get response
// 6. Read status, headers, body from response
```

See `http_get.c` and `http_post.c` for complete working examples.

## Links

- [wasi-http specification](https://github.com/WebAssembly/wasi-http) -- the HTTP interfaces in WIT
- [wasi-sdk](https://github.com/WebAssembly/wasi-sdk) -- C/C++ to WASI toolchain
- [wit-bindgen](https://github.com/bytecodealliance/wit-bindgen) -- binding generator
- [Component Model docs](https://component-model.bytecodealliance.org/language-support/c.html) -- C language support
- [wasi-libc socket tracking](https://github.com/WebAssembly/wasi-libc/issues/447) -- when POSIX sockets will work
