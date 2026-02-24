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
```

On x86-64 SysV ABI:

- `Point` and `Padding` are 8 bytes and fit in a single 64-bit register
  (`rdi`/`rsi`) when passed by value.
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

## Summary table

| Aspect | Field-by-field | memcmp | XOR accumulate | Integer cast |
|--------|---------------|--------|----------------|-------------|
| **Correct with padding?** | Yes | **NO** | **NO** | **NO** |
| **Branchless?** | GCC: No, Clang: Yes (small) | Yes | Yes | Yes |
| **Point (GCC)** | 5 insns | 2 insns | — | 2 insns |
| **Point (Clang)** | 2 insns | 2 insns | — | 2 insns |
| **Large (GCC)** | 17 insns | ~20 insns (inline) | ~16 insns | — |
| **Large (Clang)** | `call memcmp` | `call bcmp` | ~16 insns | — |
| **Padding (GCC)** | 5 insns | 2 insns (WRONG) | — | — |
| **Padding (Clang)** | 4 insns (masked!) | 2 insns (WRONG) | — | — |

## Recommendations

1. **For correctness: always use field-by-field comparison.** It is the only
   strategy that reliably ignores padding bytes. With Clang, you pay no
   performance penalty — it optimizes field-by-field into the best possible
   code, even generating padding-aware bit masks.

2. **`memcmp` is only safe when you can prove there is no padding** (e.g.,
   `struct Point` with two `int`s, no gaps). For such types, `memcmp` produces
   optimal code on both compilers.

3. **If you control initialization and always zero the struct first**
   (`memset` or `= {0}`), then `memcmp` works in practice, but this is
   fragile — any code path that skips the zeroing breaks the comparison
   silently.

4. **Clang generally produces better code than GCC for struct comparisons.**
   Clang's optimizer recognizes field-by-field patterns and emits tight,
   often branchless code. GCC tends to follow the short-circuit `&&` literally,
   producing more branches.

5. **The explicit XOR-accumulate pattern** is useful when you want guaranteed
   branchless comparison of large structs and can prove there's no padding.
   Both compilers generate good code for it.

## Files in this project

- `compare.c` — All comparison strategies for all three structs
- `compare_gcc.s` — GCC 13.3 `-O3` assembly output
- `compare_clang.s` — Clang 18.1 `-O3` assembly output
- `test_correctness.c` — Demonstrates the padding/memcmp correctness problem
