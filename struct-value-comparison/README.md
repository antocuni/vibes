# Comparing Structs by Value in C

An exploration of different strategies for comparing C structs by value (field
equality), and how GCC 13.3 and Clang 18.1 optimize each one at `-O3` on
x86-64 Linux (System V ABI).

## The structs under test

```c
struct Point {          // 8 bytes, no padding, fits in one register
    int x;
    int y;
};

struct Large {          // 48 bytes, no padding, passed on the stack
    long a, b, c, d, e, f;
};

struct Padding {        // 8 bytes, 3 bytes of padding between c and x
    char c;             // offset 0, size 1
    /* 3 bytes padding */
    int x;              // offset 4, size 4
};

struct TwoLong {        // 16 bytes, no padding, passed in two registers
    long a;
    long b;
};

struct Padded16 {       // 16 bytes, 7 bytes of padding between c and x
    char c;             // offset 0, size 1
    /* 7 bytes padding */
    long x;             // offset 8, size 8
};
```

On x86-64 SysV ABI:

- `Point` and `Padding` are 8 bytes and fit in a single 64-bit register
  (`rdi`/`rsi`) when passed by value.
- `TwoLong` and `Padded16` are 16 bytes and are passed in **two registers**
  (`rdi`+`rsi` for the first struct, `rdx`+`rcx` for the second).
- `Large` is 48 bytes and gets passed on the stack.

## Comparison strategies

| # | Strategy | Description |
|---|----------|-------------|
| 1 | **Field-by-field** | `a.x == b.x && a.y == b.y` |
| 2 | **memcmp (by value)** | `memcmp(&a, &b, sizeof(...))` on stack copies |
| 3 | **memcmp (by pointer)** | `memcmp(a, b, sizeof(...))` on original objects |
| 4 | **Integer cast** | `memcpy` into `uint64_t`, compare (small structs only) |
| 5 | **XOR accumulate** | Cast to `uint64_t*`, XOR all words, check == 0 |
| 6 | **`__builtin_memcmp`** | Compiler builtin, same semantics as memcmp |
| 7 | **Field-by-field (ptr)** | `a->x == b->x && a->y == b->y` |

## Results: generated assembly

### struct Point (8 bytes, no padding)

Since Point fits in one register, the compiler receives both structs packed
into `rdi` and `rsi`.

| Strategy | GCC 13.3 | Clang 18.1 |
|----------|----------|------------|
| Field-by-field | `cmp edi, esi` + branch + `sar rdi,32; sar rsi,32; cmp edi,esi` (5 insns) | **`cmp rdi, rsi; sete al`** (2 insns!) |
| memcmp | `cmp rdi, rsi; sete al` (2 insns) | `cmp rdi, rsi; sete al` (2 insns) |
| Integer cast | `cmp rdi, rsi; sete al` (2 insns) | `cmp rdi, rsi; sete al` (2 insns) |
| `__builtin_memcmp` | `cmp rdi, rsi; sete al` (2 insns) | `cmp rdi, rsi; sete al` (2 insns) |
| Field-by-field (ptr) | Load + cmp x, branch, load + cmp y (6 insns) | Load + cmp x, branch, load + cmp y (6 insns) |
| memcmp (ptr) | Load 8 bytes from each, single cmp (3 insns) | Load 8 bytes from each, single cmp (3 insns) |

**Key finding:** For Point by value, GCC generates *worse* code for
field-by-field than for memcmp. GCC keeps the short-circuit `&&` semantics
literally — it compares the low 32 bits, branches, then extracts and compares
the high 32 bits. Meanwhile, memcmp on 8 bytes is recognized as a single
64-bit compare.

Clang is smarter: it sees through the field-by-field `&&` chain and recognizes
that since Point has no padding, comparing both fields is equivalent to a
single 64-bit register compare. All strategies collapse to the same 2
instructions.

### struct Large (48 bytes, no padding)

