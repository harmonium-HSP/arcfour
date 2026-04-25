/*
 * SECURITY WARNING:
 * =================
 * 1. This library implements an improved version of RC4 (skipping first 50 million bytes).
 *    However, RC4 is considered cryptographically weak by modern standards.
 * 2. For production use, consider AES-GCM, ChaCha20-Poly1305, or other modern AEAD algorithms.
 * 3. This library is for educational purposes only - learning C and stream cipher principles.
 * 4. If you must use this, ensure keys are >= 256-bit (32 bytes) cryptographically random data.
 * 5. Never use this with related keys (e.g., incrementing counters).
 */

#ifndef ARCFOUR_H
#define ARCFOUR_H

#include <stdint.h>
#include <stddef.h>

#ifdef ARCFOUR_STATIC
#  define ARCFOUR_API
#elif defined(_WIN32) || defined(_WIN64)
#  ifdef ARCFOUR_EXPORTS
#    define ARCFOUR_API __declspec(dllexport)
#  else
#    define ARCFOUR_API __declspec(dllimport)
#  endif
#elif __GNUC__ >= 4
#  define ARCFOUR_API __attribute__((visibility("default")))
#else
#  define ARCFOUR_API
#endif

/* Forward declaration for dynamic API */
typedef struct arcfour arcfour_ctx;

/* Static context structure - exposed for static allocation */
struct arcfour {
    uint8_t S[256];       /* S-box state */
    uint8_t i;            /* PRGA state variable */
    uint8_t j;            /* PRGA state variable */
    uint8_t initialized;  /* Initialization flag (0 = uninitialized, 1 = initialized) */
};

/* Size of arcfour context structure (compile-time constant) */
#define ARCFOUR_CONTEXT_SIZE  (sizeof(struct arcfour))

/* Type alias for static allocation */
typedef struct arcfour arcfour_ctx_t;

/*===========================================*/
/* Dynamic API (uses malloc/free) */
/*===========================================*/
#ifndef ARCFOUR_STATIC_ONLY
ARCFOUR_API arcfour_ctx* arcfour_init(const uint8_t* key, size_t key_len);
ARCFOUR_API void arcfour_uninit(arcfour_ctx* ctx);
#endif

/*===========================================*/
/* Static API (no heap allocation) */
/*===========================================*/
ARCFOUR_API void arcfour_init_static(arcfour_ctx_t* ctx, const uint8_t* key, size_t key_len);
ARCFOUR_API void arcfour_uninit_static(arcfour_ctx_t* ctx);
ARCFOUR_API void arcfour_encrypt_static(arcfour_ctx_t* ctx, const uint8_t* plaintext, uint8_t* ciphertext, size_t len);
ARCFOUR_API void arcfour_decrypt_static(arcfour_ctx_t* ctx, const uint8_t* ciphertext, uint8_t* plaintext, size_t len);
ARCFOUR_API void arcfour_skip_static(arcfour_ctx_t* ctx, size_t n_bytes);
ARCFOUR_API void arcfour_reset_static(arcfour_ctx_t* ctx);
ARCFOUR_API int arcfour_is_valid(const arcfour_ctx_t* ctx);

/*===========================================*/
/* Unified API (works with both static and dynamic) */
/*===========================================*/
#ifndef ARCFOUR_STATIC_ONLY
ARCFOUR_API void arcfour_encrypt(arcfour_ctx* ctx, const uint8_t* plaintext, uint8_t* ciphertext, size_t len);
ARCFOUR_API void arcfour_decrypt(arcfour_ctx* ctx, const uint8_t* ciphertext, uint8_t* plaintext, size_t len);
ARCFOUR_API void arcfour_skip(arcfour_ctx* ctx, size_t n_bytes);
ARCFOUR_API void arcfour_copy(arcfour_ctx* dest, const arcfour_ctx* src);
ARCFOUR_API int arcfour_key_setup(arcfour_ctx** ctx, const uint8_t* password, size_t pass_len,
                      const uint8_t* salt, size_t salt_len, unsigned int iterations);
#endif

ARCFOUR_API int arcfour_self_test(void);

#ifdef ARCFOUR_ENABLE_POWER_API
#include "arcfour_power.h"
#endif

#ifdef ARCFOUR_ENABLE_ISR_API
#include "arcfour_isr.h"
#endif

#endif