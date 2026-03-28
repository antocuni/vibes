# I/O Abstraction Layers Across Programming Languages

A comparative analysis of how programming languages build standard I/O libraries
on top of raw operating system syscalls. Covers abstraction layers, buffering
strategies, text encoding, and standard stream handling.

## Table of Contents

- [The Fundamental Problem](#the-fundamental-problem)
- [Python 3](#python-3)
- [Python 2](#python-2)
- [C (stdio)](#c-stdio)
- [C++ (iostream)](#c-iostream)
- [Rust](#rust)
- [Go](#go)
- [Zig](#zig)
- [C# (.NET)](#c-net)
- [Java](#java)
- [Cross-Language Comparison](#cross-language-comparison)

---

## The Fundamental Problem

At the lowest level, all I/O goes through operating system syscalls: `read(fd, buf, count)`,
`write(fd, buf, count)`, `open(path, flags)`, `close(fd)`, `lseek(fd, offset, whence)`.
These operate on **file descriptors** (small integers) and **raw bytes**.

Each syscall involves a context switch from user mode to kernel mode, costing roughly
100--1000ns. Writing one byte at a time via raw `write()` calls would be catastrophically
slow. Beyond performance, applications need:

- **Buffering** to amortize syscall overhead
- **Text encoding** to convert between bytes and characters
- **Newline handling** to abstract platform differences
- **Error handling** appropriate to the language's idioms
- **Thread safety** for concurrent access

Every language solves these problems differently. The core architectural question is:
**how many layers, and how explicit?**

```
Typical I/O Stack (conceptual):

    Application Code
         |
    [ Text encoding / newline translation ]    (not all languages)
         |
    [ Buffering ]                               (user-space)
         |
    [ Raw fd / handle wrapper ]
         |
    ---- syscall boundary ----
         |
    [ Kernel page cache ]                       (kernel-space)
         |
    [ Disk / device ]
```

---

## Python 3

Python 3's I/O system (PEP 3116) is a clean layered architecture with full control
over buffering and encoding. It **bypasses C stdio entirely**, operating directly on
POSIX file descriptors.

### Architecture: The I/O Tower

```
              open("file.txt", "r")
                      |
              TextIOWrapper          str in/out, encoding, newline translation
                      |
              BufferedReader          bytes, block buffering (8 KiB default)
                      |
              FileIO                  bytes, one syscall per call, no buffering
                      |
              ---- kernel ----
              read(2) / write(2)
```

The `open()` function builds this stack automatically based on mode and buffering
arguments. The layers are implemented in C (`Modules/_io/`) for performance, with
a Python wrapper (`Lib/io.py`) that adds the ABC hierarchy.

### Layer Details

**FileIO** (`_io.FileIO`) -- Raw Layer
- Thin wrapper around a POSIX file descriptor
- Every `read(n)` makes exactly one `read(2)` syscall (short reads possible)
- Every `write(data)` makes one `write(2)` syscall
- GIL released during blocking calls (`Py_BEGIN_ALLOW_THREADS`)
- `EINTR` retried automatically (PEP 475, Python 3.5+)

**BufferedReader / BufferedWriter / BufferedRandom** -- Buffer Layer
- Default buffer size: `max(st_blksize, 8192)` bytes (typically 8 KiB)
- `BufferedReader` pre-fetches data; `read(n)` served from buffer when possible
- `BufferedWriter` accumulates writes; flushes when buffer full, on `flush()`, or `close()`
- `BufferedRandom` for read-write streams (`r+b`); flushes when switching direction
- Thread-safe via internal lock

**TextIOWrapper** -- Text Layer
- Encodes on write (`str` -> `bytes`), decodes on read (`bytes` -> `str`)
- Uses `codecs.IncrementalEncoder`/`IncrementalDecoder` for streaming codec support
- Handles multi-byte sequences split across buffer boundaries
- Newline translation (universal newlines by default)
- Complex snapshot mechanism for `tell()`/`seek()` with multi-byte encodings

### How open() Builds the Tower

| Mode | Buffering | Resulting Stack |
|------|-----------|-----------------|
| `'rb'`, `buffering=0` | unbuffered | `FileIO` alone |
| `'rb'` (default) | block | `BufferedReader` -> `FileIO` |
| `'wb'` (default) | block | `BufferedWriter` -> `FileIO` |
| `'r+b'` (default) | block | `BufferedRandom` -> `FileIO` |
| `'r'` (default) | block | `TextIOWrapper` -> `BufferedReader` -> `FileIO` |
| `'w'` (default) | block | `TextIOWrapper` -> `BufferedWriter` -> `FileIO` |
| `'r'`, `buffering=1` | line | `TextIOWrapper(line_buffering=True)` -> `BufferedWriter` -> `FileIO` |

Constraints:
- `buffering=0` only valid in binary mode (text mode raises `ValueError`)
- `buffering=1` (line buffering) only meaningful in text mode

### Buffering Strategies

| `buffering=` | Binary mode | Text mode |
|--------------|-------------|-----------|
| `0` | No buffering (raw `FileIO`) | **Invalid** |
| `1` | Deprecated (uses default + warning) | Line buffering (flush on `\n`) |
| `N > 1` | Buffer of N bytes | Buffer of N bytes in underlying buffer layer |
| `-1` (default) | Block buffering, `max(st_blksize, 8192)` | Same |

Flushing triggers: buffer full, explicit `flush()`, `close()`, `seek()`, garbage collection.

### Text Encoding

Default encoding selection:
1. Explicit `encoding=` parameter: use it
2. UTF-8 mode (`PYTHONUTF8=1`, `-X utf8`, or Python 3.15+ default): `'utf-8'`
3. Otherwise: `locale.getencoding()` (e.g., `nl_langinfo(CODESET)`)

Newline translation (`newline=` parameter):

| Value | On read | On write |
|-------|---------|----------|
| `None` (default) | `\r\n`, `\r`, `\n` all become `\n` | `\n` -> `os.linesep` |
| `''` | All endings preserved | No translation |
| `'\n'` | Only `\n` is line ending | No translation |

### Standard Streams

| Stream | Type | Buffering | Notes |
|--------|------|-----------|-------|
| `sys.stdin` | `TextIOWrapper` -> `BufferedReader` -> `FileIO(0)` | Block (TTY: line) | |
| `sys.stdout` | `TextIOWrapper` -> `BufferedWriter` -> `FileIO(1)` | Block (TTY: line) | |
| `sys.stderr` | `TextIOWrapper` -> `BufferedWriter` -> `FileIO(2)` | Always line-buffered | `errors='backslashreplace'` |

`PYTHONUNBUFFERED` / `-u`: removes buffer layers entirely; `TextIOWrapper` wraps `FileIO`
directly with `write_through=True`.

Access underlying layers: `sys.stdout.buffer` (BufferedWriter), `sys.stdout.buffer.raw` (FileIO).

### Relationship to C stdio

Python 3 **does not use C stdio** (`fopen`/`fread`/`fwrite`). `FileIO` calls `read(2)`/`write(2)`
directly from C code. All buffering is Python-managed. This eliminates double-buffering and
gives consistent cross-platform behavior. However, if C extensions use `printf()`, their
C stdio buffers are independent of Python's buffers, causing potential interleaving.

---

## Python 2

Python 2's I/O was fundamentally different: a thin wrapper around C stdio's `FILE*`.

### Architecture

```
              open("file.txt", "r")
                      |
              file object              wraps C FILE*
                      |
              C stdio (FILE*)          fread/fwrite/fseek, C-managed buffering
                      |
              ---- kernel ----
              read(2) / write(2)
```

### The file Type

The built-in `file` type (in `Objects/fileobject.c`) held a `FILE*` pointer. Operations
mapped directly to C stdio:

| Python | C stdio |
|--------|---------|
| `open(name, mode)` | `fopen()` |
| `f.read(n)` | `fread()` |
| `f.write(data)` | `fwrite()` |
| `f.seek()` / `f.tell()` | `fseek()` / `ftell()` |
| `f.flush()` | `fflush()` |
| `f.close()` | `fclose()` |

Buffering was controlled by `setvbuf()` on the underlying `FILE*`.

### Text vs Binary Mode

- On **Unix**: no difference (POSIX mandates they're identical)
- On **Windows**: text mode translates `\r\n` <-> `\n`; Ctrl+Z treated as EOF
- **No encoding/decoding** in either mode. Both returned `str` (which was bytes)

### The str/unicode Problem

`str` was bytes, `unicode` was characters. File I/O always dealt in `str` (bytes):

```python
f = open("test.txt", "r")
data = f.read()      # Always str (bytes), never unicode
```

Writing `unicode` to a default file attempted implicit ASCII encoding:
```python
f.write(u"cafe")     # OK (ASCII-compatible)
f.write(u"caf\xe9")  # UnicodeEncodeError!
```

### Unicode I/O Options

- **`codecs.open()`** (Python 2.0+): Wrapped `file` with codec StreamReader/StreamWriter.
  Had issues with newline translation interfering with multi-byte encodings on Windows.
- **`io.open()`** (Python 2.6+): Backport of Python 3's I/O system. Proper layered
  architecture, correct encoding handling. Opt-in only -- `open()` still returned old `file`.

### Standard Streams

`sys.stdin`/`stdout`/`stderr` were `file` objects wrapping C's `stdin`/`stdout`/`stderr`.
The `.encoding` attribute was advisory -- used by `print` for implicit encoding of `unicode`
objects, but `write()` still dealt in bytes.

The infamous pipe problem: `sys.stdout.encoding` became `None` when piped, causing
`UnicodeEncodeError` for any non-ASCII unicode output. Fix: `PYTHONIOENCODING=utf-8`.

### Key Differences from Python 3

| Aspect | Python 2 | Python 3 |
|--------|----------|----------|
| `open()` returns | `file` (C stdio wrapper) | `io.*` (Python I/O stack) |
| Underlying I/O | C stdio `FILE*` | OS syscalls directly |
| Text mode returns | `str` (bytes) | `str` (unicode) |
| Encoding in text mode | None | Required (defaults to locale/UTF-8) |
| Buffering control | Delegated to C stdio | Python-managed |

---

## C (stdio)

The original user-space I/O abstraction, dating to 1970s Unix. Still the foundation
that many other languages build on or deliberately bypass.

### Architecture

```
              fprintf(fp, "hello %d", 42)
                      |
              FILE* (stdio)           user-space buffer + formatting
                      |
              ---- kernel ----
              write(2)
                      |
              kernel page cache       kernel-space buffer
                      |
              disk / device
```

### The FILE* Abstraction

`FILE` is an opaque struct wrapping a file descriptor with:
- **Buffer** (pointer, size, current position within buffer)
- **File descriptor** (the underlying OS fd)
- **Mode flags** (read/write/append, buffering mode, error/EOF indicators)
- **Lock** (for thread safety, POSIX requires stdio to be thread-safe)
- **Wide-character state** (mbstate_t for multibyte conversion)

Typical sizes: glibc ~200+ bytes, musl ~100-150 bytes, BSD ~150 bytes.

### Buffering Modes

| Mode | Constant | Behavior | Default for |
|------|----------|----------|-------------|
| Fully buffered | `_IOFBF` | Flush when buffer full | Files, stdout when piped |
| Line buffered | `_IOLBF` | Flush on `\n` | stdout/stdin when TTY |
| Unbuffered | `_IONBF` | Every write -> syscall | stderr (always) |

Default buffer sizes (`BUFSIZ`): glibc 8192, musl 1024, BSD 1024, MSVC 512.
For files, implementations may use `st_blksize` from `fstat()` instead.

Control via `setvbuf(fp, buf, mode, size)` -- must be called before first I/O.

### When Flushing Happens

1. Buffer full
2. Newline written (line-buffered only)
3. Explicit `fflush(fp)` (or `fflush(NULL)` for all streams)
4. `fclose(fp)`
5. Seeking (`fseek`, `rewind`)
6. Normal program exit (`exit()` -- but NOT `_exit()`)
7. Input on line-buffered stream flushes line-buffered output streams
8. **NOT on `fork()`** -- buffers are duplicated, causing double output (classic bug)

### Text vs Binary Mode

- **POSIX**: identical (the `"b"` flag is accepted but ignored)
- **Windows**: text mode translates `\r\n` <-> `\n` and treats Ctrl+Z as EOF

### Text Encoding

Traditional C stdio is **encoding-agnostic**. Functions operate on bytes (`char`).
`fgetc()` returns one byte, not one character.

C95 added wide-character functions (`fwprintf`, `fgetwc`, etc.) using `wchar_t` and
the `LC_CTYPE` locale for implicit multibyte-to-wide conversion. Each stream has an
**orientation** (byte or wide) set by the first I/O operation; mixing is undefined behavior.

In practice, wide stdio is rarely used. Modern programs use byte-oriented stdio with
UTF-8 data, treating strings as opaque byte sequences.

### The Double-Buffering Stack

```
fprintf()  -->  [stdio buffer]  -->  write()  -->  [kernel page cache]  -->  [disk cache]  -->  disk
```

Three potential buffer layers. After `fflush()`, data is in the kernel page cache but
NOT on disk. Need `fsync(fileno(fp))` for durability.

### Key Pitfalls

- **stdout buffering changes when piped**: line-buffered with TTY, fully-buffered when piped.
  Use `setvbuf()` or the `stdbuf` utility to override.
- **Mixing FILE* and raw fd**: `fprintf()` + `write(fileno(fp), ...)` without `fflush()`
  causes interleaving. Only use one interface per fd.
- **`fork()` without `fflush()`**: duplicates unflushed buffers, producing double output.
- **Update mode switching**: in `"r+"` mode, output cannot directly follow input (or vice versa)
  without an intervening flush/seek. Violating this is undefined behavior.

### Implementation Variations

| Feature | glibc | musl | BSD |
|---------|-------|------|-----|
| `BUFSIZ` | 8192 | 1024 | 1024 |
| Custom streams | `fopencookie()` | No | `funopen()` |
| vtable design | Yes (jump tables) | No (direct dispatch) | Function pointers |
| Thread locking | Recursive mutex | Futex-based | Mutex |

---

## C++ (iostream)

C++ iostream separates **transport/buffering** (streambuf) from **formatting** (stream).
The design is elegant but carries significant performance overhead.

### Architecture

```
              cout << "value: " << 42 << endl;
                      |
              ostream                 formatting, state (good/bad/fail/eof), sentry
                      |
              streambuf               buffering, transport to device
                      |
              (typically FILE*)       glibc/libc++ wrap FILE* internally
                      |
              ---- kernel ----
```

### The streambuf / stream Separation

**streambuf** manages two buffer areas:
- Get area: `[eback, gptr, egptr)` -- for reading
- Put area: `[pbase, pptr, epptr)` -- for writing

Derived classes override virtual functions: `underflow()` (refill get area), `overflow()`
(flush put area), `sync()` (synchronize with device).

**stream** handles formatting and state. Creates a `sentry` object for each `<<`/`>>`
operation that checks state, skips whitespace (input), and flushes tied streams.

### Key Types

| Layer | Input | Output | Bidirectional |
|-------|-------|--------|---------------|
| File streams | `ifstream` | `ofstream` | `fstream` |
| String streams | `istringstream` | `ostringstream` | `stringstream` |
| Base streams | `istream` | `ostream` | `iostream` |
| Buffers | `filebuf` | `filebuf` | `filebuf` |
| | `stringbuf` | `stringbuf` | `stringbuf` |

### Buffering

`basic_filebuf` typically wraps a `FILE*` internally (in libstdc++, libc++, and MSVC).
Implementations avoid double-buffering by either setting the `FILE*` to unbuffered mode
or using the fd directly.

Default buffer sizes: libstdc++ 8192, libc++ 4096, MSVC 4096.

### Text Encoding (Locales)

Encoding conversion is handled by `codecvt` locale facets:
- `codecvt<char, char, mbstate_t>` -- identity (no-op), the default
- `codecvt<wchar_t, char, mbstate_t>` -- multibyte <-> wide, using LC_CTYPE

Every `operator<<`/`>>` for numeric types dispatches through `num_put`/`num_get` locale
facets, adding overhead even for the "C" locale.

### sync_with_stdio

When `true` (default): C++ streams delegate to C stdio, ensuring intermixed `cout`/`printf`
output is correctly ordered. C++ side effectively unbuffered (each operation goes through `FILE*`).

When `false`: C++ streams use independent buffers. **2x--10x faster** for I/O-heavy programs.
Mixing C and C++ I/O becomes unsafe.

```cpp
std::ios_base::sync_with_stdio(false);  // the competitive programmer's first line
std::cin.tie(nullptr);                   // also untie cin from cout
```

### Standard Streams

| Stream | Wraps | Buffering | Tied to |
|--------|-------|-----------|---------|
| `cin` | `stdin` | Buffered (line if TTY) | `cout` |
| `cout` | `stdout` | Buffered (line if TTY) | -- |
| `cerr` | `stderr` | **Unbuffered** (`unitbuf` set) | `cout` |
| `clog` | `stderr` | Buffered | -- |

### Why iostreams Are Slow

1. **sync_with_stdio** -- forwarding to `FILE*` kills buffering
2. **Locale facets** -- virtual dispatch for all numeric formatting
3. **Virtual functions** in streambuf -- prevents inlining
4. **Sentry objects** -- created/destroyed for every `<<`/`>>` operation
5. **Formatting flexibility** -- width/fill/precision checked every time

### C++20/23: The New Direction

- **`std::format`** (C++20): Python-style formatting. No locale overhead by default.
  Returns `std::string`. Does not use iostream.
- **`std::print`** (C++23): Direct formatted output to `FILE*`, bypassing iostream entirely.
  Unicode-aware (transcodes to UTF-16 for Windows consoles). Performance competitive with
  `fprintf`.
- The committee has effectively acknowledged iostream formatting is a dead end; streambuf
  remains useful for custom transports.

---

## Rust

Rust makes every layer of the I/O stack **explicit and composable**. There is no hidden
buffering, no hidden encoding, no hidden newline translation.

### Architecture

```
              BufReader::new(File::open("f.txt")?)
                      |
              BufReader<File>         read buffering (8 KiB default)
                      |
              File                    thin OwnedFd wrapper, no buffering
                      |
              libc::read()            FFI to C library
                      |
              ---- kernel ----
```

There is **no text encoding layer** in std. The user builds exactly the stack they need.

### Core Traits

| Trait | Key method | Purpose |
|-------|------------|---------|
| `Read` | `read(&mut self, buf: &mut [u8]) -> Result<usize>` | Read bytes (short reads allowed) |
| `Write` | `write(&mut self, buf: &[u8]) -> Result<usize>` | Write bytes (short writes allowed) |
| `Seek` | `seek(&mut self, pos: SeekFrom) -> Result<u64>` | Reposition |
| `BufRead` | `fill_buf(&mut self) -> Result<&[u8]>` | Buffered reading with zero-copy peek |

These compose via generic bounds: `fn process<R: BufRead + Seek>(r: &mut R)`.

`BufRead` enables efficient line-by-line reading:
- `fill_buf()` + `consume()`: zero-copy two-phase read pattern
- `lines()`: iterator of `Result<String>`, the standard line-reading idiom

### File

`std::fs::File` wraps an `OwnedFd` (closes on drop). Implements `Read + Write + Seek`.
**No built-in buffering** -- every `read()`/`write()` is a syscall. `Read` and `Write`
are also implemented for `&File` (shared reference), enabling concurrent access.

### Buffering

| Type | Default size | Behavior |
|------|-------------|----------|
| `BufReader<R>` | 8 KiB | Read buffering; implements `BufRead` |
| `BufWriter<W>` | 8 KiB | Write buffering; flushes on full/flush/drop |
| `LineWriter<W>` | 8 KiB | Flushes on `\n` |

All configurable via `with_capacity()`. The user explicitly constructs the stack:

```rust
let reader = BufReader::new(File::open("data.txt")?);          // buffered read
let writer = BufWriter::new(File::create("out.txt")?);         // buffered write
let big = BufReader::with_capacity(64 * 1024, File::open("big.txt")?);  // 64K buffer
```

`BufWriter` flushes on drop, but **errors during drop-flush are silently ignored**.
Best practice: call `flush()` explicitly.

### Text Encoding

Rust's approach is radically different from Python/Java/C#:
- **`String`/`&str` are always valid UTF-8** (type-level invariant)
- File I/O operates on **raw bytes** (`&[u8]`)
- `read_to_string()` reads bytes and validates UTF-8; errors if invalid
- **No automatic encoding/decoding layer** in std
- `OsStr`/`OsString` for OS-native strings (arbitrary bytes on Unix)

For non-UTF-8 encodings, use the `encoding_rs` crate (used by Firefox). There is no
built-in equivalent to Python's `open(file, encoding="shift_jis")`.

### Standard Streams

| Stream | Buffering | Thread safety | Notes |
|--------|-----------|---------------|-------|
| `stdin()` | Buffered (`BufReader` internally) | Mutex | Implements `BufRead`; `.lock()` for fast access |
| `stdout()` | Line-buffered (TTY) / block (pipe) | Reentrant mutex | `LineWriter` internally |
| `stderr()` | **Unbuffered** | Reentrant mutex | Writes go directly to fd |

The `lock()` pattern avoids per-call mutex overhead:
```rust
let stdout = std::io::stdout();
let mut locked = stdout.lock();  // lock once
for item in data {
    writeln!(locked, "{}", item)?;  // no per-call locking
}
```

### Error Handling

`std::io::Result<T> = Result<T, std::io::Error>`. No exceptions. The `?` operator
propagates errors. Unused `Result` values produce compiler warnings -- you cannot
accidentally ignore an I/O error.

---

## Go

Go follows a similar "explicit composition" philosophy to Rust, but with Go's signature
simplicity and interface-based polymorphism.

### Architecture

```
              bufio.NewReader(file)
                      |
              bufio.Reader            read buffering (4 KiB default)
                      |
              *os.File                fd wrapper + poll integration
                      |
              poll.FD                 non-blocking fd + goroutine parking
                      |
              syscall.Read()
                      |
              ---- kernel ----
```

### Core Interfaces

Go's I/O composability comes from tiny interfaces in the `io` package:

```go
type Reader interface { Read(p []byte) (n int, err error) }
type Writer interface { Write(p []byte) (n int, err error) }
type Closer interface { Close() error }
type Seeker interface { Seek(offset int64, whence int) (int64, error) }
```

These combine into aggregate interfaces: `ReadWriter`, `ReadCloser`, `ReadWriteCloser`,
`ReadWriteSeeker`, etc. Additional optimization interfaces:
- `ReaderFrom` / `WriterTo` -- enable `sendfile(2)` zero-copy paths
- `ReaderAt` -- concurrent-safe positional reads (maps to `pread(2)`)

Philosophy: "accept interfaces, return structs." Structural (implicit) interface
satisfaction -- no `implements` keyword.

### Buffering

`os.File` has **no buffering**. Explicit wrapping via `bufio`:

- **`bufio.Reader`**: Default 4096 bytes. Adds `ReadLine`, `ReadString`, `Peek`, etc.
- **`bufio.Writer`**: Default 4096 bytes. Must call `Flush()` explicitly.
- **`bufio.Scanner`**: Higher-level line/token reader. Default max token 64 KiB. Idiomatic for line-by-line reading.

### Text Encoding

Go strings are UTF-8 by convention. `range` over a string iterates runes (Unicode code
points). Standard library provides `unicode/utf8` for low-level operations.

For other encodings: `golang.org/x/text/encoding` and `golang.org/x/text/transform`:
```go
reader := transform.NewReader(file, japanese.ShiftJIS.NewDecoder())
```

No automatic encoding detection or conversion in std.

### Standard Streams

`os.Stdin`, `os.Stdout`, `os.Stderr` are `*os.File` values. **No buffering** on any of them.
Every `fmt.Println("hello")` results in a `write(1, "hello\n", 6)` syscall (after formatting
into an internal `fmt` buffer).

For buffered stdout: `w := bufio.NewWriter(os.Stdout); ...; w.Flush()`.

### Composability Utilities

The `io` package provides Unix-pipe-like composition:
- `io.Copy(dst, src)` -- the workhorse; uses `sendfile` when possible
- `io.TeeReader(r, w)` -- like Unix `tee`
- `io.MultiReader(...)` -- concatenates readers (like `cat`)
- `io.MultiWriter(...)` -- writes to all (like `tee` to multiple)
- `io.LimitReader(r, n)` -- reads at most n bytes
- `io.Pipe()` -- synchronous in-memory pipe

---

## Zig

Zig takes "no hidden allocations, no hidden control flow" to its logical conclusion
in I/O design.

### Architecture

```
              std.io.bufferedReader(file.reader())
                      |
              BufferedReader           comptime-sized buffer (in struct, stack-allocated)
                      |
              std.fs.File              thin fd wrapper
                      |
              std.posix.read()         direct syscall or libc (configurable)
                      |
              ---- kernel ----
```

### Comptime Interfaces

Zig uses **comptime duck typing** rather than runtime vtables:
- `std.io.GenericReader(Context, ReadError, readFn)` -- monomorphized at compile time
- `std.io.AnyReader` / `std.io.AnyWriter` -- opt-in runtime polymorphism (manual vtable)

Zero runtime overhead for the common case. Each instantiation is a direct function call.

### Buffering

- **`std.io.bufferedReader(comptime buffer_size)`**: Returns a type with the buffer embedded
  in the struct (stack-allocated). **No heap allocation** for the buffer.
- **`std.io.bufferedWriter(comptime buffer_size)`**: Same for writes.
- Default buffer size: 4096 bytes.

The buffer size being a comptime parameter is a distinctive Zig feature -- it enables
the buffer to live in the struct itself, avoiding any dynamic allocation.

### Text Encoding

Zig strings are `[]const u8` (byte slices). UTF-8 by convention but not enforced by the
type system. `std.unicode.utf8` provides low-level UTF-8 operations. No encoding conversion
in the standard library.

### Output Paths

- **`std.debug.print()`**: Writes to stderr, non-failable (errors ignored). For debugging.
- **`std.io.getStdOut().writer()`**: Proper writer for stdout. Errors must be handled.

### Philosophy

Every allocation is explicit (requires an `Allocator` parameter). Buffer sizes are
compile-time constants. Error handling is via error unions (`!`). No hidden buffering,
no hidden encoding, no hidden anything.

---

## C# (.NET)

C# uses a traditional class hierarchy but with modern additions like async I/O,
Span-based zero-copy, and the Pipelines API.

### Architecture

```
              new StreamReader(new FileStream("f.txt"))
                      |
              StreamReader             TextReader: decodes bytes -> chars (UTF-8 default)
                      |
              FileStream               Stream: byte I/O + internal buffer (4 KiB)
                      |
              OS file handle           P/Invoke to kernel
                      |
              ---- kernel ----
```

### Stream Hierarchy

`System.IO.Stream` (abstract base) with key implementations:
- **`FileStream`**: Has internal buffer (default 4096 bytes). Supports async via IOCP (Windows)
  or thread pool (Linux).
- **`MemoryStream`**: In-memory, backed by `byte[]`.
- **`BufferedStream`**: Generic buffer wrapper (rarely needed since FileStream already buffers).
- **`GZipStream`**, **`CryptoStream`**, etc.: Decorator streams.

### Text Layer

**`StreamReader`** / **`StreamWriter`** bridge bytes and characters:
- Wraps a `Stream`, handles encoding/decoding
- Default encoding: UTF-8 without BOM (.NET Core+)
- Has own internal buffer (default 1024 bytes) separate from FileStream's buffer
- Supports BOM detection on read

### Text Encoding

.NET strings are UTF-16 internally. `System.Text.Encoding` hierarchy:
- `Encoding.UTF8`, `Encoding.Unicode` (UTF-16LE), `Encoding.ASCII`, etc.
- `Encoding.GetEncoding(name)` for arbitrary encodings
- Stateful `Decoder`/`Encoder` for streaming with proper handling of partial sequences

### Standard Streams

- `Console.Out` / `Console.Error`: `TextWriter` instances, buffered, UTF-8 encoding (.NET Core)
- `Console.In`: `TextReader`

### Modern Additions

- **Span-based I/O** (.NET Core 2.1+): `Stream.Read(Span<byte>)` for zero-copy to stack memory
- **System.IO.Pipelines**: High-throughput pull-based I/O with backpressure. Uses
  `ReadOnlySequence<byte>` (linked segments) for zero-copy. Powers ASP.NET Core's Kestrel.
- **Async I/O**: `ReadAsync(Memory<byte>, CancellationToken)` returns `ValueTask<int>`
  (allocation-free for synchronous completion).

---

## Java

Java's classic I/O is the textbook **Decorator pattern** -- verbose but flexible.
Modern Java (NIO, NIO.2) adds buffer-oriented and channel-based alternatives.

### Architecture: Classic I/O Tower

```
              new BufferedReader(new InputStreamReader(new FileInputStream("f.txt"), UTF_8))
                      |
              BufferedReader           char buffer (8192 chars) + readLine()
                      |
              InputStreamReader        byte -> char bridge (CharsetDecoder)
                      |
              FileInputStream          raw byte I/O (no buffering, one syscall per read)
                      |
              ---- kernel ----
```

### Byte Streams (Java 1.0)

`InputStream` / `OutputStream` hierarchy:
- `FileInputStream` / `FileOutputStream` -- raw file I/O, no buffering
- `BufferedInputStream` / `BufferedOutputStream` -- default 8192 byte buffer
- `ByteArrayInputStream` / `ByteArrayOutputStream` -- in-memory
- `DataInputStream` / `DataOutputStream` -- Java primitives in big-endian binary

### Character Streams (Java 1.1)

`Reader` / `Writer` hierarchy:
- **`InputStreamReader`**: Bridge from bytes to chars. Takes `InputStream` + `Charset`.
  Uses `CharsetDecoder` internally.
- **`OutputStreamWriter`**: Reverse bridge. Takes `OutputStream` + `Charset`.
- **`BufferedReader`**: Default 8192 char buffer. Adds `readLine()` and `lines()` (Stream<String>).
- **`BufferedWriter`**: Default 8192 char buffer.
- **`FileReader`** / **`FileWriter`**: Convenience wrappers. Before Java 11, always used
  platform default charset. Java 11 added `Charset` constructors.

### Text Encoding

`java.nio.charset.Charset` with `CharsetDecoder`/`CharsetEncoder` for streaming:
- `StandardCharsets.UTF_8`, `.UTF_16`, `.US_ASCII`, `.ISO_8859_1`
- Default charset was **platform-dependent** until **Java 18 (JEP 400)** made UTF-8
  the default everywhere. This was a notorious source of "works on my machine" bugs.

Java strings are internally UTF-16 (or Latin-1 for pure-Latin-1 content, since Java 9's
compact strings -- transparent optimization).

### Standard Streams

- **`System.in`**: Raw `InputStream`, no buffering
- **`System.out`**: `PrintStream` -- auto-flushes on `println()`/`printf()` but not `print()`
- **`System.err`**: `PrintStream` -- auto-flushes on `println()`/`printf()`

### NIO (Java 1.4)

Buffer-oriented, channel-based I/O:
- **`ByteBuffer`**: Fixed-size buffer with position/limit/capacity. `allocateDirect()` for
  off-heap memory (avoids copy in I/O).
- **`FileChannel`**: `transferTo()` for zero-copy sendfile(2). `map()` for memory-mapped I/O.
  Positional reads (`read(buf, position)`) are thread-safe.
- **`Selector`**: Multiplexed non-blocking I/O (epoll/kqueue/select).

### NIO.2 (Java 7)

Modern file API:
- **`Path`** / **`Files`**: `Files.newBufferedReader(path, charset)` eliminates the decorator
  boilerplate. `Files.readString(path)` (Java 11+). `Files.lines(path)` for lazy `Stream<String>`.
- `Files` methods default to UTF-8 (always have, since Java 7).
- Custom `FileSystem` providers (e.g., ZIP as a filesystem).

---

## Cross-Language Comparison

### Abstraction Style

| Language | Approach | Runtime cost |
|----------|----------|-------------|
| C | Opaque struct (`FILE*`) with function API | Low (direct C calls) |
| C++ | Class hierarchy with virtual dispatch | Medium-high (vtables, locale facets) |
| Python 3 | Layered objects, ABCs | Medium (C-implemented layers) |
| Python 2 | Thin C stdio wrapper | Low (delegates to C) |
| Rust | Traits + explicit composition | Near-zero (monomorphized generics) |
| Go | Interfaces + explicit composition | Low (interface dispatch) |
| Zig | Comptime generics + explicit composition | Near-zero (monomorphized) |
| C# | Class hierarchy + modern Span/Pipelines | Low-medium |
| Java | Class hierarchy (Decorator pattern) | Medium |

### Buffering Philosophy

| Language | Default buffering | User control |
|----------|-------------------|--------------|
| C | Automatic (FILE* always buffered) | `setvbuf()` before first I/O |
| C++ | Automatic (synced with C stdio by default) | `sync_with_stdio(false)`, `pubsetbuf()` |
| Python 3 | Automatic (open() builds buffer layer) | `buffering=` parameter, can access `.buffer.raw` |
| Python 2 | Automatic (C stdio) | Third arg to `open()` |
| Rust | **None by default** (File is unbuffered) | Explicit `BufReader`/`BufWriter` wrapping |
| Go | **None by default** (os.File is unbuffered) | Explicit `bufio.Reader`/`bufio.Writer` |
| Zig | **None by default** | Explicit `bufferedReader`/`bufferedWriter` |
| C# | Automatic (FileStream has internal buffer) | Buffer size in constructor, `BufferedStream` |
| Java | **None** for raw streams; decorators add it | `BufferedInputStream`/`BufferedReader` wrapping |

A clear divide: **C-family languages** (C, C++, Python, C#) tend toward automatic buffering,
while **modern systems languages** (Rust, Go, Zig) and **Java** require explicit opt-in.

### Default Buffer Sizes

| Language | Read buffer | Write buffer |
|----------|------------|--------------|
| C (glibc) | 8192 | 8192 |
| C (musl) | 1024 | 1024 |
| C++ (libstdc++) | 8192 | 8192 |
| Python 3 | max(st_blksize, 8192) | max(st_blksize, 8192) |
| Rust | 8192 | 8192 |
| Go | 4096 | 4096 |
| Zig | 4096 | 4096 |
| C# (FileStream) | 4096 | 4096 |
| Java | 8192 | 8192 |

### Text Encoding Architecture

| Language | Internal string repr | File I/O default | Encoding layer |
|----------|---------------------|-------------------|----------------|
| C | bytes (no string type) | bytes (no encoding) | None (wide fns optional) |
| C++ | bytes or wchar_t | bytes (codecvt optional) | Locale facets |
| Python 3 | Unicode (UCS) | UTF-8 / locale | `TextIOWrapper` (always for text mode) |
| Python 2 | bytes (`str`) or Unicode | bytes (no encoding) | `codecs.open()` or `io.open()` opt-in |
| Rust | UTF-8 (`String`) | bytes | **None in std** (encoding_rs crate) |
| Go | UTF-8 (`string`) | bytes | **None in std** (x/text/transform) |
| Zig | byte slice | bytes | **None in std** |
| C# | UTF-16 (`string`) | UTF-8 (.NET Core) | `StreamReader`/`StreamWriter` |
| Java | UTF-16 (`String`) | UTF-8 (Java 18+) | `InputStreamReader`/`OutputStreamWriter` |

**Three camps emerge:**

1. **Full encoding tower** (Python 3, C#, Java): Automatic encoding/decoding integrated into
   the I/O stack. Convenient but has performance cost and sometimes surprising defaults.

2. **UTF-8 native, no tower** (Rust, Go, Zig): Strings are UTF-8, files are bytes, encoding is
   your problem. Simple, fast, explicit. Works well in the modern UTF-8-everywhere world.

3. **Encoding-unaware** (C, C++, Python 2): Bytes in, bytes out. Encoding is entirely the
   programmer's responsibility. Maximum flexibility, maximum rope to hang yourself with.

### Standard Stream Defaults

| Language | stdout buffering (TTY) | stdout buffering (pipe) | stderr buffering |
|----------|----------------------|------------------------|-----------------|
| C | Line-buffered | Fully buffered | Unbuffered |
| C++ | Line-buffered | Fully buffered | Unbuffered (`unitbuf`) |
| Python 3 | Line-buffered | Block-buffered | Line-buffered |
| Rust | Line-buffered | Block-buffered | Unbuffered |
| Go | **Unbuffered** | **Unbuffered** | **Unbuffered** |
| Zig | **Unbuffered** | **Unbuffered** | **Unbuffered** |
| C# | Buffered | Buffered | Buffered |
| Java | Auto-flush on println | Auto-flush on println | Auto-flush on println |

### Relationship to C stdio

| Language | Uses C stdio? | Notes |
|----------|--------------|-------|
| C | **Is** C stdio | -- |
| C++ | Wraps FILE* internally | `sync_with_stdio` controls coupling |
| Python 3 | **No** -- bypasses entirely | Uses fd syscalls directly |
| Python 2 | **Yes** -- wraps FILE* | Buffering delegated to C stdio |
| Rust | **No** -- uses libc syscalls | Through FFI, not through FILE* |
| Go | **No** -- uses syscall package | Goroutine-aware poll integration |
| Zig | **Optional** -- can bypass libc entirely | Direct syscalls on Linux |
| C# | **No** -- P/Invoke to OS APIs | .NET has own runtime layer |
| Java | **No** -- JNI to OS APIs | JVM has own runtime layer |

### Error Handling for I/O

| Language | Mechanism | Can silently ignore? |
|----------|-----------|---------------------|
| C | Return codes + errno | Yes (common) |
| C++ | Stream state flags (or exceptions opt-in) | Yes (common) |
| Python | Exceptions | Yes (with bare except) |
| Rust | `Result<T, E>` return values | Compiler warns on unused Result |
| Go | `(value, error)` return tuples | Yes (with `_`) |
| Zig | Error unions (`!`) | Must handle or explicitly discard |
| C# | Exceptions | Yes (with empty catch) |
| Java | Checked exceptions (IOException) | Must catch or declare |

---

## Key Takeaways

1. **Python 3 has the most sophisticated built-in I/O tower.** Three explicit layers
   (raw, buffered, text) with clean separation of concerns. The trade-off is complexity
   and some overhead from the layering.

2. **Rust, Go, and Zig share a "batteries not included" philosophy for encoding.**
   In the modern UTF-8-everywhere world, this is increasingly the right default. They
   provide composable buffering but leave encoding to the application.

3. **C stdio's buffering behavior (especially stdout TTY vs pipe) remains the single
   most common source of I/O confusion** across all languages, since many languages
   either build on top of it or replicate its behavior.

4. **The trend is toward explicit composition** (Rust's `BufReader::new(File::open(...))`)
   rather than automatic/magic behavior (C's `FILE*` or Python's `open()`). This makes
   the cost model transparent.

5. **Default encoding convergence on UTF-8**: Java (18+), Python (3.15+), .NET Core, Rust,
   Go, and Zig all default to or assume UTF-8. The era of platform-dependent encoding
   defaults is ending.

6. **C++ is actively moving away from its own iostream formatting** with `std::format`
   (C++20) and `std::print` (C++23), while keeping the streambuf transport layer.
   This is an acknowledgment that the locale-facet-based formatting model was a mistake.
