# I/O Library Abstraction Layers: A Cross-Language Comparison

A deep dive into how various programming languages layer abstractions on top of raw OS syscalls for I/O — covering buffering strategies, text encoding, and API design.

## Motivation

Every language ultimately makes the same syscalls (`read`, `write`, `open`, `close`), but the abstractions they build on top vary enormously. This document compares the I/O stacks of Python 3, Python 2, C, C++, Rust, Go, Zig, C#, and Java — focusing on:

- The layering of abstractions
- Buffering strategies (when and how bytes are collected before writing/reading)
- Text encoding (where bytes become characters)
- API design philosophy

---

## Python 3

**Stack:** syscall → `FileIO` → `BufferedReader/Writer` → `TextIOWrapper`

Python 3 redesigned I/O from scratch with PEP 3116, creating a clean three-tier architecture implemented in C (`_io` module) with a pure-Python fallback (`io` module).

### Layer 1: Raw I/O (`RawIOBase`)

`FileIO` wraps a file descriptor. It calls `read()`/`write()` syscalls directly with no buffering. Every call translates 1:1 to a syscall.

```python
import io
f = io.FileIO('file.txt', 'r')
f.read(10)  # one read() syscall for up to 10 bytes
```

### Layer 2: Buffered I/O (`BufferedIOBase`)

Wraps a `RawIOBase` and adds buffering:

- `BufferedReader` — maintains a read-ahead buffer (default 8 KB). Reads a large chunk from the OS, serves small reads from the buffer.
- `BufferedWriter` — accumulates writes into a buffer, flushes when full or on `flush()`/`close()`.
- `BufferedRandom` — combined read/write for seekable files. Flushes the write buffer before switching to read mode (and vice versa).
- `BytesIO` — in-memory buffer, no file descriptor involved.

```python
f = io.BufferedReader(io.FileIO('file.txt'))
f.read(1)   # fills 8 KB buffer, returns 1 byte; next read is from buffer
```

### Layer 3: Text I/O (`TextIOBase`)

`TextIOWrapper` wraps a `BufferedIOBase` and handles:

- **Encoding/decoding**: converts bytes ↔ `str` using a codec (default: `locale.getpreferredencoding(False)`, typically UTF-8 on modern systems).
- **Newline translation**: by default, normalizes `\r\n`, `\r`, `\n` to `\n` on read; writes `\n` as the platform newline on write (configurable via `newline=` parameter).
- **Incremental decoding**: uses `codecs.IncrementalDecoder` to handle multi-byte sequences split across buffer boundaries.
- `StringIO` is the in-memory text equivalent.

```python
f = io.TextIOWrapper(io.BufferedReader(io.FileIO('file.txt')), encoding='utf-8')
```

### The `open()` Tower

`open()` builds the appropriate stack:

| Mode | Stack |
|------|-------|
| `'rb'` | `BufferedReader(FileIO(...))` |
| `'r'`  | `TextIOWrapper(BufferedReader(FileIO(...)))` |
| `'wb'` | `BufferedWriter(FileIO(...))` |
| `'w'`  | `TextIOWrapper(BufferedWriter(FileIO(...)))` |
| `'rb', buffering=0` | `FileIO(...)` (no buffering) |

Pipes and TTYs get `BufferedReader`/`BufferedWriter` but with line buffering enabled on TTYs when `isatty()` returns true.

### Key design points

- Clean separation of concerns: each layer does exactly one thing.
- All layers share the same `read(n)`, `write(b)`, `seek()`, `tell()` interface where applicable.
- `flush()` propagates through layers.
- Universal newlines as a first-class feature.
- Encoding is explicit (with a warning for default platform encoding since 3.10).

---

## Python 2

**Stack:** `open()` → `file` object (wraps C `FILE*`)

Python 2 predated the redesign. The `file` type is a thin wrapper around the C standard library's `FILE*` struct.

### The `file` type

