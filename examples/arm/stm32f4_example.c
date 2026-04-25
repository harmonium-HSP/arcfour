/*
 * RC4 + Poly1305 Example for STM32F4
 * 
 * This example demonstrates how to use the RC4+Poly1305 AEAD library
 * on STM32F4 microcontrollers.
 * 
 * Note: Requires STM32 HAL Library for full functionality.
 * When not available, a simulation mode is used for compilation.
 */

#include <stdint.h>

/* Try to include STM32 HAL header, or provide simulation definitions */
#ifdef USE_STM32_HAL
#  include "stm32f4xx_hal.h"
#else
/* Simulation definitions for compilation without STM32 HAL */
#  define __IO volatile
#  define HAL_StatusTypeDef int
#  define HAL_OK 0
#  define HAL_ERROR -1
#  define GPIO_PIN_5 (1 << 5)

/* HAL_RNG simulation */
typedef struct {
    uint32_t CR;
    uint32_t SR;
    uint32_t DR;
} RNG_TypeDef;

typedef struct {
    RNG_TypeDef* Instance;
} RNG_HandleTypeDef;

RNG_HandleTypeDef hrng;

/* HAL_GPIO simulation */
typedef struct {
    uint32_t MODER;
    uint32_t OTYPER;
    uint32_t OSPEEDR;
    uint32_t PUPDR;
    uint32_t IDR;
    uint32_t ODR;
    uint32_t BSRR;
    uint32_t LCKR;
    uint32_t AFR[2];
} GPIO_TypeDef;

#  define GPIOA ((GPIO_TypeDef*)0x40020000U)

/* Function prototypes */
HAL_StatusTypeDef HAL_RNG_GenerateRandomNumber(RNG_HandleTypeDef* hrng, uint32_t* random);
void HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void HAL_Init(void);
void HAL_Delay(uint32_t Delay);
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_RNG_Init(void);
#endif

#include <string.h>
#include "arcfour.h"
#include "arcfour_arm.h"
#include "aead_rc4.h"

/* Buffer sizes */
#define BUFFER_SIZE 256
#define KEY_SIZE 32
#define NONCE_SIZE 12
#define TAG_SIZE 16

/* Global buffers */
static uint8_t key[KEY_SIZE];
static uint8_t nonce[NONCE_SIZE];
static uint8_t plaintext[BUFFER_SIZE];
static uint8_t ciphertext[BUFFER_SIZE];
static uint8_t tag[TAG_SIZE];

/* Generate random key and nonce using hardware RNG */
HAL_StatusTypeDef generate_random_data(uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i += 4) {
        uint32_t random;
        if (HAL_RNG_GenerateRandomNumber(&hrng, &random) != HAL_OK) {
            return HAL_ERROR;
        }
        buffer[i] = (uint8_t)(random & 0xFF);
        if (i + 1 < size) buffer[i + 1] = (uint8_t)((random >> 8) & 0xFF);
        if (i + 2 < size) buffer[i + 2] = (uint8_t)((random >> 16) & 0xFF);
        if (i + 3 < size) buffer[i + 3] = (uint8_t)((random >> 24) & 0xFF);
    }
    return HAL_OK;
}

/* RC4 encryption example */
void rc4_example(void) {
    uint8_t S[256];
    uint8_t i = 0, j = 0;
    uint8_t encrypted[BUFFER_SIZE];
    
    /* Initialize S-box with key */
    arcfour_ksa_arm(S, key, KEY_SIZE);
    
    /* Discard first 1536 bytes (security best practice) */
    for (int k = 0; k < 1536; k++) {
        arcfour_byte_arm(S, &i, &j);
    }
    
    /* Encrypt data using optimized ARM function */
    arcfour_encrypt_arm(S, &i, &j, plaintext, encrypted, BUFFER_SIZE);
    
    /* Verify decryption (RC4 is symmetric) */
    memset(S, 0, 256);
    i = j = 0;
    arcfour_ksa_arm(S, key, KEY_SIZE);
    
    for (int k = 0; k < 1536; k++) {
        arcfour_byte_arm(S, &i, &j);
    }
    
    arcfour_encrypt_arm(S, &i, &j, encrypted, ciphertext, BUFFER_SIZE);
    
    /* ciphertext should now equal plaintext */
    if (memcmp(plaintext, ciphertext, BUFFER_SIZE) == 0) {
        /* Success: toggle LED */
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
}

/* AEAD encryption example */
void aead_example(void) {
    uint8_t aad[16] = "STM32F4 Example";
    uint8_t decrypted[BUFFER_SIZE];
    size_t ciphertext_len;
    size_t decrypted_len;
    
    /* AEAD encryption */
    int result = aead_rc4_encrypt(
        key, KEY_SIZE,
        nonce, NONCE_SIZE,
        aad, sizeof(aad),
        plaintext, BUFFER_SIZE,
        ciphertext, &ciphertext_len,
        tag
    );
    
    if (result != 0) {
        /* Encryption failed */
        return;
    }
    
    /* AEAD decryption with authentication */
    result = aead_rc4_decrypt(
        key, KEY_SIZE,
        nonce, NONCE_SIZE,
        aad, sizeof(aad),
        ciphertext, ciphertext_len,
        tag,
        decrypted, &decrypted_len
    );
    
    if (result == 0 && decrypted_len == BUFFER_SIZE) {
        /* Success: toggle LED */
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
}

int main(void) {
    /* Initialize HAL */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_RNG_Init();
    
    /* Generate random key and nonce */
    generate_random_data(key, KEY_SIZE);
    generate_random_data(nonce, NONCE_SIZE);
    
    /* Initialize test plaintext */
    for (int i = 0; i < BUFFER_SIZE; i++) {
        plaintext[i] = (uint8_t)i;
    }
    
    /* Run examples */
    rc4_example();
    aead_example();
    
    while (1) {
        HAL_Delay(1000);
    }
}

/* Simulation implementations (only used when STM32 HAL is not available) */
#ifndef USE_STM32_HAL

static uint32_t g_rng_seed = 0x12345678;

HAL_StatusTypeDef HAL_RNG_GenerateRandomNumber(RNG_HandleTypeDef* hrng, uint32_t* random) {
    (void)hrng;
    /* Simple pseudo-random generator for simulation */
    g_rng_seed = g_rng_seed * 1103515245 + 12345;
    *random = g_rng_seed;
    return HAL_OK;
}

void HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    (void)GPIOx;
    (void)GPIO_Pin;
}

void HAL_Init(void) {
}

void HAL_Delay(uint32_t Delay) {
    (void)Delay;
}

void SystemClock_Config(void) {
}

void MX_GPIO_Init(void) {
}

void MX_RNG_Init(void) {
}

#endif /* USE_STM32_HAL */