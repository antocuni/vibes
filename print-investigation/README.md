# How print() Works in Compiled Languages

A survey of print/format implementations across compiled languages, with an eye
toward designing a Python-like `print()` for a compiled language (SPy).

## Overview

Python's `print()` is deceptively simple: it takes `*args` of any type,
converts each to a string via `__str__`, joins them with spaces, and writes the
result plus a newline to stdout. Replicating this in a compiled language
requires solving several interrelated problems:

1. **Accepting a variable number of arguments of different types** -- without runtime overhead if possible
2. **Converting each argument to a string** -- via some trait/protocol/interface mechanism
3. **Buffering and output** -- build a string vs. write pieces directly
4. **Type safety** -- catch errors at compile time vs. runtime
5. **Format strings** (optional) -- parsed at compile time or runtime

The languages below represent the main design strategies.

---

## Rust: Compiler-Intrinsic Macro + Trait Dispatch

### Mechanism

`println!()` is a **macro** that expands through a chain:

```
println!("x={}, y={}", x, y)
  -> io::_print(format_args_nl!("x={}, y={}", x, y))
     -> stdout().write_fmt(args)
```

`format_args!` is a **compiler built-in** (not a `macro_rules!` macro). It
requires the format string to be a literal so the compiler can parse it at
compile time.

### What format_args! produces

It emits a `fmt::Arguments<'a>` struct with three fields:

- **`pieces: &[&'static str]`** -- the literal string fragments between
  placeholders. For `"x={}, y={}"` this is `["x=", ", y=", ""]`.
- **`args: &[rt::Argument<'a>]`** -- type-erased arguments. Each entry is a
  reference to the value plus a function pointer to the appropriate trait
  method (e.g., `<i32 as Display>::fmt`). This is essentially a hand-built
  vtable.
- **`fmt: Option<&[rt::Placeholder]>`** -- optional format specifiers (width,
  precision, etc.). `None` when all placeholders use defaults.

Everything lives on the stack. No heap allocation.

### Variable arguments

The compiler processes each argument at compile time and generates an
`rt::Argument` pairing a value reference with a formatting function pointer.
The variadic nature is fully resolved at compile time; at runtime, the
formatting loop just calls through function pointers.

### String conversion: the Display trait

```rust
impl fmt::Display for MyType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "...")
    }
}
```

Each placeholder selects a trait: `{}` -> Display, `{:?}` -> Debug, `{:x}` ->
LowerHex, etc. Display cannot be `#[derive]`d -- the programmer must choose
the human-readable representation explicitly.

### Output strategy

**No temporary string.** `fmt::write()` iterates over pieces and args, writing
each directly to the output stream. Each argument's function pointer is called
with a `Formatter` that writes into stdout's buffer. The only case where a
String is built is `format!()`.

### Type safety

Fully checked at compile time. The compiler verifies: argument count matches
placeholders, every argument implements the required trait, every argument is
used. Zero runtime type checking.

### Buffering

stdout is wrapped in `ReentrantLock<LineWriter<StdoutRaw>>`. Each `println!()`
acquires the lock, writes through a line-buffered writer, and flushes on the
trailing newline. Each call pays lock + flush cost. Workaround: acquire
`stdout().lock()` once for multiple writes.

### Key design tradeoff

The `fmt::Arguments` struct enables a **non-generic `write_fmt` method**,
avoiding monomorphization bloat. The formatting loop is instantiated once and
dispatches through function pointers. This trades a small runtime cost
(indirect calls) for significantly smaller binaries compared to full
monomorphization.

---

## Go: Runtime Reflection + Interface Boxing

### Mechanism

`fmt.Println` is a **regular function**:

```go
func Println(a ...any) (n int, err error)
```

`...any` (alias for `...interface{}`) means the compiler packs all arguments
into a `[]any` slice, boxing each value into an interface.

### Interface boxing at machine level

Each `interface{}` is a 16-byte (on 64-bit) pair: a pointer to a
compiler-generated **type descriptor** and either the value itself (if it fits
in a word) or a pointer to heap-allocated data. The type descriptor contains
method tables, size, alignment, and other metadata.

### Call chain

```
Println(args...)
  -> Fprintln(os.Stdout, args...)
     -> pp = pool.Get()   // sync.Pool reuse
     -> pp.doPrintln()     // format into pp.buf
     -> os.Stdout.Write(pp.buf)
     -> pool.Put(pp)
```