- `open('file.txt')` returns a `file` object backed by a C `FILE*`.
- Buffering is delegated to the C library (see C section below).
- Text mode on Windows: translates `\r\n` ↔ `\n`. On Unix, text and binary modes are identical.
- **No encoding support in the built-in `file` type.** Reading returns byte strings (`str`). There is no concept of decoding at the file level.
- `u'...'` unicode strings exist but `file.read()` always returns `str` (bytes).

### Encoding workarounds

The `codecs` module provides encoding-aware streams:

```python
import codecs
f = codecs.open('file.txt', 'r', encoding='utf-8')
# returns unicode strings
```

This wraps a regular `file` with a `StreamReaderWriter` that decodes on the fly. It's not part of the main `open()` path.

### The `io` module (backport)

Python 2.6+ includes the `io` module as a preview of Python 3's design. It works the same way (3-tier stack) but is rarely used in Python 2 code.

### Key design points

- Simplicity: one object for all I/O.
- Encoding was an afterthought — the `unicode` / `str` duality was a fundamental source of bugs.
- The Python 3 redesign was largely motivated by fixing this.

---

## C

**Stack:** syscall (`read`/`write`) ← POSIX fd API; `FILE*` ← C standard library

C has two distinct I/O layers that coexist.

### Layer 0: POSIX file descriptors

Raw integer file descriptors. No buffering. Maps directly to kernel syscalls:

```c
int fd = open("file.txt", O_RDONLY);
ssize_t n = read(fd, buf, sizeof(buf));
write(fd, buf, n);
close(fd);
```

- `fd` is just an int (0=stdin, 1=stdout, 2=stderr).
- No buffering; every call is a syscall.
- Available on POSIX systems; Windows has `_open()` etc. as approximations.

### Layer 1: C Standard Library `FILE*`

`FILE*` (declared in `<stdio.h>`) wraps a file descriptor and adds:

- **Buffering**: three modes controlled by `setvbuf()`:
  - `_IOFBF` — fully buffered: flushes when buffer is full (default for regular files). Default buffer size: `BUFSIZ` (typically 4–8 KB, implementation-defined).
  - `_IOLBF` — line buffered: flushes on `\n` (default for TTYs / interactive streams).
  - `_IONBF` — unbuffered (default for `stderr`).
- **Text vs binary mode**: on Windows, text mode translates `\r\n` ↔ `\n`. On POSIX, they're identical.
- **No encoding support**: operates entirely on bytes (`char`). `wchar_t` functions (`fgetwc`, `fputws`, etc.) exist but encoding depends on the locale setting.
- **Bridge functions**: `fileno(FILE*)` extracts the underlying fd; `fdopen(fd, mode)` wraps an fd in a `FILE*`.

```c
FILE *f = fopen("file.txt", "r");
char buf[1024];
fgets(buf, sizeof(buf), f);  // may not call read() at all if data is buffered
fclose(f);  // flushes and closes
```

### Buffering subtleties

- The buffer is part of the `FILE` struct (or heap-allocated on first I/O).
- `fflush(stdout)` is necessary before mixing with `write()` on the same fd.
- Output to stdout is line-buffered when connected to a terminal, fully buffered when redirected — this is why `printf()` output may not appear when piped.
- `setbuf(stdout, NULL)` disables buffering entirely.

### Key design points

- Two separate APIs that can be mixed (carefully).
- Encoding is the application's problem.
- Buffering behavior changes based on whether output is a TTY — surprising but practical.

---

## C++

**Stack:** syscall → C `FILE*` or OS handle → `std::basic_streambuf` → `std::basic_stream`

C++ iostreams (introduced in C++98, refined in C++11) add a two-layer abstraction over the underlying I/O.

### Layer 1: `std::basic_streambuf<CharT, Traits>`

The buffer/device layer. Manages a sequence of `CharT` characters. Key virtual methods:

- `underflow()` / `uflow()` — fill the get area (read buffer)
- `overflow(c)` — flush the put area (write buffer) and optionally write `c`
- `seekpos()` / `seekoff()` — positioning

Concrete implementations:

