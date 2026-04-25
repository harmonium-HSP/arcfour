/* 
 * test_no_heap.c - No-heap environment test
 * 
 * This file is designed to test that the static version of the ARCFOUR
 * library works correctly without any heap allocation (malloc/free).
 * 
 * When compiled with ARCFOUR_STATIC_ONLY, all dynamic memory allocation
 * should be disabled. If the code accidentally calls malloc/free, it
 * will link to the wrapped implementations below which will cause a failure.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* 
 * Wrapped malloc/free implementations that will cause test failure
 * if called. Uses -Wl,--wrap linker flag to intercept calls.
 */
void* __real_malloc(size_t size);
void __real_free(void* ptr);

static int malloc_called = 0;
static int free_called = 0;

void* __wrap_malloc(size_t size) {
    (void)size;
    malloc_called = 1;
    printf("❌ ERROR: malloc() was called - this should not happen in static mode!\n");
    while(1);  /* Dead loop - test fails if we reach here */
}

void __wrap_free(void* ptr) {
    (void)ptr;
    free_called = 1;
    printf("❌ ERROR: free() was called - this should not happen in static mode!\n");
    while(1);  /* Dead loop - test fails if we reach here */
}

/* Include arcfour headers - should use static API only */
#include "arcfour.h"

/* All test data in .bss/.rodata (no heap) */
static arcfour_ctx_t g_ctx;
static const uint8_t g_key[] = "NoHeapTestKey12345";
static const uint8_t g_plaintext[] = "Testing without heap allocation";
static uint8_t g_ciphertext[64] = {0};
static uint8_t g_decrypted[64] = {0};

static int test_no_heap_basic(void) {
    printf("Testing basic encryption without heap...\n");
    
    /* Initialize using static API */
    arcfour_init_static(&g_ctx, g_key, sizeof(g_key) - 1);
    
    if (!arcfour_is_valid(&g_ctx)) {
        printf("❌ Context initialization failed\n");
        return 1;
    }
    
    /* Encrypt */
    arcfour_encrypt_static(&g_ctx, g_plaintext, g_ciphertext, sizeof(g_plaintext) - 1);
    
    /* Verify encryption changed data */
    if (memcmp(g_plaintext, g_ciphertext, sizeof(g_plaintext) - 1) == 0) {
        printf("❌ Encryption produced same data\n");
        arcfour_uninit_static(&g_ctx);
        return 1;
    }
    
    /* Re-initialize for decryption */
    arcfour_init_static(&g_ctx, g_key, sizeof(g_key) - 1);
    
    /* Decrypt */
    arcfour_decrypt_static(&g_ctx, g_ciphertext, g_decrypted, sizeof(g_plaintext) - 1);
    
    /* Verify decryption matches original */
    if (memcmp(g_plaintext, g_decrypted, sizeof(g_plaintext) - 1) != 0) {
        printf("❌ Decryption failed - data mismatch\n");
        arcfour_uninit_static(&g_ctx);
        return 1;
    }
    
    arcfour_uninit_static(&g_ctx);
    
    printf("✅ Basic no-heap test PASSED\n");
    return 0;
}

static int test_no_heap_multiple_operations(void) {
    printf("Testing multiple operations without heap...\n");
    
    uint8_t data[1024];
    uint8_t result[1024];
    
    /* Fill test data */
    for (size_t i = 0; i < 1024; i++) {
        data[i] = (uint8_t)(i & 0xFF);
    }
    
    /* Initialize context */
    arcfour_init_static(&g_ctx, g_key, sizeof(g_key) - 1);
    
    /* Multiple encrypt operations */
    for (int i = 0; i < 10; i++) {
        arcfour_encrypt_static(&g_ctx, data, result, 1024);
    }
    
    arcfour_uninit_static(&g_ctx);
    
    printf("✅ Multiple operations test PASSED\n");
    return 0;
}

static int test_no_heap_edge_cases(void) {
    printf("Testing edge cases without heap...\n");
    
    /* Test empty data */
    arcfour_init_static(&g_ctx, g_key, sizeof(g_key) - 1);
    arcfour_encrypt_static(&g_ctx, NULL, NULL, 0);
    arcfour_uninit_static(&g_ctx);
    
    /* Test with zero-length key should fail gracefully */
    arcfour_ctx_t bad_ctx = {0};
    arcfour_init_static(&bad_ctx, NULL, 0);
    if (arcfour_is_valid(&bad_ctx)) {
        printf("❌ NULL key should not initialize context\n");
        return 1;
    }
    
    printf("✅ Edge cases test PASSED\n");
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("========================================\n");
    printf("      No-Heap Environment Tests          \n");
    printf("========================================\n\n");
    
    /* If we reach here without calling malloc, static mode is working */
    printf("✅ No heap allocation detected so far\n\n");
    
    failures += test_no_heap_basic();
    failures += test_no_heap_multiple_operations();
    failures += test_no_heap_edge_cases();
    
    printf("\n========================================\n");
    if (failures == 0) {
        printf("✅ All no-heap tests PASSED (0 failures)\n");
        printf("✅ Library works correctly without malloc/free\n");
        printf("========================================\n");
        return 0;
    } else {
        printf("❌ Tests FAILED (%d failures)\n", failures);
        printf("========================================\n");
        return 1;
    }
}