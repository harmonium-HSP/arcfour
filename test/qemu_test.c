/*
 * QEMU Test for ARM Optimized RC4
 * 
 * Compile with:
 * arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -I../include -DARCFOUR_ARM_OPT=1 \
 *     qemu_test.c ../src/arm/arcfour_arm.c ../src/arcfour_key.c \
 *     -o qemu_test.elf -specs=rdimon.specs -lrdimon
 * 
 * Run with:
 * qemu-system-arm -machine lm3s6965evb -kernel qemu_test.elf -nographic -semihosting
 */

#include <stdio.h>
#include <string.h>
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

int test_known_vector(void) {
    uint8_t S[256];
    uint8_t i = 0, j = 0;
    uint8_t ciphertext[16];
    
    printf("\n=== Test: Known Vector ===\n");
    
    arcfour_ksa_arm(S, test_key, sizeof(test_key));
    
    /* Discard first 1536 bytes */
    for (int k = 0; k < 1536; k++) {
        arcfour_byte_arm(S, &i, &j);
    }
    
    /* Encrypt */
    for (int k = 0; k < 16; k++) {
        ciphertext[k] = arcfour_byte_arm(S, &i, &j) ^ test_plaintext[k];
    }
    
    TEST_ASSERT(memcmp(ciphertext, expected_ciphertext, 16) == 0,
                "Known vector test");
    
    return 0;
}

int test_consistency(void) {
    uint8_t S1[256], S2[256];
    uint8_t i1 = 0, j1 = 0, i2 = 0, j2 = 0;
    uint8_t output1[256], output2[256];
    uint8_t input[256];
    
    printf("\n=== Test: Consistency ===\n");
    
    /* Initialize input */
    for (int k = 0; k < 256; k++) {
        input[k] = (uint8_t)k;
    }
    
    /* Encrypt twice with same key */
    arcfour_ksa_arm(S1, test_key, sizeof(test_key));
    arcfour_ksa_arm(S2, test_key, sizeof(test_key));
    
    for (int k = 0; k < 256; k++) {
        output1[k] = arcfour_byte_arm(S1, &i1, &j1) ^ input[k];
        output2[k] = arcfour_byte_arm(S2, &i2, &j2) ^ input[k];
    }
    
    TEST_ASSERT(memcmp(output1, output2, 256) == 0,
                "Same key produces same output");
    
    return 0;
}

int test_bulk_operations(void) {
    uint8_t S[256];
    uint8_t i = 0, j = 0;
    uint8_t input[32], output[32], decrypted[32];
    
    printf("\n=== Test: Bulk Operations ===\n");
    
    for (int k = 0; k < 32; k++) {
        input[k] = (uint8_t)k;
    }
    
    arcfour_ksa_arm(S, test_key, sizeof(test_key));
    
    /* Test 16-byte and 4-byte operations */
    arcfour_xor_16_arm(input, output, S, &i, &j);
    arcfour_xor_4_arm(input + 16, output + 16, S, &i, &j);
    arcfour_xor_4_arm(input + 20, output + 20, S, &i, &j);
    arcfour_xor_4_arm(input + 24, output + 24, S, &i, &j);
    
    /* Decrypt */
    arcfour_ksa_arm(S, test_key, sizeof(test_key));
    i = j = 0;
    
    arcfour_xor_16_arm(output, decrypted, S, &i, &j);
    arcfour_xor_4_arm(output + 16, decrypted + 16, S, &i, &j);
    arcfour_xor_4_arm(output + 20, decrypted + 20, S, &i, &j);
    arcfour_xor_4_arm(output + 24, decrypted + 24, S, &i, &j);
    
    TEST_ASSERT(memcmp(input, decrypted, 32) == 0,
                "Bulk encrypt/decrypt roundtrip");
    
    return 0;
}

int main(void) {
    printf("========== ARM RC4 QEMU Test ==========\n");
    
    test_known_vector();
    test_consistency();
    test_bulk_operations();
    
    printf("\n========== Results: %d failures ==========\n", failures);
    
    return failures;
}