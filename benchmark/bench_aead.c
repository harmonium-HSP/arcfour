#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#define GET_TIME() (GetTickCount())
#else
#include <time.h>
#define GET_TIME() ((clock() * 1000) / CLOCKS_PER_SEC)
#endif

#include "aead_rc4.h"

static void bench_encrypt(size_t size_mb) {
    size_t size = size_mb * 1024 * 1024;
    uint8_t* plaintext = malloc(size);
    uint8_t* ciphertext = malloc(size);
    uint8_t* key = malloc(32);
    uint8_t* nonce = malloc(12);
    uint8_t tag[16];
    
    if (!plaintext || !ciphertext || !key || !nonce) {
        printf("Memory allocation failed for %zu MB\n", size_mb);
        return;
    }
    
    for (size_t i = 0; i < size; i++) {
        plaintext[i] = (uint8_t)(i & 0xFF);
    }
    for (size_t i = 0; i < 32; i++) {
        key[i] = (uint8_t)(i + 1);
    }
    for (size_t i = 0; i < 12; i++) {
        nonce[i] = (uint8_t)i;
    }
    
    printf("Benchmarking %zu MB... ", size_mb);
    fflush(stdout);
    
    uint32_t start = GET_TIME();
    
    size_t cipher_len = 0;
    int ret = aead_rc4_encrypt(key, 32, nonce, 12, NULL, 0,
                               plaintext, size, ciphertext, &cipher_len, tag);
    
    uint32_t end = GET_TIME();
    uint32_t elapsed = end - start;
    
    if (ret != 0) {
        printf("FAILED\n");
        free(plaintext);
        free(ciphertext);
        free(key);
        free(nonce);
        return;
    }
    
    double speed = (double)size_mb / ((double)elapsed / 1000.0);
    printf("Time: %u ms, Speed: %.1f MB/s\n", elapsed, speed);
    
    free(plaintext);
    free(ciphertext);
    free(key);
    free(nonce);
}

int main(void) {
    printf("RC4 + Poly1305 AEAD Performance Benchmark\n");
    printf("========================================\n\n");
    
    bench_encrypt(1);
    bench_encrypt(10);
    bench_encrypt(50);
    
    printf("\nBenchmark completed.\n");
    return 0;
}