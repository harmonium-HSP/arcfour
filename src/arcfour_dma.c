/*
 * arcfour_dma.c - DMA-friendly ARCFOUR Implementation
 * 
 * This module provides DMA-optimized encryption functions.
 */

#include "arcfour_dma.h"
#include <string.h>

/*===========================================*/
/* Internal helper functions */
/*===========================================*/

/**
 * @brief Swap two bytes
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
 * @brief 32-bit aligned XOR block operation
 * 
 * Processes data in 4-byte chunks for better performance.
 * Uses memcpy to avoid strict aliasing violations.
 */
static void xor_blocks_aligned(uint8_t* output, const uint8_t* keystream, 
                                const uint8_t* input, size_t len) {
    size_t aligned_len = len & ~3;  // Round down to multiple of 4
    
    // Process aligned portion in 32-bit chunks (using memcpy to avoid strict aliasing)
    for (size_t i = 0; i < aligned_len / 4; i++) {
        uint32_t in_val, key_val;
        memcpy(&in_val, input + i*4, 4);
        memcpy(&key_val, keystream + i*4, 4);
        uint32_t out_val = in_val ^ key_val;
        memcpy(output + i*4, &out_val, 4);
    }
    
    // Process remaining bytes
    size_t remaining = len & 3;
    if (remaining > 0) {
        uint8_t* out8 = output + aligned_len;
        const uint8_t* key8 = keystream + aligned_len;
        const uint8_t* in8 = input + aligned_len;
        for (size_t i = 0; i < remaining; i++) {
            out8[i] = in8[i] ^ key8[i];
        }
    }
}

/*===========================================*/
/* Public API implementation */
/*===========================================*/

int arcfour_encrypt_dma(arcfour_ctx_t* ctx, arcfour_dma_config_t* config) {
    if (!ctx || !config || !config->input || !config->output) {
        return -1;
    }
    
    if (!ctx->initialized) {
        return -1;
    }
    
    /* Check alignment */
    if (!arcfour_dma_is_aligned(config->input, ARCFOUR_DMA_ALIGNMENT) ||
        !arcfour_dma_is_aligned(config->output, ARCFOUR_DMA_ALIGNMENT)) {
        return -1;
    }
    
    size_t len = config->len;
    
    /* Flush input buffer cache before reading */
    ARCFOUR_CACHE_FLUSH((void*)config->input, len);
    
    if (config->use_double_buffer) {
        /* Double buffer mode: generate keystream, then XOR */
        uint8_t* keystream = arcfour_dma_get_active_buffer(
            (arcfour_dma_double_buffer_t*)config->output);
        
        /* Generate keystream */
        arcfour_prepare_keystream_dma(ctx, keystream, len);
        
        /* XOR with input */
        arcfour_xor_with_keystream(config->output, keystream, config->input, len);
    } else {
        /* Single buffer mode: encrypt directly */
        for (size_t i = 0; i < len; i++) {
            uint8_t keystream = rc4_byte(ctx);
            config->output[i] = config->input[i] ^ keystream;
        }
    }
    
    /* Invalidate output buffer cache after writing */
    ARCFOUR_CACHE_INVALIDATE(config->output, len);
    
    config->transfer_complete = 1;
    return 0;
}

int arcfour_decrypt_dma(arcfour_ctx_t* ctx, arcfour_dma_config_t* config) {
    /* RC4 decryption is identical to encryption */
    return arcfour_encrypt_dma(ctx, config);
}

size_t arcfour_prepare_keystream_dma(arcfour_ctx_t* ctx, uint8_t* keystream, size_t len) {
    if (!ctx || !keystream || !ctx->initialized) {
        return 0;
    }
    
    /* Generate keystream */
    for (size_t i = 0; i < len; i++) {
        keystream[i] = rc4_byte(ctx);
    }
    
    return len;
}

void arcfour_xor_with_keystream(uint8_t* output, const uint8_t* keystream, 
                                 const uint8_t* input, size_t len) {
    if (!output || !keystream || !input) {
        return;
    }
    
    /* Use aligned XOR if possible */
    if (arcfour_dma_is_aligned(output, 4) && 
        arcfour_dma_is_aligned(keystream, 4) && 
        arcfour_dma_is_aligned(input, 4)) {
        xor_blocks_aligned(output, keystream, input, len);
    } else {
        /* Fallback to byte-wise XOR */
        for (size_t i = 0; i < len; i++) {
            output[i] = input[i] ^ keystream[i];
        }
    }
}

void arcfour_dma_double_buffer_init(arcfour_dma_double_buffer_t* db,
                                     uint8_t* buffer0, uint8_t* buffer1, size_t size) {
    if (!db || !buffer0 || !buffer1) {
        return;
    }
    
    db->buffer0 = buffer0;
    db->buffer1 = buffer1;
    db->size = size;
    db->active_buffer = 0;
}

void arcfour_dma_double_buffer_swap(arcfour_dma_double_buffer_t* db) {
    if (!db) {
        return;
    }
    
    db->active_buffer = 1 - db->active_buffer;
}

uint8_t* arcfour_dma_get_active_buffer(arcfour_dma_double_buffer_t* db) {
    if (!db) {
        return NULL;
    }
    
    return (db->active_buffer == 0) ? db->buffer0 : db->buffer1;
}

uint8_t* arcfour_dma_get_inactive_buffer(arcfour_dma_double_buffer_t* db) {
    if (!db) {
        return NULL;
    }
    
    return (db->active_buffer == 0) ? db->buffer1 : db->buffer0;
}

/*===========================================*/
/* Aligned memory allocation */
/*===========================================*/

void* arcfour_dma_alloc_aligned(size_t size) {
#ifdef ARCFOUR_STATIC_ONLY
    (void)size;
    return NULL;  // No dynamic allocation in static-only mode
#else
    size_t alignment = ARCFOUR_DMA_ALIGNMENT;
    
    /* Allocate extra bytes to ensure we can align */
    uint8_t* ptr = (uint8_t*)malloc(size + alignment - 1 + sizeof(void*));
    if (!ptr) {
        return NULL;
    }
    
    /* Calculate aligned address */
    uintptr_t aligned_addr = ((uintptr_t)ptr + sizeof(void*) + alignment - 1) & ~(alignment - 1);
    uint8_t* aligned_ptr = (uint8_t*)aligned_addr;
    
    /* Store original pointer before aligned address for free */
    ((void**)(aligned_ptr))[-1] = ptr;
    
    return aligned_ptr;
#endif
}

void arcfour_dma_free_aligned(void* ptr) {
#ifdef ARCFOUR_STATIC_ONLY
    (void)ptr;
    return;
#else
    if (ptr) {
        /* Retrieve original pointer */
        void* original_ptr = ((void**)(ptr))[-1];
        free(original_ptr);
    }
#endif
}