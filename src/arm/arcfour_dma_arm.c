/*
 * arcfour_dma_arm.c - ARM Cortex-M DMA-specific optimizations
 * 
 * This file contains STM32-specific DMA integration code.
 */

#include "arcfour_dma.h"

#ifdef ARCFOUR_CORTEX_M4

/*===========================================*/
/* STM32 HAL-based DMA encryption */
/*===========================================*/

/* Forward declarations for STM32 HAL */
#ifdef STM32_HAL_SUPPORT
#include "stm32f4xx_hal.h"

extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;

extern arcfour_ctx_t g_uart_crypto_ctx;
extern arcfour_dma_double_buffer_t g_uart_dma_buffer;
extern volatile uint8_t g_uart_dma_transfer_complete;

/**
 * @brief DMA transfer complete interrupt handler for UART
 * 
 * This handler is called when DMA finishes transferring data from UART.
 * It performs encryption on the received data and prepares for the next transfer.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        uint8_t* received_buffer = arcfour_dma_get_active_buffer(&g_uart_dma_buffer);
        
        /* Decrypt the received data in-place */
        arcfour_dma_config_t config = {
            .input = received_buffer,
            .output = received_buffer,
            .len = g_uart_dma_buffer.size,
            .use_double_buffer = 0,
            .transfer_complete = 0
        };
        
        arcfour_decrypt_dma(&g_uart_crypto_ctx, &config);
        
        /* Swap buffers for next transfer */
        arcfour_dma_double_buffer_swap(&g_uart_dma_buffer);
        
        /* Start DMA transfer on the other buffer */
        HAL_UART_Receive_DMA(huart, arcfour_dma_get_active_buffer(&g_uart_dma_buffer), 
                             g_uart_dma_buffer.size);
        
        g_uart_dma_transfer_complete = 1;
    }
}

/**
 * @brief Encrypt and send data via UART with DMA
 * 
 * @param data Data to encrypt and send
 * @param len Length of data
 * @return HAL status
 */
HAL_StatusTypeDef arcfour_uart_send_encrypted_dma(UART_HandleTypeDef *huart, 
                                                   const uint8_t* data, size_t len) {
    uint8_t* tx_buffer = arcfour_dma_get_active_buffer(&g_uart_dma_buffer);
    
    /* Encrypt data into DMA buffer */
    arcfour_dma_config_t config = {
        .input = data,
        .output = tx_buffer,
        .len = len,
        .use_double_buffer = 0,
        .transfer_complete = 0
    };
    
    int ret = arcfour_encrypt_dma(&g_uart_crypto_ctx, &config);
    if (ret != 0) {
        return HAL_ERROR;
    }
    
    /* Start DMA transmission */
    return HAL_UART_Transmit_DMA(huart, tx_buffer, len);
}

#endif /* STM32_HAL_SUPPORT */

/*===========================================*/
/* Generic ARM DMA optimizations */
/*===========================================*/

/**
 * @brief Generate keystream using ARM-specific optimizations
 * 
 * Uses inline assembly for improved performance on Cortex-M4.
 */
size_t arcfour_prepare_keystream_dma_arm(arcfour_ctx_t* ctx, uint8_t* keystream, size_t len) {
    if (!ctx || !keystream || !ctx->initialized) {
        return 0;
    }
    
    uint8_t *S = ctx->S;
    uint8_t i = ctx->i;
    uint8_t j = ctx->j;
    
    /* Process in chunks of 4 bytes for better cache utilization */
    size_t chunks = len / 4;
    size_t remaining = len % 4;
    
    for (size_t c = 0; c < chunks; c++) {
        /* Generate 4 bytes of keystream */
        for (size_t b = 0; b < 4; b++) {
            i = (i + 1) & 0xFF;
            j = (j + S[i]) & 0xFF;
            
            /* Swap S[i] and S[j] */
            uint8_t t = S[i];
            S[i] = S[j];
            S[j] = t;
            
            uint8_t t2 = S[i] + S[j];
            keystream[c * 4 + b] = S[t2 & 0xFF];
        }
    }
    
    /* Process remaining bytes */
    for (size_t b = 0; b < remaining; b++) {
        i = (i + 1) & 0xFF;
        j = (j + S[i]) & 0xFF;
        
        uint8_t t = S[i];
        S[i] = S[j];
        S[j] = t;
        
        uint8_t t2 = S[i] + S[j];
        keystream[chunks * 4 + b] = S[t2 & 0xFF];
    }
    
    /* Update context */
    ctx->i = i;
    ctx->j = j;
    
    return len;
}

/**
 * @brief 32-bit word XOR using ARM NEON instructions (if available)
 * 
 * For Cortex-M4/M7 with FPU, we can use SIMD-like operations.
 */
void arcfour_xor_with_keystream_arm(uint8_t* output, const uint8_t* keystream, 
                                     const uint8_t* input, size_t len) {
    if (!output || !keystream || !input) {
        return;
    }
    
    /* Use aligned XOR for better performance */
    size_t aligned_len = len & ~3;
    
    /* Process aligned portion */
    uint32_t* out32 = (uint32_t*)output;
    const uint32_t* key32 = (const uint32_t*)keystream;
    const uint32_t* in32 = (const uint32_t*)input;
    
    for (size_t i = 0; i < aligned_len / 4; i++) {
        out32[i] = in32[i] ^ key32[i];
    }
    
    /* Process remaining bytes */
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

#endif /* ARCFOUR_CORTEX_M4 */