/*
 * static_alloc.c - Bare-metal static memory allocation example
 * 
 * This example demonstrates how to use ARCFOUR in a bare-metal embedded
 * system without heap allocation (no malloc/free).
 * 
 * Memory distribution:
 * - .text: Code (~1.2KB)
 * - .rodata: Key constants (32 bytes)
 * - .bss: Context + buffers (~516 bytes)
 * - Total: ~1.8KB
 */

#include <stdint.h>
#include <string.h>

/* Include static-only API */
#define ARCFOUR_STATIC_ONLY
#include "arcfour.h"

/* 
 * Static memory allocation - all in .bss or .rodata sections
 * No heap allocation anywhere!
 */

/* Key stored in .rodata (read-only) */
static const uint8_t g_encryption_key[32] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00
};

/* Encryption context in .bss */
static arcfour_ctx_t g_encrypt_ctx;

/* TX/RX buffers in .bss */
static uint8_t g_tx_buffer[256];
static uint8_t g_rx_buffer[256];

/* 
 * Initialize encryption context once at system startup
 * This function should be called during initialization before any encryption
 */
void encryption_init(void) {
    /* Initialize static context with the key */
    arcfour_init_static(&g_encrypt_ctx, g_encryption_key, sizeof(g_encryption_key));
}

/* 
 * Encrypt data using static context
 * Parameters:
 *   data - input data to encrypt
 *   output - encrypted output buffer
 *   len - length of data
 */
void encrypt_data(const uint8_t* data, uint8_t* output, size_t len) {
    arcfour_encrypt_static(&g_encrypt_ctx, data, output, len);
}

/* 
 * Decrypt data using static context
 * Parameters:
 *   data - input data to decrypt
 *   output - decrypted output buffer
 *   len - length of data
 */
void decrypt_data(const uint8_t* data, uint8_t* output, size_t len) {
    arcfour_decrypt_static(&g_encrypt_ctx, data, output, len);
}

/* 
 * Reset encryption context to initial state
 * Useful when reusing the same key for multiple operations
 */
void encryption_reset(void) {
    arcfour_reset_static(&g_encrypt_ctx);
    arcfour_init_static(&g_encrypt_ctx, g_encryption_key, sizeof(g_encryption_key));
}

/* 
 * Example usage: Encrypt data before transmission
 */
void send_encrypted_data(const uint8_t* payload, size_t len) {
    /* Ensure we don't exceed buffer size */
    if (len > sizeof(g_tx_buffer)) {
        len = sizeof(g_tx_buffer);
    }
    
    /* Encrypt into TX buffer */
    encrypt_data(payload, g_tx_buffer, len);
    
    /* Send encrypted data over communication interface */
    /* In real system, this would call UART/SPI/I2C send function */
    // uart_send(g_tx_buffer, len);
}

/* 
 * Example usage: Decrypt received data
 */
void process_received_data(const uint8_t* received, size_t len) {
    /* Ensure we don't exceed buffer size */
    if (len > sizeof(g_rx_buffer)) {
        len = sizeof(g_rx_buffer);
    }
    
    /* Decrypt into RX buffer */
    decrypt_data(received, g_rx_buffer, len);
    
    /* Process decrypted data */
    // process_data(g_rx_buffer, len);
}

/* 
 * Main function - demonstrates usage
 * In real bare-metal system, this would be the main loop
 */
int main(void) {
    /* Initialize encryption at startup */
    encryption_init();
    
    /* Test data */
    uint8_t test_data[] = "Hello from bare-metal embedded system!";
    uint8_t encrypted[64] = {0};
    uint8_t decrypted[64] = {0};
    
    /* Encrypt test data */
    encrypt_data(test_data, encrypted, sizeof(test_data) - 1);
    
    /* Reset context for decryption */
    encryption_reset();
    
    /* Decrypt data */
    decrypt_data(encrypted, decrypted, sizeof(test_data) - 1);
    
    /* Verify (in real system, this would be assertions) */
    if (memcmp(test_data, decrypted, sizeof(test_data) - 1) == 0) {
        /* Success - indicator could be LED toggle */
        // led_toggle(LED_OK);
    } else {
        /* Failure - indicator could be error LED */
        // led_toggle(LED_ERROR);
    }
    
    /* Main loop */
    while (1) {
        /* 
         * In real application:
         * - Check for received data
         * - Process incoming packets
         * - Send encrypted responses
         */
    }
}
