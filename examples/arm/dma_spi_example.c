/*
 * dma_spi_example.c - DMA + SPI Encryption Example for STM32
 * 
 * This example demonstrates how to integrate ARCFOUR encryption with 
 * STM32 SPI DMA for efficient encrypted communication.
 */

#include <stdint.h>
#include <string.h>
#include "arcfour_dma.h"

/* Hardware definitions (STM32F4 specific) */
#define SPI1_BASE          0x40013000U
#define SPI_CR1_OFFSET     0x00U
#define SPI_CR2_OFFSET     0x04U
#define SPI_SR_OFFSET      0x08U
#define SPI_DR_OFFSET      0x0CU

#define DMA2_BASE          0x40026400U

#define RCC_BASE           0x40023800U
#define RCC_AHB1ENR_OFFSET 0x30U
#define RCC_APB2ENR_OFFSET 0x44U

/* Register access macros */
#define REG32(addr)        (*(volatile uint32_t*)((uintptr_t)(addr)))
#define REG16(addr)        (*(volatile uint16_t*)((uintptr_t)(addr)))
#define REG8(addr)         (*(volatile uint8_t*)((uintptr_t)(addr)))

/* Buffer configuration */
#define SPI_BUFFER_SIZE    256

/* Global variables */
arcfour_ctx_t g_spi_crypto_ctx;
arcfour_dma_double_buffer_t g_spi_tx_dma_buffer;
arcfour_dma_double_buffer_t g_spi_rx_dma_buffer;

/* DMA-aligned buffers */
uint8_t ARCFOUR_DMA_BUFFER g_spi_tx_buffer0[SPI_BUFFER_SIZE];
uint8_t ARCFOUR_DMA_BUFFER g_spi_tx_buffer1[SPI_BUFFER_SIZE];
uint8_t ARCFOUR_DMA_BUFFER g_spi_rx_buffer0[SPI_BUFFER_SIZE];
uint8_t ARCFOUR_DMA_BUFFER g_spi_rx_buffer1[SPI_BUFFER_SIZE];

volatile uint8_t g_spi_tx_complete = 0;
volatile uint8_t g_spi_rx_complete = 0;

/*===========================================*/
/* Helper functions */
/*===========================================*/

static void rcc_enable_spi_peripherals(void) {
    /* Enable GPIOA clock (PA5=SCK, PA6=MISO, PA7=MOSI) */
    REG32(RCC_BASE + RCC_AHB1ENR_OFFSET) |= (1 << 0);
    
    /* Enable SPI1 clock */
    REG32(RCC_BASE + RCC_APB2ENR_OFFSET) |= (1 << 12);
    
    /* Enable DMA2 clock */
    REG32(RCC_BASE + RCC_AHB1ENR_OFFSET) |= (1 << 22);
}

static void gpio_configure_spi(void) {
    uint32_t gpioa_base = 0x40020000U;
    
    /* PA5, PA6, PA7 in AF5 mode (SPI1) */
    REG32(gpioa_base + 0x00) &= ~((0xF << 20) | (0xF << 24) | (0xF << 28));
    REG32(gpioa_base + 0x00) |= ((0xA << 20) | (0xA << 24) | (0xA << 28));
    
    /* High speed */
    REG32(gpioa_base + 0x08) &= ~((0x3 << 20) | (0x3 << 24) | (0x3 << 28));
    REG32(gpioa_base + 0x08) |= ((0x3 << 20) | (0x3 << 24) | (0x3 << 28));
    
    /* AF5 for SPI1 */
    REG32(gpioa_base + 0x24) &= ~((0xF << 12) | (0xF << 16) | (0xF << 20));
    REG32(gpioa_base + 0x24) |= ((0x5 << 12) | (0x5 << 16) | (0x5 << 20));
}

