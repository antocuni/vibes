#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

struct Padding {
    char c;
    /* 3 bytes of padding here */
    int x;
};

struct Padded16 {
    char c;
    /* 7 bytes of padding here */
    long x;
};

bool padding_eq_fields(struct Padding a, struct Padding b) {
    return a.c == b.c && a.x == b.x;
}

bool padding_eq_memcmp(struct Padding a, struct Padding b) {
    return memcmp(&a, &b, sizeof(struct Padding)) == 0;
}

bool padded16_eq_fields(struct Padded16 a, struct Padded16 b) {
    return a.c == b.c && a.x == b.x;
}

bool padded16_eq_memcmp(struct Padded16 a, struct Padded16 b) {
    return memcmp(&a, &b, sizeof(struct Padded16)) == 0;
}

int main(void) {
    /* ── 8-byte Padding struct (3 bytes padding) ──────────────────── */
    printf("=== struct Padding (8 bytes, 3 bytes padding) ===\n");
    printf("sizeof(struct Padding) = %zu\n", sizeof(struct Padding));
    printf("  offsetof c = %zu, offsetof x = %zu\n",
           __builtin_offsetof(struct Padding, c),
           __builtin_offsetof(struct Padding, x));

    /* Create two structs that are field-equal but have different padding */
    struct Padding p1, p2;

    /* Fill with different byte patterns first */
    memset(&p1, 0xAA, sizeof(p1));
    memset(&p2, 0x55, sizeof(p2));

    /* Set the actual fields to the same values */
    p1.c = 'Z';
    p1.x = 42;
    p2.c = 'Z';
    p2.x = 42;

    printf("\nTest: structs with same fields but different padding bytes\n");
    printf("  field-by-field: %s\n", padding_eq_fields(p1, p2) ? "EQUAL" : "NOT EQUAL");
    printf("  memcmp:         %s\n", padding_eq_memcmp(p1, p2) ? "EQUAL" : "NOT EQUAL");

    /* Now test with zero-initialized structs */
    struct Padding p3 = {0};
    struct Padding p4 = {0};
    p3.c = 'Z';
    p3.x = 42;
    p4.c = 'Z';
    p4.x = 42;

    printf("\nTest: zero-initialized structs with same fields\n");
    printf("  field-by-field: %s\n", padding_eq_fields(p3, p4) ? "EQUAL" : "NOT EQUAL");
    printf("  memcmp:         %s\n", padding_eq_memcmp(p3, p4) ? "EQUAL" : "NOT EQUAL");

    /* Demonstrate the problem: field-equal but memcmp-unequal */
    bool fields_say_equal = padding_eq_fields(p1, p2);
    bool memcmp_says_equal = padding_eq_memcmp(p1, p2);

    printf("\n--- Padding Summary ---\n");
    if (fields_say_equal && !memcmp_says_equal) {
        printf("CONFIRMED: memcmp gives WRONG result for structs with padding!\n");
        printf("  Fields are equal but padding bytes differ.\n");
    } else if (fields_say_equal && memcmp_says_equal) {
        printf("NOTE: compiler may have zeroed padding on copy (ABI-dependent).\n");
    }

    /* ── 16-byte Padded16 struct (7 bytes padding) ────────────────── */
    printf("\n=== struct Padded16 (16 bytes, 7 bytes padding) ===\n");
    printf("sizeof(struct Padded16) = %zu\n", sizeof(struct Padded16));
    printf("  offsetof c = %zu, offsetof x = %zu\n",
           __builtin_offsetof(struct Padded16, c),
           __builtin_offsetof(struct Padded16, x));

    struct Padded16 q1, q2;

    memset(&q1, 0xAA, sizeof(q1));
    memset(&q2, 0x55, sizeof(q2));

    q1.c = 'Z';
    q1.x = 42;
    q2.c = 'Z';
    q2.x = 42;

    printf("\nTest: structs with same fields but different padding bytes\n");
    printf("  field-by-field: %s\n", padded16_eq_fields(q1, q2) ? "EQUAL" : "NOT EQUAL");
    printf("  memcmp:         %s\n", padded16_eq_memcmp(q1, q2) ? "EQUAL" : "NOT EQUAL");

    struct Padded16 q3 = {0};
    struct Padded16 q4 = {0};
    q3.c = 'Z';
    q3.x = 42;
    q4.c = 'Z';
    q4.x = 42;

    printf("\nTest: zero-initialized structs with same fields\n");
    printf("  field-by-field: %s\n", padded16_eq_fields(q3, q4) ? "EQUAL" : "NOT EQUAL");
    printf("  memcmp:         %s\n", padded16_eq_memcmp(q3, q4) ? "EQUAL" : "NOT EQUAL");

    bool fields16_equal = padded16_eq_fields(q1, q2);
    bool memcmp16_equal = padded16_eq_memcmp(q1, q2);

    printf("\n--- Padded16 Summary ---\n");
    if (fields16_equal && !memcmp16_equal) {
        printf("CONFIRMED: memcmp gives WRONG result for 16-byte structs with padding!\n");
        printf("  Fields are equal but padding bytes differ.\n");
    } else if (fields16_equal && memcmp16_equal) {
        printf("NOTE: compiler may have zeroed padding on copy (ABI-dependent).\n");
    }

    return 0;
}
