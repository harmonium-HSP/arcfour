#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arcfour.h"
#include "arcfour_arm.h"

#ifdef __ARM_ARCH_7M__
#include "stm32f4xx_hal.h"
#endif

#define TEST_SIZE_1KB    1024
#define TEST_SIZE_1MB    (1024 * 1024)
#define TEST_SIZE_10MB   (10 * 1024 * 1024)

/* Global test buffer */
static uint8_t* test_buffer = NULL;
static uint8_t test_key[32];

#ifdef __ARM_ARCH_7M__
static uint32_t get_cycle_count(void) {
    uint32_t cycles;
    __asm__ volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r"(cycles));
    return cycles;
}

static uint32_t get_tick_count(void) {
    return HAL_GetTick();
}
#else
#include <time.h>

static uint32_t get_cycle_count(void) {
    return (uint32_t)clock();
}

static uint32_t get_tick_count(void) {
    return (uint32_t)(clock() * 1000 / CLOCKS_PER_SEC);
}
#endif

void init_random_key(void) {
    for (int i = 0; i < 32; i++) {
        test_key[i] = (uint8_t)(rand() & 0xFF);
    }
}

void bench_ksa(void) {
    uint8_t S[256];
    uint32_t start, end;
    int iterations = 1000;
    
    printf("\n=== KSA Benchmark ===\n");
    
    start = get_tick_count();
    for (int i = 0; i < iterations; i++) {
        arcfour_ksa_arm(S, test_key, 32);
    }
    end = get_tick_count();
    
    uint32_t elapsed_ms = end - start;
    double avg_ms = (double)elapsed_ms / iterations;
    
    printf("Iterations: %d\n", iterations);
    printf("Total time: %lu ms\n", elapsed_ms);
    printf("Average per KSA: %.3f ms\n", avg_ms);
}

void bench_encryption(size_t size) {
    uint8_t S[256];
    uint8_t i = 0, j = 0;
    uint32_t start, end;
    
    printf("\n=== Encryption Benchmark (%zu bytes) ===\n", size);
    
    /* Allocate buffer if needed */
    if (test_buffer == NULL || size > TEST_SIZE_1KB) {
        test_buffer = realloc(test_buffer, size);
        for (size_t k = 0; k < size; k++) {
            test_buffer[k] = (uint8_t)(k & 0xFF);
        }
    }
    
    /* Initialize S-box */
    arcfour_ksa_arm(S, test_key, 32);
    
    /* Discard first 1536 bytes */
    for (int k = 0; k < 1536; k++) {
        arcfour_byte_arm(S, &i, &j);
    }
    
    /* Benchmark encryption */
    start = get_cycle_count();
    arcfour_encrypt_arm(S, &i, &j, test_buffer, test_buffer, size);
    end = get_cycle_count();
    
    uint32_t cycles = end - start;
    double cycles_per_byte = (double)cycles / size;
    double mbps = (double)size / (1024.0 * 1024.0) / ((double)(end - start) / CLOCKS_PER_SEC);
    
    printf("Cycles: %lu\n", cycles);
    printf("Cycles per byte: %.2f\n", cycles_per_byte);
    printf("Throughput: %.2f MB/s\n", mbps);
}

void bench_compare_implementations(void) {
    uint8_t S1[256], S2[256];
    uint8_t i1 = 0, j1 = 0, i2 = 0, j2 = 0;
    uint32_t start1, end1, start2, end2;
    const size_t size = TEST_SIZE_1KB;
    
    printf("\n=== Implementation Comparison (%zu bytes) ===\n", size);
    
    memcpy(S1, S2, 256);
    arcfour_ksa_arm(S1, test_key, 32);
    memcpy(S2, S1, 256);
    
    /* ARM optimized version */
    start1 = get_cycle_count();
    arcfour_encrypt_arm(S1, &i1, &j1, test_buffer, test_buffer, size);
    end1 = get_cycle_count();
    
    /* Reset for generic version */
    memcpy(S2, S1, 256);
    i2 = j2 = 0;
    
    /* Generic version */
    start2 = get_cycle_count();
    for (size_t k = 0; k < size; k++) {
        test_buffer[k] ^= arcfour_byte_arm(S2, &i2, &j2);
    }
    end2 = get_cycle_count();
    
    printf("ARM optimized: %lu cycles (%.2f cycles/byte)\n", 
           end1 - start1, (double)(end1 - start1) / size);
    printf("Byte-by-byte:  %lu cycles (%.2f cycles/byte)\n", 
           end2 - start2, (double)(end2 - start2) / size);
    printf("Speedup: %.2fx\n", (double)(end2 - start2) / (end1 - start1));
}

int main(void) {
    printf("========== ARM RC4 Benchmark ==========\n");
    
    /* Initialize */
    init_random_key();
    test_buffer = malloc(TEST_SIZE_1KB);
    
    /* Run benchmarks */
    bench_ksa();
    bench_encryption(TEST_SIZE_1KB);
    
    /* Cleanup */
    free(test_buffer);
    
    printf("\n========== Benchmark Complete ==========\n");
    
    return 0;
}