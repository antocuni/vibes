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

bool padding_eq_fields(struct Padding a, struct Padding b) {
    return a.c == b.c && a.x == b.x;
}

bool padding_eq_memcmp(struct Padding a, struct Padding b) {
    return memcmp(&a, &b, sizeof(struct Padding)) == 0;
}

int main(void) {
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

    printf("\n--- Summary ---\n");
    if (fields_say_equal && !memcmp_says_equal) {
        printf("CONFIRMED: memcmp gives WRONG result for structs with padding!\n");
        printf("  Fields are equal but padding bytes differ.\n");
    } else if (fields_say_equal && memcmp_says_equal) {
        printf("NOTE: compiler may have zeroed padding on copy (ABI-dependent).\n");
    }

    return 0;
}
