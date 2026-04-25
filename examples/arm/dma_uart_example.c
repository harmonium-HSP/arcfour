/*
 * dma_uart_example.c - DMA + UART Encryption Example for STM32
 * 
 * This example demonstrates how to integrate ARCFOUR encryption with 
 * STM32 UART DMA for efficient encrypted communication.
 */

#include <stdint.h>
#include <string.h>
#include "arcfour_dma.h"

/* Hardware definitions (STM32F4 specific) */
#define USART2_BASE        0x40004400U
#define USART_SR_OFFSET    0x00U
#define USART_DR_OFFSET    0x04U
#define USART_BRR_OFFSET   0x08U
#define USART_CR1_OFFSET   0x0CU
#define USART_CR2_OFFSET   0x10U
#define USART_CR3_OFFSET   0x14U

#define DMA1_BASE          0x40026000U
#define DMA_SxCR_OFFSET    0x00U
#define DMA_SxNDTR_OFFSET  0x04U
#define DMA_SxPAR_OFFSET   0x08U
#define DMA_SxM0AR_OFFSET  0x0CU
#define DMA_SxM1AR_OFFSET  0x10U
#define DMA_SxCR_OFFSET    0x00U

#define RCC_BASE           0x40023800U
#define RCC_AHB1ENR_OFFSET 0x30U
#define RCC_APB1ENR_OFFSET 0x40U

/* Register access macros */
#include <stdint.h>
#define REG32(addr)        (*(volatile uint32_t*)((uintptr_t)(addr)))
#define REG16(addr)        (*(volatile uint16_t*)((uintptr_t)(addr)))
#define REG8(addr)         (*(volatile uint8_t*)((uintptr_t)(addr)))

/* Buffer configuration */
#define RX_BUFFER_SIZE     128
#define TX_BUFFER_SIZE     128

/* Global variables */
arcfour_ctx_t g_uart_crypto_ctx;
arcfour_dma_double_buffer_t g_uart_rx_dma_buffer;
arcfour_dma_double_buffer_t g_uart_tx_dma_buffer;

/* DMA-aligned buffers */
uint8_t ARCFOUR_DMA_BUFFER g_rx_buffer0[RX_BUFFER_SIZE];
uint8_t ARCFOUR_DMA_BUFFER g_rx_buffer1[RX_BUFFER_SIZE];
uint8_t ARCFOUR_DMA_BUFFER g_tx_buffer0[TX_BUFFER_SIZE];
uint8_t ARCFOUR_DMA_BUFFER g_tx_buffer1[TX_BUFFER_SIZE];

volatile uint8_t g_rx_complete = 0;
volatile uint8_t g_tx_complete = 0;

/*===========================================*/
/* Helper functions */
/*===========================================*/

static void rcc_enable_peripherals(void) {
    /* Enable GPIOA clock (PA2=USART2_TX, PA3=USART2_RX) */
    REG32(RCC_BASE + RCC_AHB1ENR_OFFSET) |= (1 << 0);
    
    /* Enable USART2 clock */
    REG32(RCC_BASE + RCC_APB1ENR_OFFSET) |= (1 << 17);
    
    /* Enable DMA1 clock */
    REG32(RCC_BASE + RCC_AHB1ENR_OFFSET) |= (1 << 21);
}

static void gpio_configure_uart(void) {
    uint32_t gpioa_base = 0x40020000U;
    
    /* PA2 and PA3 in AF7 mode */
    REG32(gpioa_base + 0x00) &= ~((0xF << 8) | (0xF << 12));
    REG32(gpioa_base + 0x00) |= ((0xA << 8) | (0xA << 12));
    
    /* High speed */
    REG32(gpioa_base + 0x08) &= ~((0x3 << 8) | (0x3 << 12));
    REG32(gpioa_base + 0x08) |= ((0x3 << 8) | (0x3 << 12));
    
    /* AF7 for USART2 */
    REG32(gpioa_base + 0x20) &= ~((0xF << 8) | (0xF << 12));
    REG32(gpioa_base + 0x20) |= ((0x7 << 8) | (0x7 << 12));
}

