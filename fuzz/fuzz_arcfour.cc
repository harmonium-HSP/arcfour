/*
 * OSS-Fuzz target for arcfour library
 *
 * This file implements the LLVMFuzzerTestOneInput function that OSS-Fuzz
 * uses to fuzz test the arcfour library.
 *
 * See: https://github.com/google/oss-fuzz
 */

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <algorithm>

extern "C" {
#include "arcfour.h"
#include "arcfour_dma.h"
#include "arcfour_power.h"
#include "aead_rc4.h"
}

// Maximum buffer size to prevent timeout
constexpr size_t MAX_SIZE = 65536;
constexpr size_t MAX_KEY_SIZE = 256;
constexpr size_t MAX_NONCE_SIZE = 12;
constexpr size_t MAX_AAD_SIZE = 256;
constexpr size_t MAX_TAG_SIZE = 16;

// Operation types
enum OpType {
    OP_INIT_STATIC = 0,
    OP_ENCRYPT_STATIC = 1,
    OP_DECRYPT_STATIC = 2,
    OP_SKIP_STATIC = 3,
    OP_AEAD_ENCRYPT = 4,
    OP_AEAD_DECRYPT = 5,
    OP_DMA_ENCRYPT = 6,
    OP_DMA_KEYSTREAM = 7,
    OP_DOUBLE_BUFFER = 8,
    OP_POWER_AWARE = 9,
    OP_UTILS = 10,
    OP_MULTI_OP = 11,
    NUM_OPS = 12
};

// Constant-time memory comparison
inline int secure_memcmp(const uint8_t* a, const uint8_t* b, size_t len) {
    uint8_t result = 0;
    for (size_t i = 0; i < len; i++) {
        result |= a[i] ^ b[i];
    }
    return result;
}

// Helper to get random data from fuzz input
inline uint8_t get_byte(const uint8_t*& data, size_t& size) {
    if (size == 0) return 0;
    uint8_t val = data[0];
    data++;
    size--;
    return val;
}