static void spi_configure(void) {
    uint32_t spi_base = SPI1_BASE;
    
    /* Disable SPI first */
    REG32(spi_base + SPI_CR1_OFFSET) &= ~(1 << 6);
    
    /* Configure SPI: master mode, 8-bit, CPOL=0, CPHA=0 */
    REG32(spi_base + SPI_CR1_OFFSET) = 0;
    REG32(spi_base + SPI_CR1_OFFSET) |= (1 << 2);   /* Master mode */
    REG32(spi_base + SPI_CR1_OFFSET) |= (0x7 << 3); /* Prescaler = 128 */
    
    /* Enable DMA for RX and TX */
    REG32(spi_base + SPI_CR2_OFFSET) |= (1 << 0) | (1 << 1);
    
    /* Enable SPI */
    REG32(spi_base + SPI_CR1_OFFSET) |= (1 << 6);
}

static void dma_configure_spi_tx(void) {
    uint32_t dma_stream = DMA2_BASE + 0x1C;  /* DMA2 Stream5 for SPI1_TX */
    
    /* Disable DMA stream */
    REG32(dma_stream + 0x00) &= ~(1 << 0);
    while (REG32(dma_stream + 0x00) & (1 << 0));
    
    /* Configure DMA: channel 3, memory increment */
    REG32(dma_stream + 0x00) = 0;
    REG32(dma_stream + 0x00) |= (3 << 25);  /* Channel 3 */
    REG32(dma_stream + 0x00) |= (1 << 10);  /* Memory increment */
    REG32(dma_stream + 0x00) |= (1 << 1);   /* Transfer complete interrupt */
    
    /* Set peripheral address (SPI1_DR) */
    REG32(dma_stream + 0x08) = SPI1_BASE + SPI_DR_OFFSET;
    
    /* Set memory addresses */
    REG32(dma_stream + 0x0C) = (uint32_t)g_spi_tx_buffer0;
    REG32(dma_stream + 0x10) = (uint32_t)g_spi_tx_buffer1;
    
    /* Enable DMA stream */
    REG32(dma_stream + 0x00) |= (1 << 0);
}

static void dma_configure_spi_rx(void) {
    uint32_t dma_stream = DMA2_BASE + 0x28;  /* DMA2 Stream6 for SPI1_RX */
    
    /* Disable DMA stream */
    REG32(dma_stream + 0x00) &= ~(1 << 0);
    while (REG32(dma_stream + 0x00) & (1 << 0));
    
    /* Configure DMA: channel 3, memory increment, circular mode */
    REG32(dma_stream + 0x00) = 0;
    REG32(dma_stream + 0x00) |= (3 << 25);  /* Channel 3 */
    REG32(dma_stream + 0x00) |= (1 << 10);  /* Memory increment */
    REG32(dma_stream + 0x00) |= (1 << 12);  /* Circular mode */
    REG32(dma_stream + 0x00) |= (1 << 14);  /* Double buffer mode */
    REG32(dma_stream + 0x00) |= (1 << 1);   /* Transfer complete interrupt */
    
    /* Set peripheral address (SPI1_DR) */
    REG32(dma_stream + 0x08) = SPI1_BASE + SPI_DR_OFFSET;
    
    /* Set memory addresses */
    REG32(dma_stream + 0x0C) = (uint32_t)g_spi_rx_buffer0;
    REG32(dma_stream + 0x10) = (uint32_t)g_spi_rx_buffer1;
    
    /* Set number of data */
    REG32(dma_stream + 0x04) = SPI_BUFFER_SIZE;
    
    /* Enable DMA stream */
    REG32(dma_stream + 0x00) |= (1 << 0);
}

/*===========================================*/
/* DMA Interrupt Handlers */
/*===========================================*/

void DMA2_Stream5_IRQHandler(void) {
    /* Check transfer complete flag */
    if (REG32(DMA2_BASE + 0x4C) & (1 << 21)) {
        /* Clear flag */
        REG32(DMA2_BASE + 0x4C) |= (1 << 21);
        
        g_spi_tx_complete = 1;
    }
}

void DMA2_Stream6_IRQHandler(void) {
    /* Check transfer complete flag */
    if (REG32(DMA2_BASE + 0x54) & (1 << 21)) {
        /* Clear flag */
        REG32(DMA2_BASE + 0x54) |= (1 << 21);
        
        /* Get buffer with received data */
        uint8_t* received_buffer = arcfour_dma_get_active_buffer(&g_spi_rx_dma_buffer);
        
        /* Decrypt in-place */
        arcfour_dma_config_t config = {
            .input = received_buffer,
            .output = received_buffer,
            .len = SPI_BUFFER_SIZE,
            .use_double_buffer = 0,
            .transfer_complete = 0
        };
        
        arcfour_decrypt_dma(&g_spi_crypto_ctx, &config);
        
        /* Swap buffers */
        arcfour_dma_double_buffer_swap(&g_spi_rx_dma_buffer);
        
        g_spi_rx_complete = 1;
    }
}

