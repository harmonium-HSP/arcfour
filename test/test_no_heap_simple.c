/* 
 * Simplified no-heap test for debugging
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Simple test without malloc/free wrapping - just test basic functionality */
#include "arcfour.h"

/* All test data in .bss/.rodata (no heap) */
static arcfour_ctx_t g_ctx;
static const uint8_t g_key[] = "NoHeapTestKey12345";
static const uint8_t g_plaintext[] = "Testing without heap";
static uint8_t g_ciphertext[64] = {0};
static uint8_t g_decrypted[64] = {0};

int main(void) {
    printf("=== Simplified No-Heap Test ===\n");
    printf("Context size: %zu bytes\n", sizeof(g_ctx));
    printf("Key length: %zu\n", sizeof(g_key) - 1);
    printf("Plaintext length: %zu\n", sizeof(g_plaintext) - 1);
    fflush(stdout);
    
    printf("\nStep 1: Initializing context...\n");
    fflush(stdout);
    
    /* Initialize using static API */
    arcfour_init_static(&g_ctx, g_key, sizeof(g_key) - 1);
    
    printf("Step 2: Checking if valid...\n");
    fflush(stdout);
    
    if (!arcfour_is_valid(&g_ctx)) {
        printf("❌ Context initialization failed\n");
        return 1;
    }
    
    printf("Step 3: Encrypting...\n");
    fflush(stdout);
    
    /* Encrypt */
    arcfour_encrypt_static(&g_ctx, g_plaintext, g_ciphertext, sizeof(g_plaintext) - 1);
    
    printf("Step 4: Checking encryption result...\n");
    fflush(stdout);
    
    /* Verify encryption changed data */
    if (memcmp(g_plaintext, g_ciphertext, sizeof(g_plaintext) - 1) == 0) {
        printf("❌ Encryption produced same data\n");
        arcfour_uninit_static(&g_ctx);
        return 1;
    }
    
    printf("Step 5: Re-initializing for decryption...\n");
    fflush(stdout);
    
    /* Re-initialize for decryption */
    arcfour_init_static(&g_ctx, g_key, sizeof(g_key) - 1);
    
    printf("Step 6: Decrypting...\n");
    fflush(stdout);
    
    /* Decrypt */
    arcfour_decrypt_static(&g_ctx, g_ciphertext, g_decrypted, sizeof(g_plaintext) - 1);
    
    printf("Step 7: Verifying decryption...\n");
    fflush(stdout);
    
    /* Verify decryption matches original */
    if (memcmp(g_plaintext, g_decrypted, sizeof(g_plaintext) - 1) != 0) {
        printf("❌ Decryption failed - data mismatch\n");
        arcfour_uninit_static(&g_ctx);
        return 1;
    }
    
    arcfour_uninit_static(&g_ctx);
    
    printf("\n✅ All no-heap tests PASSED\n");
    return 0;
}