Large is passed on the stack. Both copies live at known offsets from `rsp`.

| Strategy | GCC 13.3 | Clang 18.1 |
|----------|----------|------------|
| Field-by-field | Sequential cmp + branch chain (6 pairs) | **`call memcmp@PLT`** (!!) |
| memcmp | Inline XOR pairs with early-exit (grouped 2-at-a-time) | `call bcmp@PLT` |
| XOR accumulate | Inline XOR chain, fully branchless, single result | Inline XOR chain, fully branchless |
| `__builtin_memcmp` | Same as memcmp (inline XOR pairs) | `call bcmp@PLT` |
| Field-by-field (ptr) | Sequential cmp + branch chain (6 pairs) | Sequential cmp + branch chain (6 pairs) |
| memcmp (ptr) | Inline XOR pairs with early-exit | `call bcmp@PLT` |

**Key finding:** Clang recognizes the field-by-field comparison of Large and
*replaces it with a `memcmp` call*. This is a remarkable optimization —
Clang proves to itself that because Large has no padding, comparing all 6
fields with `&&` is equivalent to `memcmp`, and delegates to a library
function that may use SIMD internally.

GCC inlines the memcmp into XOR-pair comparisons with early-exit branches
(checking 2 qwords at a time, bailing out as soon as a mismatch is found).
This is a reasonable approach that avoids the function call overhead.

The explicit XOR-accumulate strategy produces fully branchless code on both
compilers — all 6 XOR results are OR'd together into a single register, then
tested once. This trades off the possibility of early-exit for eliminating
branch misprediction entirely.

### struct TwoLong (16 bytes, no padding)

TwoLong is 16 bytes and passed in two registers: `rdi`=a.a, `rsi`=a.b,
`rdx`=b.a, `rcx`=b.b. This is the critical size boundary where the struct
no longer fits in a single register.

| Strategy | GCC 13.3 | Clang 18.1 |
|----------|----------|------------|
| Field-by-field | `cmp rdi, rdx` + branch + `cmp rsi, rcx` (4 insns, branching) | **`xor rdi,rdx; xor rsi,rcx; or; sete`** (4 insns, branchless!) |
| memcmp | XOR pairs: `xor rdx,rdi; xor rcx,rsi; or; sete` (4 insns, branchless) + dead stores | **SSE: `pcmpeqb + pmovmskb`** (8 insns, register spill to XMM!) |
| Integer cast | `cmp rdi,rdx; sete; cmp rsi,rcx; sete; and` (5 insns, branchless) | XOR+OR (4 insns, branchless, same as field-by-field) |
| `__builtin_memcmp` | Same as memcmp (XOR pairs) | Same as memcmp (SSE) |
| Field-by-field (ptr) | Load + cmp + branch x2 (6 insns) | Load + cmp + branch x2 (6 insns) |
| memcmp (ptr) | XOR pairs from memory (7 insns, branchless) | **SSE: `movdqu` x2 + `pcmpeqb + pmovmskb`** (6 insns, branchless) |

**Key finding:** The 16-byte size brings out a fascinating divergence between
the compilers:

- **Clang's field-by-field is optimal.** It generates the same 4-instruction
  XOR+OR pattern regardless of whether you write `a.a == b.a && a.b == b.b` or
  use `memcpy`-based integer comparison. Clang sees through the `&&` and fuses
  the two comparisons.

- **Clang uses SSE for memcmp at 16 bytes!** For `memcmp`, Clang spills the
  registers to the stack, loads them into XMM registers with `movdqu`, then uses
  `pcmpeqb` + `pmovmskb` to compare all 16 bytes at once. This is actually
  *slower* than the field-by-field approach (8 instructions vs 4) because of
  the register→stack→XMM round-trip. The SSE path makes sense for
  pointer-based memcmp (data is already in memory) but is wasteful when the
  values are already in GPRs.

