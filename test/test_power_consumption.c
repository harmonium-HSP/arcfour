#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arcfour_power.h"
#include "arcfour.h"

#ifdef ARCFOUR_STATIC_ONLY
#define TEST_USE_STATIC_API
#endif

/* Mock power measurement */
static uint32_t mock_current_ua = 0;
static uint32_t total_current_ua = 0;
static uint32_t measurement_samples = 0;

/* Simulated power states */
#define POWER_IDLE_UA      2500    /* 2.5µA idle */
#define POWER_ACTIVE_UA    4000000 /* 4mA active */

/* Mock tick function - weak to avoid linking conflicts */
__attribute__((weak)) uint32_t arcfour_get_tick_ms(void) {
    static uint32_t counter = 0;
    return counter++;
}

/* Mock battery voltage - weak to avoid linking conflicts */
__attribute__((weak)) uint16_t arcfour_read_battery_mv(void) {
    return 3300;
}

/* Power-aware hooks for testing */
static int power_aware_before(void) {
    mock_current_ua = POWER_ACTIVE_UA;
    return 0;
}

static void power_aware_after(void) {
    mock_current_ua = POWER_IDLE_UA;
}

static int low_battery_handler(uint16_t voltage) {
    (void)voltage;
    return 1;
}

/* Measure average current */
static void measure_current(void) {
    total_current_ua += mock_current_ua;
    measurement_samples++;
}

/* Run test without power management */
void test_without_power_management(size_t iterations) {
    uint8_t key[32] = "test_power_management_key";
    uint8_t plaintext[1024];
    uint8_t ciphertext[1024];
    
    /* Initialize plaintext */
    for (size_t i = 0; i < 1024; i++) {
        plaintext[i] = (uint8_t)(i & 0xFF);
    }
    
    /* Reset measurement */
    total_current_ua = 0;
    measurement_samples = 0;
    mock_current_ua = POWER_ACTIVE_UA;  /* Always active */
    
    /* Run encryption iterations */
    for (size_t i = 0; i < iterations; i++) {
#ifdef TEST_USE_STATIC_API
        arcfour_ctx_t ctx;
        arcfour_init_static(&ctx, key, 32);
        arcfour_encrypt_static(&ctx, plaintext, ciphertext, 1024);
#else
        arcfour_ctx* ctx = arcfour_init(key, 32);
        arcfour_encrypt(ctx, plaintext, ciphertext, 1024);
        arcfour_uninit(ctx);
#endif
        
        /* Simulate some idle time between operations */
        for (int j = 0; j < 100; j++) {
            measure_current();
        }
    }
    
    /* Calculate average current */
    uint32_t avg_current_ua = total_current_ua / measurement_samples;
    printf("Without power management: %u µA average\n", avg_current_ua);
}

/* Run test with power management */
void test_with_power_management(size_t iterations) {
    uint8_t key[32] = "test_power_management_key";
    uint8_t plaintext[1024];
    uint8_t ciphertext[1024];
    
    /* Initialize plaintext */
    for (size_t i = 0; i < 1024; i++) {
        plaintext[i] = (uint8_t)(i & 0xFF);
    }
    
    /* Power hooks */
    arcfour_power_hooks_t hooks = {
        .before_operation = power_aware_before,
        .after_operation = power_aware_after,
        .on_low_battery = low_battery_handler,
        .timeout_ms = 100
    };
    
    /* Reset measurement */
    total_current_ua = 0;
    measurement_samples = 0;
    mock_current_ua = POWER_IDLE_UA;  /* Start in idle */
    
    /* Run power-aware encryption iterations */
    for (size_t i = 0; i < iterations; i++) {
#ifdef TEST_USE_STATIC_API
        arcfour_ctx_t ctx;
        arcfour_init_static(&ctx, key, 32);
        arcfour_encrypt_power_aware((arcfour_ctx*)&ctx, plaintext, ciphertext, 1024, &hooks);
#else
        arcfour_ctx* ctx = arcfour_init_power_aware(key, 32, &hooks);
        arcfour_encrypt_power_aware(ctx, plaintext, ciphertext, 1024, &hooks);
        arcfour_uninit(ctx);
#endif
        
        /* Simulate idle time between operations */
        mock_current_ua = POWER_IDLE_UA;
        for (int j = 0; j < 100; j++) {
            measure_current();
        }
    }
    
    /* Calculate average current */
    uint32_t avg_current_ua = total_current_ua / measurement_samples;
    printf("With power management:    %u µA average\n", avg_current_ua);
}

int main(void) {
    const size_t iterations = 100;
    
    printf("========== Power Consumption Test ==========\n");
    printf("Iterations: %zu\n", iterations);
    printf("Idle current: %u µA\n", POWER_IDLE_UA);
    printf("Active current: %u µA\n", POWER_ACTIVE_UA);
    printf("\n");
    
    printf("=== Test 1: Without Power Management ===\n");
    test_without_power_management(iterations);
    
    printf("\n=== Test 2: With Power Management ===\n");
    test_with_power_management(iterations);
    
    printf("\n=== Analysis ===\n");
    printf("Expected savings: ~99.9%% (when idle most of the time)\n");
    printf("Note: This is a simulation. Real hardware testing recommended.\n");
    
    return 0;
}