/*
 * arcfour_dma.h - DMA-friendly ARCFOUR API
 * 
 * This module provides DMA-optimized encryption functions that support:
 * - Buffer alignment for DMA transfers
 * - Cache coherence management
 * - Double buffering for pipelined processing
 * - Keystream pre-generation for DMA XOR operations
 * 
 * Security Warning:
 * =================
 * This is an improved RC4 implementation, but RC4 is considered weak.
 * For production use, consider AES-GCM or ChaCha20-Poly1305.
 */

#ifndef ARCFOUR_DMA_H
#define ARCFOUR_DMA_H

#include <stdint.h>
#include <stddef.h>
#include "arcfour.h"
#include "arcfour_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
typedef struct arcfour_dma_config arcfour_dma_config_t;

/* DMA encryption configuration structure */
struct arcfour_dma_config {
    const uint8_t* input;       /* Input buffer (must be DMA-aligned) */
    uint8_t* output;            /* Output buffer (must be DMA-aligned) */
    size_t len;                 /* Length of data to process */
    uint8_t use_double_buffer;  /* Flag: use double buffering */
    volatile uint8_t transfer_complete;  /* Transfer completion flag */
};

/*===========================================*/
/* DMA API Functions */
/*===========================================*/

/**
 * @brief Check if pointer is properly aligned for DMA
 * 
 * @param ptr Pointer to check
 * @param alignment Required alignment (typically 4 or 8)
 * @return 1 if aligned, 0 otherwise
 */
ARCFOUR_INLINE int arcfour_dma_is_aligned(const void* ptr, size_t alignment) {
    return ((uintptr_t)ptr & (alignment - 1)) == 0;
}

/**
 * @brief DMA-friendly encryption
 * 
 * Performs encryption with proper buffer alignment checking and cache management.
 * 
 * @param ctx ARCFOUR context
 * @param config DMA configuration
 * @return 0 on success, -1 on alignment error
 */
int arcfour_encrypt_dma(arcfour_ctx_t* ctx, arcfour_dma_config_t* config);

/**
 * @brief DMA-friendly decryption (same as encryption for RC4)
 */
int arcfour_decrypt_dma(arcfour_ctx_t* ctx, arcfour_dma_config_t* config);

/**
 * @brief Pre-generate keystream for DMA XOR operations
 * 
 * Generates keystream into a buffer that can be used for DMA-based XOR.
 * This allows the DMA controller to perform the actual XOR operation
 * without CPU intervention.
 * 
 * @param ctx ARCFOUR context
 * @param keystream Output buffer for keystream (must be DMA-aligned)
 * @param len Length of keystream to generate
 * @return Number of bytes generated
 */
size_t arcfour_prepare_keystream_dma(arcfour_ctx_t* ctx, uint8_t* keystream, size_t len);

/**
 * @brief Perform XOR with pre-generated keystream
 * 
 * This is a pure XOR operation that can be safely called from DMA interrupts.
 * The actual encryption work was done by arcfour_prepare_keystream_dma().
 * 
 * @param output Output buffer (can be same as input for in-place)
 * @param keystream Pre-generated keystream
 * @param input Input data
 * @param len Length of data
 */
void arcfour_xor_with_keystream(uint8_t* output, const uint8_t* keystream, 
                                 const uint8_t* input, size_t len);

/*===========================================*/
/* Double Buffer API */
/*===========================================*/

/**
 * @brief Initialize double buffer structure
 * 
 * @param db Double buffer structure to initialize
 * @param buffer0 First buffer pointer (DMA-aligned)
 * @param buffer1 Second buffer pointer (DMA-aligned)
 * @param size Size of each buffer
 */
void arcfour_dma_double_buffer_init(arcfour_dma_double_buffer_t* db,
                                     uint8_t* buffer0, uint8_t* buffer1, size_t size);

/**
 * @brief Swap active buffer in double buffer setup
 * 
 * @param db Double buffer structure
 */
void arcfour_dma_double_buffer_swap(arcfour_dma_double_buffer_t* db);

/**
 * @brief Get pointer to currently active buffer
 * 
 * @param db Double buffer structure
 * @return Pointer to active buffer
 */
uint8_t* arcfour_dma_get_active_buffer(arcfour_dma_double_buffer_t* db);

/**
 * @brief Get pointer to inactive buffer (for DMA transfer)
 * 
 * @param db Double buffer structure
 * @return Pointer to inactive buffer
 */
uint8_t* arcfour_dma_get_inactive_buffer(arcfour_dma_double_buffer_t* db);

/*===========================================*/
/* Aligned buffer allocation helpers */
/*===========================================*/

/**
 * @brief Allocate DMA-aligned memory
 * 
 * @param size Size of memory to allocate
 * @return Pointer to aligned memory, NULL on failure
 */
void* arcfour_dma_alloc_aligned(size_t size);

/**
 * @brief Free DMA-aligned memory
 * 
 * @param ptr Memory pointer to free
 */
void arcfour_dma_free_aligned(void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* ARCFOUR_DMA_H */