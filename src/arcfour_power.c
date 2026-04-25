#include "arcfour_power.h"
#include "arcfour.h"
#include "arcfour_port.h"
#include <stdint.h>

/* Forward declaration for platform-specific tick function */
uint32_t arcfour_get_tick_ms(void);

/* Global power hooks */
static arcfour_power_hooks_t global_hooks = {
    .before_operation = NULL,
    .after_operation = NULL,
    .on_low_battery = NULL,
    .timeout_ms = 0
};

/* Last activity timestamp */
static volatile uint32_t last_activity_ms = 0;

/* Helper to get current tick count */
static uint32_t get_tick_ms(void) {
#ifdef __ARM_ARCH_7M__
    /* Use DWT cycle counter if available */
    extern uint32_t SystemCoreClock;
    if (SystemCoreClock != 0) {
        return (uint32_t)(*(volatile uint32_t*)0xE0001004 / (SystemCoreClock / 1000));
    }
#endif
    
    /* Fallback to platform-specific implementation */
    return arcfour_get_tick_ms();
}

/* Helper to call before hook */
static int call_before_hook(const arcfour_power_hooks_t* hooks) {
    if (hooks == NULL) {
        hooks = &global_hooks;
    }
    
    if (hooks->before_operation != NULL) {
        return hooks->before_operation();
    }
    
    return 0;
}

/* Helper to call after hook */
static void call_after_hook(const arcfour_power_hooks_t* hooks) {
    if (hooks == NULL) {
        hooks = &global_hooks;
    }
    
    if (hooks->after_operation != NULL) {
        hooks->after_operation();
    }
}

/* Helper to check battery level */
static int check_battery(const arcfour_power_hooks_t* hooks) {
    uint16_t voltage = arcfour_get_battery_voltage();
    
    if (hooks == NULL) {
        hooks = &global_hooks;
    }
    
    /* If voltage is 0, battery monitoring is not supported */
    if (voltage == 0) {
        return 0;
    }
    
    /* Check if below typical low battery threshold (2.0V = 2000mV) */
    if (voltage < 2000) {
        if (hooks->on_low_battery != NULL) {
            return hooks->on_low_battery(voltage);
        }
        /* Default behavior: abort operation on low battery */
        return -1;
    }
    
    return 0;
}

arcfour_ctx* arcfour_init_power_aware(const uint8_t* key, size_t key_len,
                                       const arcfour_power_hooks_t* hooks) {
    if (check_battery(hooks) != 0) {
        return NULL;
    }
    
    if (call_before_hook(hooks) != 0) {
        return NULL;
    }
    
#ifdef ARCFOUR_STATIC_ONLY
    (void)key;
    (void)key_len;
    /* In static-only mode, context must be provided by caller */
    return NULL;
#else
    arcfour_ctx* ctx = arcfour_init(key, key_len);
    
    arcfour_update_last_activity();
    
    call_after_hook(hooks);
    
    return ctx;
#endif
}

int arcfour_encrypt_power_aware(arcfour_ctx* ctx, const uint8_t* plaintext,
                                uint8_t* ciphertext, size_t len,
                                const arcfour_power_hooks_t* hooks) {
    if (ctx == NULL || ciphertext == NULL) {
        return -1;
    }
    
    if (check_battery(hooks) != 0) {
        return -2;
    }
    
    if (call_before_hook(hooks) != 0) {
        return -3;
    }
    
#ifdef ARCFOUR_STATIC_ONLY
    arcfour_encrypt_static((arcfour_ctx_t*)ctx, plaintext, ciphertext, len);
#else
    arcfour_encrypt(ctx, plaintext, ciphertext, len);
#endif
    
    arcfour_update_last_activity();
    
    call_after_hook(hooks);
    
    return 0;
}

int arcfour_decrypt_power_aware(arcfour_ctx* ctx, const uint8_t* ciphertext,
                                uint8_t* plaintext, size_t len,
                                const arcfour_power_hooks_t* hooks) {
    return arcfour_encrypt_power_aware(ctx, ciphertext, plaintext, len, hooks);
}

uint16_t arcfour_get_battery_voltage(void) {
    /* Default implementation: return 0 (not supported) */
    /* Platform-specific implementations should override this */
    return 0;
}

int arcfour_should_sleep(const arcfour_power_hooks_t* hooks) {
    if (hooks == NULL) {
        hooks = &global_hooks;
    }
    
    /* If timeout is 0, auto-sleep is disabled */
    if (hooks->timeout_ms == 0) {
        return 0;
    }
    
    return arcfour_get_idle_time_ms() >= hooks->timeout_ms;
}

void arcfour_set_global_power_hooks(const arcfour_power_hooks_t* hooks) {
    if (hooks != NULL) {
        global_hooks = *hooks;
    }
}

const arcfour_power_hooks_t* arcfour_get_global_power_hooks(void) {
    return &global_hooks;
}

void arcfour_update_last_activity(void) {
    last_activity_ms = get_tick_ms();
}

uint32_t arcfour_get_idle_time_ms(void) {
    uint32_t current = get_tick_ms();
    /* Handle counter wrap-around */
    return (current >= last_activity_ms) ? 
        (current - last_activity_ms) : 
        (UINT32_MAX - last_activity_ms + current + 1);
}

/* Default platform-specific implementations for non-ARM platforms */
#if !defined(__ARM_ARCH_7M__) && !defined(__ARM_ARCH_6M__) && !defined(__ARM_ARCH_8M_BASE__)

#ifdef _WIN32
#include <stdint.h>
/* Declare GetTickCount without including windows.h */
extern uint32_t __stdcall GetTickCount(void);
#endif

#include <time.h>

/* Static default implementations - can be overridden by platform-specific code */
static uint32_t arcfour_get_tick_ms_default(void) {
    #ifdef _WIN32
    return GetTickCount();
    #else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
    #endif
}

static void arcfour_enter_sleep_default(void) {
}

static uint16_t arcfour_read_battery_mv_default(void) {
    return 0;
}

/* Exported functions that call default implementations */
uint32_t arcfour_get_tick_ms(void) __attribute__((weak, alias("arcfour_get_tick_ms_default")));
void arcfour_enter_sleep(void) __attribute__((weak, alias("arcfour_enter_sleep_default")));
uint16_t arcfour_read_battery_mv(void) __attribute__((weak, alias("arcfour_read_battery_mv_default")));

#endif