/*===========================================*/
/* High-level API */
/*===========================================*/

void spi_crypto_init(const uint8_t* key, size_t key_len) {
    /* Initialize crypto context */
    arcfour_init(&g_spi_crypto_ctx, key, key_len);
    
    /* Initialize double buffers */
    arcfour_dma_double_buffer_init(&g_spi_tx_dma_buffer,
                                    g_spi_tx_buffer0, g_spi_tx_buffer1, SPI_BUFFER_SIZE);
    arcfour_dma_double_buffer_init(&g_spi_rx_dma_buffer,
                                    g_spi_rx_buffer0, g_spi_rx_buffer1, SPI_BUFFER_SIZE);
    
    /* Configure hardware */
    rcc_enable_spi_peripherals();
    gpio_configure_spi();
    spi_configure();
    dma_configure_spi_tx();
    dma_configure_spi_rx();
}

int spi_send_encrypted(const uint8_t* data, size_t len) {
    if (len > SPI_BUFFER_SIZE) {
        return -1;
    }
    
    /* Wait for previous transmission */
    while (!g_spi_tx_complete);
    g_spi_tx_complete = 0;
    
    /* Get active buffer */
    uint8_t* tx_buffer = arcfour_dma_get_active_buffer(&g_spi_tx_dma_buffer);
    
    /* Encrypt data */
    arcfour_dma_config_t config = {
        .input = data,
        .output = tx_buffer,
        .len = len,
        .use_double_buffer = 0,
        .transfer_complete = 0
    };
    
    int ret = arcfour_encrypt_dma(&g_spi_crypto_ctx, &config);
    if (ret != 0) {
        return -1;
    }
    
    /* Start DMA transfer */
    uint32_t dma_stream = DMA2_BASE + 0x1C;
    REG32(dma_stream + 0x00) &= ~(1 << 0);
    while (REG32(dma_stream + 0x00) & (1 << 0));
    
    REG32(dma_stream + 0x0C) = (uint32_t)tx_buffer;
    REG32(dma_stream + 0x04) = len;
    REG32(dma_stream + 0x00) |= (1 << 0);
    
    /* Swap buffers */
    arcfour_dma_double_buffer_swap(&g_spi_tx_dma_buffer);
    
    return 0;
}

size_t spi_receive_decrypted(uint8_t* buffer, size_t max_len) {
    if (!g_spi_rx_complete) {
        return 0;
    }
    
    /* Get buffer with received data */
    uint8_t* rx_buffer = arcfour_dma_get_inactive_buffer(&g_spi_rx_dma_buffer);
    
    /* Data is already decrypted */
    size_t copy_len = (max_len < SPI_BUFFER_SIZE) ? max_len : SPI_BUFFER_SIZE;
    memcpy(buffer, rx_buffer, copy_len);
    
    g_spi_rx_complete = 0;
    
    return copy_len;
}

/*===========================================*/
/* Example usage */
/*===========================================*/
void spi_example_usage(void) {
    uint8_t key[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                       0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    
    /* Initialize encrypted SPI */
    spi_crypto_init(key, sizeof(key));
    
    /* Send encrypted data */
    uint8_t tx_data[SPI_BUFFER_SIZE];
    for (size_t i = 0; i < SPI_BUFFER_SIZE; i++) {
        tx_data[i] = (uint8_t)i;
    }
    
    spi_send_encrypted(tx_data, SPI_BUFFER_SIZE);
    
    /* Wait for transmission */
    while (!g_spi_tx_complete);
    
    /* Receive encrypted data (typically in main loop) */
    uint8_t rx_data[SPI_BUFFER_SIZE];
    size_t len = spi_receive_decrypted(rx_data, SPI_BUFFER_SIZE);
    if (len > 0) {
        /* Process received data */
    }
}