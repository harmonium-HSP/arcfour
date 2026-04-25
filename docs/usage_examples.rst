Usage Examples
==============

Basic Encryption
----------------

.. code-block:: c

    #include <stdio.h>
    #include <string.h>
    #include "arcfour.h"

    int main() {
        // Create a secure random key
        uint8_t key[32];
        // Fill key from secure random source...

        // Initialize context
        arcfour_ctx* ctx = arcfour_init(key, sizeof(key));
        if (!ctx) {
            fprintf(stderr, "Initialization failed\n");
            return 1;
        }

        // Data to encrypt
        const char* message = "Hello, ARCFOUR!";
        uint8_t ciphertext[100];
        uint8_t decrypted[100];

        // Encrypt
        arcfour_encrypt(ctx, (const uint8_t*)message, ciphertext, strlen(message));

        // Re-initialize for decryption
        arcfour_ctx* ctx2 = arcfour_init(key, sizeof(key));
        
        // Decrypt
        arcfour_decrypt(ctx2, ciphertext, decrypted, strlen(message));

        printf("Original: %s\n", message);
        printf("Decrypted: %s\n", decrypted);

        // Cleanup
        arcfour_uninit(ctx);
        arcfour_uninit(ctx2);

        return 0;
    }

Static Memory Allocation (Embedded)
-----------------------------------

.. code-block:: c

    #include "arcfour_static.h"

    void encrypt_data(const uint8_t* key, const uint8_t* input, 
                      uint8_t* output, size_t len) {
        // Static context - no heap allocation
        arcfour_ctx_t ctx;
        
        // Initialize with key
        arcfour_init_static(&ctx, key, 32);
        
        // Encrypt in-place
        arcfour_encrypt_static(&ctx, input, output, len);
        
        // No cleanup needed - context is static
    }

Power-Aware Encryption
----------------------

.. code-block:: c

    #include "arcfour_power.h"

    // Power management callbacks
    int power_up_callback(void) {
        // Enable high-performance mode
        set_cpu_frequency(MAX_FREQ);
        return 0;
    }

    void power_down_callback(void) {
        // Return to low-power mode
        set_cpu_frequency(MIN_FREQ);
    }

    void low_battery_callback(void) {
        // Handle low battery condition
        save_critical_data();
    }

    int main() {
        uint8_t key[32];
        // Fill key...

        // Configure power hooks
        arcfour_power_hooks_t hooks = {
            .before_operation = power_up_callback,
            .after_operation = power_down_callback,
            .on_low_battery = low_battery_callback,
            .timeout_ms = 100
        };

        // Create power-aware context
        arcfour_ctx* ctx = arcfour_init_power_aware(key, 32, &hooks);
        
        uint8_t data[256];
        uint8_t encrypted[256];

        // Encrypt with power management
        arcfour_encrypt_power_aware(ctx, data, encrypted, 256, &hooks);

        arcfour_uninit(ctx);
        return 0;
    }

File Encryption
---------------

.. code-block:: c

    #include "arcfour_utils.h"

    int encrypt_file(const char* input_path, const char* output_path, 
                     const uint8_t* key, size_t key_len) {
        // Encrypt file using AEAD mode
        return arcfour_encrypt_file(input_path, output_path, key, key_len);
    }

    int decrypt_file(const char* input_path, const char* output_path, 
                     const uint8_t* key, size_t key_len) {
        // Decrypt file
        return arcfour_decrypt_file(input_path, output_path, key, key_len);
    }

Python Bindings
---------------

.. code-block:: python

    from arcfour import ARCFOUR

    # Create cipher instance
    cipher = ARCFOUR(b'my_secret_key_32_bytes')

    # Encrypt
    plaintext = b'Sensitive data'
    ciphertext = cipher.encrypt(plaintext)

    # Decrypt
    decrypted = cipher.decrypt(ciphertext)

    print(f"Original: {plaintext}")
    print(f"Decrypted: {decrypted}")