- **GCC uses XOR+OR for memcmp**, avoiding SSE entirely. This is the same
  pattern as field-by-field but branchless. However, for field-by-field itself,
  GCC stubbornly preserves the short-circuit branch.

### struct Padded16 (16 bytes, with 7 padding bytes)

Like TwoLong, Padded16 is 16 bytes and passed in two registers. But it has 7
bytes of padding: `rdi` holds `c` (low byte) + 7 padding bytes, `rsi` holds
`x` (full 8-byte long).

| Strategy | GCC 13.3 | Clang 18.1 |
|----------|----------|------------|
| Field-by-field | `cmp dil, dl` + branch + `cmp rsi, rcx` (5 insns, branching) | **`cmp dil,dl; sete; cmp rsi,rcx; sete; and`** (5 insns, branchless!) |
| memcmp | XOR full registers + OR (WRONG — compares padding!) | **SSE: pcmpeqb + pmovmskb** (WRONG — compares padding!) |
| `__builtin_memcmp` | Same as memcmp (WRONG) | Same as memcmp (WRONG) |
| Field-by-field (ptr) | Load byte + cmp + branch + load long + cmp (6 insns) | Load byte + cmp + branch + load long + cmp (6 insns) |
| memcmp (ptr) | XOR pairs from memory (WRONG) | SSE from memory (WRONG) |

**Key finding:** The 16-byte padded struct reveals a crucial difference from
the 8-byte `Padding`:

- **No masking trick.** With 8-byte `Padding`, Clang generated a brilliant
  XOR + mask (`0xFFFFFFFF000000FF`) to ignore padding within a single register.
  But with `Padded16`, the two fields live in *separate registers* (`rdi` for
  `char c`, `rsi` for `long x`). There's no single mask that can span two
  registers, so Clang falls back to comparing each field independently with
  two `sete` + `and`. Still branchless, still correct, but the mask trick
  only works within a single register.

- **GCC still branches** on the `&&`, comparing `dil` vs `dl` first, branching,
  then comparing `rsi` vs `rcx`.

### struct Padding (8 bytes, with 3 padding bytes)

Like Point, Padding fits in a single register. But it has 3 bytes of padding
between `c` (offset 0) and `x` (offset 4).

| Strategy | GCC 13.3 | Clang 18.1 |
|----------|----------|------------|
| Field-by-field | `cmp dil, sil` + branch + `sar rdi,32; sar rsi,32; cmp` | **XOR + mask `0xFFFFFFFF000000FF`** (branchless!) |
| memcmp | `cmp rdi, rsi` (WRONG — compares padding!) | `cmp rdi, rsi` (WRONG — compares padding!) |
| `__builtin_memcmp` | `cmp rdi, rsi` (WRONG — compares padding!) | `cmp rdi, rsi` (WRONG — compares padding!) |
| Field-by-field (ptr) | Load byte + cmp, branch, load int + cmp | Load byte + cmp, branch, load int + cmp |
| memcmp (ptr) | Load 8 bytes, single cmp (WRONG) | Load 8 bytes, single cmp (WRONG) |

**Key finding:** Clang generates a beautiful branchless comparison for
`padding_eq_fields`:

```asm
padding_eq_fields:
    xor     rdi, rsi              ; XOR the two registers
    movabs  rax, 0xFFFFFFFF000000FF  ; mask: bits for c (byte 0) and x (bytes 4-7)
    test    rdi, rax              ; test only the field bits, ignore padding
    sete    al                    ; equal if all tested bits are zero
    ret
```

The mask `0xFFFFFFFF000000FF` selects exactly the bits that correspond to
actual fields:

```
Byte 0   (char c):  0xFF         ← selected
Bytes 1-3 (padding): 0x000000    ← masked out
Bytes 4-7 (int x):  0xFFFFFFFF   ← selected
```

This is correct, branchless, and only 4 instructions. Clang figured out which
bits are padding and generated a mask to ignore them.