A `pp` (printer) struct is obtained from a `sync.Pool` to avoid allocation.
It contains an internal byte buffer and a pre-allocated 68-byte `intbuf` for
integer formatting.

### String conversion

`printArg` uses a multi-level dispatch:

1. **Fast-path type switch** on concrete types: bool, all int/uint sizes,
   float32/64, complex64/128, string, `[]byte`. These call specialized
   formatters directly with no reflection.
2. **Interface checks** (in order): `fmt.Formatter`, `fmt.GoStringer`,
   `error`, `fmt.Stringer`. If any match, the interface method is called.
3. **Reflect fallback** for structs, maps, slices, etc. -- inspects
   `reflect.Kind` and formats generically.

### Output strategy

**Builds a complete buffer, then writes in one call.** The `pp.buf` is filled
by `doPrintln()`, then written to `os.Stdout` via a single `Write([]byte)`.
This means each `Println` call is atomic at the syscall level -- concurrent
goroutines won't interleave within a single call.

### Type safety

**Runtime only.** There is no compile-time checking of argument types. Any
value can be passed. `go vet` checks `Printf` format strings, but `Println`
has no format string to check. If a type doesn't implement `Stringer`, it falls
through to reflection -- never panics, just produces a generic representation.

### Buffering

`os.Stdout` wraps fd 1 with an `fdMutex` for concurrent access. The mutex
serializes `Write` calls, so concurrent goroutines produce non-interleaved
lines (since each `Println` writes its entire buffer in one `Write`), but line
ordering is non-deterministic.

### Key design tradeoff

Maximum simplicity and flexibility at the cost of performance. Every argument
is boxed (potential heap allocation), and non-builtin types go through
reflection. The `sync.Pool` mitigates allocation cost. The interface-based
approach means any type is printable without explicit opt-in, but there's no
way to get compile-time errors for formatting issues.

---

## Modern C++: Two Eras

### iostream (C++98): Operator Overloading

`std::cout << x << y` chains `operator<<` calls. Each returns a reference to
the same `ostream`, parsed as `(cout.operator<<(x)).operator<<(y)`.

- **User opt-in**: free function `ostream& operator<<(ostream&, const T&)`,
  found via ADL.
- **Output**: each `<<` writes immediately to the underlying `streambuf`. No
  atomicity across a chained expression -- concurrent threads can interleave.
- **Code bloat**: every unique chain generates distinct template instantiations.
  Significant binary size overhead.

### std::format (C++20) / std::print (C++23): Type-Erased Templates

```cpp
template<class... Args>
std::string format(std::format_string<Args...> fmt, Args&&... args);

template<class... Args>
void print(std::format_string<Args...> fmt, Args&&... args);
```

The critical design: the thin template layer immediately calls
`std::make_format_args(args...)`, which **type-erases** all arguments into an
array of tagged unions or function-pointer-based erasure objects. The actual
formatting is done by a **non-template** function (`std::vformat` or
`std::vprint_unicode`).

### Compile-time format string validation

`std::format_string` has a **`consteval` constructor** that parses the format
string and validates it against the `Args...` type pack during compilation. Wrong
argument count, invalid specifiers, or type mismatches are compile-time errors.
This gives printf convenience with full compile-time safety.

For runtime format strings, C++26 adds `std::runtime_format()`.

### User opt-in

Specialize `std::formatter<YourType>` with:
- `constexpr parse(parse_context&)` -- reads format specifiers (enabling
  compile-time validation even for user types)
- `format(const T&, format_context&)` -- writes via `ctx.out()` iterator

### Output strategy

`std::print` builds a complete `std::string` via `vformat`, then writes it to
`FILE*` (not `std::cout`) in a single call. This provides practical atomicity
and avoids iostream's piece-by-piece problem.

### Code bloat

The type-erasure approach means the heavy formatting machinery is instantiated
**once**, regardless of how many different type combinations appear across the
program. The {fmt} library (basis for `std::format`) compiles to roughly the
same size as equivalent `printf`, while iostream is several times larger.

### Key design tradeoff

C++23's `std::print` is the most direct analogue to Python's approach:
compile-time safety via `consteval`, type-erasure to avoid bloat, single-write
atomicity. The cost is one temporary string allocation per call.

