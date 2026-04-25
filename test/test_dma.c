/*
 * test_dma.c - DMA optimization tests
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "arcfour_dma.h"

#define TEST_BUFFER_SIZE 512
#define NUM_TESTS 4

static int failures = 0;

/* Helper macro for tests */
#define TEST_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            printf("❌ %s\n", msg); \
            failures++; \
        } else { \
            printf("✅ %s\n", msg); \
        } \
    } while (0)

/*===========================================*/
/* Test 1: DMA alignment verification */
/*===========================================*/
static int test_dma_alignment(void) {
    printf("\n=== Test 1: DMA Alignment ===\n");
    
    /* Test alignment macro */
    uint8_t ARCFOUR_DMA_BUFFER aligned_buffer[TEST_BUFFER_SIZE];
    uint8_t unaligned_buffer[TEST_BUFFER_SIZE];
    
    /* Check aligned buffer */
    int aligned = arcfour_dma_is_aligned(aligned_buffer, ARCFOUR_DMA_ALIGNMENT);
    TEST_ASSERT(aligned == 1, "Aligned buffer check");
    
    /* Check unaligned buffer (first byte) */
    int unaligned = arcfour_dma_is_aligned(&unaligned_buffer[1], ARCFOUR_DMA_ALIGNMENT);
    TEST_ASSERT(unaligned == 0, "Unaligned buffer check");
    
    /* Check allocation function (only in non-static mode) */
#ifndef ARCFOUR_STATIC_ONLY
    void* allocated = arcfour_dma_alloc_aligned(TEST_BUFFER_SIZE);
    TEST_ASSERT(allocated != NULL, "Aligned memory allocation");
    
    if (allocated) {
        int alloc_aligned = arcfour_dma_is_aligned(allocated, ARCFOUR_DMA_ALIGNMENT);
        TEST_ASSERT(alloc_aligned == 1, "Allocated memory alignment");
        arcfour_dma_free_aligned(allocated);
    }
#else
    printf("  (Skipping aligned allocation test in static mode)\n");
#endif
    
    return failures;
}

/*===========================================*/
/* Test 2: DMA encryption */
/*===========================================*/
static int test_dma_encrypt(void) {
    printf("\n=== Test 2: DMA Encryption ===\n");
    
    uint8_t key[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                       0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
    
    uint8_t ARCFOUR_DMA_BUFFER plaintext[TEST_BUFFER_SIZE];
    uint8_t ARCFOUR_DMA_BUFFER ciphertext[TEST_BUFFER_SIZE];
    uint8_t ARCFOUR_DMA_BUFFER decrypted[TEST_BUFFER_SIZE];
    
    /* Initialize plaintext with test pattern */
    for (size_t i = 0; i < TEST_BUFFER_SIZE; i++) {
        plaintext[i] = (uint8_t)(i & 0xFF);
    }
    
    /* Initialize context - use static API for STATIC_ONLY mode */
    arcfour_ctx_t ctx;
    arcfour_init_static(&ctx, key, sizeof(key));
    
    /* Configure DMA encryption */
    arcfour_dma_config_t config = {
        .input = plaintext,
        .output = ciphertext,
        .len = TEST_BUFFER_SIZE,
        .use_double_buffer = 0,
        .transfer_complete = 0
    };
    
    /* Encrypt */
    int ret = arcfour_encrypt_dma(&ctx, &config);
    TEST_ASSERT(ret == 0, "DMA encryption");
    TEST_ASSERT(config.transfer_complete == 1, "Transfer complete flag");
    
    /* Re-initialize for decryption */
    arcfour_ctx_t ctx2;
    arcfour_init_static(&ctx2, key, sizeof(key));
    
    /* Decrypt */
    arcfour_dma_config_t config2 = {
        .input = ciphertext,
        .output = decrypted,
        .len = TEST_BUFFER_SIZE,
        .use_double_buffer = 0,
        .transfer_complete = 0
    };
    
    ret = arcfour_decrypt_dma(&ctx2, &config2);
    TEST_ASSERT(ret == 0, "DMA decryption");
    
    /* Verify */
    int match = (memcmp(plaintext, decrypted, TEST_BUFFER_SIZE) == 0);
    TEST_ASSERT(match, "Encrypt/decrypt consistency");
    
    return failures;
}

/*===========================================*/
/* Test 3: Keystream generation */
/*===========================================*/
static int test_dma_keystream(void) {
    printf("\n=== Test 3: Keystream Generation ===\n");
    
    uint8_t key[16] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                       0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00};
    
    uint8_t ARCFOUR_DMA_BUFFER keystream[TEST_BUFFER_SIZE];
    uint8_t ARCFOUR_DMA_BUFFER plaintext[TEST_BUFFER_SIZE];
    uint8_t ARCFOUR_DMA_BUFFER ciphertext1[TEST_BUFFER_SIZE];
    uint8_t ARCFOUR_DMA_BUFFER ciphertext2[TEST_BUFFER_SIZE];
    
    /* Initialize plaintext */
    for (size_t i = 0; i < TEST_BUFFER_SIZE; i++) {
        plaintext[i] = (uint8_t)(i * 3);
    }
    
    /* Generate keystream - use static API */
    arcfour_ctx_t ctx;
    arcfour_init_static(&ctx, key, sizeof(key));
    
    size_t generated = arcfour_prepare_keystream_dma(&ctx, keystream, TEST_BUFFER_SIZE);
    TEST_ASSERT(generated == TEST_BUFFER_SIZE, "Keystream generation");
    
    /* XOR manually */
    for (size_t i = 0; i < TEST_BUFFER_SIZE; i++) {
        ciphertext1[i] = plaintext[i] ^ keystream[i];
    }
    
    /* Use provided XOR function */
    arcfour_xor_with_keystream(ciphertext2, keystream, plaintext, TEST_BUFFER_SIZE);
    
    /* Compare results */
    int match = (memcmp(ciphertext1, ciphertext2, TEST_BUFFER_SIZE) == 0);
    TEST_ASSERT(match, "XOR function consistency");
    
    return failures;
}

