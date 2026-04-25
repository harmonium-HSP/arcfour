/*
 * uart_isr_example.c - UART ISR encryption example for STM32
 * 
 * This example demonstrates how to use the interrupt-safe ARCFOUR API
 * in a UART interrupt handler for real-time encryption/decryption.
 * 
 * Hardware: STM32F4xx with USART2
 * 
 * The example:
 * 1. Initializes USART2 for 115200 baud
 * 2. Encrypts data before transmission
 * 3. Decrypts data upon reception in ISR
 */

#include <stdint.h>
#include <string.h>
#include "arcfour.h"
#include "arcfour_isr.h"

/* UART register addresses (STM32F4xx) */
#define USART2_BASE    0x40004400
#define USART_SR       (*((volatile uint32_t*)(USART2_BASE + 0x00)))
#define USART_DR       (*((volatile uint32_t*)(USART2_BASE + 0x04)))
#define USART_BRR      (*((volatile uint32_t*)(USART2_BASE + 0x08)))
#define USART_CR1      (*((volatile uint32_t*)(USART2_BASE + 0x0C)))

/* GPIO register addresses (STM32F4xx) */
#define GPIOA_BASE     0x40020000
#define GPIOA_MODER    (*((volatile uint32_t*)(GPIOA_BASE + 0x00)))
#define GPIOA_AFRL     (*((volatile uint32_t*)(GPIOA_BASE + 0x20)))

/* RCC register addresses */
#define RCC_BASE       0x40023800
#define RCC_AHB1ENR    (*((volatile uint32_t*)(RCC_BASE + 0x30)))
#define RCC_APB1ENR    (*((volatile uint32_t*)(RCC_BASE + 0x40)))

/* UART flags */
#define USART_SR_TXE   (1 << 7)
#define USART_SR_RXNE  (1 << 5)

/* Encryption context - stored in .bss (no heap) */
static arcfour_ctx_t g_uart_encrypt_ctx;
static arcfour_ctx_t g_uart_decrypt_ctx;

/* Encryption keys */
static const uint8_t g_tx_key[] = "TxEncryptKey1234567";
static const uint8_t g_rx_key[] = "RxDecryptKey1234567";

/* Ring buffer for received data */
#define RX_BUFFER_SIZE 64
static uint8_t g_rx_buffer[RX_BUFFER_SIZE];
static volatile size_t g_rx_head = 0;
static volatile size_t g_rx_tail = 0;

/**
 * @brief Initialize USART2 for 115200 baud
 */
void uart_init(void) {
    /* Enable clock for GPIOA and USART2 */
    RCC_AHB1ENR |= (1 << 0);   // GPIOA clock
    RCC_APB1ENR |= (1 << 17);  // USART2 clock
    
    /* Set PA2 and PA3 to AF mode (USART2 TX/RX) */
    GPIOA_MODER &= ~((3 << 4) | (3 << 6));
    GPIOA_MODER |= ((2 << 4) | (2 << 6));
    
    /* Set AF7 for USART2 */
    GPIOA_AFRL &= ~((0xF << 8) | (0xF << 12));
    GPIOA_AFRL |= ((7 << 8) | (7 << 12));
    
    /* Configure USART2: 115200 baud, 8N1 */
    USART_BRR = 0x008B;  // 115200 @ 42MHz APB1 clock
    
    /* Enable USART, TX, RX, and RXNE interrupt */
    USART_CR1 = (1 << 13) | (1 << 3) | (1 << 2) | (1 << 5);
}

/**
 * @brief Initialize encryption contexts
 */
void crypto_init(void) {
    /* Initialize encryption contexts using ISR-safe function */
    arcfour_init_isr(&g_uart_encrypt_ctx, g_tx_key, sizeof(g_tx_key) - 1);
    arcfour_init_isr(&g_uart_decrypt_ctx, g_rx_key, sizeof(g_rx_key) - 1);
}

/**
 * @brief Send encrypted byte over UART
 */
void uart_send_encrypted(uint8_t data) {
    /* Encrypt in ISR-safe manner */
    uint8_t encrypted = arcfour_encrypt_byte_isr(&g_uart_encrypt_ctx, data);
    
    /* Wait for transmit buffer empty */
    while (!(USART_SR & USART_SR_TXE));
    
    /* Send encrypted byte */
    USART_DR = encrypted;
}

/**
 * @brief Send encrypted string over UART
 */
void uart_send_string_encrypted(const char* str) {
    while (*str) {
        uart_send_encrypted((uint8_t)*str++);
    }
}

/**
 * @brief USART2 interrupt handler
 */
void USART2_IRQHandler(void) {
    if (USART_SR & USART_SR_RXNE) {
        /* Read received byte */
        uint8_t received = (uint8_t)USART_DR;
        
        /* Decrypt using ISR-safe function */
        uint8_t decrypted = arcfour_encrypt_byte_isr(&g_uart_decrypt_ctx, received);
        
        /* Add to ring buffer */
        size_t next_head = (g_rx_head + 1) % RX_BUFFER_SIZE;
        if (next_head != g_rx_tail) {
            g_rx_buffer[g_rx_head] = decrypted;
            g_rx_head = next_head;
        }
        /* If buffer is full, data is dropped */
    }
}

/**
 * @brief Process received data
 */
void process_received_data(void) {
    while (g_rx_tail != g_rx_head) {
        uint8_t data = g_rx_buffer[g_rx_tail];
        g_rx_tail = (g_rx_tail + 1) % RX_BUFFER_SIZE;
        
        /* Echo back the decrypted character */
        uart_send_encrypted(data);
    }
}

/**
 * @brief Main application entry point
 */
int main(void) {
    /* Initialize peripherals */
    uart_init();
    crypto_init();
    
    /* Send welcome message */
    uart_send_string_encrypted("Hello from encrypted UART!\r\n");
    
    /* Main loop */
    while (1) {
        /* Process received data */
        process_received_data();
        
        /* Other application tasks... */
    }
}