#include <string.h>
#include "arcfour_port.h"
#include "arcfour.h"

/* Include test vectors only for self-test */
#ifdef ARCFOUR_SELF_TEST
#include "../test/test_vectors.h"
#endif

/* Platform-specific implementations */
#if defined(ARCFOUR_CORTEX_M4) || defined(ARCFOUR_CORTEX_M33) || defined(ARCFOUR_ARM_V7A) || defined(ARCFOUR_USE_ARM_OPT)
#  include "arcfour_arm.h"
#  if !defined(ARCFOUR_USE_ARM_OPT)
#    define ARCFOUR_USE_ARM_OPT 1
#  endif
#else
#  define ARCFOUR_USE_GENERIC 1
#endif

/*===========================================*/
/* Dynamic API implementations (malloc/free) */
/*===========================================*/

#ifndef ARCFOUR_STATIC_ONLY

static void simple_hash(const uint8_t* input, size_t len, uint8_t output[32]) {
    for (size_t i = 0; i < 32; i++) {
        output[i] = (uint8_t)(0x7F ^ i);
    }
    
    for (size_t i = 0; i < len; i++) {
        size_t idx = i % 32;
        output[idx] ^= input[i];
        output[(idx + 1) % 32] += input[i];
        output[(idx + 5) % 32] ^= input[i] << 1;
    }
    
    for (size_t i = 0; i < 32; i++) {
        output[i] = (output[i] * 0x9E3779B9) >> 24;
    }
    
    for (size_t i = 0; i < 32; i++) {
        output[i] ^= output[(i + 17) % 32];
    }
}

arcfour_ctx* arcfour_init(const uint8_t* key, size_t key_len) {
    if (key == NULL || key_len == 0) {
        return NULL;
    }
    
    /* Use ARCFOUR_MALLOC macro for memory allocation */
    arcfour_ctx* ctx = (arcfour_ctx*)ARCFOUR_MALLOC(sizeof(struct arcfour));
    if (ctx == NULL) {
        return NULL;
    }
    
    /* Reuse static initialization */
    arcfour_init_static(ctx, key, key_len);
    
    return ctx;
}

void arcfour_uninit(arcfour_ctx* ctx) {
    if (ctx != NULL) {
        /* Clear sensitive data first */
        arcfour_uninit_static(ctx);
        /* Use ARCFOUR_FREE macro for memory deallocation */
        ARCFOUR_FREE(ctx);
    }
}

void arcfour_encrypt(arcfour_ctx* ctx, const uint8_t* plaintext, uint8_t* ciphertext, size_t len) {
    arcfour_encrypt_static((arcfour_ctx_t*)ctx, plaintext, ciphertext, len);
}

void arcfour_decrypt(arcfour_ctx* ctx, const uint8_t* ciphertext, uint8_t* plaintext, size_t len) {
    arcfour_decrypt_static((arcfour_ctx_t*)ctx, ciphertext, plaintext, len);
}

void arcfour_skip(arcfour_ctx* ctx, size_t n_bytes) {
    arcfour_skip_static((arcfour_ctx_t*)ctx, n_bytes);
}

int arcfour_key_setup(arcfour_ctx** ctx, const uint8_t* password, size_t pass_len,
                      const uint8_t* salt, size_t salt_len, unsigned int iterations) {
    if (ctx == NULL || password == NULL || pass_len == 0 || 
        salt == NULL || salt_len < 8 || iterations == 0) {
        return -1;
    }
    
    uint8_t derived_key[32];
    uint8_t* temp_buf = (uint8_t*)ARCFOUR_MALLOC(pass_len + salt_len + 32);
    if (temp_buf == NULL) {
        return -1;
    }
    
    memcpy(temp_buf, password, pass_len);
    memcpy(temp_buf + pass_len, salt, salt_len);
    simple_hash(temp_buf, pass_len + salt_len, derived_key);
    
    for (unsigned int i = 1; i < iterations; i++) {
        memcpy(temp_buf, derived_key, 32);
        memcpy(temp_buf + 32, password, pass_len);
        memcpy(temp_buf + 32 + pass_len, salt, salt_len);
        simple_hash(temp_buf, 32 + pass_len + salt_len, derived_key);
    }
    
    *ctx = arcfour_init(derived_key, 32);
    
    /* Clear sensitive data */
    volatile uint8_t* p = derived_key;
    for (size_t i = 0; i < 32; i++) {
        p[i] = 0;
    }
    ARCFOUR_FREE(temp_buf);
    
    return (*ctx != NULL) ? 0 : -1;
}

void arcfour_copy(arcfour_ctx* dest, const arcfour_ctx* src) {
    if (dest == NULL || src == NULL) {
        return;
    }
    memcpy(dest, src, sizeof(struct arcfour));
}

#endif /* ARCFOUR_STATIC_ONLY */

/*===========================================*/
/* Self-test implementation */
/*===========================================*/

