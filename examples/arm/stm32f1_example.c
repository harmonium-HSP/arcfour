/*
 * RC4 + Poly1305 Example for STM32F1
 * 
 * This example demonstrates how to use the RC4+Poly1305 AEAD library
 * on STM32F1 microcontrollers.
 * 
 * Note: Requires STM32 Standard Peripheral Library for full functionality.
 * When not available, a simulation mode is used for compilation.
 */

#include <stdint.h>

/* Try to include STM32 header, or provide simulation definitions */
#ifdef USE_STM32_HAL
#  include "stm32f10x.h"
#else
/* Simulation definitions for compilation without STM32 HAL */
#  define __IO volatile
#  define ENABLE 1
#  define DISABLE 0

/* GPIO simulation */
typedef struct {
    uint32_t CRL;
    uint32_t CRH;
    uint32_t IDR;
    uint32_t ODR;
    uint32_t BSRR;
    uint32_t BRR;
    uint32_t LCKR;
} GPIO_TypeDef;

#  define GPIOC ((GPIO_TypeDef*)0x40011000U)
#  define GPIO_Pin_13 (1 << 13)

/* RCC simulation */
typedef struct {
    uint32_t CR;
    uint32_t CFGR;
    uint32_t CIR;
    uint32_t APB2RSTR;
    uint32_t APB1RSTR;
    uint32_t AHBENR;
    uint32_t APB2ENR;
    uint32_t APB1ENR;
    uint32_t BDCR;
    uint32_t CSR;
} RCC_TypeDef;

#  define RCC ((RCC_TypeDef*)0x40021000U)
#  define RCC_APB2Periph_GPIOC (1 << 4)

/* GPIO Init structure simulation */
typedef struct {
    uint16_t GPIO_Pin;
    uint32_t GPIO_Speed;
    uint16_t GPIO_Mode;
} GPIO_InitTypeDef;

#  define GPIO_Mode_Out_PP 0x01
#  define GPIO_Speed_50MHz 0x03

typedef enum {
    DISABLE_ENUM = 0,
    ENABLE_ENUM = !DISABLE_ENUM
} FunctionalState;

/* Function prototypes */
static void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
static void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
static void GPIO_ToggleBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
static void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct);
static void RCC_APB2PeriphClockCmd(uint32_t RCC_APB2Periph, FunctionalState NewState);
static void SystemInit(void);
#endif

#include <string.h>
#include "arcfour.h"
#include "arcfour_arm.h"

/* Buffer sizes */
#define BUFFER_SIZE 128
#define KEY_SIZE 16

/* Global buffers */
static uint8_t key[KEY_SIZE] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0
};

static uint8_t plaintext[BUFFER_SIZE];
static uint8_t ciphertext[BUFFER_SIZE];

/* Simple pseudo-random generator for STM32F1 (no hardware RNG) */
void generate_pseudo_random(uint8_t* buffer, size_t size, uint32_t seed) {
    for (size_t i = 0; i < size; i++) {
        seed = seed * 1103515245 + 12345;
        buffer[i] = (uint8_t)(seed >> 16);
    }
}

/* Initialize GPIO for LED */
void GPIO_Configuration(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/* Delay function */
void Delay(__IO uint32_t nCount) {
    while(nCount--) {
    }
}

/* RC4 encryption example */
void rc4_example(void) {
    uint8_t S[256];
    uint8_t i = 0, j = 0;
    
    /* Initialize S-box with key */
    arcfour_ksa_arm(S, key, KEY_SIZE);
    
    /* Discard first 1536 bytes */
    for (int k = 0; k < 1536; k++) {
        arcfour_byte_arm(S, &i, &j);
    }
    
    /* Encrypt data */
    arcfour_encrypt_arm(S, &i, &j, plaintext, ciphertext, BUFFER_SIZE);
    
    /* Verify by decrypting */
    memset(S, 0, 256);
    i = j = 0;
    arcfour_ksa_arm(S, key, KEY_SIZE);
    
    for (int k = 0; k < 1536; k++) {
        arcfour_byte_arm(S, &i, &j);
    }
    
    arcfour_encrypt_arm(S, &i, &j, ciphertext, ciphertext, BUFFER_SIZE);
    
    /* Check result */
    if (memcmp(plaintext, ciphertext, BUFFER_SIZE) == 0) {
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
    } else {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
    }
}

int main(void) {
    /* Initialize system */
    SystemInit();
    
    /* Initialize GPIO */
    GPIO_Configuration();
    
    /* Initialize test data with pseudo-random values */
    generate_pseudo_random(plaintext, BUFFER_SIZE, 0x12345678);
    
    /* Run RC4 example */
    rc4_example();
    
    /* Main loop */
    while (1) {
        Delay(0xFFFFF);
        GPIO_ToggleBits(GPIOC, GPIO_Pin_13);
    }
}

/* Simulation implementations (only used when STM32 HAL is not available) */
#ifndef USE_STM32_HAL

static void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    (void)GPIOx;
    (void)GPIO_Pin;
}

static void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    (void)GPIOx;
    (void)GPIO_Pin;
}

static void GPIO_ToggleBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    (void)GPIOx;
    (void)GPIO_Pin;
}

static void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct) {
    (void)GPIOx;
    (void)GPIO_InitStruct;
}

static void RCC_APB2PeriphClockCmd(uint32_t RCC_APB2Periph, FunctionalState NewState) {
    (void)RCC_APB2Periph;
    (void)NewState;
}

static void SystemInit(void) {
}

#endif /* USE_STM32_HAL */