- `std::basic_filebuf<CharT>` — file-backed; on many implementations wraps `FILE*`, but the standard doesn't mandate this. Can use OS handles directly.
- `std::basic_stringbuf<CharT>` — string-backed.

`filebuf` handles:
- **Buffering**: maintains separate get/put buffers, typically `BUFSIZ` bytes.
- **Encoding/wide characters**: when `CharT = wchar_t`, uses the `codecvt` facet from the stream's locale to convert between wide characters and the external byte representation. `std::locale::classic()` uses basic ASCII/ANSI; setting a UTF-8 locale enables UTF-8 ↔ `wchar_t` conversion.

### Layer 2: `std::basic_ios` → `std::basic_istream` / `std::basic_ostream`

The formatting layer. Does not do buffering itself — delegates to `streambuf`.

- `std::basic_istream<CharT>` — `operator>>` for formatted input, `get()`, `getline()`, `read()`.
- `std::basic_ostream<CharT>` — `operator<<` for formatted output.
- `std::basic_iostream<CharT>` — both.

Concrete file classes:
- `std::ifstream`, `std::ofstream`, `std::fstream` — own a `filebuf`.
- `std::istringstream`, `std::ostringstream`, `std::stringstream` — own a `stringbuf`.

Global objects: `std::cin`, `std::cout`, `std::cerr`, `std::clog`.

### Sync with C stdio

By default, `std::cin`/`std::cout`/etc. are synchronized with `stdin`/`stdout` so they can be interleaved safely. This synchronization can be disabled for performance:

```cpp
std::ios_base::sync_with_stdio(false);
std::cin.tie(nullptr);
```

Disabling sync makes C++ streams faster (no interop overhead) but means you must not mix C and C++ I/O.

### Encoding

- `std::fstream` (char) = bytes, no encoding.
- `std::wfstream` (wchar_t) = wide chars; `codecvt` facet in locale handles byte ↔ wide conversion on the `filebuf` level.
- `codecvt` is deprecated in C++17 (intended for replacement, never actually replaced).
- In practice, most code uses `std::fstream` with UTF-8 bytes and handles encoding at the application level.
- C++20 adds `char8_t` but `std::u8fstream` does not exist; `<cuchar>` provides conversion functions.

### C++23: `std::print` / `std::println`

C++23 adds `std::print`/`std::println` which write directly to `FILE*` (bypassing iostream buffering) using `std::format`-style formatting. This sidesteps the iostream stack entirely for simple output.

### Key design points

- Elegant separation: `streambuf` for buffering/device, `stream` for formatting.
- Locale-based encoding is principled but complex and largely abandoned in practice.
- Performance was historically poor; `sync_with_stdio(false)` is a common optimization.
- The `<<`/`>>` operator API is idiomatic but verbose and error-prone for complex formatting.

---

## Rust

**Stack:** syscall → `OwnedFd` / `File` → `BufReader`/`BufWriter` → UTF-8 string API

Rust's I/O is trait-based and compositional, with no hidden buffering by default.

### Layer 0: Raw file descriptors

`std::os::unix::io::RawFd` is just an `i32`. `OwnedFd` (stabilized 1.63) owns an fd and closes it on drop. `BorrowedFd` borrows one.

### Layer 1: `std::fs::File`

Wraps an OS file handle. Implements `Read`, `Write`, `Seek`.

```rust
use std::fs::File;
use std::io::Read;
let mut f = File::open("file.txt")?;
let mut buf = vec![0u8; 1024];
f.read(&mut buf)?;  // one read() syscall
```

**No buffering.** Each `read()`/`write()` call goes to the OS.

### The `Read`/`Write`/`BufRead` traits

The core I/O traits defined in `std::io`:

- `Read` — `read(&mut [u8]) -> Result<usize>`, `read_to_end`, `read_exact`, etc.
- `Write` — `write(&[u8]) -> Result<usize>`, `write_all`, `flush`.
- `Seek` — `seek(SeekFrom) -> Result<u64>`.
- `BufRead` — extends `Read` with `fill_buf()`, `consume()`, `read_line()`, `lines()`.

