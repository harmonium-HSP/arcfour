#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arcfour_power.h"
#include "arcfour.h"

#ifdef ARCFOUR_STATIC_ONLY
#define TEST_USE_STATIC_API
#endif

/* Test hooks for verification */
static int before_count = 0;
static int after_count = 0;
static int low_battery_count = 0;
static uint32_t mock_tick_ms = 0;

/* Mock tick function - static to avoid linking conflicts */
static uint32_t arcfour_get_tick_ms_impl(void) {
    return mock_tick_ms;
}

/* Mock battery voltage - static to avoid linking conflicts */
static uint16_t arcfour_read_battery_mv_impl(void) {
    return 3300;  /* 3.3V */
}

/* Weak override of platform functions for testing */
__attribute__((weak)) uint32_t arcfour_get_tick_ms(void) {
    return arcfour_get_tick_ms_impl();
}

__attribute__((weak)) uint16_t arcfour_read_battery_mv(void) {
    return arcfour_read_battery_mv_impl();
}

/* Test hooks */
static int test_before_hook(void) {
    before_count++;
    printf("  before_operation called (%d)\n", before_count);
    return 0;
}

static void test_after_hook(void) {
    after_count++;
    printf("  after_operation called (%d)\n", after_count);
}

static int test_low_battery_hook(uint16_t voltage_mv) {
    low_battery_count++;
    printf("  on_low_battery called (%d mV)\n", voltage_mv);
    return 1;  /* Continue operation */
}

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

int test_power_aware_encrypt(void) {
    uint8_t key[16] = "test_key_123456";
    uint8_t plaintext[32];
    uint8_t ciphertext[32];
    arcfour_power_hooks_t hooks = {
        .before_operation = test_before_hook,
        .after_operation = test_after_hook,
        .on_low_battery = test_low_battery_hook,
        .timeout_ms = 1000
    };
    
    printf("\n=== Test: Power-Aware Encryption ===\n");
    
    /* Initialize plaintext */
    for (int i = 0; i < 32; i++) {
        plaintext[i] = (uint8_t)i;
    }
    
    /* Create context with power awareness */
#ifdef TEST_USE_STATIC_API
    arcfour_ctx_t ctx;
    arcfour_init_static(&ctx, key, 16);
    TEST_ASSERT(ctx.initialized == 1, "Context creation");
#else
    arcfour_ctx* ctx = arcfour_init_power_aware(key, 16, &hooks);
    TEST_ASSERT(ctx != NULL, "Context creation");
#endif
    
    /* Reset counters */
    before_count = 0;
    after_count = 0;
    
    /* Perform power-aware encryption */
    int result = arcfour_encrypt_power_aware((arcfour_ctx*)&ctx, plaintext, ciphertext, 32, &hooks);
    TEST_ASSERT(result == 0, "Encryption successful");
    
    /* Verify hooks were called */
    TEST_ASSERT(before_count == 1, "Before hook called once");
    TEST_ASSERT(after_count == 1, "After hook called once");
    
    /* Verify encryption result by decrypting */
#ifdef TEST_USE_STATIC_API
    arcfour_ctx_t ctx2;
    arcfour_init_static(&ctx2, key, 16);
    uint8_t decrypted[32];
    arcfour_decrypt_static(&ctx2, ciphertext, decrypted, 32);
    TEST_ASSERT(memcmp(plaintext, decrypted, 32) == 0, "Encrypt/decrypt roundtrip");
#else
    arcfour_ctx* ctx2 = arcfour_init(key, 16);
    uint8_t decrypted[32];
    arcfour_decrypt(ctx2, ciphertext, decrypted, 32);
    TEST_ASSERT(memcmp(plaintext, decrypted, 32) == 0, "Encrypt/decrypt roundtrip");
    
    arcfour_uninit(ctx);
    arcfour_uninit(ctx2);
#endif
    
    return 0;
}

