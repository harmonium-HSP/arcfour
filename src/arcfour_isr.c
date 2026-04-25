/*
 * arcfour_isr.c - Interrupt-Safe ARCFOUR Implementation
 * 
 * This module provides interrupt-safe versions of ARCFOUR functions
 * that can be safely called from Interrupt Service Routines (ISRs).
 */

#include "arcfour_isr.h"
#include "arcfour_port.h"

/* ISR nesting counter (declared in arcfour_port.h) */
#if defined(ARCFOUR_CORTEX_M4) || defined(ARCFOUR_CORTEX_M33)
volatile uint32_t arcfour_isr_nest_count = 0;
#endif

/* Pending request queue */
static volatile arcfour_pending_request_t g_pending_queue[ARCFOUR_MAX_PENDING_REQUESTS];
static volatile size_t g_queue_head = 0;
static volatile size_t g_queue_tail = 0;

/*===========================================*/
/* Internal helper functions */
/*===========================================*/

/**
 * @brief Swap two bytes in S-box
 */
static inline void swap_bytes(uint8_t* a, uint8_t* b) {
    uint8_t temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief Generate next byte of keystream
 */
static inline uint8_t rc4_byte(arcfour_ctx_t* ctx) {
    ctx->i = (ctx->i + 1) & 0xFF;
    ctx->j = (ctx->j + ctx->S[ctx->i]) & 0xFF;
    swap_bytes(&ctx->S[ctx->i], &ctx->S[ctx->j]);
    return ctx->S[(ctx->S[ctx->i] + ctx->S[ctx->j]) & 0xFF];
}

/**
 * @brief Key Scheduling Algorithm (KSA)
 */
static void ksa(arcfour_ctx_t* ctx, const uint8_t* key, size_t key_len) {
    size_t i, j;
    
    /* Initialize S-box */
    for (i = 0; i < 256; i++) {
        ctx->S[i] = (uint8_t)i;
    }
    
    /* Mix key into S-box */
    j = 0;
    for (i = 0; i < 256; i++) {
        j = (j + ctx->S[i] + key[i % key_len]) & 0xFF;
        swap_bytes(&ctx->S[i], &ctx->S[j]);
    }
    
    ctx->i = 0;
    ctx->j = 0;
}

/**
 * @brief Discard initial keystream bytes for security
 */
static void discard_initial(arcfour_ctx_t* ctx, size_t n_bytes) {
    for (size_t i = 0; i < n_bytes; i++) {
        (void)rc4_byte(ctx);
    }
}

/*===========================================*/
/* No-critical-section functions */
/*===========================================*/

void arcfour_encrypt_no_critical(arcfour_ctx_t* ctx, const uint8_t* plaintext, 
                                  uint8_t* ciphertext, size_t len) {
    if (ctx == NULL || ciphertext == NULL || !ctx->initialized) {
        return;
    }
    
    for (size_t i = 0; i < len; i++) {
        uint8_t keystream = rc4_byte(ctx);
        ciphertext[i] = (plaintext ? plaintext[i] : 0) ^ keystream;
    }
}

void arcfour_decrypt_no_critical(arcfour_ctx_t* ctx, const uint8_t* ciphertext, 
                                  uint8_t* plaintext, size_t len) {
    /* RC4 is symmetric - decryption is same as encryption */
    arcfour_encrypt_no_critical(ctx, ciphertext, plaintext, len);
}

/*===========================================*/
/* ISR-safe functions with full protection */
/*===========================================*/

void arcfour_init_isr(arcfour_ctx_t* ctx, const uint8_t* key, size_t key_len) {
    if (ctx == NULL || key == NULL || key_len == 0 || key_len > 256) {
        return;
    }
    
    ARCFOUR_ENTER_CRITICAL();
    
    ksa(ctx, key, key_len);
    
    /* Discard initial bytes for security */
#ifdef ARCFOUR_DISCARD_INITIAL
    discard_initial(ctx, 50000000);
#else
    discard_initial(ctx, 256);
#endif
    
    ctx->initialized = 1;
    
    ARCFOUR_EXIT_CRITICAL();
}

void arcfour_encrypt_isr(arcfour_ctx_t* ctx, const uint8_t* plaintext, 
                         uint8_t* ciphertext, size_t len) {
    ARCFOUR_ENTER_CRITICAL();
    arcfour_encrypt_no_critical(ctx, plaintext, ciphertext, len);
    ARCFOUR_EXIT_CRITICAL();
}

void arcfour_decrypt_isr(arcfour_ctx_t* ctx, const uint8_t* ciphertext, 
                         uint8_t* plaintext, size_t len) {
    ARCFOUR_ENTER_CRITICAL();
    arcfour_decrypt_no_critical(ctx, ciphertext, plaintext, len);
    ARCFOUR_EXIT_CRITICAL();
}

/*===========================================*/
/* Priority-based ISR-safe functions */
/*===========================================*/

#ifdef ARCFOUR_CORTEX_M4
void arcfour_encrypt_isr_priority(arcfour_ctx_t* ctx, const uint8_t* plaintext, 
                                   uint8_t* ciphertext, size_t len, uint8_t priority) {
    ARCFOUR_ENTER_CRITICAL_PRIO(priority);
    arcfour_encrypt_no_critical(ctx, plaintext, ciphertext, len);
    ARCFOUR_EXIT_CRITICAL_PRIO();
}

void arcfour_decrypt_isr_priority(arcfour_ctx_t* ctx, const uint8_t* ciphertext, 
                                   uint8_t* plaintext, size_t len, uint8_t priority) {
    ARCFOUR_ENTER_CRITICAL_PRIO(priority);
    arcfour_decrypt_no_critical(ctx, ciphertext, plaintext, len);
    ARCFOUR_EXIT_CRITICAL_PRIO();
}
#else
/* Fallback for non-Cortex-M4 platforms */
void arcfour_encrypt_isr_priority(arcfour_ctx_t* ctx, const uint8_t* plaintext, 
                                   uint8_t* ciphertext, size_t len, uint8_t priority) {
    (void)priority;
    arcfour_encrypt_isr(ctx, plaintext, ciphertext, len);
}

void arcfour_decrypt_isr_priority(arcfour_ctx_t* ctx, const uint8_t* ciphertext, 
                                   uint8_t* plaintext, size_t len, uint8_t priority) {
    (void)priority;
    arcfour_decrypt_isr(ctx, ciphertext, plaintext, len);
}
#endif

/*===========================================*/
/* Deferred request queue functions */
/*===========================================*/

int arcfour_request_encrypt(arcfour_ctx_t* ctx, const uint8_t* input, 
                            uint8_t* output, size_t length) {
    ARCFOUR_ENTER_CRITICAL();
    
    size_t current_head = g_queue_head;
    size_t current_tail = g_queue_tail;
    size_t next_head = (current_head + 1) % ARCFOUR_MAX_PENDING_REQUESTS;
    
    /* Check if queue is full */
    if (next_head == current_tail) {
        ARCFOUR_EXIT_CRITICAL();
        return -1;
    }
    
    /* Add request to queue */
    arcfour_pending_request_t req = {ctx, input, output, length};
    g_pending_queue[current_head] = req;
    
    g_queue_head = next_head;
    
    ARCFOUR_EXIT_CRITICAL();
    return 0;
}

int arcfour_request_decrypt(arcfour_ctx_t* ctx, const uint8_t* input, 
                            uint8_t* output, size_t length) {
    /* Decryption is same as encryption for RC4 */
    return arcfour_request_encrypt(ctx, input, output, length);
}

size_t arcfour_process_pending(void) {
    size_t processed = 0;
    
    while (g_queue_tail != g_queue_head) {
        /* Copy request outside critical section for efficiency */
        arcfour_pending_request_t req;
        
        ARCFOUR_ENTER_CRITICAL();
        size_t current_tail = g_queue_tail;
        req = g_pending_queue[current_tail];
        g_queue_tail = (current_tail + 1) % ARCFOUR_MAX_PENDING_REQUESTS;
        ARCFOUR_EXIT_CRITICAL();
        
        /* Process request outside critical section */
        arcfour_encrypt_no_critical(req.ctx, req.input, req.output, req.length);
        processed++;
    }
    
    return processed;
}

size_t arcfour_get_pending_count(void) {
    size_t count;
    
    ARCFOUR_ENTER_CRITICAL();
    size_t head = g_queue_head;
    size_t tail = g_queue_tail;
    
    if (head >= tail) {
        count = head - tail;
    } else {
        /* Use safe addition to prevent overflow */
        size_t part1 = ARCFOUR_MAX_PENDING_REQUESTS - tail;
        count = (part1 <= ARCFOUR_MAX_PENDING_REQUESTS - head) ? 
                (part1 + head) : ARCFOUR_MAX_PENDING_REQUESTS;
    }
    ARCFOUR_EXIT_CRITICAL();
    
    return count;
}

void arcfour_clear_pending(void) {
    ARCFOUR_ENTER_CRITICAL();
    g_queue_head = g_queue_tail = 0;
    ARCFOUR_EXIT_CRITICAL();
}