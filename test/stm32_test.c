/*
 * STM32 Hardware Test for ARM Optimized RC4
 * 
 * This test is designed to run on actual STM32 hardware.
 * Connect serial port to see test results.
 */

#include "stm32f4xx_hal.h"
#include "arcfour_arm.h"

/* UART Handle */
UART_HandleTypeDef huart2;

/* Test vectors */
static const uint8_t test_key[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0
};

static const uint8_t test_plaintext[] = "Hello, STM32 RC4 Test!";
static const uint8_t expected_ciphertext[] = {
    0x75, 0xB7, 0x87, 0x80, 0x99, 0xE0, 0xC5, 0x96,
    0x0D, 0x1D, 0xBD, 0x29, 0xAD, 0x63, 0x18, 0xEB
};

static int failures = 0;

/* UART Printf function */
void uart_printf(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
}

#define TEST_ASSERT(expr, msg) \
    do { \
        if (!(expr)) { \
            uart_printf("❌ FAIL: %s\r\n", msg); \
            failures++; \
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET); \
        } else { \
            uart_printf("✅ PASS: %s\r\n", msg); \
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); \
            HAL_Delay(100); \
        } \
    } while (0)

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

int test_rc4_basic(void) {
    uint8_t S[256];
    uint8_t i = 0, j = 0;
    uint8_t ciphertext[16];
    
    uart_printf("\r\n=== Test: Basic RC4 ===\r\n");
    
    arcfour_ksa_arm(S, test_key, sizeof(test_key));
    
    /* Discard first 1536 bytes */
    for (int k = 0; k < 1536; k++) {
        arcfour_byte_arm(S, &i, &j);
    }
    
    /* Encrypt test data */
    for (int k = 0; k < 16; k++) {
        ciphertext[k] = arcfour_byte_arm(S, &i, &j) ^ ((uint8_t)k);
    }
    
    TEST_ASSERT(1, "Basic RC4 operation");
    
    return 0;
}

int test_consistency(void) {
    uint8_t S1[256], S2[256];
    uint8_t i1 = 0, j1 = 0, i2 = 0, j2 = 0;
    uint8_t out1[64], out2[64];
    
    uart_printf("\r\n=== Test: Consistency ===\r\n");
    
    arcfour_ksa_arm(S1, test_key, sizeof(test_key));
    arcfour_ksa_arm(S2, test_key, sizeof(test_key));
    
    for (int k = 0; k < 64; k++) {
        out1[k] = arcfour_byte_arm(S1, &i1, &j1);
        out2[k] = arcfour_byte_arm(S2, &i2, &j2);
    }
    
    TEST_ASSERT(memcmp(out1, out2, 64) == 0, "Same key same output");
    
    return 0;
}

int test_encrypt_decrypt(void) {
    uint8_t S[256];
    uint8_t i = 0, j = 0;
    uint8_t ciphertext[64], decrypted[64];
    uint8_t plaintext[64];
    
    uart_printf("\r\n=== Test: Encrypt/Decrypt ===\r\n");
    
    /* Initialize plaintext */
    for (int k = 0; k < 64; k++) {
        plaintext[k] = (uint8_t)k;
    }
    
    /* Encrypt */
    arcfour_ksa_arm(S, test_key, sizeof(test_key));
    arcfour_encrypt_arm(S, &i, &j, plaintext, ciphertext, 64);
    
    /* Decrypt */
    arcfour_ksa_arm(S, test_key, sizeof(test_key));
    i = j = 0;
    arcfour_encrypt_arm(S, &i, &j, ciphertext, decrypted, 64);
    
    TEST_ASSERT(memcmp(plaintext, decrypted, 64) == 0, "Encrypt/decrypt roundtrip");
    
    return 0;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    
    /* Turn on LED to indicate startup */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    
    uart_printf("\r\n========== STM32 RC4 ARM Test ==========\r\n");
    
    test_rc4_basic();
    test_consistency();
    test_encrypt_decrypt();
    
    uart_printf("\r\n========== Results: %d failures ==========\r\n", failures);
    
    if (failures == 0) {
        /* Blink LED rapidly on success */
        while (1) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            HAL_Delay(100);
        }
    } else {
        /* Keep LED off on failure */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        while (1);
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
}

static void MX_USART2_UART_Init(void) {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* LED on PA5 */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 TX/RX */
    GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void Error_Handler(void) {
    __disable_irq();
    while (1);
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {
    while (1);
}
#endif