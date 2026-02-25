#include <stdbool.h>
#include <string.h>
#include <stdint.h>

/* ── Struct definitions ────────────────────────────────────────────── */

struct Point {
    int x;
    int y;
};

struct Large {
    long a;
    long b;
    long c;
    long d;
    long e;
    long f;
};

struct Padding {
    char c;
    int x;
};

struct TwoLong {        /* 16 bytes, no padding, passed in 2 registers */
    long a;
    long b;
};

struct Padded16 {       /* 16 bytes, 7 bytes padding between c and x */
    char c;
    long x;
};

/* ── Strategy 1: field-by-field comparison ─────────────────────────── */

bool point_eq_fields(struct Point a, struct Point b) {
    return a.x == b.x && a.y == b.y;
}

bool large_eq_fields(struct Large a, struct Large b) {
    return a.a == b.a && a.b == b.b && a.c == b.c &&
           a.d == b.d && a.e == b.e && a.f == b.f;
}

bool padding_eq_fields(struct Padding a, struct Padding b) {
    return a.c == b.c && a.x == b.x;
}

/* ── Strategy 2: memcmp ───────────────────────────────────────────── */

bool point_eq_memcmp(struct Point a, struct Point b) {
    return memcmp(&a, &b, sizeof(struct Point)) == 0;
}

bool large_eq_memcmp(struct Large a, struct Large b) {
    return memcmp(&a, &b, sizeof(struct Large)) == 0;
}

bool padding_eq_memcmp(struct Padding a, struct Padding b) {
    return memcmp(&a, &b, sizeof(struct Padding)) == 0;
}

/* ── Strategy 3: memcmp via pointers (avoid copies) ───────────────── */

bool point_eq_memcmp_ptr(const struct Point *a, const struct Point *b) {
    return memcmp(a, b, sizeof(struct Point)) == 0;
}

bool large_eq_memcmp_ptr(const struct Large *a, const struct Large *b) {
    return memcmp(a, b, sizeof(struct Large)) == 0;
}

bool padding_eq_memcmp_ptr(const struct Padding *a, const struct Padding *b) {
    return memcmp(a, b, sizeof(struct Padding)) == 0;
}

/* ── Strategy 4: cast to integer (only for small structs) ─────────── */

bool point_eq_intcast(struct Point a, struct Point b) {
    /* Point is 8 bytes = uint64_t on most platforms */
    uint64_t va, vb;
    memcpy(&va, &a, sizeof(va));
    memcpy(&vb, &b, sizeof(vb));
    return va == vb;
}

/* ── Strategy 5: XOR-based bitwise comparison ─────────────────────── */

bool large_eq_xor(const struct Large *a, const struct Large *b) {
    const uint64_t *pa = (const uint64_t *)a;
    const uint64_t *pb = (const uint64_t *)b;
    /* Large has 6 longs = 6 uint64_t on LP64 */
    uint64_t diff = 0;
    diff |= pa[0] ^ pb[0];
    diff |= pa[1] ^ pb[1];
    diff |= pa[2] ^ pb[2];
    diff |= pa[3] ^ pb[3];
    diff |= pa[4] ^ pb[4];
    diff |= pa[5] ^ pb[5];
    return diff == 0;
}

/* ── Strategy 6: __builtin_memcmp (GCC/Clang) ────────────────────── */

bool point_eq_builtin(struct Point a, struct Point b) {
    return __builtin_memcmp(&a, &b, sizeof(struct Point)) == 0;
}

bool large_eq_builtin(struct Large a, struct Large b) {
    return __builtin_memcmp(&a, &b, sizeof(struct Large)) == 0;
}

bool padding_eq_builtin(struct Padding a, struct Padding b) {
    return __builtin_memcmp(&a, &b, sizeof(struct Padding)) == 0;
}

/* ── Strategy 7: field-by-field via pointers ───────────────────────── */

bool point_eq_fields_ptr(const struct Point *a, const struct Point *b) {
    return a->x == b->x && a->y == b->y;
}

bool large_eq_fields_ptr(const struct Large *a, const struct Large *b) {
    return a->a == b->a && a->b == b->b && a->c == b->c &&
           a->d == b->d && a->e == b->e && a->f == b->f;
}

bool padding_eq_fields_ptr(const struct Padding *a, const struct Padding *b) {
    return a->c == b->c && a->x == b->x;
}

/* ── TwoLong: 16 bytes, no padding, 2-register passing ────────────── */

bool twolong_eq_fields(struct TwoLong a, struct TwoLong b) {
    return a.a == b.a && a.b == b.b;
}

bool twolong_eq_memcmp(struct TwoLong a, struct TwoLong b) {
    return memcmp(&a, &b, sizeof(struct TwoLong)) == 0;
}

bool twolong_eq_memcmp_ptr(const struct TwoLong *a, const struct TwoLong *b) {
    return memcmp(a, b, sizeof(struct TwoLong)) == 0;
}

bool twolong_eq_builtin(struct TwoLong a, struct TwoLong b) {
    return __builtin_memcmp(&a, &b, sizeof(struct TwoLong)) == 0;
}

bool twolong_eq_fields_ptr(const struct TwoLong *a, const struct TwoLong *b) {
    return a->a == b->a && a->b == b->b;
}

bool twolong_eq_intcast(struct TwoLong a, struct TwoLong b) {
    /* 16 bytes = two uint64_t, can do XOR-based comparison */
    uint64_t va1, va2, vb1, vb2;
    memcpy(&va1, &a, 8);
    memcpy(&va2, (char *)&a + 8, 8);
    memcpy(&vb1, &b, 8);
    memcpy(&vb2, (char *)&b + 8, 8);
    return (va1 == vb1) & (va2 == vb2);
}

/* ── Padded16: 16 bytes, 7 bytes padding, 2-register passing ──────── */

bool padded16_eq_fields(struct Padded16 a, struct Padded16 b) {
    return a.c == b.c && a.x == b.x;
}

bool padded16_eq_memcmp(struct Padded16 a, struct Padded16 b) {
    return memcmp(&a, &b, sizeof(struct Padded16)) == 0;
}

bool padded16_eq_memcmp_ptr(const struct Padded16 *a, const struct Padded16 *b) {
    return memcmp(a, b, sizeof(struct Padded16)) == 0;
}

bool padded16_eq_builtin(struct Padded16 a, struct Padded16 b) {
    return __builtin_memcmp(&a, &b, sizeof(struct Padded16)) == 0;
}

bool padded16_eq_fields_ptr(const struct Padded16 *a, const struct Padded16 *b) {
    return a->c == b->c && a->x == b->x;
}
