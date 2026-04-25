#ifndef AEAD_RC4_H
#define AEAD_RC4_H

#include <stddef.h>
#include <stdint.h>

#ifdef AEAD_RC4_STATIC
#  define AEAD_RC4_API
#elif defined(_WIN32)
#  ifdef AEAD_RC4_EXPORTS
#    define AEAD_RC4_API __declspec(dllexport)
#  else
#    define AEAD_RC4_API __declspec(dllimport)
#  endif
#else
#  define AEAD_RC4_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AEAD_RC4_TAG_SIZE 16

AEAD_RC4_API int aead_rc4_encrypt(
    const uint8_t* key, size_t key_len,
    const uint8_t* nonce, size_t nonce_len,
    const uint8_t* aad, size_t aad_len,
    const uint8_t* plaintext, size_t plaintext_len,
    uint8_t* ciphertext, size_t* ciphertext_len,
    uint8_t* tag
);

AEAD_RC4_API int aead_rc4_decrypt(
    const uint8_t* key, size_t key_len,
    const uint8_t* nonce, size_t nonce_len,
    const uint8_t* aad, size_t aad_len,
    const uint8_t* ciphertext, size_t ciphertext_len,
    const uint8_t* tag,
    uint8_t* plaintext, size_t* plaintext_len
);

AEAD_RC4_API int aead_secure_memcmp(const uint8_t* a, const uint8_t* b, size_t len);

#ifdef __cplusplus
}
#endif

#endif