/*===========================================*/
/* Test 4: Double buffering */
/*===========================================*/
static int test_dma_double_buffer(void) {
    printf("\n=== Test 4: Double Buffering ===\n");
    
    uint8_t ARCFOUR_DMA_BUFFER buffer0[64];
    uint8_t ARCFOUR_DMA_BUFFER buffer1[64];
    
    arcfour_dma_double_buffer_t db;
    
    /* Initialize double buffer */
    arcfour_dma_double_buffer_init(&db, buffer0, buffer1, 64);
    TEST_ASSERT(db.buffer0 == buffer0, "Buffer0 assignment");
    TEST_ASSERT(db.buffer1 == buffer1, "Buffer1 assignment");
    TEST_ASSERT(db.size == 64, "Buffer size");
    TEST_ASSERT(db.active_buffer == 0, "Initial active buffer");
    
    /* Get active buffer */
    uint8_t* active = arcfour_dma_get_active_buffer(&db);
    TEST_ASSERT(active == buffer0, "Active buffer (first)");
    
    uint8_t* inactive = arcfour_dma_get_inactive_buffer(&db);
    TEST_ASSERT(inactive == buffer1, "Inactive buffer (first)");
    
    /* Swap buffers */
    arcfour_dma_double_buffer_swap(&db);
    TEST_ASSERT(db.active_buffer == 1, "Active buffer after swap");
    
    active = arcfour_dma_get_active_buffer(&db);
    TEST_ASSERT(active == buffer1, "Active buffer (second)");
    
    inactive = arcfour_dma_get_inactive_buffer(&db);
    TEST_ASSERT(inactive == buffer0, "Inactive buffer (second)");
    
    /* Swap back */
    arcfour_dma_double_buffer_swap(&db);
    TEST_ASSERT(db.active_buffer == 0, "Active buffer after second swap");
    
    return failures;
}

/*===========================================*/
/* Main test function */
/*===========================================*/
int main(void) {
    printf("========================================\n");
    printf("    DMA Optimization Tests\n");
    printf("========================================\n");
    
    failures = 0;
    
    test_dma_alignment();
    test_dma_encrypt();
    test_dma_keystream();
    test_dma_double_buffer();
    
    printf("\n========================================\n");
    if (failures == 0) {
        printf("✅ All tests PASSED (%d failures)\n", failures);
        return 0;
    } else {
        printf("❌ Tests FAILED (%d failures)\n", failures);
        return 1;
    }
}