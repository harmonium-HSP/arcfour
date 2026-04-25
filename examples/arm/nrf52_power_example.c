/*
 * Nordic nRF52 Power-Aware RC4 Example
 * 
 * This example demonstrates how to use the power-aware encryption API
 * on Nordic nRF52 series microcontrollers.
 */

#include "nrf52840.h"
#include "nrf52840_bitfields.h"
#include "arcfour_power.h"
#include "arcfour.h"

/* Power management hooks */
arcfour_power_hooks_t nrf52_hooks;

/* UART configuration */
#define UART_TX_PIN 6
#define UART_RX_PIN 8

/* UART initialization */
void uart_init(void) {
    /* Enable UART clock */
    NRF_UARTE0->PSEL.TXD = UART_TX_PIN;
    NRF_UARTE0->PSEL.RXD = UART_RX_PIN;
    NRF_UARTE0->BAUDRATE = UARTE_BAUDRATE_BAUDRATE_Baud115200;
    NRF_UARTE0->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
    NRF_UARTE0->TASKS_STARTTX = 1;
}

/* UART transmit function */
void uart_tx(const char* data, size_t len) {
    NRF_UARTE0->TXD.PTR = (uint32_t)data;
    NRF_UARTE0->TXD.MAXCNT = len;
    NRF_UARTE0->TASKS_STARTTX = 1;
    while (NRF_UARTE0->EVENTS_TXEND == 0);
    NRF_UARTE0->EVENTS_TXEND = 0;
}

/* UART printf */
void uart_printf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    uart_tx(buffer, strlen(buffer));
}

/* Wakeup handler - enable HF clock and peripherals */
static int nrf52_wakeup_handler(void) {
    /* Enable HF clock */
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
    
    /* Enable UART */
    NRF_UARTE0->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
    
    uart_printf("Woke up\n");
    return 0;
}

/* Sleep handler - enter System ON or OFF mode */
static void nrf52_sleep_handler(void) {
    uart_printf("Entering sleep...\n");
    
    /* Disable UART */
    NRF_UARTE0->ENABLE = UARTE_ENABLE_ENABLE_Disabled;
    
    /* Enter System ON sleep mode (low power, wake on interrupt) */
    __WFE();
}

/* Low battery handler */
static int nrf52_low_battery_handler(uint16_t voltage_mv) {
    uart_printf("Low battery: %d mV\n", voltage_mv);
    
    if (voltage_mv < 1800) {
        return 0;  /* Critical - abort */
    }
    return 1;  /* Continue */
}

/* Read battery voltage using SAADC */
uint16_t arcfour_read_battery_mv(void) {
    /* Configure SAADC for battery measurement */
    NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled;
    NRF_SAADC->CH[0].CONFIG = (SAADC_CH_CONFIG_GAIN_Gain1_4 << SAADC_CH_CONFIG_GAIN_Pos) |
                              (SAADC_CH_CONFIG_REFERENCE_VDD1_4 << SAADC_CH_CONFIG_REFERENCE_Pos) |
                              (SAADC_CH_CONFIG_RESOLUTION_12bit << SAADC_CH_CONFIG_RESOLUTION_Pos) |
                              (SAADC_CH_CONFIG_BURST_Disabled << SAADC_CH_CONFIG_BURST_Pos);
    NRF_SAADC->CH[0].PSELP = SAADC_CH_PSELP_PSELP_VDD << SAADC_CH_PSELP_PSELP_Pos;
    
    /* Start conversion */
    NRF_SAADC->EVENTS_DONE = 0;
    NRF_SAADC->TASKS_START = 1;
    NRF_SAADC->TASKS_SAMPLE = 1;
    
    while (NRF_SAADC->EVENTS_DONE == 0);
    
    /* Read result */
    uint16_t raw = NRF_SAADC->RESULT[0];
    
    /* Calculate voltage (VDD/4 referenced to VDD/4, so multiply by 4) */
    uint16_t voltage_mv = (raw * 3600) / 4095;  /* Assuming 3.6V max */
    
    /* Disable SAADC */
    NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Disabled;
    
    return voltage_mv;
}

/* Random number generation */
void generate_random(uint8_t* buffer, size_t len) {
    for (size_t i = 0; i < len; i += 4) {
        NRF_RNG->TASKS_START = 1;
        while (NRF_RNG->EVENTS_VALRDY == 0);
        buffer[i] = (uint8_t)NRF_RNG->VALUE;
        if (i + 1 < len) buffer[i + 1] = (uint8_t)(NRF_RNG->VALUE >> 8);
        if (i + 2 < len) buffer[i + 2] = (uint8_t)(NRF_RNG->VALUE >> 16);
        if (i + 3 < len) buffer[i + 3] = (uint8_t)(NRF_RNG->VALUE >> 24);
        NRF_RNG->EVENTS_VALRDY = 0;
    }
    NRF_RNG->TASKS_STOP = 1;
}

/* Encryption test with power management */
void power_aware_encryption_test(void) {
    uint8_t key[32];
    uint8_t plaintext[64] = "nRF52 Power-Aware Encryption Test";
    uint8_t ciphertext[64];
    uint8_t decrypted[64];
    
    /* Generate random key */
    generate_random(key, 32);
    
    uart_printf("\n=== Power-Aware Encryption Test ===\n");
    
    /* Initialize with power management */
    arcfour_ctx* ctx = arcfour_init_power_aware(key, 32, &nrf52_hooks);
    if (!ctx) {
        uart_printf("❌ Initialization failed\n");
        return;
    }
    
    /* Power-aware encryption */
    int result = arcfour_encrypt_power_aware(ctx, plaintext, ciphertext, 64, &nrf52_hooks);
    if (result != 0) {
        uart_printf("❌ Encryption failed (%d)\n", result);
        arcfour_uninit(ctx);
        return;
    }
    
    uart_printf("✅ Encryption successful\n");
    
    /* Decrypt to verify */
    arcfour_ctx* ctx2 = arcfour_init(key, 32);
    arcfour_decrypt(ctx2, ciphertext, decrypted, 64);
    
    if (memcmp(plaintext, decrypted, 64) == 0) {
        uart_printf("✅ Decryption verified\n");
    } else {
        uart_printf("❌ Decryption failed\n");
    }
    
    arcfour_uninit(ctx);
    arcfour_uninit(ctx2);
}

int main(void) {
    /* Initialize UART */
    uart_init();
    
    /* Enable RNG */
    NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Enabled;
    NRF_RNG->ENABLE = RNG_ENABLE_ENABLE_Enabled;
    
    /* Configure power management hooks */
    nrf52_hooks.before_operation = nrf52_wakeup_handler;
    nrf52_hooks.after_operation = nrf52_sleep_handler;
    nrf52_hooks.on_low_battery = nrf52_low_battery_handler;
    nrf52_hooks.timeout_ms = 3000;  /* 3 second auto-sleep */
    
    /* Set as global hooks */
    arcfour_set_global_power_hooks(&nrf52_hooks);
    
    uart_printf("========== nRF52 Power-Aware RC4 ==========\n");
    
    while (1) {
        /* Run encryption test */
        power_aware_encryption_test();
        
        /* Check if we should sleep */
        if (arcfour_should_sleep(&nrf52_hooks)) {
            uart_printf("Auto-sleep triggered\n");
            nrf52_sleep_handler();
        }
        
        /* Small delay */
        for (volatile int i = 0; i < 1000000; i++);
    }
}