---

## Zig: Comptime Evaluation + Monomorphization

### Mechanism

`std.debug.print()` is a **regular function** (not a macro or built-in):

```zig
pub fn format(writer: anytype, comptime fmt: []const u8, args: anytype) !void
```

The `comptime` keyword forces the format string to be known at compile time.
`anytype` uses Zig's compile-time duck typing, monomorphizing for each
concrete (writer, args) combination.

### Variable arguments via comptime tuples

No variadic functions exist. Arguments are passed as an **anonymous struct**
(tuple): `.{ x, y, z }`. The format function inspects this at comptime via
`@typeInfo(T)` to get field count and types. A bitset (max 32 args) tracks
consumption; unused arguments produce a compile error.

### Format string parsing

Parsed **entirely at comptime** by a `Parser` struct using `inline for` loops
that the compiler fully unrolls. At runtime, there is zero parsing overhead --
the compiler has emitted a straight-line sequence of write calls.

### Type dispatch: formatType()

A comptime switch on `@typeInfo(T)`:

1. Check for `pub fn format(self, comptime fmt, options, writer)` method -- if
   present, call it directly (user opt-in).
2. Primitives (int, float): dispatch to `formatValue()`.
3. Dedicated handlers for enums, unions, structs, optionals, errors, pointers,
   slices, arrays.
4. `max_depth` parameter prevents infinite recursion on self-referential types.

All dispatch resolved at comptime -> direct calls, no vtable, no type switch at
runtime.

### Output strategy

**Writes directly to the writer, one piece at a time.** No intermediate string.
Memory usage is constant regardless of output length. `std.debug.print()` uses
a 64-byte stack buffer over stderr. File I/O uses `BufferedWriter` with a
4096-byte buffer.

### Type safety

Everything checked at compile time: wrong argument count, unused arguments,
type mismatches, malformed format strings.

### Key design tradeoff

Zig achieves the same goals as Rust's macros but **without macros**: comptime
execution of normal Zig code does the parsing, `@typeInfo` does the reflection,
and `anytype` does the monomorphization. The generated code is fully
transparent and debuggable. The cost is full monomorphization (every unique
format call generates unique code), whereas Rust's `fmt::Arguments` avoids
this.

---

## Swift: Protocol Witnesses + Existential Containers

### Mechanism

`print()` is a **regular library function**:

```swift
func print(_ items: Any..., separator: String = " ", terminator: String = "\n")
```

### Variable arguments

`Any...` packs arguments into an `[Any]` array. Each value is boxed into an
**existential container** (3 words of inline storage + type metadata pointer +
protocol witness table pointer). Values larger than 3 words are heap-allocated.

### String conversion

Checks protocol conformance at runtime (in order):
1. `CustomStringConvertible` -> `.description` property
2. `CustomDebugStringConvertible` -> `.debugDescription`
3. Fallback to `Mirror`-based reflection

### Output strategy

Builds a `String` per argument via `String(describing:)`, then writes
sequentially to a `TextOutputStream` (stdout by default).

### Type safety

Fully type-safe (no format strings to mismatch), but all dispatch is runtime.
Worst case is unexpected string representation, never UB.

---

## D: Variadic Templates + Sink Protocol

### Mechanism

`writeln` is a **template function**:

```d
void writeln(Args...)(Args args)
```

The compiler generates a specialized instantiation for each unique combination
of argument types.

### String conversion

Fast paths for builtins. For user types, D looks for `toString` -- either
`string toString()` or the zero-allocation sink form:
`void toString(scope void delegate(const(char)[]) sink)`. Falls back to
compile-time reflection via `tupleof` for a default representation.

### Compile-time format checking

`writefln!"%d %s"(42, "hello")` validates the format string against argument
types at compile time via template instantiation.

### Output strategy

Sink-based: the delegate writes directly to the C `FILE*` buffer. With the
sink form of `toString`, data flows from user type to stdio buffer with **zero
intermediate allocation**.

### Key design tradeoff

Maximum performance through compile-time specialization + the sink protocol
avoids GC allocations (important since D has a GC). Cost is binary bloat from
template instantiation.

---

## Nim: Compiler Built-in + Conversion Operator

### Mechanism

