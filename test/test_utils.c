#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "arcfour_utils.h"

static void progress_callback(size_t bytes_processed, size_t total_bytes, void* user_data) {
    (void)bytes_processed;
    (void)total_bytes;
    (void)user_data;
}

static int test_file_encrypt_decrypt(void) {
    const char* plain_file = "test_plain.txt";
    const char* encrypted_file = "test_encrypted.arcf";
    const char* decrypted_file = "test_decrypted.txt";
    
    FILE* f = fopen(plain_file, "wb");
    if (!f) {
        printf("❌ Failed to create test file\n");
        return -1;
    }
    
    const char* test_data = "Hello, file encryption test! This is a test message to verify file encryption and decryption functionality.\n";
    fwrite(test_data, 1, strlen(test_data), f);
    fclose(f);
    
    uint8_t key[32];
    for (size_t i = 0; i < 32; i++) {
        key[i] = (uint8_t)(i + 1);
    }
    
    int ret = arcfour_encrypt_file(plain_file, encrypted_file, key, 32, progress_callback, NULL);
    if (ret != 0) {
        printf("❌ File encryption failed\n");
        return -1;
    }
    
    ret = arcfour_decrypt_file(encrypted_file, decrypted_file, key, 32, progress_callback, NULL);
    if (ret != 0) {
        printf("❌ File decryption failed\n");
        return -1;
    }
    
    f = fopen(decrypted_file, "rb");
    if (!f) {
        printf("❌ Failed to open decrypted file\n");
        return -1;
    }
    
    char buffer[200];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer) - 1, f);
    buffer[bytes_read] = '\0';
    fclose(f);
    
    if (strcmp(buffer, test_data) != 0) {
        printf("❌ Decrypted content mismatch\n");
        return -1;
    }
    
    remove(plain_file);
    remove(encrypted_file);
    remove(decrypted_file);
    
    printf("✅ File encrypt/decrypt test PASSED\n");
    return 0;
}

static int test_integrity_check(void) {
    const char* plain_file = "test_plain2.txt";
    const char* encrypted_file = "test_encrypted2.arcf";
    const char* decrypted_file = "test_decrypted2.txt";
    
    FILE* f = fopen(plain_file, "wb");
    if (!f) {
        printf("❌ Failed to create test file\n");
        return -1;
    }
    
    const char* test_data = "Integrity test data";
    fwrite(test_data, 1, strlen(test_data), f);
    fclose(f);
    
    uint8_t key[32];
    for (size_t i = 0; i < 32; i++) {
        key[i] = (uint8_t)(i + 1);
    }
    
    int ret = arcfour_encrypt_file(plain_file, encrypted_file, key, 32, progress_callback, NULL);
    if (ret != 0) {
        printf("❌ File encryption failed\n");
        return -1;
    }
    
    f = fopen(encrypted_file, "r+b");
    if (!f) {
        printf("❌ Failed to open encrypted file for tampering\n");
        return -1;
    }
    
    fseek(f, 10, SEEK_SET);
    uint8_t val = 0xFF;
    fwrite(&val, 1, 1, f);
    fclose(f);
    
    ret = arcfour_decrypt_file(encrypted_file, decrypted_file, key, 32, progress_callback, NULL);
    if (ret == 0) {
        printf("❌ Tampered file should not decrypt successfully\n");
        return -1;
    }
    
    remove(plain_file);
    remove(encrypted_file);
    if (remove(decrypted_file) == 0) {
        remove(decrypted_file);
    }
    
    printf("✅ Integrity check test PASSED\n");
    return 0;
}

int main(void) {
    int failures = 0;
    
    printf("========== ARCFOUR Utils Tests ==========\n\n");
    
    printf("=== Test 1: File Encrypt/Decrypt ===\n");
    if (test_file_encrypt_decrypt() != 0) failures++;
    
    printf("\n=== Test 2: Integrity Check ===\n");
    if (test_integrity_check() != 0) failures++;
    
    printf("\n========== Results: %d failures ==========\n", failures);
    
    return failures != 0 ? 1 : 0;
}