Any type implementing `Read` or `Write` can be wrapped with buffering adapters.

### Layer 2: `BufReader<R>` / `BufWriter<W>`

Explicit buffering wrappers. The user opts into buffering.

```rust
use std::io::{BufReader, BufWriter, BufRead};
let f = BufReader::new(File::open("file.txt")?);
for line in f.lines() {  // reads a buffer at a time, serves lines from it
    let line = line?;
    println!("{}", line);
}
```

- `BufReader` — default 8 KB read-ahead buffer. Implements `BufRead`.
- `BufWriter` — default 8 KB write buffer. Flushes on `flush()` or drop (drop flush errors are silently ignored — a known footgun; use `into_inner()` to recover).
- `LineWriter` — flushes on every `\n` (used for stdout when connected to a TTY).

### Text encoding

Rust strings (`String`, `&str`) are **always** valid UTF-8. The I/O layer deals in bytes (`[u8]`).

The bridge:
- `BufRead::lines()` returns `Result<String>` — reads bytes, calls `String::from_utf8()`, returns `Err` on invalid UTF-8.
- `std::io::stdin().read_line(&mut String)` similarly validates UTF-8.
- For non-UTF-8 files: use `read_to_end` for raw bytes, then handle encoding manually or with a crate like `encoding_rs`.

**No TextIOWrapper equivalent in stdlib.** Encoding conversion is the application's responsibility.

### Stdin/Stdout/Stderr

```rust
let stdin = std::io::stdin();   // Stdin
let stdout = std::io::stdout(); // Stdout
let stderr = std::io::stderr(); // Stderr
```

- `Stdin` / `Stdout` / `Stderr` are wrapper types. They are **not** `BufReader`/`BufWriter`.
- `stdout` is **line-buffered** when connected to a TTY, **fully buffered** otherwise (in most Rust standard library implementations via OS behavior).
- `stdin().lock()` returns a `StdinLock` implementing `BufRead` with a mutex held — efficient single-threaded access.
- For high-throughput output: wrap in `BufWriter::new(stdout())`.

### Key design points

- Explicit by default: no hidden buffering surprises.
- Composability: any `Read` + `BufReader` = buffered read. Any `Write` + `BufWriter` = buffered write.
- Compile-time generics: `BufReader<File>` is a monomorphized type, not a dynamic dispatch interface (though `Box<dyn Read>` gives dynamic dispatch when needed).
- Strong UTF-8 guarantee: encoding errors surface at the point of string creation.

---

## Go

**Stack:** syscall → `os.File` → `bufio.Reader`/`bufio.Writer` → UTF-8 strings

Go's I/O is interface-based with small, composable interfaces.

### Layer 1: `os.File`

Wraps a file descriptor (`*os.File`). Implements `io.Reader`, `io.Writer`, `io.Seeker`, `io.Closer`.

```go
f, _ := os.Open("file.txt")
buf := make([]byte, 1024)
n, _ := f.Read(buf)  // one read() syscall
```

**No buffering.** Direct syscall per call.

### The `io` package interfaces

Minimal interfaces:

```go
type Reader interface { Read(p []byte) (n int, err error) }
type Writer interface { Write(p []byte) (n int, err error) }
type Seeker interface { Seek(offset int64, whence int) (int64, error) }
type Closer interface { Close() error }
// Composed:
type ReadWriter interface { Reader; Writer }
type ReadCloser interface { Reader; Closer }
// etc.
```

The `io.EOF` sentinel (not an error, just end-of-stream) is a distinctive Go design choice.

### Layer 2: `bufio` package

```go
r := bufio.NewReader(f)          // 4 KB buffer by default
line, _ := r.ReadString('\n')    // reads from buffer

w := bufio.NewWriter(f)          // 4 KB buffer
fmt.Fprintln(w, "hello")         // writes to buffer
w.Flush()                        // flush to underlying writer
```

