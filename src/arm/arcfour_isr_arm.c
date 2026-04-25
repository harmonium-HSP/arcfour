/*
 * arcfour_isr_arm.c - ARM Cortex-M specific ISR optimizations
 * 
 * This file contains ARM Cortex-M specific optimizations for interrupt-safe
 * ARCFOUR encryption, including:
 * - Priority-based interrupt masking using BASEPRI
 * - DWT-based cycle counting for timeout detection
 * - Thumb-2 optimized encryption loops
 */

#include "arcfour_isr.h"
#include "arcfour_port.h"

#if defined(ARCFOUR_CORTEX_M4) || defined(ARCFOUR_CORTEX_M33)

/* DWT cycle counter for timeout detection */
#define DWT_CTRL    (*((volatile uint32_t*)0xE0001000))
#define DWT_CYCCNT  (*((volatile uint32_t*)0xE0001004))
#define DEMCR       (*((volatile uint32_t*)0xE000EDFC))

/**
 * @brief Enable DWT cycle counter
 */
void arcfour_dwt_enable(void) {
    /* Enable DWT module */
    DEMCR |= (1 << 24);
    /* Enable cycle counter */
    DWT_CTRL |= (1 << 0);
}

/**
 * @brief Get current cycle count
 */
static inline uint32_t arcfour_get_cycles(void) {
    return DWT_CYCCNT;
}

/**
 * @brief Set interrupt priority mask using BASEPRI
 * 
 * @param priority Priority threshold (0-15, 0=highest)
 */
void arcfour_set_interrupt_priority(uint8_t priority) {
    uint32_t basepri = priority << 4;
    __asm__ __volatile__ ("msr BASEPRI, %0" : : "r"(basepri) : "memory");
}

/**
 * @brief Clear interrupt priority mask
 */
void arcfour_clear_interrupt_priority(void) {
    __asm__ __volatile__ ("msr BASEPRI, #0" : : : "memory");
}

/**
 * @brief Encrypt with timeout detection
 * 
 * Performs encryption with cycle-based timeout detection to prevent
 * deadlock in case of unexpected conditions.
 * 
 * @param ctx Context pointer
 * @param plaintext Input data
 * @param ciphertext Output data
 * @param len Length of data
 * @param max_cycles Maximum cycles to spend (0 = no limit)
 * @return 0 on success, -1 on timeout
 */
int arcfour_encrypt_isr_timeout(arcfour_ctx_t* ctx, const uint8_t* plaintext,
                                 uint8_t* ciphertext, size_t len, uint32_t max_cycles) {
    uint32_t start_cycles = 0;
    uint32_t cycles_elapsed = 0;
    
    if (max_cycles > 0) {
        start_cycles = arcfour_get_cycles();
    }
    
    ARCFOUR_ENTER_CRITICAL();
    
    for (size_t i = 0; i < len; i++) {
        /* Check timeout if enabled */
        if (max_cycles > 0) {
            cycles_elapsed = arcfour_get_cycles() - start_cycles;
            if (cycles_elapsed > max_cycles) {
                ARCFOUR_EXIT_CRITICAL();
                return -1;
            }
        }
        
        /* RC4 encryption */
        ctx->i = (ctx->i + 1) & 0xFF;
        ctx->j = (ctx->j + ctx->S[ctx->i]) & 0xFF;
        
        /* Swap using inline asm for speed */
        __asm__ __volatile__ (
            "ldrb r3, [%2, %0]\n\t"
            "ldrb r4, [%2, %1]\n\t"
            "strb r4, [%2, %0]\n\t"
            "strb r3, [%2, %1]"
            :
            : "r"(ctx->i), "r"(ctx->j), "r"(ctx->S)
            : "r3", "r4", "memory"
        );
        
        uint8_t s_i = ctx->S[ctx->i];
        uint8_t s_j = ctx->S[ctx->j];
        uint8_t keystream = ctx->S[(s_i + s_j) & 0xFF];
        
        ciphertext[i] = (plaintext ? plaintext[i] : 0) ^ keystream;
    }
    
    ARCFOUR_EXIT_CRITICAL();
    return 0;
}

/**
 * @brief Encrypt using priority-based interrupt masking
 * 
 * Uses BASEPRI to only mask interrupts below the specified priority,
 * allowing higher-priority interrupts to continue being serviced.
 * 
 * @param ctx Context pointer
 * @param plaintext Input data
 * @param ciphertext Output data
 * @param len Length of data
 * @param priority Priority threshold (0-15, 0=highest)
 */
void arcfour_encrypt_isr_priority(arcfour_ctx_t* ctx, const uint8_t* plaintext,
                                   uint8_t* ciphertext, size_t len, uint8_t priority) {
    uint32_t old_basepri;
    
    /* Save current BASEPRI and set new priority */
    __asm__ __volatile__ ("mrs %0, BASEPRI" : "=r"(old_basepri));
    arcfour_set_interrupt_priority(priority);
    
    /* Perform encryption */
    arcfour_encrypt_no_critical(ctx, plaintext, ciphertext, len);
    
    /* Restore BASEPRI */
    __asm__ __volatile__ ("msr BASEPRI, %0" : : "r"(old_basepri) : "memory");
}

/**
 * @brief Decrypt using priority-based interrupt masking
 */
void arcfour_decrypt_isr_priority(arcfour_ctx_t* ctx, const uint8_t* ciphertext,
                                   uint8_t* plaintext, size_t len, uint8_t priority) {
    /* RC4 decryption is same as encryption */
    arcfour_encrypt_isr_priority(ctx, ciphertext, plaintext, len, priority);
}

/**
 * @brief Fast single-byte encryption for ISR use
 * 
 * Optimized for encrypting single bytes quickly in ISRs where
 * minimal latency is critical.
 * 
 * @param ctx Context pointer
 * @param input Input byte
 * @return Encrypted byte
 */
uint8_t arcfour_encrypt_byte_isr(arcfour_ctx_t* ctx, uint8_t input) {
    uint8_t result;
    
    ARCFOUR_ENTER_CRITICAL();
    
    ctx->i = (ctx->i + 1) & 0xFF;
    ctx->j = (ctx->j + ctx->S[ctx->i]) & 0xFF;
    
    /* Fast swap */
    uint8_t temp = ctx->S[ctx->i];
    ctx->S[ctx->i] = ctx->S[ctx->j];
    ctx->S[ctx->j] = temp;
    
    uint8_t s_i = ctx->S[ctx->i];
    uint8_t s_j = ctx->S[ctx->j];
    uint8_t keystream = ctx->S[(s_i + s_j) & 0xFF];
    
    result = input ^ keystream;
    
    ARCFOUR_EXIT_CRITICAL();
    
    return result;
}

#endif /* ARCFOUR_CORTEX_M4 || ARCFOUR_CORTEX_M33 */