`echo` is a **compiler built-in** declared as:

```nim
proc echo*(x: varargs[string, `$`])
```

The `varargs[string, `$`]` syntax means: accept variable arguments, applying
the `$` operator (Nim's `__str__` equivalent) to each automatically.

### String conversion

The compiler inserts a call to the appropriate `$` overload for each
argument's type at compile time. User types define `proc `$`*(x: MyType): string`.

### Output strategy

Despite the `varargs[string]` signature, the compiler does **not** create an
intermediate array. It generates code that writes each converted argument
sequentially to stdout, wrapped in `flockfile`/`funlockfile` for thread safety.
Each argument is a temporary string, written and discarded one at a time.

### Type safety

Fully checked at compile time. Missing `$` overload = compile error.

---

## Comparison Matrix

| Aspect | Rust | Go | C++23 | Zig | Swift | D | Nim |
|---|---|---|---|---|---|---|---|
| **Nature** | Macro (compiler intrinsic) | Function | Function template | Function (comptime) | Function | Template function | Compiler built-in |
| **Variadic mechanism** | Macro expansion | `...any` boxing | Variadic templates -> type erasure | Comptime tuples | `Any...` boxing | Variadic templates | `varargs` with conversion |
| **Type resolution** | Compile-time | Runtime (reflection) | Compile-time | Compile-time | Runtime (protocol witness) | Compile-time | Compile-time |
| **String conversion** | `Display` trait | `Stringer` interface / reflect | `std::formatter` specialization | `format()` method / `@typeInfo` | `CustomStringConvertible` | `toString()` / sink | `$` operator |
| **Builds temp string?** | No (direct write) | Yes (pooled buffer) | Yes (one string) | No (direct write) | Yes (per argument) | No (sink to FILE*) | Yes (per argument) |
| **Format string checking** | Compile-time | `go vet` only (Printf) | Compile-time (`consteval`) | Compile-time | N/A | Compile-time | N/A (`fmt` macro separate) |
| **Monomorphization bloat** | Avoided (type-erased `write_fmt`) | N/A (no generics in fmt) | Avoided (type-erased `vformat`) | Full monomorphization | N/A (runtime dispatch) | Full monomorphization | Minimal (compiler special-cases) |
| **Atomicity** | Per-`println!` (lock + flush) | Per-`Println` (single Write) | Per-`print` (single write) | No guarantee | No guarantee | Per-`writeln` (stdio lock) | Per-`echo` (flockfile) |

---

## Design Lessons for SPy

### The fundamental tension

There are two main strategies, with a hybrid middle ground:

1. **Full monomorphization** (Zig, D): generate specialized code per call site.
   Zero runtime overhead, but binary bloat scales with number of unique print
   call sites.

2. **Type erasure** (Rust, C++23): a thin generic layer packs arguments into a
   type-erased container, then a single non-generic function does the
   formatting via indirect calls. Small constant runtime cost, but binary size
   stays flat.

3. **Runtime boxing** (Go, Swift): fully dynamic, maximum flexibility, but pays
   boxing + reflection cost.

### Recommendations to consider

- **Python's `print()` has no format string** -- it just calls `str()` on each
  arg and joins with spaces. This simplifies the design enormously: no
  compile-time format parsing needed for the basic case. Nim's `echo` is the
  closest analogue.

- **The conversion protocol is the core design decision.** Every language needs
  a way for types to define "how do I become a string." In a compiled language,
  this is typically a trait/interface/protocol. For SPy, this is the equivalent
  of `__str__`.

- **Rust/C++23's type-erasure approach** is worth studying if SPy cares about
  binary size. It lets you have one copy of the output loop regardless of how
  many different type combinations appear across the program.

- **The "build string vs. write directly" choice** matters for performance.
  Go's approach (build buffer, single write) gives atomicity. Rust/Zig's
  approach (write pieces directly) avoids allocation. For a Python-like
  `print()`, Go's approach is probably more natural since Python's `print()`
  also builds the full output string before writing.

- **Consider the no-format-string case first.** Python's `print(a, b, c)` is
  much more common than `print(f"{a} {b} {c}")`. The simple case (convert each
  arg, join with sep, write with end) can be a straightforward function if SPy
  has a way to express "a function that takes N arguments of different types,
  each of which implements the Stringable trait."