- `bufio.Reader` — buffered reader, implements `io.Reader` + additional methods (`ReadString`, `ReadBytes`, `ReadLine`, `ReadRune`).
- `bufio.Writer` — buffered writer.
- `bufio.ReadWriter` — combines both.
- `bufio.Scanner` — line/token scanner with customizable split functions; more ergonomic than `ReadString`.

### Text encoding

Go strings and `[]byte` are both sequences of bytes. Strings happen to be UTF-8 by convention (and the Go spec states source files must be UTF-8), but the runtime doesn't enforce this.

- `string` is an immutable byte sequence; `[]byte` is mutable.
- The `unicode/utf8` package provides `RuneCountInString`, `DecodeRune`, etc.
- `bufio.Reader.ReadRune()` reads a single UTF-8 rune.
- For non-UTF-8 encodings: `golang.org/x/text/encoding` (external module) provides charset converters that wrap `io.Reader`/`io.Writer`.

**No encoding layer in stdlib.** Go assumes UTF-8 everywhere; other encodings require an external package.

### `fmt` package

`fmt.Fprintf(w io.Writer, ...)` writes formatted output to any `io.Writer`. This is the standard pattern for formatted output — not tied to files.

### Stdin/Stdout/Stderr

`os.Stdin`, `os.Stdout`, `os.Stderr` are `*os.File` — **no buffering**. For efficient output:

```go
w := bufio.NewWriter(os.Stdout)
// ... write a lot ...
w.Flush()
```

### Key design points

- Minimal interfaces: `io.Reader` has just one method. Easy to implement, easy to compose.
- `io.Copy(dst Writer, src Reader)` is a universal copy primitive.
- No built-in encoding handling is intentional: Go bets on UTF-8 ubiquity.
- `bufio.Scanner` is notably ergonomic for line-by-line processing.

---

## Zig

**Stack:** syscall → `std.fs.File` → `std.io.BufferedReader`/`BufferedWriter` → UTF-8 by convention

Zig takes a low-level, explicit approach with comptime-based generics replacing runtime dispatch.

### Layer 0: Syscall wrappers

`std.os` (now `std.posix`) exposes direct syscall wrappers:

```zig
const fd = try std.posix.open("file.txt", .{}, 0);
var buf: [1024]u8 = undefined;
const n = try std.posix.read(fd, &buf);
```

### Layer 1: `std.fs.File`

Wraps a file descriptor. Has `read()`, `write()`, `seekTo()`, `getPos()` methods. No buffering.

```zig
const file = try std.fs.cwd().openFile("file.txt", .{});
defer file.close();
var buf: [1024]u8 = undefined;
const n = try file.read(&buf);
```

### The Reader/Writer interface pattern

Zig uses comptime generics, not runtime vtables, for its I/O interfaces. `std.fs.File` provides:

- `file.reader()` — returns a `std.fs.File.Reader` which is a thin `GenericReader` wrapper.
- `file.writer()` — returns a `std.fs.File.Writer`.

`GenericReader(Context, ReadError, readFn)` is a struct type instantiated at comptime. The same for `GenericWriter`. This means:

- Full inlining at compile time — zero overhead.
- But: cannot easily store "a reader" in a struct without knowing its concrete type (unless using `std.io.AnyReader` which does use function pointers).

### Layer 2: Buffered I/O

```zig
var buf_reader = std.io.bufferedReader(file.reader());
const reader = buf_reader.reader();  // now buffered
```

- `std.io.bufferedReader(reader)` — default 4 KB buffer.
- `std.io.bufferedWriter(writer)` — default 4 KB buffer.
- Buffers are stack-allocated (part of the buffered reader struct) — no heap allocation.

### Text encoding

Zig strings are `[]const u8` — byte slices. UTF-8 by convention. The compiler uses UTF-8 for source files.

- `std.unicode` provides UTF-8 utilities: `utf8ValidateSlice`, `Utf8View`, `Utf8Iterator`.
- No automatic encoding conversion anywhere in the stdlib.
- Zig's philosophy: everything explicit, no magic.

### Stdin/Stdout/Stderr

```zig
const stdout = std.io.getStdOut().writer();
try stdout.print("hello {s}\n", .{"world"});
```

