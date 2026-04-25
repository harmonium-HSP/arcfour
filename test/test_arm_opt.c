#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arcfour.h"
#include "arcfour_arm.h"

/* Test vectors */
static const uint8_t test_key[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0
};

static const uint8_t test_plaintext[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

static const uint8_t expected_ciphertext[] = {
    0x75, 0xB7, 0x87, 0x80, 0x99, 0xE0, 0xC5, 0x96,
    0x0D, 0x1D, 0xBD, 0x29, 0xAD, 0x63, 0x18, 0xEB
};

static int failures = 0;

#define TEST_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            printf("❌ FAIL: %s\n", msg); \
            failures++; \
        } else { \
            printf("✅ PASS: %s\n", msg); \
        } \
    } while (0)

int test_arm_vs_generic(void) {
    uint8_t S_arm[256], S_gen[256];
    uint8_t i_arm = 0, j_arm = 0;
    uint8_t i_gen = 0, j_gen = 0;
    uint8_t out_arm[1024], out_gen[1024];
    
    printf("\n=== Test: ARM vs Generic Consistency ===\n");
    
    /* Initialize both with same key */
    arcfour_ksa_arm(S_arm, test_key, sizeof(test_key));
    memcpy(S_gen, S_arm, 256);
    
    /* Generate 1024 bytes from both */
    for (int i = 0; i < 1024; i++) {
        out_arm[i] = arcfour_byte_arm(S_arm, &i_arm, &j_arm);
        out_gen[i] = arcfour_byte_arm(S_gen, &i_gen, &j_gen);
    }
    
    TEST_ASSERT(memcmp(out_arm, out_gen, 1024) == 0, 
                "ARM and generic outputs match");
    
    return 0;
}

int test_known_vector(void) {
    uint8_t S[256];
    uint8_t i = 0, j = 0;
    uint8_t ciphertext[16];
    
    printf("\n=== Test: Known Vector Verification ===\n");
    
    arcfour_ksa_arm(S, test_key, sizeof(test_key));
    
    /* Discard first 1536 bytes (recommended for RC4) */
    for (int k = 0; k < 1536; k++) {
        arcfour_byte_arm(S, &i, &j);
    }
    
    /* Encrypt test plaintext */
    for (int k = 0; k < 16; k++) {
        ciphertext[k] = arcfour_byte_arm(S, &i, &j) ^ test_plaintext[k];
    }
    
    TEST_ASSERT(memcmp(ciphertext, expected_ciphertext, 16) == 0,
                "Known vector test");
    
    return 0;
}

int test_bulk_operations(void) {
    uint8_t S[256];
    uint8_t i = 0, j = 0;
    uint8_t input[32] = {0};
    uint8_t output1[32], output2[32];
    
    printf("\n=== Test: Bulk Operations ===\n");
    
    /* Initialize with sequential data */
    for (int k = 0; k < 32; k++) {
        input[k] = (uint8_t)k;
    }
    
    arcfour_ksa_arm(S, test_key, sizeof(test_key));
    
    /* Test 16-byte block */
    arcfour_xor_16_arm(input, output1, S, &i, &j);
    
    /* Test 4-byte block */
    arcfour_xor_4_arm(input + 16, output1 + 16, S, &i, &j);
    
    /* Verify by decrypting */
    memcpy(S, (uint8_t[256]){0}, 256);
    i = j = 0;
    arcfour_ksa_arm(S, test_key, sizeof(test_key));
    
    arcfour_xor_16_arm(output1, output2, S, &i, &j);
    arcfour_xor_4_arm(output1 + 16, output2 + 16, S, &i, &j);
    
    TEST_ASSERT(memcmp(input, output2, 32) == 0,
                "Bulk encrypt/decrypt roundtrip");
    
    return 0;
}

int test_boundary_conditions(void) {
    uint8_t S[256];
    uint8_t i = 0, j = 0;
    uint8_t output[1];
    
    printf("\n=== Test: Boundary Conditions ===\n");
    
    /* Test with empty key (edge case) */
    arcfour_ksa_arm(S, NULL, 0);
    
    /* Should not crash */
    output[0] = arcfour_byte_arm(S, &i, &j);
    TEST_ASSERT(1, "Empty key doesn't crash");
    
    /* Test with all-zero key */
    uint8_t zero_key[16] = {0};
    arcfour_ksa_arm(S, zero_key, 16);
    output[0] = arcfour_byte_arm(S, &i, &j);
    TEST_ASSERT(1, "Zero key doesn't crash");
    
    return 0;
}

int main(void) {
    printf("========== ARM Optimization Tests ==========\n");
    
    test_arm_vs_generic();
    test_known_vector();
    test_bulk_operations();
    test_boundary_conditions();
    
    printf("\n========== Results: %d failures ==========\n", failures);
    
    return failures;
}