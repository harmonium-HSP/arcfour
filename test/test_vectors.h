#ifndef TEST_VECTORS_H
#define TEST_VECTORS_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    const uint8_t* key;
    size_t key_len;
    const uint8_t* plaintext;
    size_t plaintext_len;
    const uint8_t* expected_ciphertext;
} rc4_test_vector_t;

typedef struct {
    const uint8_t* key;
    size_t key_len;
    const uint8_t* nonce;
    size_t nonce_len;
    const uint8_t* aad;
    size_t aad_len;
    const uint8_t* plaintext;
    size_t plaintext_len;
    const uint8_t* expected_tag;
} aead_test_vector_t;

extern const uint8_t rc4_key1[];
extern const size_t rc4_key1_len;
extern const uint8_t rc4_plain1[];
extern const size_t rc4_plain1_len;
extern const uint8_t rc4_cipher1[];
extern const size_t rc4_cipher1_len;

extern const uint8_t rc4_key2[];
extern const size_t rc4_key2_len;
extern const uint8_t rc4_plain2[];
extern const size_t rc4_plain2_len;
extern const uint8_t rc4_cipher2[];
extern const size_t rc4_cipher2_len;

extern const uint8_t aead_key1[];
extern const size_t aead_key1_len;
extern const uint8_t aead_nonce1[];
extern const size_t aead_nonce1_len;
extern const uint8_t aead_aad1[];
extern const size_t aead_aad1_len;
extern const uint8_t aead_plain1[];
extern const size_t aead_plain1_len;
extern const uint8_t aead_tag1[];
extern const size_t aead_tag1_len;

#endif