`std.io.getStdOut()` returns a `File`. No buffering by default.

For buffered output (important for performance):
```zig
var bw = std.io.bufferedWriter(std.io.getStdOut().writer());
const stdout = bw.writer();
try stdout.print("hello\n", .{});
try bw.flush();
```

### Allocators

Unlike other languages, Zig has no global allocator. Reading into a `String`-like buffer requires passing an allocator:

```zig
const content = try file.readToEndAlloc(allocator, max_size);
defer allocator.free(content);
```

### Key design points

- Comptime generics = zero-cost abstractions with no runtime dispatch overhead.
- Explicit buffering: the language forces you to think about it.
- Stack-allocated buffers: no heap allocation for I/O buffering.
- Maximum control, minimum magic.

---

## C# / .NET

**Stack:** OS handle → `Stream` → `StreamReader`/`StreamWriter` (encoding) → `TextReader`/`TextWriter`

.NET has a clean two-tier I/O model with explicit encoding handling.

### Layer 1: `System.IO.Stream` (byte streams)

Abstract base class. Key methods: `Read(byte[], offset, count)`, `Write(byte[], offset, count)`, `Seek(long, SeekOrigin)`, `Flush()`, `Close()`.

Concrete implementations:
- `FileStream` — wraps a Windows HANDLE or POSIX fd. **Has its own internal buffer** (default 4 KB, configurable). Also supports async I/O via Windows IOCP / POSIX AIO.
- `MemoryStream` — backed by a `byte[]`.
- `BufferedStream` — adds a buffer to an unbuffered stream (rarely needed since `FileStream` already buffers).
- `NetworkStream` — wraps a socket.
- `CryptoStream` — transform pipeline (encrypt/decrypt/hash on the fly).
- `GZipStream` / `DeflateStream` / `BrotliStream` — compression.

Streams compose via wrapping: `new GZipStream(new FileStream(...))`.

### Layer 2: `TextReader` / `TextWriter` (text streams)

Adds encoding. Abstract base classes with `Read()`, `ReadLine()`, `ReadToEnd()` / `Write()`, `WriteLine()`.

- `StreamReader` — wraps a `Stream`. Constructor accepts `Encoding` parameter. Detects BOM by default.
  - Default encoding: UTF-8 with BOM detection (since .NET Core; was `Encoding.Default` = platform ANSI in .NET Framework).
  - Uses an internal `Decoder` (stateful, handles partial multi-byte sequences across buffer boundaries).
  - Read buffer: default 1 KB of chars (plus the byte buffer of the underlying stream).
- `StreamWriter` — wraps a `Stream`. Encodes `string` → bytes using specified `Encoding`.
  - Auto-flush: configurable; default is to buffer until `Flush()` or `Close()`.
- `StringReader` / `StringWriter` — in-memory.

```csharp
using var reader = new StreamReader("file.txt", Encoding.UTF8);
string line;
while ((line = reader.ReadLine()) != null)
    Console.WriteLine(line);
```

### Encoding

`System.Text.Encoding` is a rich hierarchy: `UTF8Encoding`, `UTF32Encoding`, `UnicodeEncoding` (UTF-16), `ASCIIEncoding`, `Latin1Encoding`, and arbitrary code pages via `CodePagesEncodingProvider`.

.NET internal string format is **UTF-16** (like Java). Encoding handles UTF-16 ↔ whatever-the-file-uses.

### `Console` class

`Console.In` / `Console.Out` / `Console.Error` are `TextReader`/`TextWriter` instances.
- `Console.OutputEncoding` sets the encoding for stdout (default: UTF-8 in .NET Core/5+).
- On Windows, also calls `SetConsoleOutputCP` for the console.

### `System.IO.Pipelines` (modern .NET)

Added in .NET Core 2.1. High-performance I/O pipeline for minimizing copies:

- `PipeReader` / `PipeWriter` — zero-copy buffer management.
- The reader gets a `ReadOnlySequence<byte>` (possibly non-contiguous buffers).
- Used internally by ASP.NET Core's Kestrel server.

