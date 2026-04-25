/*
 * rtos_isr_example.c - RTOS interrupt-safe encryption example
 * 
 * This example demonstrates how to use the interrupt-safe ARCFOUR API
 * in a preemptive RTOS environment with FreeRTOS.
 * 
 * Features demonstrated:
 * 1. ISR-safe encryption in interrupt handlers
 * 2. Deferred encryption using pending request queue
 * 3. Thread-safe context sharing between ISR and task
 * 4. Priority-based interrupt masking
 * 
 * Hardware: STM32F4xx with FreeRTOS
 */

#include <stdint.h>
#include <string.h>
#include "arcfour.h"
#include "arcfour_isr.h"
#include "arcfour_port.h"

/* FreeRTOS includes (assumed to be available) */
#ifdef FREERTOS_AVAILABLE
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#endif

/* Encryption contexts */
static arcfour_ctx_t g_encryption_ctx;
static arcfour_ctx_t g_decryption_ctx;

/* Encryption keys */
static const uint8_t g_encryption_key[] = "RTOS_Encrypt_Key_256bit";
static const uint8_t g_decryption_key[] = "RTOS_Decrypt_Key_256bit";

/* Task priorities */
#define CRYPTO_TASK_PRIORITY    (configMAX_PRIORITIES - 2)
#define APP_TASK_PRIORITY       (configMAX_PRIORITIES - 3)

/* Semaphore for pending request processing */
#ifdef FREERTOS_AVAILABLE
static SemaphoreHandle_t g_pending_semaphore = NULL;
#endif

/**
 * @brief Initialize encryption contexts
 */
void crypto_init(void) {
    /* Initialize encryption contexts using ISR-safe function */
    arcfour_init_isr(&g_encryption_ctx, g_encryption_key, sizeof(g_encryption_key) - 1);
    arcfour_init_isr(&g_decryption_ctx, g_decryption_key, sizeof(g_decryption_key) - 1);
    
#ifdef FREERTOS_AVAILABLE
    /* Create semaphore for pending request notification */
    g_pending_semaphore = xSemaphoreCreateBinary();
#endif
}

/**
 * @brief ISR handler for incoming data (e.g., UART, SPI, DMA)
 * 
 * This ISR demonstrates deferred encryption - instead of performing
 * encryption in the ISR (which may be time-sensitive), it queues
 * the request for processing by a background task.
 */
