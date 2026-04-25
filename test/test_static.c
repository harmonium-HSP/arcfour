#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "arcfour.h"

/* Test context allocated in .bss section (no heap) */
static arcfour_ctx_t g_ctx1;
static arcfour_ctx_t g_ctx2;

/* Test data */
static const uint8_t test_key1[] = "TestKey123456789";
static const uint8_t test_key2[] = "DifferentKey789012";
static const uint8_t test_plaintext[] = "Hello, Static Memory World!";

static int test_static_encrypt_decrypt(void) {
    uint8_t ciphertext[64] = {0};
    uint8_t decrypted[64] = {0};
    
    /* Initialize context using static API */
    arcfour_init_static(&g_ctx1, test_key1, sizeof(test_key1) - 1);
    
    /* Encrypt */
    arcfour_encrypt_static(&g_ctx1, test_plaintext, ciphertext, sizeof(test_plaintext) - 1);
    
    /* Verify encryption changed data */
    if (memcmp(test_plaintext, ciphertext, sizeof(test_plaintext) - 1) == 0) {
        printf("❌ Test 1: Encryption failed - ciphertext equals plaintext\n");
        return 1;
    }
    
    /* Decrypt with fresh context */
    arcfour_init_static(&g_ctx2, test_key1, sizeof(test_key1) - 1);
    arcfour_decrypt_static(&g_ctx2, ciphertext, decrypted, sizeof(test_plaintext) - 1);
    
    /* Verify decryption matches original */
    if (memcmp(test_plaintext, decrypted, sizeof(test_plaintext) - 1) != 0) {
        printf("❌ Test 1: Decryption failed - data mismatch\n");
        return 1;
    }
    
    /* Cleanup */
    arcfour_uninit_static(&g_ctx1);
    arcfour_uninit_static(&g_ctx2);
    
    printf("✅ Test 1: Basic static encrypt/decrypt PASSED\n");
    return 0;
}

static int test_multiple_contexts(void) {
    uint8_t output1[32] = {0};
    uint8_t output2[32] = {0};
    uint8_t plaintext[32];
    
    for (size_t i = 0; i < 32; i++) {
        plaintext[i] = (uint8_t)i;
    }
    
    /* Initialize two independent contexts */
    arcfour_init_static(&g_ctx1, test_key1, sizeof(test_key1) - 1);
    arcfour_init_static(&g_ctx2, test_key2, sizeof(test_key2) - 1);
    
    /* Encrypt same data with different keys */
    arcfour_encrypt_static(&g_ctx1, plaintext, output1, 32);
    arcfour_encrypt_static(&g_ctx2, plaintext, output2, 32);
    
    /* Different keys should produce different outputs */
    if (memcmp(output1, output2, 32) == 0) {
        printf("❌ Test 2: Multiple contexts failed - same output with different keys\n");
        arcfour_uninit_static(&g_ctx1);
        arcfour_uninit_static(&g_ctx2);
        return 1;
    }
    
    arcfour_uninit_static(&g_ctx1);
    arcfour_uninit_static(&g_ctx2);
    
    printf("✅ Test 2: Multiple independent contexts PASSED\n");
    return 0;
}

static int test_reset(void) {
    uint8_t output1[16] = {0};
    uint8_t output2[16] = {0};
    uint8_t plaintext[16];
    
    for (size_t i = 0; i < 16; i++) {
        plaintext[i] = (uint8_t)i;
    }
    
    /* Initialize context */
    arcfour_init_static(&g_ctx1, test_key1, sizeof(test_key1) - 1);
    
    /* First encryption */
    arcfour_encrypt_static(&g_ctx1, plaintext, output1, 16);
    
    /* Reset context */
    arcfour_reset_static(&g_ctx1);
    
    /* Second encryption with same key (should produce same output) */
    arcfour_init_static(&g_ctx1, test_key1, sizeof(test_key1) - 1);
    arcfour_encrypt_static(&g_ctx1, plaintext, output2, 16);
    
    if (memcmp(output1, output2, 16) != 0) {
        printf("❌ Test 3: Reset failed - outputs differ after reset\n");
        arcfour_uninit_static(&g_ctx1);
        return 1;
    }
    
    arcfour_uninit_static(&g_ctx1);
    
    printf("✅ Test 3: Reset functionality PASSED\n");
    return 0;
}