GCC takes the literal approach: compare the low byte (char), branch, shift
right 32 to extract the int, compare. Correct but involves a branch.

## The padding trap

**`memcmp` and `__builtin_memcmp` are WRONG for structs with padding** unless
you can guarantee all padding bytes are identical (e.g., via `memset(&s, 0, sizeof(s))`
before initialization).

### 8-byte Padding struct

Our correctness test confirms this:

```
Test: structs with same fields but different padding bytes
  field-by-field: EQUAL
  memcmp:         NOT EQUAL     ← WRONG

Test: zero-initialized structs with same fields
  field-by-field: EQUAL
  memcmp:         EQUAL         ← only correct by luck (padding is zeroed)
```

Both GCC and Clang optimize `memcmp` on Padding to a single `cmp rdi, rsi`,
which compares all 8 bytes including the 3 padding bytes. If those padding
bytes differ (which is legal — C makes no guarantees about padding contents),
the comparison gives a false negative.

### 16-byte Padded16 struct — zero-init is NOT enough with GCC!

The 16-byte case reveals a more insidious problem:

```
GCC output:
  Test: zero-initialized structs with same fields
    field-by-field: EQUAL
    memcmp:         NOT EQUAL     ← WRONG even with zero-init!

Clang output:
  Test: zero-initialized structs with same fields
    field-by-field: EQUAL
    memcmp:         EQUAL         ← happens to work
```

**With GCC, even zero-initializing the struct doesn't save you.** When GCC
passes the 16-byte `Padded16` by value, it loads the `char c` field into the
low byte of `rdi`. But the upper 7 bytes of `rdi` (which correspond to padding)
may contain garbage from the register's prior contents — GCC doesn't
necessarily clear them, even if the struct was zero-initialized in memory. When
`memcmp` then compares these registers, it sees the garbage padding bytes and
reports inequality.

Clang happens to get this right (it clears or preserves the zero padding when
loading into registers), but this is an ABI/optimizer detail you should never
rely on. **The C standard explicitly states that writing to a struct member can
leave padding bytes with unspecified values, and compilers are free to exploit
this.**

## Summary table

| Aspect | Field-by-field | memcmp | XOR accumulate | Integer cast |
|--------|---------------|--------|----------------|-------------|
| **Correct with padding?** | Yes | **NO** | **NO** | **NO** |
| **Branchless?** | GCC: No, Clang: Yes (small) | Yes | Yes | Yes |
| **Point (GCC)** | 5 insns | 2 insns | — | 2 insns |
| **Point (Clang)** | 2 insns | 2 insns | — | 2 insns |
| **TwoLong (GCC)** | 4 insns (branch) | 4 insns (XOR+OR) | — | 5 insns |
| **TwoLong (Clang)** | 4 insns (XOR+OR!) | 8 insns (SSE!) | — | 4 insns |
| **Large (GCC)** | 17 insns | ~20 insns (inline) | ~16 insns | — |
| **Large (Clang)** | `call memcmp` | `call bcmp` | ~16 insns | — |
| **Padding (GCC)** | 5 insns | 2 insns (WRONG) | — | — |
| **Padding (Clang)** | 4 insns (masked!) | 2 insns (WRONG) | — | — |
| **Padded16 (GCC)** | 5 insns (branch) | ~6 insns (WRONG) | — | — |
| **Padded16 (Clang)** | 5 insns (branchless) | ~8 insns SSE (WRONG) | — | — |

## Recommendations

1. **For correctness: always use field-by-field comparison.** It is the only
   strategy that reliably ignores padding bytes. With Clang, you pay no
   performance penalty — it optimizes field-by-field into the best possible
   code, even generating padding-aware bit masks.

2. **`memcmp` is only safe when you can prove there is no padding** (e.g.,
   `struct Point` with two `int`s, no gaps). For such types, `memcmp` produces
   optimal code on both compilers.

