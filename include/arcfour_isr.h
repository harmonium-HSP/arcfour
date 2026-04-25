/*
 * arcfour_isr.h - Interrupt-Safe ARCFOUR API
 * 
 * This module provides interrupt-safe versions of the ARCFOUR encryption
 * functions that can be safely called from Interrupt Service Routines (ISRs).
 * 
 * Key features:
 * - Critical section protection for shared context access
 * - Priority-based interrupt masking (Cortex-M4/M33)
 * - Deferred encryption queue for time-sensitive ISRs
 * - Support for nested interrupts
 * 
 * Security Warning:
 * =================
 * This is an improved RC4 implementation, but RC4 is considered weak.
 * For production use, consider AES-GCM or ChaCha20-Poly1305.
 */

#ifndef ARCFOUR_ISR_H
#define ARCFOUR_ISR_H

#include <stdint.h>
#include <stddef.h>
#include "arcfour.h"
#include "arcfour_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of pending encryption requests */
#define ARCFOUR_MAX_PENDING_REQUESTS 8

/* Pending request structure for deferred encryption */
typedef struct {
    arcfour_ctx_t* ctx;
    const uint8_t* input;
    uint8_t* output;
    size_t length;
} arcfour_pending_request_t;

/*===========================================*/
/* ISR-Safe API Functions */
/*===========================================*/

/**
 * @brief Interrupt-safe initialization
 * 
 * Initializes the ARCFOUR context with the given key, protected by
 * a critical section to prevent interrupts from corrupting the S-box.
 * 
 * @param ctx Pointer to context structure
 * @param key Encryption key
 * @param key_len Length of key (1-256 bytes)
 */
void arcfour_init_isr(arcfour_ctx_t* ctx, const uint8_t* key, size_t key_len);

/**
 * @brief Interrupt-safe encryption
 * 
 * Encrypts data with full critical section protection.
 * Can be called from ISRs or regular code.
 * 
 * @param ctx Pointer to context structure
 * @param plaintext Input data to encrypt (NULL for keystream generation)
 * @param ciphertext Output buffer for encrypted data
 * @param len Length of data to encrypt
 */
void arcfour_encrypt_isr(arcfour_ctx_t* ctx, const uint8_t* plaintext, uint8_t* ciphertext, size_t len);

/**
 * @brief Interrupt-safe decryption
 * 
 * Decrypts data with full critical section protection.
 * Can be called from ISRs or regular code.
 * 
 * @param ctx Pointer to context structure
 * @param ciphertext Input data to decrypt
 * @param plaintext Output buffer for decrypted data
 * @param len Length of data to decrypt
 */
void arcfour_decrypt_isr(arcfour_ctx_t* ctx, const uint8_t* ciphertext, uint8_t* plaintext, size_t len);

/**
 * @brief Interrupt-safe encryption with priority masking
 * 
 * Encrypts data using BASEPRI to only mask interrupts below a certain
 * priority level. Higher priority interrupts can still be serviced.
 * 
 * @param ctx Pointer to context structure
 * @param plaintext Input data to encrypt
 * @param ciphertext Output buffer for encrypted data
 * @param len Length of data to encrypt
 * @param priority Interrupt priority threshold (0-15, 0=highest)
 */
void arcfour_encrypt_isr_priority(arcfour_ctx_t* ctx, const uint8_t* plaintext, 
                                   uint8_t* ciphertext, size_t len, uint8_t priority);

/**
 * @brief Interrupt-safe decryption with priority masking
 * 
 * Decrypts data using BASEPRI to only mask interrupts below a certain
 * priority level. Higher priority interrupts can still be serviced.
 * 
 * @param ctx Pointer to context structure
 * @param ciphertext Input data to decrypt
 * @param plaintext Output buffer for decrypted data
 * @param len Length of data to decrypt
 * @param priority Interrupt priority threshold (0-15, 0=highest)
 */
void arcfour_decrypt_isr_priority(arcfour_ctx_t* ctx, const uint8_t* ciphertext, 
                                   uint8_t* plaintext, size_t len, uint8_t priority);

/**
 * @brief Request deferred encryption
 * 
 * Queues an encryption request for later processing. This is useful
 * for time-sensitive ISRs that cannot afford to perform encryption
 * immediately.
 * 
 * @param ctx Pointer to context structure
 * @param input Input data
 * @param output Output buffer
 * @param length Length of data
 * @return 0 on success, -1 if queue is full
 */
int arcfour_request_encrypt(arcfour_ctx_t* ctx, const uint8_t* input, 
                            uint8_t* output, size_t length);

/**
 * @brief Request deferred decryption
 * 
 * Queues a decryption request for later processing.
 * 
 * @param ctx Pointer to context structure
 * @param input Input data
 * @param output Output buffer
 * @param length Length of data
 * @return 0 on success, -1 if queue is full
 */
int arcfour_request_decrypt(arcfour_ctx_t* ctx, const uint8_t* input, 
                            uint8_t* output, size_t length);

/**
 * @brief Process pending encryption/decryption requests
 * 
 * Processes all queued requests. Should be called from a background
 * task or main loop.
 * 
 * @return Number of requests processed
 */
size_t arcfour_process_pending(void);

/**
 * @brief Get number of pending requests
 * 
 * @return Current number of pending requests in the queue
 */
size_t arcfour_get_pending_count(void);

/**
 * @brief Clear all pending requests
 * 
 * Discards all queued requests without processing them.
 */
void arcfour_clear_pending(void);

/*===========================================*/
/* Internal helper functions (no protection) */
/*===========================================*/

/**
 * @brief Encrypt without critical section
 * 
 * Internal function that performs encryption without any interrupt
 * protection. Caller must ensure proper synchronization.
 * 
 * @param ctx Pointer to context structure
 * @param plaintext Input data
 * @param ciphertext Output buffer
 * @param len Length of data
 */
void arcfour_encrypt_no_critical(arcfour_ctx_t* ctx, const uint8_t* plaintext, 
                                  uint8_t* ciphertext, size_t len);

/**
 * @brief Decrypt without critical section
 * 
 * Internal function that performs decryption without any interrupt
 * protection. Caller must ensure proper synchronization.
 * 
 * @param ctx Pointer to context structure
 * @param ciphertext Input data
 * @param plaintext Output buffer
 * @param len Length of data
 */
void arcfour_decrypt_no_critical(arcfour_ctx_t* ctx, const uint8_t* ciphertext, 
                                  uint8_t* plaintext, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ARCFOUR_ISR_H */