#ifdef ARCFOUR_SELF_TEST
#include <stdio.h>

static int run_single_test(const rc4_test_vector_t* tv, int test_num) {
    arcfour_ctx_t ctx;
    arcfour_init_static(&ctx, tv->key, tv->key_len);
    
    uint8_t result[tv->plaintext_len];
    arcfour_encrypt_static(&ctx, tv->plaintext, result, tv->plaintext_len);
    arcfour_uninit_static(&ctx);
    
    if (memcmp(result, tv->expected_ciphertext, tv->plaintext_len) != 0) {
        printf("Test %d: FAILED (ciphertext mismatch)\n", test_num);
        return 1;
    }
    
    printf("Test %d: PASSED\n", test_num);
    return 0;
}

static int test_consistency(void) {
    const uint8_t key[] = "consistency_test_key";
    uint8_t original[1024];
    uint8_t encrypted[1024];
    uint8_t decrypted[1024];
    
    for (size_t i = 0; i < 1024; i++) {
        original[i] = (uint8_t)(i & 0xFF);
    }
    
    arcfour_ctx_t ctx;
    arcfour_init_static(&ctx, key, sizeof(key) - 1);
    arcfour_encrypt_static(&ctx, original, encrypted, 1024);
    arcfour_uninit_static(&ctx);
    
    arcfour_ctx_t ctx2;
    arcfour_init_static(&ctx2, key, sizeof(key) - 1);
    arcfour_decrypt_static(&ctx2, encrypted, decrypted, 1024);
    arcfour_uninit_static(&ctx2);
    
    if (memcmp(original, decrypted, 1024) != 0) {
        printf("Consistency test: FAILED (encrypt/decrypt mismatch)\n");
        return 1;
    }
    
    printf("Consistency test: PASSED\n");
    return 0;
}

static int test_in_place_encrypt(void) {
    const uint8_t key[] = "inplace_key";
    uint8_t buffer[256];
    uint8_t expected[256];
    
    for (size_t i = 0; i < 256; i++) {
        buffer[i] = (uint8_t)i;
    }
    
    arcfour_ctx_t ctx;
    arcfour_init_static(&ctx, key, sizeof(key) - 1);
    
    memcpy(expected, buffer, 256);
    arcfour_encrypt_static(&ctx, expected, expected, 256);
    
    arcfour_ctx_t ctx2;
    arcfour_init_static(&ctx2, key, sizeof(key) - 1);
    
    arcfour_encrypt_static(&ctx2, buffer, buffer, 256);
    arcfour_uninit_static(&ctx);
    arcfour_uninit_static(&ctx2);
    
    if (memcmp(buffer, expected, 256) != 0) {
        printf("In-place test: FAILED (in-place != separate)\n");
        return 1;
    }
    
    printf("In-place test: PASSED\n");
    return 0;
}

static int test_edge_cases(void) {
    const uint8_t key[] = "edge_case_key";
    uint8_t data[10] = {0x00};
    
    arcfour_ctx_t ctx;
    arcfour_init_static(&ctx, key, sizeof(key) - 1);
    
    arcfour_encrypt_static(&ctx, NULL, NULL, 0);
    arcfour_encrypt_static(&ctx, data, NULL, 0);
    arcfour_encrypt_static(NULL, data, data, 10);
    
    arcfour_uninit_static(NULL);
    
    /* Test null key behavior */
    arcfour_ctx_t bad_ctx = {0};
    arcfour_init_static(&bad_ctx, NULL, 0);
    if (arcfour_is_valid(&bad_ctx)) {
        printf("Edge case test: FAILED (NULL key should not initialize)\n");
        return 1;
    }
    
    printf("Edge case test: PASSED\n");
    return 0;
}

int arcfour_self_test(void) {
    int failures = 0;
    
    printf("Running ARCFOUR self-tests...\n\n");
    
    rc4_test_vector_t tv1 = {
        .key = rc4_key1,
        .key_len = rc4_key1_len,
        .plaintext = rc4_plain1,
        .plaintext_len = rc4_plain1_len,
        .expected_ciphertext = rc4_cipher1
    };
    failures += run_single_test(&tv1, 1);
    
    rc4_test_vector_t tv2 = {
        .key = rc4_key2,
        .key_len = rc4_key2_len,
        .plaintext = rc4_plain2,
        .plaintext_len = rc4_plain2_len,
        .expected_ciphertext = rc4_cipher2
    };
    failures += run_single_test(&tv2, 2);
    
    failures += test_consistency();
    failures += test_in_place_encrypt();
    failures += test_edge_cases();
    
    printf("\n");
    if (failures == 0) {
        printf("All self-tests passed.\n");
    } else {
        printf("Self-tests failed: %d failures.\n", failures);
    }
    
    return failures;
}

int main(void) {
    return arcfour_self_test();
}
#endif