### Async I/O

`Stream` has `ReadAsync`/`WriteAsync` returning `Task<int>`. `StreamReader` has `ReadLineAsync()`. Full async support via `await`.

### Key design points

- Clean two-tier separation: `Stream` for bytes, `TextReader`/`TextWriter` for text.
- Encoding is first-class and explicit at the text layer.
- `FileStream` has its own buffering; `BufferedStream` is mostly redundant.
- UTF-16 internal strings mean encoding/decoding happens on every file read/write.
- `System.IO.Pipelines` represents the state of the art for high-performance scenarios.

---

## Java

**Stack:** OS fd → `FileInputStream`/`FileOutputStream` → `BufferedInputStream`/`BufferedOutputStream` → `InputStreamReader`/`OutputStreamWriter` → `BufferedReader`/`BufferedWriter`

Java has two I/O frameworks: the original `java.io` (1996) and `java.nio` (2002, with `java.nio.file` in 2011).

### `java.io`: The Original I/O

**Byte streams** (all abstract base classes):
- `InputStream` / `OutputStream` — `read(byte[])` / `write(byte[])`.
- `FileInputStream` / `FileOutputStream` — wraps a file descriptor. **No buffering**.
- `BufferedInputStream` / `BufferedOutputStream` — decorator that adds buffering (default 8 KB). Classic decorator pattern.
- `DataInputStream` / `DataOutputStream` — reads/writes primitive types in binary.
- `ByteArrayInputStream` / `ByteArrayOutputStream` — in-memory.

**Character streams** (added Java 1.1):
- `Reader` / `Writer` — `read(char[])` / `write(char[])`.
- `InputStreamReader` / `OutputStreamWriter` — **the encoding bridge**: wraps a byte stream + `Charset`. Converts bytes ↔ `char`.
- `FileReader` / `FileWriter` — convenience class wrapping `FileInputStream` + `InputStreamReader`. **Uses platform default charset** — considered bad practice; use `new InputStreamReader(new FileInputStream(...), StandardCharsets.UTF_8)` instead.
- `BufferedReader` / `BufferedWriter` — buffering decorators for character streams (default 8 KB chars).
- `PrintWriter` — adds `println()`, `printf()` convenience methods with auto-flush option.

The typical "proper" stack for reading text:
```java
BufferedReader br = new BufferedReader(
    new InputStreamReader(
        new FileInputStream("file.txt"),
        StandardCharsets.UTF_8));
```

Or with `java.nio.file.Files` (preferred modern approach):
```java
BufferedReader br = Files.newBufferedReader(Path.of("file.txt"), StandardCharsets.UTF_8);
```

### `java.nio`: New I/O

Introduced for scalable, non-blocking I/O:

- `Channel` — represents a connection to an I/O device (`FileChannel`, `SocketChannel`).
- `ByteBuffer` — explicit buffer with position/limit/capacity. `FileChannel.read(ByteBuffer)`.
- `MappedByteBuffer` — memory-mapped file.
- `Charset` / `CharsetDecoder` / `CharsetEncoder` — encoding framework (all encodings via `java.nio.charset`).
- **Non-blocking I/O**: `Selector` + non-blocking `SocketChannel` for async network I/O.

`java.nio.file.Files` (Java 7, NIO.2) — utility methods:
```java
List<String> lines = Files.readAllLines(Path.of("file.txt"), StandardCharsets.UTF_8);
byte[] bytes = Files.readAllBytes(Path.of("file.txt"));
```

### Java strings

Java `String` is internally UTF-16 (compact strings in Java 9+ use Latin-1 or UTF-16 depending on content). All encoding/decoding converts between bytes and UTF-16.

### Key design points

- Classic OOP decorator pattern: explicit stacking of `Buffered*` on top of raw streams.
- `FileReader`/`FileWriter` are historical mistakes (platform-default encoding).
- `java.nio` adds explicit buffers (`ByteBuffer`) and non-blocking I/O.
- `java.nio.file.Files` is the modern, concise API.
- The layering is maximally explicit but also maximally verbose.

