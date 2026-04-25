/*
 * test_isr.c - Interrupt-safe ARCFOUR tests
 * 
 * This file tests the interrupt-safe API of the ARCFOUR library.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "arcfour.h"
#include "arcfour_isr.h"
#include "arcfour_port.h"

/* Test contexts */
static arcfour_ctx_t g_ctx;
static const uint8_t g_key[] = "ISRTestKey12345678";
static const uint8_t g_plaintext[] = "Test data for ISR encryption";

/* Simulated interrupt nesting counter */
static uint32_t g_simulated_isr_nest = 0;

/* Test helper: simulate entering ISR */
static void simulate_isr_enter(void) {
    g_simulated_isr_nest++;
    printf("Entering ISR (nest level: %u)\n", g_simulated_isr_nest);
}

/* Test helper: simulate exiting ISR */
static void simulate_isr_exit(void) {
    printf("Exiting ISR (nest level: %u)\n", g_simulated_isr_nest);
    if (g_simulated_isr_nest > 0) {
        g_simulated_isr_nest--;
    }
}

/* Test 1: Basic ISR encryption */
static int test_isr_basic(void) {
    uint8_t ciphertext[64] = {0};
    uint8_t decrypted[64] = {0};
    
    printf("\n=== Test 1: Basic ISR encryption ===\n");
    
    /* Initialize context using ISR-safe function */
    arcfour_init_isr(&g_ctx, g_key, sizeof(g_key) - 1);
    
    if (!arcfour_is_valid(&g_ctx)) {
        printf("❌ Context initialization failed\n");
        return 1;
    }
    
    /* Simulate ISR and encrypt */
    simulate_isr_enter();
    arcfour_encrypt_isr(&g_ctx, g_plaintext, ciphertext, sizeof(g_plaintext) - 1);
    simulate_isr_exit();
    
    /* Verify encryption changed data */
    if (memcmp(g_plaintext, ciphertext, sizeof(g_plaintext) - 1) == 0) {
        printf("❌ Encryption produced same data\n");
        return 1;
    }
    
    /* Reinitialize and decrypt */
    arcfour_init_isr(&g_ctx, g_key, sizeof(g_key) - 1);
    
    simulate_isr_enter();
    arcfour_decrypt_isr(&g_ctx, ciphertext, decrypted, sizeof(g_plaintext) - 1);
    simulate_isr_exit();
    
    /* Verify decryption matches original */
    if (memcmp(g_plaintext, decrypted, sizeof(g_plaintext) - 1) != 0) {
        printf("❌ Decryption failed - data mismatch\n");
        return 1;
    }
    
    printf("✅ Basic ISR encryption/decryption PASSED\n");
    return 0;
}

/* Test 2: Nested ISR encryption */
static int test_nested_isr(void) {
    uint8_t output1[32] = {0};
    uint8_t output2[32] = {0};
    uint8_t input[32];
    
    for (size_t i = 0; i < 32; i++) {
        input[i] = (uint8_t)i;
    }
    
    printf("\n=== Test 2: Nested ISR encryption ===\n");
    
    /* Initialize context */
    arcfour_init_isr(&g_ctx, g_key, sizeof(g_key) - 1);
    
    /* Simulate nested interrupts */
    simulate_isr_enter();  // Level 1
    
    arcfour_encrypt_isr(&g_ctx, input, output1, 16);
    
    simulate_isr_enter();  // Level 2 (nested)
    
    arcfour_encrypt_isr(&g_ctx, input + 16, output2, 16);
    
    simulate_isr_exit();   // Level 2
    
    arcfour_encrypt_isr(&g_ctx, input, output1, 16);  // Continue level 1
    
    simulate_isr_exit();   // Level 1
    
    /* Verify data integrity */
    if (output1[0] == 0 && output2[0] == 0) {
        printf("❌ Nested ISR produced zero output\n");
        return 1;
    }
    
    printf("✅ Nested ISR encryption PASSED\n");
    return 0;
}

