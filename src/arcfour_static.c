/*
 * Static memory allocation version of ARCFOUR
 * 
 * This file implements the static API which operates on user-provided
 * context structures without any heap allocation.
 * 
 * Memory layout of arcfour_ctx_t:
 * - uint8_t S[256]   : 256 bytes (S-box)
 * - uint8_t i        : 1 byte (PRGA state)
 * - uint8_t j        : 1 byte (PRGA state)
 * - uint8_t initialized : 1 byte (flag)
 * Total: 259 bytes (typically aligned to 260 or 264 bytes)
 */

#include "arcfour.h"

/* Number of initial bytes to discard for security */
/* 
 * Note: 50000000 (50 million) is recommended for high-security applications,
 * but may cause watchdog timeout on resource-constrained embedded systems.
 * Define ARCFOUR_DISCARD_BYTES at compile time to override this value.
 */
#ifdef ARCFOUR_DISCARD_BYTES
#define DISCARD_BYTES ARCFOUR_DISCARD_BYTES
#elif defined(ARCFOUR_EMBEDDED)
#define DISCARD_BYTES 256
#else
#define DISCARD_BYTES 50000000
#endif

/* Local helper functions */
static void ksa_static(uint8_t* S, const uint8_t* key, size_t key_len);
static uint8_t rc4_byte_static(uint8_t* S, uint8_t* i, uint8_t* j);
static void discard_initial_static(uint8_t* S, uint8_t* i, uint8_t* j);

/**
 * @brief Key Scheduling Algorithm (KSA) - static version
 * 
 * @param S The 256-byte S-box array
 * @param key The encryption key
 * @param key_len Key length in bytes
 */
static void ksa_static(uint8_t* S, const uint8_t* key, size_t key_len) {
    /* Initialize S-box with identity permutation */
    for (size_t i = 0; i < 256; i++) {
        S[i] = (uint8_t)i;
    }
    
    /* Randomize S-box with key */
    uint8_t j = 0;
    for (size_t i = 0; i < 256; i++) {
        j = (j + S[i] + key[i % key_len]) & 0xFF;
        
        /* Swap S[i] and S[j] */
        uint8_t tmp = S[i];
        S[i] = S[j];
        S[j] = tmp;
    }
}

/**
 * @brief Generate single RC4 keystream byte - static version
 * 
 * @param S The 256-byte S-box array
 * @param i Pointer to PRGA state variable i
 * @param j Pointer to PRGA state variable j
 * @return Next keystream byte
 */
static uint8_t rc4_byte_static(uint8_t* S, uint8_t* i, uint8_t* j) {
    *i = (*i + 1) & 0xFF;
    *j = (*j + S[*i]) & 0xFF;
    
    /* Swap S[i] and S[j] */
    uint8_t tmp = S[*i];
    S[*i] = S[*j];
    S[*j] = tmp;
    
    uint8_t t = (S[*i] + S[*j]) & 0xFF;
    return S[t];
}

/**
 * @brief Discard initial keystream bytes for security
 * 
 * @param S The 256-byte S-box array
 * @param i Pointer to PRGA state variable i
 * @param j Pointer to PRGA state variable j
 */
static void discard_initial_static(uint8_t* S, uint8_t* i, uint8_t* j) {
    for (size_t k = 0; k < DISCARD_BYTES; k++) {
        (void)rc4_byte_static(S, i, j);
    }
}

/**
 * @brief Initialize ARCFOUR context with static memory
 * 
 * @param ctx Pointer to user-allocated context structure
 * @param key The encryption key
 * @param key_len Key length in bytes
 */
void arcfour_init_static(arcfour_ctx_t* ctx, const uint8_t* key, size_t key_len) {
    if (ctx == NULL || key == NULL || key_len == 0) {
        return;
    }
    
    /* Initialize state variables */
    ctx->i = 0;
    ctx->j = 0;
    
    /* Run KSA to initialize S-box */
    ksa_static(ctx->S, key, key_len);
    
    /* Discard initial bytes for security */
    discard_initial_static(ctx->S, &ctx->i, &ctx->j);
    
    /* Mark as initialized */
    ctx->initialized = 1;
}

/**
 * @brief Uninitialize ARCFOUR context (clear sensitive data)
 * 
 * @param ctx Pointer to context structure
 */
void arcfour_uninit_static(arcfour_ctx_t* ctx) {
    if (ctx == NULL) {
        return;
    }
    
    /* Clear S-box (security best practice) */
    for (size_t i = 0; i < 256; i++) {
        ctx->S[i] = 0;
    }
    
    ctx->i = 0;
    ctx->j = 0;
    ctx->initialized = 0;
}

/**
 * @brief Encrypt/decrypt data using static context
 * 
 * @param ctx Pointer to initialized context
 * @param plaintext Input data (NULL for keystream generation only)
 * @param ciphertext Output data
 * @param len Length in bytes
 */
void arcfour_encrypt_static(arcfour_ctx_t* ctx, const uint8_t* plaintext, uint8_t* ciphertext, size_t len) {
    if (ctx == NULL || ciphertext == NULL || ctx->initialized == 0) {
        return;
    }
    
    for (size_t i = 0; i < len; i++) {
        uint8_t keystream = rc4_byte_static(ctx->S, &ctx->i, &ctx->j);
        
        if (plaintext != NULL) {
            ciphertext[i] = plaintext[i] ^ keystream;
        } else {
            ciphertext[i] = keystream;
        }
    }
}

/**
 * @brief Decrypt data using static context (same as encrypt for stream cipher)
 * 
 * @param ctx Pointer to initialized context
 * @param ciphertext Input data
 * @param plaintext Output data
 * @param len Length in bytes
 */
void arcfour_decrypt_static(arcfour_ctx_t* ctx, const uint8_t* ciphertext, uint8_t* plaintext, size_t len) {
    /* RC4 is a stream cipher - decryption is identical to encryption */
    arcfour_encrypt_static(ctx, ciphertext, plaintext, len);
}

/**
 * @brief Skip a number of keystream bytes
 * 
 * @param ctx Pointer to initialized context
 * @param n_bytes Number of bytes to skip
 */
void arcfour_skip_static(arcfour_ctx_t* ctx, size_t n_bytes) {
    if (ctx == NULL || ctx->initialized == 0) {
        return;
    }
    
    for (size_t i = 0; i < n_bytes; i++) {
        (void)rc4_byte_static(ctx->S, &ctx->i, &ctx->j);
    }
}

/**
 * @brief Reset context to initial state (reuse same key)
 * 
 * This preserves the S-box but resets i and j to 0, allowing
 * the same key to be used again without re-running KSA.
 * 
 * @param ctx Pointer to initialized context
 */
void arcfour_reset_static(arcfour_ctx_t* ctx) {
    if (ctx == NULL) {
        return;
    }
    
    /* Only reset state variables, keep S-box intact */
    ctx->i = 0;
    ctx->j = 0;
    
    /* Re-discard initial bytes */
    discard_initial_static(ctx->S, &ctx->i, &ctx->j);
}

/**
 * @brief Check if context is properly initialized
 * 
 * @param ctx Pointer to context structure
 * @return 1 if initialized, 0 otherwise
 */
int arcfour_is_valid(const arcfour_ctx_t* ctx) {
    if (ctx == NULL) {
        return 0;
    }
    return ctx->initialized;
}