int test_auto_sleep(void) {
    arcfour_power_hooks_t hooks = {
        .before_operation = NULL,
        .after_operation = NULL,
        .on_low_battery = NULL,
        .timeout_ms = 1000  /* 1 second timeout */
    };
    
    printf("\n=== Test: Auto Sleep Detection ===\n");
    
    /* Reset mock time */
    mock_tick_ms = 0;
    arcfour_update_last_activity();
    
    /* Should not sleep immediately */
    int should_sleep = arcfour_should_sleep(&hooks);
    TEST_ASSERT(should_sleep == 0, "Should not sleep initially");
    
    /* Advance time to just before timeout */
    mock_tick_ms = 999;
    should_sleep = arcfour_should_sleep(&hooks);
    TEST_ASSERT(should_sleep == 0, "Should not sleep before timeout");
    
    /* Advance time past timeout */
    mock_tick_ms = 1000;
    should_sleep = arcfour_should_sleep(&hooks);
    TEST_ASSERT(should_sleep == 1, "Should sleep after timeout");
    
    /* Reset activity */
    arcfour_update_last_activity();
    mock_tick_ms = 1500;
    should_sleep = arcfour_should_sleep(&hooks);
    TEST_ASSERT(should_sleep == 0, "Should not sleep after activity");
    
    return 0;
}

int test_global_hooks(void) {
    printf("\n=== Test: Global Hooks ===\n");
    
    arcfour_power_hooks_t hooks = {
        .before_operation = test_before_hook,
        .after_operation = test_after_hook,
        .on_low_battery = NULL,
        .timeout_ms = 5000
    };
    
    /* Set global hooks */
    arcfour_set_global_power_hooks(&hooks);
    
    /* Verify global hooks are set */
    const arcfour_power_hooks_t* global = arcfour_get_global_power_hooks();
    TEST_ASSERT(global->before_operation == test_before_hook, "Global before hook set");
    TEST_ASSERT(global->after_operation == test_after_hook, "Global after hook set");
    TEST_ASSERT(global->timeout_ms == 5000, "Global timeout set");
    
    /* Test encryption with NULL hooks (should use global) */
    uint8_t key[16] = "global_test_key";
    uint8_t plaintext[16] = "test_data";
    uint8_t ciphertext[16];
    
#ifdef TEST_USE_STATIC_API
    arcfour_ctx_t ctx;
    arcfour_init_static(&ctx, key, 16);
#else
    arcfour_ctx* ctx = arcfour_init(key, 16);
#endif
    
    before_count = 0;
    after_count = 0;
    
    int result = arcfour_encrypt_power_aware((arcfour_ctx*)&ctx, plaintext, ciphertext, 16, NULL);
    TEST_ASSERT(result == 0, "Encryption with global hooks");
    TEST_ASSERT(before_count == 1, "Global before hook called");
    TEST_ASSERT(after_count == 1, "Global after hook called");
    
#ifndef TEST_USE_STATIC_API
    arcfour_uninit(ctx);
#endif
    
    return 0;
}

int test_battery_check(void) {
    printf("\n=== Test: Battery Check ===\n");
    
    /* Test with normal battery voltage */
    arcfour_power_hooks_t hooks = {
        .before_operation = NULL,
        .after_operation = NULL,
        .on_low_battery = test_low_battery_hook,
        .timeout_ms = 0
    };
    
    low_battery_count = 0;
    
    uint8_t key[16] = "battery_test";
    uint8_t plaintext[16] = "test";
    uint8_t ciphertext[16];
    
#ifdef TEST_USE_STATIC_API
    arcfour_ctx_t ctx;
    arcfour_init_static(&ctx, key, 16);
#else
    arcfour_ctx* ctx = arcfour_init(key, 16);
#endif
    
    int result = arcfour_encrypt_power_aware((arcfour_ctx*)&ctx, plaintext, ciphertext, 16, &hooks);
    TEST_ASSERT(result == 0, "Encryption with normal battery");
    
#ifndef TEST_USE_STATIC_API
    arcfour_uninit(ctx);
#endif
    
    return 0;
}

int main(void) {
    printf("========== Power Management Tests ==========\n");
    
    test_power_aware_encrypt();
    test_auto_sleep();
    test_global_hooks();
    test_battery_check();
    
    printf("\n========== Results: %d failures ==========\n", failures);
    
    return failures;
}