#ifndef ARCFOUR_POWER_H
#define ARCFOUR_POWER_H

#include <stdint.h>
#include <stddef.h>

#include "arcfour.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Power management hooks structure
 * 
 * This structure defines callbacks that are called before and after
 * cryptographic operations to manage power states.
 */
typedef struct {
    /**
     * @brief Called before encryption/decryption operations
     * 
     * Use this to wake up the system, enable peripherals, etc.
     * Return 0 on success, non-zero to abort the operation.
     */
    int (*before_operation)(void);
    
    /**
     * @brief Called after encryption/decryption operations
     * 
     * Use this to put the system back to low-power mode.
     */
    void (*after_operation)(void);
    
    /**
     * @brief Called when low battery is detected
     * 
     * Return 0 to abort operation, non-zero to continue with reduced performance.
     */
    int (*on_low_battery)(uint16_t voltage_mv);
    
    /**
     * @brief Timeout in milliseconds before auto-sleep
     * 
     * Set to 0 to disable auto-sleep feature.
     */
    uint32_t timeout_ms;
} arcfour_power_hooks_t;

/**
 * @brief Initialize RC4 with power management hooks
 * 
 * @param key Encryption key
 * @param key_len Key length in bytes
 * @param hooks Power management hooks (can be NULL for defaults)
 * @return Pointer to RC4 context, or NULL on failure
 */
arcfour_ctx* arcfour_init_power_aware(const uint8_t* key, size_t key_len,
                                       const arcfour_power_hooks_t* hooks);

/**
 * @brief Encrypt data with power management
 * 
 * This function calls the before_operation hook before encryption
 * and the after_operation hook after encryption.
 * 
 * @param ctx RC4 context
 * @param plaintext Input plaintext (can be NULL for keystream only)
 * @param ciphertext Output ciphertext
 * @param len Length of data to encrypt
 * @param hooks Power management hooks (NULL uses defaults)
 * @return 0 on success, non-zero on failure
 */
int arcfour_encrypt_power_aware(arcfour_ctx* ctx, const uint8_t* plaintext,
                                uint8_t* ciphertext, size_t len,
                                const arcfour_power_hooks_t* hooks);

/**
 * @brief Decrypt data with power management
 * 
 * This function calls the before_operation hook before decryption
 * and the after_operation hook after decryption.
 * 
 * @param ctx RC4 context
 * @param ciphertext Input ciphertext
 * @param plaintext Output plaintext
 * @param len Length of data to decrypt
 * @param hooks Power management hooks (NULL uses defaults)
 * @return 0 on success, non-zero on failure
 */
int arcfour_decrypt_power_aware(arcfour_ctx* ctx, const uint8_t* ciphertext,
                                uint8_t* plaintext, size_t len,
                                const arcfour_power_hooks_t* hooks);

/**
 * @brief Get current battery voltage
 * 
 * @return Battery voltage in millivolts, or 0 if not supported
 */
uint16_t arcfour_get_battery_voltage(void);

/**
 * @brief Check if the system should enter sleep mode
 * 
 * @param hooks Power management hooks (NULL uses defaults)
 * @return 1 if should sleep, 0 otherwise
 */
int arcfour_should_sleep(const arcfour_power_hooks_t* hooks);

/**
 * @brief Set global power management hooks
 * 
 * These hooks will be used as defaults when hooks parameter is NULL.
 * 
 * @param hooks Power management hooks to set as global defaults
 */
void arcfour_set_global_power_hooks(const arcfour_power_hooks_t* hooks);

/**
 * @brief Get global power management hooks
 * 
 * @return Pointer to global hooks structure
 */
const arcfour_power_hooks_t* arcfour_get_global_power_hooks(void);

/**
 * @brief Update last activity timestamp
 * 
 * Call this to reset the auto-sleep timer.
 */
void arcfour_update_last_activity(void);

/**
 * @brief Get milliseconds since last activity
 * 
 * @return Milliseconds since last activity
 */
uint32_t arcfour_get_idle_time_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* ARCFOUR_POWER_H */