3. **Zero-initializing structs does NOT guarantee memcmp safety.** Our tests
   show that GCC may not preserve zero padding in registers when passing
   16-byte structs by value. Even with `= {0}` initialization, `memcmp` can
   give false negatives. This makes the zero-init workaround even more fragile
   than commonly believed.

4. **Clang generally produces better code than GCC for struct comparisons.**
   Clang's optimizer recognizes field-by-field patterns and emits tight,
   often branchless code. GCC tends to follow the short-circuit `&&` literally,
   producing more branches.

5. **The explicit XOR-accumulate pattern** is useful when you want guaranteed
   branchless comparison of large structs and can prove there's no padding.
   Both compilers generate good code for it.

6. **At 16 bytes, Clang's memcmp uses SSE (pcmpeqb) instead of scalar ops.**
   When the struct is already in GPRs (passed by value), this is worse than
   field-by-field because it requires spilling registers to the stack for
   XMM load. For pointer-based memcmp where data is in memory, the SSE
   approach is reasonable. Field-by-field comparison avoids this overhead
   entirely.

## Files in this project

- `compare.c` — All comparison strategies for all five struct types
- `compare_gcc.s` — GCC 13.3 `-O3` assembly output
- `compare_clang.s` — Clang 18.1 `-O3` assembly output
- `test_correctness.c` — Demonstrates the padding/memcmp correctness problem (8-byte and 16-byte)

## Further reading

Resources on struct comparison, padding, and how compilers/languages deal with
this problem.

### The padding problem