static int test_memory_size(void) {
    /* Compile-time check: context size should be 259 bytes (256 + 1 + 1 + 1) */
    if (sizeof(arcfour_ctx_t) != ARCFOUR_CONTEXT_SIZE) {
        printf("❌ Test 4: Memory size mismatch - expected %u, got %zu\n", 
               ARCFOUR_CONTEXT_SIZE, sizeof(arcfour_ctx_t));
        return 1;
    }
    
    /* Expected size: 256 (S-box) + 1 (i) + 1 (j) + 1 (initialized) = 259 */
    if (ARCFOUR_CONTEXT_SIZE != 259) {
        printf("❌ Test 4: ARCFOUR_CONTEXT_SIZE incorrect - expected 259, got %u\n",
               ARCFOUR_CONTEXT_SIZE);
        return 1;
    }
    
    printf("✅ Test 4: Memory size verification PASSED (%u bytes)\n", ARCFOUR_CONTEXT_SIZE);
    return 0;
}

static int test_stack_allocation(void) {
    /* Allocate context on stack */
    arcfour_ctx_t stack_ctx;
    uint8_t plaintext[] = "Stack allocated test";
    uint8_t ciphertext[32] = {0};
    uint8_t decrypted[32] = {0};
    
    /* Initialize stack context */
    arcfour_init_static(&stack_ctx, test_key1, sizeof(test_key1) - 1);
    
    if (!arcfour_is_valid(&stack_ctx)) {
        printf("❌ Test 5: Stack allocation failed - context not valid\n");
        return 1;
    }
    
    /* Encrypt/decrypt */
    arcfour_encrypt_static(&stack_ctx, plaintext, ciphertext, sizeof(plaintext) - 1);
    arcfour_init_static(&stack_ctx, test_key1, sizeof(test_key1) - 1);
    arcfour_decrypt_static(&stack_ctx, ciphertext, decrypted, sizeof(plaintext) - 1);
    
    if (memcmp(plaintext, decrypted, sizeof(plaintext) - 1) != 0) {
        printf("❌ Test 5: Stack allocation failed - decryption mismatch\n");
        arcfour_uninit_static(&stack_ctx);
        return 1;
    }
    
    arcfour_uninit_static(&stack_ctx);
    
    printf("✅ Test 5: Stack context allocation PASSED\n");
    return 0;
}

static int test_null_parameters(void) {
    arcfour_ctx_t ctx;
    
    /* Test null parameters - should not crash */
    arcfour_init_static(NULL, test_key1, sizeof(test_key1) - 1);
    arcfour_init_static(&ctx, NULL, 0);
    arcfour_encrypt_static(NULL, test_plaintext, NULL, 0);
    arcfour_decrypt_static(NULL, NULL, NULL, 0);
    arcfour_reset_static(NULL);
    arcfour_uninit_static(NULL);
    
    if (arcfour_is_valid(NULL)) {
        printf("❌ Test 6: NULL validation failed - NULL context considered valid\n");
        return 1;
    }
    
    printf("✅ Test 6: NULL parameter handling PASSED\n");
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("========================================\n");
    printf("    Static Memory Allocation Tests      \n");
    printf("========================================\n\n");
    
    failures += test_static_encrypt_decrypt();
    failures += test_multiple_contexts();
    failures += test_reset();
    failures += test_memory_size();
    failures += test_stack_allocation();
    failures += test_null_parameters();
    
    printf("\n========================================\n");
    if (failures == 0) {
        printf("✅ All tests PASSED (0 failures)\n");
        printf("========================================\n");
        return 0;
    } else {
        printf("❌ Tests FAILED (%d failures)\n", failures);
        printf("========================================\n");
        return 1;
    }
}
