#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "aead_rc4.h"

static int test_encrypt_decrypt(void) {
    uint8_t key[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                       0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    uint8_t nonce[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4a};
    uint8_t plaintext[16] = "Hello, World!";
    uint8_t aad[8] = "AADTEST";
    uint8_t ciphertext[16] = {0};
    uint8_t decrypted[16] = {0};
    uint8_t tag[16] = {0};
    size_t cipher_len = 0;
    size_t decrypt_len = 0;
    
    int ret = aead_rc4_encrypt(key, 16, nonce, 8, aad, 7, plaintext, 13, ciphertext, &cipher_len, tag);
    if (ret != 0) {
        printf("❌ Encryption failed\n");
        return -1;
    }
    
    ret = aead_rc4_decrypt(key, 16, nonce, 8, aad, 7, ciphertext, cipher_len, tag, decrypted, &decrypt_len);
    if (ret != 0) {
        printf("❌ Decryption failed\n");
        return -1;
    }
    
    if (memcmp(plaintext, decrypted, 13) != 0) {
        printf("❌ Plaintext mismatch\n");
        return -1;
    }
    
    printf("✅ Encrypt/Decrypt test PASSED\n");
    return 0;
}

static int test_tamper_detection(void) {
    uint8_t key[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                       0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    uint8_t nonce[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4a};
    uint8_t plaintext[16] = "Secret Data";
    uint8_t ciphertext[16] = {0};
    uint8_t decrypted[16] = {0};
    uint8_t tag[16] = {0};
    size_t cipher_len = 0;
    size_t decrypt_len = 0;
    
    int ret = aead_rc4_encrypt(key, 16, nonce, 8, NULL, 0, plaintext, 11, ciphertext, &cipher_len, tag);
    if (ret != 0) {
        printf("❌ Encryption failed\n");
        return -1;
    }
    
    tag[0] ^= 0xFF;
    
    ret = aead_rc4_decrypt(key, 16, nonce, 8, NULL, 0, ciphertext, cipher_len, tag, decrypted, &decrypt_len);
    if (ret == 0) {
        printf("❌ Tamper detection: Should have failed\n");
        return -1;
    }
    
    printf("✅ Tamper detection: Tag corruption detected\n");
    return 0;
}

static int test_with_aad(void) {
    uint8_t key[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                       0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    uint8_t nonce[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4a};
    uint8_t plaintext[20] = "Sensitive Data";
    uint8_t aad1[10] = "METADATA1";
    uint8_t aad2[10] = "METADATA2";
    uint8_t ciphertext[20] = {0};
    uint8_t decrypted[20] = {0};
    uint8_t tag[16] = {0};
    size_t cipher_len = 0;
    size_t decrypt_len = 0;
    
    int ret = aead_rc4_encrypt(key, 16, nonce, 8, aad1, 9, plaintext, 14, ciphertext, &cipher_len, tag);
    if (ret != 0) {
        printf("❌ Encryption failed\n");
        return -1;
    }
    
    ret = aead_rc4_decrypt(key, 16, nonce, 8, aad2, 9, ciphertext, cipher_len, tag, decrypted, &decrypt_len);
    if (ret == 0) {
        printf("❌ AAD test: Wrong AAD should be rejected\n");
        return -1;
    }
    
    printf("✅ AAD test: Wrong AAD rejected\n");
    return 0;
}

static int test_empty_data(void) {
    uint8_t key[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
                       0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f};
    uint8_t nonce[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4a};
    uint8_t ciphertext[1] = {0};
    uint8_t decrypted[1] = {0};
    uint8_t tag[16] = {0};
    size_t cipher_len = 0;
    size_t decrypt_len = 0;
    
    int ret = aead_rc4_encrypt(key, 16, nonce, 8, NULL, 0, NULL, 0, ciphertext, &cipher_len, tag);
    if (ret != 0) {
        printf("❌ Empty encryption failed\n");
        return -1;
    }
    
    ret = aead_rc4_decrypt(key, 16, nonce, 8, NULL, 0, ciphertext, cipher_len, tag, decrypted, &decrypt_len);
    if (ret != 0) {
        printf("❌ Empty decryption failed\n");
        return -1;
    }
    
    printf("✅ Empty data test PASSED\n");
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("========== RC4 + Poly1305 AEAD Tests ==========\n\n");
    
    printf("=== Test 1: Encrypt/Decrypt Consistency ===\n");
    if (test_encrypt_decrypt() != 0) failures++;
    
    printf("\n=== Test 2: Tamper Detection ===\n");
    if (test_tamper_detection() != 0) failures++;
    
    printf("\n=== Test 3: With Associated Data ===\n");
    if (test_with_aad() != 0) failures++;
    
    printf("\n=== Test 4: Empty Data ===\n");
    if (test_empty_data() != 0) failures++;
    
    printf("\n========== Results: %d failures ==========\n", failures);
    
    return failures != 0 ? 1 : 0;
}