- [EXP42-C. Do not compare padding data](https://wiki.sei.cmu.edu/confluence/display/c/EXP42-C.+Do+not+compare+padding+data)
  — SEI CERT C Coding Standard. The authoritative secure coding guideline:
  "padding values are unspecified, attempting a byte-by-byte comparison between
  structures can lead to incorrect results." Provides compliant and
  non-compliant examples.

- [C Structure Padding Initialization](https://interrupt.memfault.com/blog/c-struct-padding-initialization)
  — Memfault/Interrupt blog. Deep dive into when padding is zeroed (static
  storage) vs not (automatic storage), C23's `= {}` change, GCC 12's
  `-ftrivial-auto-var-init`, `-Wpadded`, and strategies for safe struct
  comparison.

- [Padding the struct: How a compiler optimization can disclose stack memory](https://research.nccgroup.com/2019/10/30/padding-the-struct-how-a-compiler-optimization-can-disclose-stack-memory/)
  — NCC Group Research. Security angle: compiler optimizations on struct
  assignment can leak uninitialized padding bytes to userspace, demonstrated
  in real kernel drivers.

- [The Lost Art of Structure Packing](http://www.catb.org/esr/structure-packing/)
  — Eric S. Raymond. The classic guide to understanding alignment, padding,
  and how to reorder struct members to minimize wasted space. Essential
  background for anyone working with struct layouts.

### Optimizing struct equality in practice

- [Optimising struct equality checks](https://nextmovesoftware.com/blog/2020/01/23/optimising-struct-equality-checks/)
  — NextMove Software blog. Shows how replacing field-by-field comparison
  with `memcmp` on a padding-free struct dramatically reduces generated
  instructions. Compares GCC, Clang, and ICC output. Key insight: compilers
  struggle to auto-merge field comparisons into wider loads, so `memcmp` on
  known-no-padding structs is a practical manual optimization.

- [LLVM MergeICmps pass](https://llvm.org/doxygen/MergeICmps_8cpp_source.html)
  — LLVM source code. This pass transforms chains of field-by-field integer
  comparisons on contiguous memory into a single `memcmp` call, which is
  then expanded by ExpandMemCmp into optimal hardware comparisons. This is
  why Clang can turn `a.a == b.a && a.b == b.b && ...` into `memcmp` for
  large structs.

- [GCC mailing list: Help with comparison-merging optimization pass](https://www.mail-archive.com/gcc@gcc.gnu.org/msg105652.html)
  — Discussion of bringing LLVM's MergeICmps optimization to GCC. Covers
  challenges with nested struct comparisons, array comparisons, and safety
  concerns with flexible array members. GCC doesn't yet have this pass,
  which explains why GCC's field-by-field codegen is often worse than Clang's.

### How other languages handle struct equality

- [Ensmallening Go binaries by prohibiting comparisons](https://dave.cheney.net/2020/05/09/ensmallening-go-binaries-by-prohibiting-comparisons)
  — Dave Cheney. Explains how Go's `==` operator on structs works: the
  compiler generates per-type equality functions that compare fields while
  skipping padding. If a struct has no padding, the compiler may use
  `memcmp` directly. Also covers the trick of adding `[0]func()` fields to
  prevent comparison and shrink binaries.

- [Rust #140167: Bad codegen for comparing struct of two 16-bit ints](https://github.com/rust-lang/rust/issues/140167)
  — Rust/LLVM bug (April 2025). `derive(PartialEq)` on `struct { u16, u16 }`
  generates four separate loads + XOR + OR instead of a single 32-bit
  compare. Root cause: LLVM doesn't merge the short-circuit `&&` pattern
  into a wider load. Using bitwise `&` instead of `&&` produces optimal code.
  Directly relevant to our findings about GCC's literal `&&` codegen.

- [windows-rs #2229: PartialEq with memcmp shouldn't compare padding](https://github.com/microsoft/windows-rs/issues/2229)
  — Real-world bug in Microsoft's Rust Windows bindings. The code generator
  emitted `memcmp`-based `PartialEq` for structs with padding, causing
  equality checks to fail on logically equal values. Fixed by switching to
  field-by-field comparison.

- [Nim: Generate default `==` for object variants (#6676)](https://github.com/nim-lang/Nim/issues/6676)
  — Nim compiles to C and generates field-by-field equality via its `fields`
  iterator. This issue tracks limitations with object variants (tagged
  unions), where the default `==` doesn't work due to iterator limitations.

### C++ approaches and proposals

- [C++20 Default comparisons](http://en.cppreference.com/w/cpp/language/default_comparisons.html)
  — cppreference. C++20's `= default` for `operator==` performs memberwise
  comparison, safely ignoring padding. This is the approach C lacks due to
  having no operator overloading.

- [Comparisons in C++20](https://brevzin.github.io/c++/2019/07/28/comparisons-cpp20/)
  — Barry Revzin's comprehensive blog post on C++20's comparison framework,
  including the spaceship operator and defaulted equality.

- [P0732R0 and "trivially comparable"](https://quuxplusone.github.io/blog/2018/03/19/p0732r0-and-trivially-comparable/)
  — Arthur O'Dwyer. Discusses the C++ proposal for class types as template
  parameters, and the critical distinction between "trivially comparable"
  (memcmp-safe, no padding) and "strong structural equality" (memberwise,
  padding-safe). The proposal initially "forgot about padding bytes."

- [`has_no_padding_bits`](https://quuxplusone.github.io/blog/2018/06/08/no-padding-bits/)
  — Arthur O'Dwyer. Argues for a C++ type trait to detect padding, motivated
  by `std::atomic::compare_exchange` which uses `memcmp` internally and
  silently breaks on structs with padding. The same problem we demonstrate
  with our correctness tests.

- [Bjarne Stroustrup: Background for the default comparison proposal](https://isocpp.org/blog/2016/02/a-bit-of-background-for-the-default-comparison-proposal-bjarne-stroustrup)
  — Historical context: Stroustrup asked Ritchie why C had `=` but not `==`
  for structs. Answer: until 1978 C didn't even have struct assignment, and
  comparison couldn't be done efficiently with memcmp due to padding holes.
