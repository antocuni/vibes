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
