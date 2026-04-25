#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "arcfour.h"
#include "aead_rc4.h"

static void print_hex(const uint8_t* data, size_t len, const char* label) {
    printf("%s (%zu bytes): ", label, len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0 && i + 1 < len) {
            printf("\n   ");
        }
    }
    printf("\n");
}

static void demo_aead_rc4(void) {
    printf("=== Demo 4: RC4 + Poly1305 AEAD encryption ===\n");
    
    uint8_t key[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                       0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    uint8_t nonce[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a};
    uint8_t aad[8] = "METADATA";
    const char* plaintext_str = "Hello, AEAD RC4!";
    size_t plaintext_len = strlen(plaintext_str);
    
    printf("Key: 128-bit key\n");
    printf("Nonce: ");
    for (size_t i = 0; i < sizeof(nonce); i++) {
        printf("%02X", nonce[i]);
    }
    printf("\n");
    printf("AAD: %s\n\n", aad);
    
    uint8_t ciphertext[100];
    uint8_t tag[16];
    size_t cipher_len = 0;
    
    int ret = aead_rc4_encrypt(key, 16, nonce, 8, aad, 8, 
                               (const uint8_t*)plaintext_str, plaintext_len,
                               ciphertext, &cipher_len, tag);
    if (ret != 0) {
        printf("Error: AEAD encryption failed\n");
        return;
    }
    
    print_hex((const uint8_t*)plaintext_str, plaintext_len, "Plaintext");
    print_hex(ciphertext, cipher_len, "Ciphertext");
    print_hex(tag, 16, "Authentication Tag");
    
    uint8_t decrypted[100];
    size_t decrypt_len = 0;
    
    ret = aead_rc4_decrypt(key, 16, nonce, 8, aad, 8, 
                           ciphertext, cipher_len, tag,
                           decrypted, &decrypt_len);
    if (ret != 0) {
        printf("Error: AEAD decryption failed\n");
        return;
    }
    
    decrypted[decrypt_len] = '\0';
    printf("Decrypted: %s\n", decrypted);
    
    if (memcmp(plaintext_str, decrypted, plaintext_len) == 0) {
        printf("SUCCESS: Demo 4 passed!\n\n");
    } else {
        printf("FAILED: Demo 4 failed!\n");
    }
}

int main(void) {
    const uint8_t key[] = "testkey123";
    const char* plaintext_str = "Hello, RC4!";
    size_t plaintext_len = strlen(plaintext_str);
    
    printf("=== Demo 1: Direct key encryption ===\n");
    printf("Key: %s (%zu bytes)\n\n", key, sizeof(key) - 1);
    
    arcfour_ctx* ctx = arcfour_init(key, sizeof(key) - 1);
    if (!ctx) {
        printf("Error: Failed to initialize ARCFOUR context\n");
        return 1;
    }
    
    uint8_t ciphertext[100];
    arcfour_encrypt(ctx, (const uint8_t*)plaintext_str, ciphertext, plaintext_len);
    
    print_hex((const uint8_t*)plaintext_str, plaintext_len, "Plaintext");
    print_hex(ciphertext, plaintext_len, "Ciphertext");
    
    arcfour_uninit(ctx);
    
    ctx = arcfour_init(key, sizeof(key) - 1);
    if (!ctx) {
        printf("Error: Failed to reinitialize ARCFOUR context\n");
        return 1;
    }
    
    uint8_t decrypted[100];
    arcfour_decrypt(ctx, ciphertext, decrypted, plaintext_len);
    decrypted[plaintext_len] = '\0';
    
    printf("Decrypted: %s\n", decrypted);
    
    if (memcmp(plaintext_str, decrypted, plaintext_len) == 0) {
        printf("SUCCESS: Demo 1 passed!\n\n");
    } else {
        printf("FAILED: Demo 1 failed!\n");
        arcfour_uninit(ctx);
        return 1;
    }
    
    arcfour_uninit(ctx);

    printf("=== Demo 2: Password-based key derivation ===\n");
    const char* password = "MySecretPassword123!";
    const uint8_t salt[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    const char* message = "This is a secret message.";
    size_t message_len = strlen(message);
    
    printf("Password: %s\n", password);
    printf("Salt: ");
    for (size_t i = 0; i < sizeof(salt); i++) {
        printf("%02X", salt[i]);
    }
    printf("\n");
    
    if (arcfour_key_setup(&ctx, (const uint8_t*)password, strlen(password),
                          salt, sizeof(salt), 10000) != 0) {
        printf("Error: Key setup failed!\n");
        return 1;
    }
    
    uint8_t encrypted_msg[200];
    arcfour_encrypt(ctx, (const uint8_t*)message, encrypted_msg, message_len);
    
    print_hex((const uint8_t*)message, message_len, "Message");
    print_hex(encrypted_msg, message_len, "Encrypted");
    
    arcfour_uninit(ctx);
    
    if (arcfour_key_setup(&ctx, (const uint8_t*)password, strlen(password),
                          salt, sizeof(salt), 10000) != 0) {
        printf("Error: Key setup failed!\n");
        return 1;
    }
    
    uint8_t decrypted_msg[200];
    arcfour_decrypt(ctx, encrypted_msg, decrypted_msg, message_len);
    decrypted_msg[message_len] = '\0';
    
    printf("Decrypted: %s\n", decrypted_msg);
    
    if (memcmp(message, decrypted_msg, message_len) == 0) {
        printf("SUCCESS: Demo 2 passed!\n\n");
    } else {
        printf("FAILED: Demo 2 failed!\n");
        arcfour_uninit(ctx);
        return 1;
    }
    
    arcfour_uninit(ctx);

    printf("=== Demo 3: Context copying ===\n");
    ctx = arcfour_init((const uint8_t*)"clone_test_key", 15);
    if (!ctx) {
        printf("Error: Failed to initialize context\n");
        return 1;
    }
    
    arcfour_ctx* ctx_clone = arcfour_init((const uint8_t*)"dummy_key", 9);
    if (!ctx_clone) {
        printf("Error: Failed to initialize clone context\n");
        arcfour_uninit(ctx);
        return 1;
    }
    
    arcfour_copy(ctx_clone, ctx);
    
    uint8_t stream1[10];
    uint8_t stream2[10];
    
    for (size_t i = 0; i < 10; i++) {
        uint8_t temp[1] = {0};
        arcfour_encrypt(ctx, temp, stream1 + i, 1);
        arcfour_encrypt(ctx_clone, temp, stream2 + i, 1);
    }
    
    printf("Stream from original: ");
    for (size_t i = 0; i < 10; i++) {
        printf("%02X ", stream1[i]);
    }
    printf("\n");
    
    printf("Stream from clone:    ");
    for (size_t i = 0; i < 10; i++) {
        printf("%02X ", stream2[i]);
    }
    printf("\n");
    
    if (memcmp(stream1, stream2, 10) == 0) {
        printf("SUCCESS: Demo 3 passed! Cloned context produces identical stream.\n\n");
    } else {
        printf("FAILED: Demo 3 failed! Streams differ.\n");
        arcfour_uninit(ctx);
        arcfour_uninit(ctx_clone);
        return 1;
    }
    
    arcfour_uninit(ctx);
    arcfour_uninit(ctx_clone);

    demo_aead_rc4();

    printf("=== All demonstrations completed successfully! ===\n");
    return 0;
}