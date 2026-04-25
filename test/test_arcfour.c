#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "arcfour.h"

#define RUN_TEST(func, failed_ptr) \
    do { \
        printf("Running %s... ", #func); \
        fflush(stdout); \
        if (func() == 0) { \
            printf("PASSED\n"); \
        } else { \
            printf("FAILED\n"); \
            (*failed_ptr)++; \
        } \
    } while(0)

static int test_null_key(void) {
    arcfour_ctx* ctx = arcfour_init(NULL, 0);
    if (ctx != NULL) {
        arcfour_uninit(ctx);
        return 1;
    }
    
    const uint8_t key[10] = {0};
    ctx = arcfour_init(key, 0);
    if (ctx != NULL) {
        arcfour_uninit(ctx);
        return 1;
    }
    
    return 0;
}

static int test_empty_data(void) {
    const uint8_t key[] = "testkey";
    arcfour_ctx* ctx = arcfour_init(key, sizeof(key) - 1);
    if (!ctx) return 1;
    
    arcfour_encrypt(ctx, NULL, NULL, 0);
    arcfour_decrypt(ctx, NULL, NULL, 0);
    
    arcfour_uninit(ctx);
    return 0;
}

static int test_consistency(void) {
    const uint8_t key[] = "consistency_test";
    uint8_t original[1024];
    uint8_t encrypted[1024];
    uint8_t decrypted[1024];
    
    for (size_t i = 0; i < 1024; i++) {
        original[i] = (uint8_t)(i & 0xFF);
    }
    
    arcfour_ctx* ctx = arcfour_init(key, sizeof(key) - 1);
    if (!ctx) return 1;
    
    arcfour_encrypt(ctx, original, encrypted, 1024);
    arcfour_uninit(ctx);
    
    ctx = arcfour_init(key, sizeof(key) - 1);
    if (!ctx) return 1;
    
    arcfour_decrypt(ctx, encrypted, decrypted, 1024);
    arcfour_uninit(ctx);
    
    return memcmp(original, decrypted, 1024);
}

static int test_inplace_encryption(void) {
    const uint8_t key[] = "inplace_test";
    uint8_t data[256];
    uint8_t expected[256];
    
    for (size_t i = 0; i < 256; i++) {
        data[i] = (uint8_t)i;
        expected[i] = (uint8_t)i;
    }
    
    arcfour_ctx* ctx = arcfour_init(key, sizeof(key) - 1);
    if (!ctx) return 1;
    
    arcfour_encrypt(ctx, expected, expected, 256);
    
    arcfour_ctx* ctx2 = arcfour_init(key, sizeof(key) - 1);
    if (!ctx2) {
        arcfour_uninit(ctx);
        return 1;
    }
    
    arcfour_encrypt(ctx2, data, data, 256);
    arcfour_uninit(ctx);
    arcfour_uninit(ctx2);
    
    return memcmp(data, expected, 256);
}

static int test_copy_context(void) {
    const uint8_t key[] = "copy_test_key";
    arcfour_ctx* ctx = arcfour_init(key, sizeof(key) - 1);
    if (!ctx) return 1;
    
    arcfour_ctx* ctx_clone = arcfour_init(key, sizeof(key) - 1);
    if (!ctx_clone) {
        arcfour_uninit(ctx);
        return 1;
    }
    
    arcfour_copy(ctx_clone, ctx);
    
    uint8_t stream1[100];
    uint8_t stream2[100];
    
    for (size_t i = 0; i < 100; i++) {
        uint8_t temp = 0;
        arcfour_encrypt(ctx, &temp, stream1 + i, 1);
        arcfour_encrypt(ctx_clone, &temp, stream2 + i, 1);
    }
    
    arcfour_uninit(ctx);
    arcfour_uninit(ctx_clone);
    
    return memcmp(stream1, stream2, 100);
}

static int test_key_derivation(void) {
    const char* password = "test_password_123";
    const uint8_t salt[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    const char* message = "test message";
    uint8_t encrypted1[100];
    uint8_t encrypted2[100];
    
    arcfour_ctx* ctx1 = NULL;
    if (arcfour_key_setup(&ctx1, (const uint8_t*)password, strlen(password),
                         salt, sizeof(salt), 1000) != 0) {
        return 1;
    }
    
    arcfour_encrypt(ctx1, (const uint8_t*)message, encrypted1, strlen(message));
    arcfour_uninit(ctx1);
    
    arcfour_ctx* ctx2 = NULL;
    if (arcfour_key_setup(&ctx2, (const uint8_t*)password, strlen(password),
                         salt, sizeof(salt), 1000) != 0) {
        return 1;
    }
    
    arcfour_encrypt(ctx2, (const uint8_t*)message, encrypted2, strlen(message));
    arcfour_uninit(ctx2);
    
    return memcmp(encrypted1, encrypted2, strlen(message));
}

static int test_skip_bytes(void) {
    const uint8_t key[] = "skip_test_key";
    uint8_t stream1[10];
    uint8_t stream2[10];
    
    arcfour_ctx* ctx = arcfour_init(key, sizeof(key) - 1);
    if (!ctx) return 1;
    
    arcfour_skip(ctx, 1000);
    
    for (size_t i = 0; i < 10; i++) {
        uint8_t temp = 0;
        arcfour_encrypt(ctx, &temp, stream1 + i, 1);
    }
    arcfour_uninit(ctx);
    
    ctx = arcfour_init(key, sizeof(key) - 1);
    if (!ctx) return 1;
    
    arcfour_skip(ctx, 1000);
    
    for (size_t i = 0; i < 10; i++) {
        uint8_t temp = 0;
        arcfour_encrypt(ctx, &temp, stream2 + i, 1);
    }
    arcfour_uninit(ctx);
    
    return memcmp(stream1, stream2, 10);
}

static int test_large_data(void) {
    const uint8_t key[] = "large_data_test";
    size_t size = 1024 * 1024;
    uint8_t* original = malloc(size);
    uint8_t* encrypted = malloc(size);
    uint8_t* decrypted = malloc(size);
    
    if (!original || !encrypted || !decrypted) {
        free(original);
        free(encrypted);
        free(decrypted);
        return 1;
    }
    
    for (size_t i = 0; i < size; i++) {
        original[i] = (uint8_t)(i & 0xFF);
    }
    
    arcfour_ctx* ctx = arcfour_init(key, sizeof(key) - 1);
    if (!ctx) {
        free(original);
        free(encrypted);
        free(decrypted);
        return 1;
    }
    
    arcfour_encrypt(ctx, original, encrypted, size);
    arcfour_uninit(ctx);
    
    ctx = arcfour_init(key, sizeof(key) - 1);
    if (!ctx) {
        free(original);
        free(encrypted);
        free(decrypted);
        return 1;
    }
    
    arcfour_decrypt(ctx, encrypted, decrypted, size);
    arcfour_uninit(ctx);
    
    int result = memcmp(original, decrypted, size);
    
    free(original);
    free(encrypted);
    free(decrypted);
    
    return result;
}

static int test_known_vectors(void) {
    const uint8_t key[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    const uint8_t plaintext[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint8_t expected[] = {0x15, 0x51, 0x4E, 0x80, 0x9C, 0xBA, 0xF7, 0xBB};
    
    arcfour_ctx* ctx = arcfour_init(key, sizeof(key));
    if (!ctx) return 1;
    
    uint8_t result[8];
    arcfour_encrypt(ctx, plaintext, result, 8);
    arcfour_uninit(ctx);
    
    return memcmp(result, expected, 8);
}

int main(void) {
    int failed = 0;
    
    printf("=== ARCFOUR Unit Tests ===\n\n");
    
    RUN_TEST(test_null_key, &failed);
    RUN_TEST(test_empty_data, &failed);
    RUN_TEST(test_consistency, &failed);
    RUN_TEST(test_inplace_encryption, &failed);
    RUN_TEST(test_copy_context, &failed);
    RUN_TEST(test_key_derivation, &failed);
    RUN_TEST(test_skip_bytes, &failed);
    RUN_TEST(test_large_data, &failed);
    RUN_TEST(test_known_vectors, &failed);
    
    printf("\n=== Results: %d/%d tests passed ===\n", 9 - failed, 9);
    
    return failed ? 1 : 0;
}