void DataReceivedISR(void) {
    /* Example: Read data from peripheral */
    static uint8_t received_buffer[32];
    static uint8_t output_buffer[32];
    size_t received_length = 16; /* Simulated received length */
    
    /* Increment ISR nesting counter */
    ARCFOUR_ISR_ENTER();
    
    /* Queue the encryption request (non-blocking) */
    int result = arcfour_request_encrypt(&g_decryption_ctx, received_buffer, output_buffer, received_length);
    
    if (result == 0) {
        /* Request queued successfully */
#ifdef FREERTOS_AVAILABLE
        /* Notify background task (from ISR) */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(g_pending_semaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
#endif
    } else {
        /* Queue is full - handle overflow */
        /* Could drop data, use secondary buffer, or raise error */
    }
    
    /* Decrement ISR nesting counter */
    ARCFOUR_ISR_EXIT();
}

/**
 * @brief Fast ISR encryption/decryption for time-critical operations
 * 
 * This function demonstrates using priority-based interrupt masking
 * to allow higher-priority interrupts to continue while performing
 * encryption.
 */
uint8_t encrypt_byte_fast_isr(uint8_t input) {
    /* Use priority-based masking - only block interrupts below priority 5 */
    /* Higher priority interrupts (0-4) can still be serviced */
    
    uint8_t encrypted;
    
    ARCFOUR_ENTER_CRITICAL_PRIO(5);
    
    /* Perform single byte encryption */
    g_encryption_ctx.i = (g_encryption_ctx.i + 1) & 0xFF;
    g_encryption_ctx.j = (g_encryption_ctx.j + g_encryption_ctx.S[g_encryption_ctx.i]) & 0xFF;
    
    uint8_t temp = g_encryption_ctx.S[g_encryption_ctx.i];
    g_encryption_ctx.S[g_encryption_ctx.i] = g_encryption_ctx.S[g_encryption_ctx.j];
    g_encryption_ctx.S[g_encryption_ctx.j] = temp;
    
    uint8_t s_i = g_encryption_ctx.S[g_encryption_ctx.i];
    uint8_t s_j = g_encryption_ctx.S[g_encryption_ctx.j];
    uint8_t keystream = g_encryption_ctx.S[(s_i + s_j) & 0xFF];
    
    encrypted = input ^ keystream;
    
    ARCFOUR_EXIT_CRITICAL_PRIO();
    
    return encrypted;
}

/**
 * @brief Crypto background task
 * 
 * Processes pending encryption/decryption requests queued by ISRs.
 */
#ifdef FREERTOS_AVAILABLE
void vCryptoTask(void *pvParameters) {
    (void)pvParameters;
    
    for (;;) {
        /* Wait for pending requests */
        if (xSemaphoreTake(g_pending_semaphore, portMAX_DELAY) == pdTRUE) {
            /* Process all pending requests */
            size_t processed = arcfour_process_pending();
            
            if (processed > 0) {
                /* Requests processed successfully */
                /* Could send notification to other tasks here */
            }
        }
    }
}

/**
 * @brief Application task that uses encryption
 */
void vAppTask(void *pvParameters) {
    (void)pvParameters;
    
    uint8_t plaintext[] = "Hello from RTOS task!";
    uint8_t ciphertext[32];
    uint8_t decrypted[32];
    
    for (;;) {
        /* Encrypt data using ISR-safe function */
        arcfour_encrypt_isr(&g_encryption_ctx, plaintext, ciphertext, sizeof(plaintext) - 1);
        
        /* Decrypt data */
        arcfour_init_isr(&g_encryption_ctx, g_encryption_key, sizeof(g_encryption_key) - 1);
        arcfour_decrypt_isr(&g_encryption_ctx, ciphertext, decrypted, sizeof(plaintext) - 1);
        
        /* Verify */
        if (memcmp(plaintext, decrypted, sizeof(plaintext) - 1) == 0) {
            /* Success - continue */
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
#endif

/**
 * @brief Task synchronization example with critical section
 * 
 * Demonstrates protecting shared data between task and ISR.
 */
void shared_resource_access_example(void) {
    static uint32_t shared_counter = 0;
    
    /* In task context */
    ARCFOUR_ENTER_CRITICAL();
    shared_counter++;
    ARCFOUR_EXIT_CRITICAL();
    
    /* In ISR context */
    /* Note: In ISR, use ISR-safe functions */
    ARCFOUR_ISR_ENTER();
    ARCFOUR_ENTER_CRITICAL();
    shared_counter++;
    ARCFOUR_EXIT_CRITICAL();
    ARCFOUR_ISR_EXIT();
}

/**
 * @brief Main application entry point
 */
int main(void) {
    /* Initialize hardware and peripherals */
    // hardware_init();
    
    /* Initialize crypto contexts */
    crypto_init();
    
#ifdef FREERTOS_AVAILABLE
    /* Create tasks */
    xTaskCreate(vCryptoTask, "CryptoTask", configMINIMAL_STACK_SIZE * 2, 
                NULL, CRYPTO_TASK_PRIORITY, NULL);
    
    xTaskCreate(vAppTask, "AppTask", configMINIMAL_STACK_SIZE * 2, 
                NULL, APP_TASK_PRIORITY, NULL);
    
    /* Start scheduler */
    vTaskStartScheduler();
#endif
    
    /* Should never reach here */
    while (1) {
        /* Process pending requests in bare-metal mode */
        arcfour_process_pending();
    }
}