/* Test 3: Pending request queue */
static int test_pending_queue(void) {
    uint8_t input[64];
    uint8_t output[64];
    
    for (size_t i = 0; i < 64; i++) {
        input[i] = (uint8_t)i;
    }
    
    printf("\n=== Test 3: Pending request queue ===\n");
    
    /* Initialize context */
    arcfour_init_isr(&g_ctx, g_key, sizeof(g_key) - 1);
    
    /* Clear any pending requests */
    arcfour_clear_pending();
    
    /* Queue multiple requests */
    for (int i = 0; i < 4; i++) {
        int result = arcfour_request_encrypt(&g_ctx, input + i*16, output + i*16, 16);
        if (result != 0) {
            printf("❌ Failed to queue request %d\n", i);
            return 1;
        }
    }
    
    /* Verify queue has 4 items */
    if (arcfour_get_pending_count() != 4) {
        printf("❌ Queue count mismatch: expected 4, got %zu\n", 
               arcfour_get_pending_count());
        return 1;
    }
    
    /* Process pending requests */
    size_t processed = arcfour_process_pending();
    if (processed != 4) {
        printf("❌ Processed count mismatch: expected 4, got %zu\n", processed);
        return 1;
    }
    
    /* Verify queue is empty */
    if (arcfour_get_pending_count() != 0) {
        printf("❌ Queue not empty after processing\n");
        return 1;
    }
    
    /* Verify encryption was performed */
    if (memcmp(input, output, 64) == 0) {
        printf("❌ Pending queue encryption produced same data\n");
        return 1;
    }
    
    printf("✅ Pending request queue PASSED\n");
    return 0;
}

/* Test 4: Priority-based interrupt masking */
static int test_priority_masking(void) {
    uint8_t input[32];
    uint8_t output[32];
    
    for (size_t i = 0; i < 32; i++) {
        input[i] = (uint8_t)i;
    }
    
    printf("\n=== Test 4: Priority-based masking ===\n");
    
    /* Initialize context */
    arcfour_init_isr(&g_ctx, g_key, sizeof(g_key) - 1);
    
    /* Test with different priorities */
    for (int prio = 0; prio <= 15; prio += 5) {
        /* Reinitialize for each test */
        arcfour_init_isr(&g_ctx, g_key, sizeof(g_key) - 1);
        
        arcfour_encrypt_isr_priority(&g_ctx, input, output, 32, (uint8_t)prio);
        
        if (memcmp(input, output, 32) == 0) {
            printf("❌ Priority %d encryption produced same data\n", prio);
            return 1;
        }
    }
    
    printf("✅ Priority-based masking PASSED\n");
    return 0;
}

/* Test 5: Context isolation in ISR */
static int test_context_isolation(void) {
    arcfour_ctx_t ctx1, ctx2;
    uint8_t output1[16], output2[16];
    uint8_t input[16];
    const uint8_t key1[] = "KeyOne123456789";
    const uint8_t key2[] = "KeyTwo987654321";
    
    for (size_t i = 0; i < 16; i++) {
        input[i] = (uint8_t)i;
    }
    
    printf("\n=== Test 5: Context isolation ===\n");
    
    /* Initialize two contexts with different keys */
    arcfour_init_isr(&ctx1, key1, sizeof(key1) - 1);
    arcfour_init_isr(&ctx2, key2, sizeof(key2) - 1);
    
    /* Encrypt from ISR with both contexts */
    simulate_isr_enter();
    arcfour_encrypt_isr(&ctx1, input, output1, 16);
    arcfour_encrypt_isr(&ctx2, input, output2, 16);
    simulate_isr_exit();
    
    /* Different keys should produce different outputs */
    if (memcmp(output1, output2, 16) == 0) {
        printf("❌ Different keys produced same output\n");
        return 1;
    }
    
    printf("✅ Context isolation PASSED\n");
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("========================================\n");
    printf("    Interrupt-Safe ARCFOUR Tests         \n");
    printf("========================================\n");
    
    failures += test_isr_basic();
    failures += test_nested_isr();
    failures += test_pending_queue();
    failures += test_priority_masking();
    failures += test_context_isolation();
    
    printf("\n========================================\n");
    if (failures == 0) {
        printf("✅ All tests PASSED (%d failures)\n", failures);
        printf("========================================\n");
        return 0;
    } else {
        printf("❌ Tests FAILED (%d failures)\n", failures);
        printf("========================================\n");
        return 1;
    }
}