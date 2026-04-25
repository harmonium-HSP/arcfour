#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define EMSCRIPTEN_KEEPALIVE __attribute__((used))
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include <stdlib.h>
#include <string.h>
#include "arcfour.h"
#include "aead_rc4.h"

EMSCRIPTEN_KEEPALIVE
uint8_t* aead_rc4_encrypt_wasm(
    const uint8_t* key, size_t key_len,
    const uint8_t* nonce, size_t nonce_len,
    const uint8_t* aad, size_t aad_len,
    const uint8_t* plaintext, size_t plaintext_len,
    size_t* output_len
) {
    size_t total_len = plaintext_len + 16;
    uint8_t* output = (uint8_t*)malloc(total_len);
    
    if (!output) {
        *output_len = 0;
        return NULL;
    }
    
    uint8_t* ciphertext = output;
    uint8_t* tag = output + plaintext_len;
    
    size_t cipher_len = 0;
    int result = aead_rc4_encrypt(
        key, key_len,
        nonce, nonce_len,
        aad, aad_len,
        plaintext, plaintext_len,
        ciphertext, &cipher_len,
        tag
    );
    
    if (result != 0 || cipher_len != plaintext_len) {
        free(output);
        *output_len = 0;
        return NULL;
    }
    
    *output_len = total_len;
    return output;
}

EMSCRIPTEN_KEEPALIVE
uint8_t* aead_rc4_decrypt_wasm(
    const uint8_t* key, size_t key_len,
    const uint8_t* nonce, size_t nonce_len,
    const uint8_t* aad, size_t aad_len,
    const uint8_t* ciphertext, size_t ciphertext_len,
    const uint8_t* tag,
    size_t* output_len
) {
    uint8_t* plaintext = (uint8_t*)malloc(ciphertext_len);
    
    if (!plaintext) {
        *output_len = 0;
        return NULL;
    }
    
    size_t plain_len = 0;
    int result = aead_rc4_decrypt(
        key, key_len,
        nonce, nonce_len,
        aad, aad_len,
        ciphertext, ciphertext_len,
        tag,
        plaintext, &plain_len
    );
    
    if (result != 0) {
        free(plaintext);
        *output_len = 0;
        return NULL;
    }
    
    *output_len = plain_len;
    return plaintext;
}

EMSCRIPTEN_KEEPALIVE
uint8_t* arcfour_encrypt_wasm(
    const uint8_t* key, size_t key_len,
    const uint8_t* plaintext, size_t plaintext_len,
    size_t* output_len
) {
    uint8_t* ciphertext = (uint8_t*)malloc(plaintext_len);
    
    if (!ciphertext) {
        *output_len = 0;
        return NULL;
    }
    
    arcfour_ctx* ctx = arcfour_init(key, key_len);
    if (!ctx) {
        free(ciphertext);
        *output_len = 0;
        return NULL;
    }
    
    arcfour_encrypt(ctx, plaintext, ciphertext, plaintext_len);
    arcfour_uninit(ctx);
    
    *output_len = plaintext_len;
    return ciphertext;
}

EMSCRIPTEN_KEEPALIVE
void free_wasm(uint8_t* ptr) {
    free(ptr);
}

EMSCRIPTEN_KEEPALIVE
uint8_t* generate_nonce_wasm(size_t* output_len) {
    *output_len = 12;
    uint8_t* nonce = (uint8_t*)malloc(12);
    
    if (!nonce) {
        *output_len = 0;
        return NULL;
    }
    
    for (size_t i = 0; i < 12; i++) {
        nonce[i] = (uint8_t)(rand() & 0xFF);
    }
    
    return nonce;
}