---

## Summary Comparison

| Language | Raw I/O | Buffering | Text/Encoding Layer | Newline Handling |
|----------|---------|-----------|---------------------|-----------------|
| **Python 3** | `FileIO` (fd) | `BufferedReader/Writer` (opt-out) | `TextIOWrapper` (explicit encoding) | Universal newlines |
| **Python 2** | `file` (FILE*) | C stdlib buffering (auto) | `codecs.open()` (manual) | Platform-dependent |
| **C** | fd (`open/read`) | `FILE*` (auto, mode-based) | None (bytes only) | Text mode on Windows |
| **C++** | fd or FILE* | `filebuf` (in streambuf) | `codecvt` in locale (deprecated) | Locale-dependent |
| **Rust** | `File` (fd) | `BufReader/BufWriter` (explicit opt-in) | None (UTF-8 validation only) | None (bytes) |
| **Go** | `os.File` (fd) | `bufio.Reader/Writer` (explicit opt-in) | None (UTF-8 by convention) | None |
| **Zig** | `File` (fd) | `bufferedReader/Writer` (explicit) | None (UTF-8 by convention) | None |
| **C#** | `FileStream` (handle) | Built into `FileStream` + `BufferedStream` | `StreamReader/Writer` (explicit Encoding) | `TextReader/Writer` |
| **Java** | `FileInputStream` (fd) | `Buffered*` decorators (explicit) | `InputStreamReader/Writer` (explicit Charset) | None |

### Buffering strategies at a glance

- **Automatic and invisible**: C (`FILE*`), Python 3 (via `open()`), C++ (via `fstream`).
- **Explicit opt-in**: Rust (`BufReader`), Go (`bufio`), Zig (`bufferedReader`), Java (`Buffered*`).
- **Built-in at the file level**: C# (`FileStream` has internal buffer by default).

### Where encoding lives

- **Integrated with open()**: Python 3 (`TextIOWrapper`, encoding parameter).
- **Separate decorator**: Java (`InputStreamReader`), C# (`StreamReader`).
- **External/manual**: C, C++, Rust, Go, Zig.
- **Convention-only (UTF-8)**: Go, Zig, Rust (with validation).

### Key design tensions

1. **Explicit vs implicit buffering**: Explicit (Rust, Go, Zig) avoids surprises but requires boilerplate. Implicit (C's `FILE*`, Python's `open()`) is convenient but can surprise when output doesn't appear.

2. **Encoding at the right layer**: Python 3 puts it in the I/O stack; Rust puts it at the `String` type level. Both work, but Python catches encoding errors at read time while Rust's `from_utf8` can be called late.

3. **Internal string format**: Java and C# use UTF-16 internally, creating overhead on every text I/O. Rust and Go's UTF-8 strings match what files typically contain, reducing conversion work.

4. **TTY detection and auto-buffering**: C and Python adjust buffering based on `isatty()`. Rust partially delegates this to the OS. This is why `printf("loading...")` may not appear immediately when stdout is a pipe.

---

## Further Reading

- [PEP 3116 – New I/O](https://peps.python.org/pep-3116/) — Python 3 I/O redesign rationale
- [Python `io` module docs](https://docs.python.org/3/library/io.html)
- [The GNU C Library: I/O on Streams](https://www.gnu.org/software/libc/manual/html_node/I_002fO-on-Streams.html)
- [C++ `<fstream>` reference](https://en.cppreference.com/w/cpp/io)
- [Rust `std::io` docs](https://doc.rust-lang.org/std/io/)
- [Go `bufio` package](https://pkg.go.dev/bufio)
- [Zig `std.io` source](https://github.com/ziglang/zig/blob/master/lib/std/io.zig)
- [.NET `System.IO.Pipelines`](https://learn.microsoft.com/en-us/dotnet/standard/io/pipelines)
- [Java NIO tutorial](https://docs.oracle.com/javase/tutorial/essential/io/fileio.html)