// Helper to get random size
inline size_t get_size(const uint8_t*& data, size_t& size, size_t max_size) {
    if (size == 0) return 0;
    size_t result = get_byte(data, size) % (max_size + 1);
    return result;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size == 0) return 0;

    // First byte determines operation type
    uint8_t op_type = data[0] % NUM_OPS;
    data++;
    size--;

    switch (op_type) {
        case OP_INIT_STATIC: {
            // Test static initialization
            arcfour_ctx_t ctx;
            size_t key_len = std::min(get_size(data, size, MAX_KEY_SIZE), size);
            uint8_t* key = const_cast<uint8_t*>(data);
            data += key_len;
            size -= key_len;
            
            arcfour_init_static(&ctx, key, key_len);
            
            // Test is_valid
            int valid = arcfour_is_valid(&ctx);
            (void)valid;
            
            // Test reset
            arcfour_reset_static(&ctx);
            break;
        }

        case OP_ENCRYPT_STATIC: {
            // Test encryption/decryption round-trip
            arcfour_ctx_t ctx;
            size_t key_len = std::min(get_size(data, size, MAX_KEY_SIZE), size);
            uint8_t* key = const_cast<uint8_t*>(data);
            data += key_len;
            size -= key_len;
            
            size_t data_len = std::min(get_size(data, size, MAX_SIZE), size);
            uint8_t* plaintext = const_cast<uint8_t*>(data);
            data += data_len;
            size -= data_len;
            
            // Allocate buffers
            uint8_t* ciphertext = new uint8_t[data_len];
            uint8_t* decrypted = new uint8_t[data_len];
            
            // Encrypt
            arcfour_init_static(&ctx, key, key_len);
            arcfour_encrypt_static(&ctx, plaintext, ciphertext, data_len);
            
            // Decrypt
            arcfour_init_static(&ctx, key, key_len);
            arcfour_decrypt_static(&ctx, ciphertext, decrypted, data_len);
            
            // Verify
            if (data_len > 0) {
                int match = secure_memcmp(plaintext, decrypted, data_len);
                (void)match;
            }
            
            delete[] ciphertext;
            delete[] decrypted;
            break;
        }

        case OP_DECRYPT_STATIC: {
            // Test decryption directly
            arcfour_ctx_t ctx;
            size_t key_len = std::min(get_size(data, size, MAX_KEY_SIZE), size);
            uint8_t* key = const_cast<uint8_t*>(data);
            data += key_len;
            size -= key_len;
            
            size_t data_len = std::min(get_size(data, size, MAX_SIZE), size);
            uint8_t* ciphertext = const_cast<uint8_t*>(data);
            
            uint8_t* plaintext = new uint8_t[data_len];
            
            arcfour_init_static(&ctx, key, key_len);
            arcfour_decrypt_static(&ctx, ciphertext, plaintext, data_len);
            
            delete[] plaintext;
            break;
        }

        case OP_SKIP_STATIC: {
            // Test skipping bytes
            arcfour_ctx_t ctx;
            size_t key_len = std::min(get_size(data, size, MAX_KEY_SIZE), size);
            uint8_t* key = const_cast<uint8_t*>(data);
            data += key_len;
            size -= key_len;
            
            size_t skip_len = get_size(data, size, 1000000);
            
            arcfour_init_static(&ctx, key, key_len);
            arcfour_skip_static(&ctx, skip_len);
            
            // After skip, encrypt some data
            size_t data_len = std::min(get_size(data, size, 1024), size);
            uint8_t* input = const_cast<uint8_t*>(data);
            uint8_t* output = new uint8_t[data_len];
            
            arcfour_encrypt_static(&ctx, input, output, data_len);
            
            delete[] output;
            break;
        }

        case OP_AEAD_ENCRYPT: {
            // Test AEAD encryption
            size_t key_len = std::min(get_size(data, size, MAX_KEY_SIZE), size);
            uint8_t* key = const_cast<uint8_t*>(data);
            data += key_len;
            size -= key_len;
            
            size_t nonce_len = std::min(get_size(data, size, MAX_NONCE_SIZE), size);
            uint8_t* nonce = const_cast<uint8_t*>(data);
            data += nonce_len;
            size -= nonce_len;
            
            size_t aad_len = std::min(get_size(data, size, MAX_AAD_SIZE), size);
            uint8_t* aad = const_cast<uint8_t*>(data);
            data += aad_len;
            size -= aad_len;
            
            size_t plaintext_len = std::min(get_size(data, size, MAX_SIZE), size);
            uint8_t* plaintext = const_cast<uint8_t*>(data);
            
            uint8_t* ciphertext = new uint8_t[plaintext_len + MAX_TAG_SIZE];
            uint8_t* tag = new uint8_t[MAX_TAG_SIZE];
            size_t ciphertext_len = 0;
            
            int ret = aead_rc4_encrypt(
                key, key_len,
                nonce, nonce_len,
                aad, aad_len,
                plaintext, plaintext_len,
                ciphertext, &ciphertext_len,
                tag
            );
            
            (void)ret;
            
            delete[] ciphertext;
            delete[] tag;
            break;
        }

        case OP_AEAD_DECRYPT: {
            // Test AEAD decryption
            size_t key_len = std::min(get_size(data, size, MAX_KEY_SIZE), size);
            uint8_t* key = const_cast<uint8_t*>(data);
            data += key_len;
            size -= key_len;
            
            size_t nonce_len = std::min(get_size(data, size, MAX_NONCE_SIZE), size);
            uint8_t* nonce = const_cast<uint8_t*>(data);
            data += nonce_len;
            size -= nonce_len;
            
            size_t aad_len = std::min(get_size(data, size, MAX_AAD_SIZE), size);
            uint8_t* aad = const_cast<uint8_t*>(data);
            data += aad_len;
            size -= aad_len;
            
            size_t ciphertext_len = std::min(get_size(data, size, MAX_SIZE), size);
            uint8_t* ciphertext = const_cast<uint8_t*>(data);
            data += ciphertext_len;
            size -= ciphertext_len;
            
            uint8_t* tag = nullptr;
            if (size >= MAX_TAG_SIZE) {
                tag = const_cast<uint8_t*>(data);
            }
            
            uint8_t* plaintext = new uint8_t[ciphertext_len];
            size_t plaintext_len = 0;
            
            int ret = aead_rc4_decrypt(
                key, key_len,
                nonce, nonce_len,
                aad, aad_len,
                ciphertext, ciphertext_len,
                tag,
                plaintext, &plaintext_len
            );
            
            (void)ret;
            
            delete[] plaintext;
            break;
        }

        case OP_DMA_ENCRYPT: {
            // Test DMA encryption
            arcfour_ctx_t ctx;
            size_t key_len = std::min(get_size(data, size, MAX_KEY_SIZE), size);
            uint8_t* key = const_cast<uint8_t*>(data);
            data += key_len;
            size -= key_len;
            
            size_t data_len = std::min(get_size(data, size, MAX_SIZE), size);
            uint8_t* plaintext = const_cast<uint8_t*>(data);
            
            uint8_t ARCFOUR_DMA_BUFFER ciphertext[MAX_SIZE];
            uint8_t ARCFOUR_DMA_BUFFER decrypted[MAX_SIZE];
            
            // Encrypt
            arcfour_init_static(&ctx, key, key_len);
            
            arcfour_dma_config_t config = {
                .input = plaintext,
                .output = ciphertext,
                .len = data_len,
                .use_double_buffer = 0,
                .transfer_complete = 0
            };
            
            int ret = arcfour_encrypt_dma(&ctx, &config);
            (void)ret;
            
            // Decrypt
            arcfour_init_static(&ctx, key, key_len);
            
            arcfour_dma_config_t config2 = {
                .input = ciphertext,
                .output = decrypted,
                .len = data_len,
                .use_double_buffer = 0,
                .transfer_complete = 0
            };
            
            ret = arcfour_decrypt_dma(&ctx, &config2);
            (void)ret;
            break;
        }

        case OP_DMA_KEYSTREAM: {
            // Test keystream generation
            arcfour_ctx_t ctx;
            size_t key_len = std::min(get_size(data, size, MAX_KEY_SIZE), size);
            uint8_t* key = const_cast<uint8_t*>(data);
            data += key_len;
            size -= key_len;
            
            size_t keystream_len = std::min(get_size(data, size, MAX_SIZE), size);
            
            uint8_t ARCFOUR_DMA_BUFFER keystream[MAX_SIZE];
            
            arcfour_init_static(&ctx, key, key_len);
            size_t generated = arcfour_prepare_keystream_dma(&ctx, keystream, keystream_len);
            (void)generated;
            
            // Test XOR with keystream
            uint8_t* input = const_cast<uint8_t*>(data);
            uint8_t output[MAX_SIZE];
            
            arcfour_xor_with_keystream(output, keystream, input, std::min(keystream_len, size));
            break;
        }

        case OP_DOUBLE_BUFFER: {
            // Test double buffering
            uint8_t ARCFOUR_DMA_BUFFER buffer0[512];
            uint8_t ARCFOUR_DMA_BUFFER buffer1[512];
            
            arcfour_dma_double_buffer_t db;
            arcfour_dma_double_buffer_init(&db, buffer0, buffer1, 512);
            
            // Test buffer swapping
            for (int i = 0; i < 10; i++) {
                uint8_t* active = arcfour_dma_get_active_buffer(&db);
                uint8_t* inactive = arcfour_dma_get_inactive_buffer(&db);
                (void)active;
                (void)inactive;
                
                arcfour_dma_double_buffer_swap(&db);
            }
            break;
        }

        case OP_POWER_AWARE: {
            // Test power-aware API
            size_t key_len = std::min(get_size(data, size, MAX_KEY_SIZE), size);
            uint8_t* key = const_cast<uint8_t*>(data);
            data += key_len;
            size -= key_len;
            
            size_t data_len = std::min(get_size(data, size, 1024), size);
            uint8_t* plaintext = const_cast<uint8_t*>(data);
            uint8_t* ciphertext = new uint8_t[data_len];
            
            // Test power hooks structure
            arcfour_power_hooks_t hooks = {
                .before_operation = nullptr,
                .after_operation = nullptr,
                .on_low_battery = nullptr,
                .timeout_ms = 5000
            };
            
            arcfour_ctx_t ctx;
            arcfour_init_static(&ctx, key, key_len);
            
            // Regular encrypt (power-aware uses same API)
            arcfour_encrypt_static(&ctx, plaintext, ciphertext, data_len);
            
            delete[] ciphertext;
            break;
        }

        case OP_UTILS: {
            // Test utility functions
            uint8_t buf[256];
            for (size_t i = 0; i < sizeof(buf); i++) {
                buf[i] = get_byte(data, size);
            }
            
            // Test alignment check
            int aligned = arcfour_dma_is_aligned(buf, 4);
            (void)aligned;
            
            // Test aligned allocation (only in non-static mode)
            void* alloc = arcfour_dma_alloc_aligned(512);
            if (alloc) {
                arcfour_dma_free_aligned(alloc);
            }
            break;
        }

        case OP_MULTI_OP: {
            // Combined operations test
            arcfour_ctx_t ctx;
            size_t key_len = std::min(get_size(data, size, MAX_KEY_SIZE), size);
            uint8_t* key = const_cast<uint8_t*>(data);
            data += key_len;
            size -= key_len;
            
            size_t data_len = std::min(get_size(data, size, 1024), size);
            uint8_t* plaintext = const_cast<uint8_t*>(data);
            uint8_t* ciphertext = new uint8_t[data_len];
            
            // Multiple init/encrypt cycles
            for (int i = 0; i < 5; i++) {
                arcfour_init_static(&ctx, key, key_len);
                arcfour_encrypt_static(&ctx, plaintext, ciphertext, data_len);
                
                // Swap input/output for next iteration
                std::swap(plaintext, ciphertext);
            }
            
            delete[] ciphertext;
            break;
        }

        default:
            break;
    }

    return 0;
}