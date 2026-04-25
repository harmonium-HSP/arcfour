/*
 * no_os_example.c - No-OS interrupt-based encryption example
 * 
 * This example demonstrates ARCFOUR usage in a completely bare-metal
 * environment without any operating system or RTOS.
 * 
 * Key features:
 * - Interrupt service routine (ISR) encryption
 * - No critical section protection needed (no preemption in single-task)
 * - Zero heap allocation
 * - Polling and interrupt-based operation
 */

#include <stdint.h>
#include <string.h>

/* Include static-only API */
#define ARCFOUR_STATIC_ONLY
#include "arcfour.h"

/* 
 * Memory layout:
 * - .rodata: Key constants
 * - .bss: Context and buffers
 * - .text: Code
 */

/* Peripheral register definitions (STM32-like example) */
#define UART_DR        ((volatile uint32_t*)0x40004400)  /* Data register */
#define UART_SR        ((volatile uint32_t*)0x40004404)  /* Status register */
#define UART_CR1       ((volatile uint32_t*)0x4000440C)  /* Control register */

#define UART_SR_TXE    (1 << 7)  /* Transmit data register empty */
#define UART_SR_RXNE   (1 << 5)  /* Receive data register not empty */

/* Encryption key (stored in .rodata) */
static const uint8_t g_uart_key[16] = {
    'S', 'e', 'c', 'u', 'r', 'e', 'K', 'e',
    'y', '1', '2', '3', '4', '5', '6', '7'
};

/* Encryption context for UART communication (stored in .bss) */
static arcfour_ctx_t g_uart_encrypt_ctx;
static arcfour_ctx_t g_uart_decrypt_ctx;

/* 
 * Initialize hardware peripherals
 */
void hw_init(void) {
    /* Enable UART clock (example STM32 register) */
    // RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    
    /* Configure UART baud rate, parity, stop bits */
    // UART->BRR = SystemCoreClock / BAUD_RATE;
    // UART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    
    /* Enable RX interrupt */
    // UART->CR1 |= USART_CR1_RXNEIE;
    // NVIC_EnableIRQ(USART2_IRQn);
}

/* 
 * Initialize encryption contexts
 */
void crypto_init(void) {
    /* Initialize encryption context */
    arcfour_init_static(&g_uart_encrypt_ctx, g_uart_key, sizeof(g_uart_key));
    
    /* Initialize decryption context with same key */
    arcfour_init_static(&g_uart_decrypt_ctx, g_uart_key, sizeof(g_uart_key));
}

/* 
 * Send single byte via UART (polling)
 */
void uart_send_byte(uint8_t byte) {
    /* Wait for TX buffer empty */
    while (!((*UART_SR) & UART_SR_TXE));
    
    /* Send byte */
    *UART_DR = byte;
}

/* 
 * Send encrypted data via UART
 */
void uart_send_encrypted(const uint8_t* data, size_t len) {
    uint8_t encrypted_byte;
    
    for (size_t i = 0; i < len; i++) {
        /* Encrypt single byte */
        arcfour_encrypt_static(&g_uart_encrypt_ctx, &data[i], &encrypted_byte, 1);
        
        /* Send encrypted byte */
        uart_send_byte(encrypted_byte);
    }
}

/* 
 * UART Interrupt Service Routine
 * Called when data is received
 * 
 * In this no-OS example, ISR directly processes the data
 * No OS context switching, no task scheduling
 */
void UART_IRQHandler(void) {
    if ((*UART_SR) & UART_SR_RXNE) {
        /* Read received byte */
        uint8_t received = (uint8_t)(*UART_DR);
        
        /* Decrypt byte using static context */
        uint8_t decrypted;
        arcfour_decrypt_static(&g_uart_decrypt_ctx, &received, &decrypted, 1);
        
        /* Process decrypted byte */
        // process_received_byte(decrypted);
    }
}

/* 
 * Polling-based main loop
 * In no-OS environment, everything runs in this loop or ISRs
 */
void main_loop(void) {
    /* Periodic tasks */
    static uint32_t counter = 0;
    
    while (1) {
        /* Blink LED (heartbeat) */
        // if (counter++ % 1000000 == 0) {
        //     led_toggle();
        // }
        
        /* 
         * Application-specific tasks:
         * - Poll sensors
         * - Handle buttons
         * - Send periodic encrypted status messages
         */
        
        /* Example: Send encrypted status message every 1000 iterations */
        if (counter % 1000 == 0) {
            uint8_t status_msg[] = "Status OK";
            uart_send_encrypted(status_msg, sizeof(status_msg) - 1);
        }
    }
}

/* 
 * System initialization and entry point
 * In bare-metal system, this is called after reset
 */
int main(void) {
    /* Initialize hardware */
    hw_init();
    
    /* Initialize encryption */
    crypto_init();
    
    /* Start main loop (never returns) */
    main_loop();
}

/* 
 * Reset handler - entry point after power-on reset
 * In real system, this would set up stack pointer and call main
 */
void Reset_Handler(void) {
    /* Set up stack (in real system) */
    // __set_MSP((uint32_t)&_estack);
    
    /* Call main */
    main();
}
