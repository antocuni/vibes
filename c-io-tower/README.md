# Python-like IO Tower in C

A layered I/O stack in C, modeled after Python's `io` module hierarchy, using raw syscalls at the bottom and GC-managed `spy_Str *` strings at the top.

## What was explored

Python's I/O system is built as a tower of three layers: raw I/O (syscalls), buffered I/O, and text I/O. This project implements the same architecture in C, targeting a runtime where strings are GC-managed structs (`spy_Str *`) with a length, hash, and UTF-8 payload.

The goal: efficient, zero-copy-where-possible text I/O with proper newline handling, buffering, and support for both files and sockets.

## Architecture

```
┌─────────────────────────────────┐
│  Text IO  (spy_TextIO)          │  spy_Str * read/write
│  - newline translation          │  - universal, LF, CRLF, CR, none
│  - UTF-8 codepoint counting     │
├─────────────────────────────────┤
│  Buffered IO  (spy_BufferedIO)  │  byte[] read/write
│  - read buffer with peek/consume│  - zero-copy peek API
│  - write buffer with auto-flush │
├─────────────────────────────────┤
│  Raw IO  (spy_RawIO)            │  raw syscalls
│  - SYS_read / SYS_write         │  - SYS_openat / SYS_lseek
│  - files and sockets            │  - short-write retry loop
└─────────────────────────────────┘
```

### Layer 0: Raw IO

Thin wrapper around libc I/O calls (`open`, `read`, `write`, `lseek`, `close`). Handles short writes with a retry loop. Can wrap an existing fd (for sockets, pipes, etc.).

### Layer 1: Buffered IO

Adds read and write buffers (default 8 KiB). Key feature: **peek/consume API** — the text layer can look at buffered data in-place without copying, then consume only what it needs. The read buffer compacts with `memmove` only when necessary (data consumed from the front).

### Layer 2: Text IO

Converts between raw bytes and `spy_Str *`. Features:

- **Newline modes** (like Python): universal (`\r\n`/`\r`/`\n` → `\n`), none, LF, CRLF, CR
- **UTF-8 codepoint counting** for `read(nchars)` — correctly handles multi-byte sequences
- **Newline translation on write** — `\n` → target sequence (e.g., `\r\n` for CRLF mode)
- **Accumulation buffer** for lines that span multiple buffer fills
- **Fast path**: when a complete line fits in one peek and no translation is needed, builds `spy_Str` directly from the peek buffer (single `memcpy`)

## Key findings

- The **peek/consume pattern** is the key to efficiency — the text layer reads from the buffered layer's memory directly, avoiding an intermediate copy
- **Universal newline handling** is tricky when `\r\n` can be split across buffer boundaries. The solution: when only `\r` remains in the buffer, consume it, peek for a following `\n`, and resolve
- The **spy_Str flexible array member** (`const char utf8[]`) works well with `gc_malloc` — single allocation for header + payload, and the string is always null-terminated for C interop
- Using libc `read`/`write`/`open`/`close` keeps Layer 0 portable and simple

## Usage

```c
#include "spy_io.h"

// Write a text file
spy_TextIO *f = spy_open_text("output.txt", "w");
spy_Str *line = spy_str_new("Hello, world!\n", 14);
spy_text_write(f, line);
spy_text_close(f);

// Read lines from a text file (universal newlines)
spy_TextIO *f = spy_open_text("input.txt", "r");
spy_Str *line;
while ((line = spy_text_readline(f)) != NULL) {
    printf("%.*s", (int)line->length, line->utf8);
}
spy_text_close(f);

// Manual layer construction (e.g., for sockets)
int sockfd = /* ... */;
spy_RawIO *raw = spy_raw_from_fd(sockfd, /*own_fd=*/1);
spy_BufferedIO *bio = spy_buffered_new(raw, SPY_BUF_RW, 4096);
spy_TextIO *tio = spy_text_new(bio, SPY_NL_LF);
// ... use tio ...
spy_text_close(tio);  // closes all layers

// Binary mode (buffered, no text layer)
spy_BufferedIO *bin = spy_open_binary("data.bin", "rb");
char buf[1024];
ssize_t n = spy_buffered_read(bin, buf, sizeof(buf));
spy_buffered_close(bin);
```

Build and test:

```bash
make test
```