static void usart_configure(uint32_t baud_rate) {
    uint32_t usart_base = USART2_BASE;
    uint32_t apb1_freq = 42000000U;  /* APB1 clock frequency */
    
    /* Disable USART first */
    REG32(usart_base + USART_CR1_OFFSET) &= ~(1 << 13);
    
    /* Set baud rate */
    uint32_t brr = (apb1_freq + baud_rate / 2) / baud_rate;
    REG32(usart_base + USART_BRR_OFFSET) = brr;
    
    /* Enable RX, TX, and RXNE interrupt */
    REG32(usart_base + USART_CR1_OFFSET) |= (1 << 2) | (1 << 3);
    
    /* Enable DMA for RX and TX */
    REG32(usart_base + USART_CR3_OFFSET) |= (1 << 6) | (1 << 7);
    
    /* Enable USART */
    REG32(usart_base + USART_CR1_OFFSET) |= (1 << 13);
}

static void dma_configure_rx(void) {
    uint32_t dma_stream = DMA1_BASE + 0x1C;  /* DMA1 Stream5 for USART2_RX */
    
    /* Disable DMA stream */
    REG32(dma_stream + DMA_SxCR_OFFSET) &= ~(1 << 0);
    while (REG32(dma_stream + DMA_SxCR_OFFSET) & (1 << 0));
    
    /* Configure DMA: channel 4, memory increment, circular mode */
    REG32(dma_stream + DMA_SxCR_OFFSET) = 0;
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (4 << 25);  /* Channel 4 */
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (1 << 10);  /* Memory increment */
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (1 << 8);   /* Peripheral increment */
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (1 << 12);  /* Circular mode */
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (1 << 14);  /* Double buffer mode */
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (1 << 1);   /* Transfer complete interrupt */
    
    /* Set peripheral address (USART2_DR) */
    REG32(dma_stream + DMA_SxPAR_OFFSET) = USART2_BASE + USART_DR_OFFSET;
    
    /* Set memory addresses */
    REG32(dma_stream + DMA_SxM0AR_OFFSET) = (uint32_t)g_rx_buffer0;
    REG32(dma_stream + DMA_SxM1AR_OFFSET) = (uint32_t)g_rx_buffer1;
    
    /* Set number of data */
    REG32(dma_stream + DMA_SxNDTR_OFFSET) = RX_BUFFER_SIZE;
    
    /* Enable DMA stream */
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (1 << 0);
}

static void dma_configure_tx(void) {
    uint32_t dma_stream = DMA1_BASE + 0x28;  /* DMA1 Stream6 for USART2_TX */
    
    /* Disable DMA stream */
    REG32(dma_stream + DMA_SxCR_OFFSET) &= ~(1 << 0);
    while (REG32(dma_stream + DMA_SxCR_OFFSET) & (1 << 0));
    
    /* Configure DMA: channel 4, memory increment */
    REG32(dma_stream + DMA_SxCR_OFFSET) = 0;
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (4 << 25);  /* Channel 4 */
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (1 << 10);  /* Memory increment */
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (1 << 1);   /* Transfer complete interrupt */
    
    /* Set peripheral address (USART2_DR) */
    REG32(dma_stream + DMA_SxPAR_OFFSET) = USART2_BASE + USART_DR_OFFSET;
    
    /* Set memory addresses */
    REG32(dma_stream + DMA_SxM0AR_OFFSET) = (uint32_t)g_tx_buffer0;
    REG32(dma_stream + DMA_SxM1AR_OFFSET) = (uint32_t)g_tx_buffer1;
    
    /* Enable DMA stream */
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (1 << 0);
}

/*===========================================*/
/* DMA Interrupt Handlers */
/*===========================================*/

void DMA1_Stream5_IRQHandler(void) {
    /* Check transfer complete flag */
    if (REG32(DMA1_BASE + 0x4C) & (1 << 21)) {
        /* Clear flag */
        REG32(DMA1_BASE + 0x4C) |= (1 << 21);
        
        /* Get current buffer being filled by DMA */
        uint8_t* received_buffer = arcfour_dma_get_active_buffer(&g_uart_rx_dma_buffer);
        
        /* Decrypt the received data in-place */
        arcfour_dma_config_t config = {
            .input = received_buffer,
            .output = received_buffer,
            .len = RX_BUFFER_SIZE,
            .use_double_buffer = 0,
            .transfer_complete = 0
        };
        
        arcfour_decrypt_dma(&g_uart_crypto_ctx, &config);
        
        /* Swap buffers for next transfer */
        arcfour_dma_double_buffer_swap(&g_uart_rx_dma_buffer);
        
        g_rx_complete = 1;
    }
}

void DMA1_Stream6_IRQHandler(void) {
    /* Check transfer complete flag */
    if (REG32(DMA1_BASE + 0x54) & (1 << 21)) {
        /* Clear flag */
        REG32(DMA1_BASE + 0x54) |= (1 << 21);
        
        g_tx_complete = 1;
    }
}

/*===========================================*/
/* High-level API */
/*===========================================*/

void uart_crypto_init(const uint8_t* key, size_t key_len) {
    /* Initialize crypto context */
    arcfour_init(&g_uart_crypto_ctx, key, key_len);
    
    /* Initialize double buffers */
    arcfour_dma_double_buffer_init(&g_uart_rx_dma_buffer,
                                    g_rx_buffer0, g_rx_buffer1, RX_BUFFER_SIZE);
    arcfour_dma_double_buffer_init(&g_uart_tx_dma_buffer,
                                    g_tx_buffer0, g_tx_buffer1, TX_BUFFER_SIZE);
    
    /* Configure hardware */
    rcc_enable_peripherals();
    gpio_configure_uart();
    usart_configure(115200);
    dma_configure_rx();
    dma_configure_tx();
}

int uart_send_encrypted(const uint8_t* data, size_t len) {
    if (len > TX_BUFFER_SIZE) {
        return -1;  /* Data too large */
    }
    
    /* Wait for previous transmission to complete */
    while (!g_tx_complete);
    g_tx_complete = 0;
    
    /* Get active buffer */
    uint8_t* tx_buffer = arcfour_dma_get_active_buffer(&g_uart_tx_dma_buffer);
    
    /* Encrypt data into buffer */
    arcfour_dma_config_t config = {
        .input = data,
        .output = tx_buffer,
        .len = len,
        .use_double_buffer = 0,
        .transfer_complete = 0
    };
    
    int ret = arcfour_encrypt_dma(&g_uart_crypto_ctx, &config);
    if (ret != 0) {
        return -1;
    }
    
    /* Configure DMA for this transfer */
    uint32_t dma_stream = DMA1_BASE + 0x28;
    REG32(dma_stream + DMA_SxCR_OFFSET) &= ~(1 << 0);
    while (REG32(dma_stream + DMA_SxCR_OFFSET) & (1 << 0));
    
    REG32(dma_stream + DMA_SxM0AR_OFFSET) = (uint32_t)tx_buffer;
    REG32(dma_stream + DMA_SxNDTR_OFFSET) = len;
    REG32(dma_stream + DMA_SxCR_OFFSET) |= (1 << 0);
    
    /* Swap buffers for next time */
    arcfour_dma_double_buffer_swap(&g_uart_tx_dma_buffer);
    
    return 0;
}

size_t uart_receive_decrypted(uint8_t* buffer, size_t max_len) {
    if (!g_rx_complete) {
        return 0;
    }
    
    /* Get buffer with received data */
    uint8_t* rx_buffer = arcfour_dma_get_inactive_buffer(&g_uart_rx_dma_buffer);
    
    /* Data is already decrypted in DMA interrupt */
    size_t copy_len = (max_len < RX_BUFFER_SIZE) ? max_len : RX_BUFFER_SIZE;
    memcpy(buffer, rx_buffer, copy_len);
    
    g_rx_complete = 0;
    
    return copy_len;
}

/*===========================================*/
/* Example usage */
/*===========================================*/
void example_usage(void) {
    uint8_t key[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                       0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    
    /* Initialize encrypted UART */
    uart_crypto_init(key, sizeof(key));
    
    /* Send encrypted message */
    uint8_t message[] = "Hello, encrypted world!";
    uart_send_encrypted(message, sizeof(message) - 1);
    
    /* Wait for transmission */
    while (!g_tx_complete);
    
    /* Receive encrypted data (typically in main loop) */
    uint8_t received[RX_BUFFER_SIZE];
    size_t len = uart_receive_decrypted(received, RX_BUFFER_SIZE);
    if (len > 0) {
        /* Process received data */
    }
}