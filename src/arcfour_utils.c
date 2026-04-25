#include "arcfour_utils.h"
#include "aead_rc4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#endif

static void generate_nonce(uint8_t* nonce, size_t len) {
#ifdef _WIN32
    HCRYPTPROV hProv;
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        CryptGenRandom(hProv, (DWORD)len, nonce);
        CryptReleaseContext(hProv, 0);
        return;
    }
#endif
    srand((unsigned int)time(NULL));
    for (size_t i = 0; i < len; i++) {
        nonce[i] = (uint8_t)(rand() & 0xFF);
    }
}

ARCFOUR_UTILS_API int arcfour_file_size(const char* path, size_t* size) {
    if (!path || !size) return -1;
    
#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA attr;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &attr)) {
        return -1;
    }
    *size = (size_t)attr.nFileSizeLow;
    if (attr.nFileSizeHigh > 0) {
        #if defined(_WIN64) || (SIZE_MAX > 0xFFFFFFFF)
        *size |= ((size_t)attr.nFileSizeHigh) << 32;
        #else
        /* File too large for 32-bit system */
        return -1;
        #endif
    }
#else
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }
    *size = (size_t)st.st_size;
#endif
    return 0;
}

ARCFOUR_UTILS_API int arcfour_encrypt_file(
    const char* input_path,
    const char* output_path,
    const uint8_t* key,
    size_t key_len,
    arcfour_progress_callback progress_cb,
    void* user_data
) {
    if (!input_path || !output_path || !key) {
        return -1;
    }
    
    if (key_len != 16 && key_len != 24 && key_len != 32) {
        return -1;
    }
    
    FILE* fin = fopen(input_path, "rb");
    if (!fin) {
        return -1;
    }
    
    size_t total_size = 0;
    if (arcfour_file_size(input_path, &total_size) != 0) {
        fclose(fin);
        return -1;
    }
    
    uint8_t* plaintext = malloc(total_size);
    if (!plaintext) {
        fclose(fin);
        return -1;
    }
    
    /* Read with EINTR handling */
    size_t bytes_read = 0;
    while (bytes_read < total_size) {
        size_t ret = fread(plaintext + bytes_read, 1, total_size - bytes_read, fin);
        if (ret == 0) {
            if (ferror(fin)) {
                int should_retry = 0;
                #ifdef _WIN32
                if (GetLastError() == ERROR_OPERATION_ABORTED) {
                    should_retry = 1;
                }
                #else
                if (errno == EINTR) {
                    should_retry = 1;
                }
                #endif
                if (!should_retry) {
                    free(plaintext);
                    fclose(fin);
                    return -1;
                }
            } else {
                break;
            }
        }
        bytes_read += ret;
    }
    fclose(fin);
    
    uint8_t nonce[ARCFOUR_NONCE_SIZE];
    generate_nonce(nonce, ARCFOUR_NONCE_SIZE);
    
    size_t cipher_len = 0;
    uint8_t* ciphertext = malloc(total_size);
    uint8_t tag[ARCFOUR_TAG_SIZE];
    
    if (!ciphertext) {
        free(plaintext);
        return -1;
    }
    
    uint8_t aad[8] = "FILEAEAD";
    size_t aad_len = 8;
    
    int ret = aead_rc4_encrypt(key, key_len, nonce, ARCFOUR_NONCE_SIZE,
                               aad, aad_len, plaintext, total_size,
                               ciphertext, &cipher_len, tag);
    
    free(plaintext);
    
    if (ret != 0) {
        free(ciphertext);
        return -1;
    }
    
    FILE* fout = fopen(output_path, "wb");
    if (!fout) {
        free(ciphertext);
        return -1;
    }
    
    uint8_t magic[ARCFOUR_MAGIC_SIZE];
    memcpy(magic, ARCFOUR_MAGIC, ARCFOUR_MAGIC_SIZE);
    
    if (fwrite(magic, 1, ARCFOUR_MAGIC_SIZE, fout) != ARCFOUR_MAGIC_SIZE) {
        free(ciphertext);
        fclose(fout);
        return -1;
    }
    
    if (fwrite(nonce, 1, ARCFOUR_NONCE_SIZE, fout) != ARCFOUR_NONCE_SIZE) {
        free(ciphertext);
        fclose(fout);
        return -1;
    }
    
    if (fwrite(tag, 1, ARCFOUR_TAG_SIZE, fout) != ARCFOUR_TAG_SIZE) {
        free(ciphertext);
        fclose(fout);
        return -1;
    }
    
    if (fwrite(ciphertext, 1, cipher_len, fout) != cipher_len) {
        free(ciphertext);
        fclose(fout);
        return -1;
    }
    
    free(ciphertext);
    fclose(fout);
    
    return 0;
}

ARCFOUR_UTILS_API int arcfour_decrypt_file(
    const char* input_path,
    const char* output_path,
    const uint8_t* key,
    size_t key_len,
    arcfour_progress_callback progress_cb,
    void* user_data
) {
    if (!input_path || !output_path || !key) {
        return -1;
    }
    
    if (key_len != 16 && key_len != 24 && key_len != 32) {
        return -1;
    }
    
    FILE* fin = fopen(input_path, "rb");
    if (!fin) {
        return -1;
    }
    
    uint8_t magic[ARCFOUR_MAGIC_SIZE];
    if (fread(magic, 1, ARCFOUR_MAGIC_SIZE, fin) != ARCFOUR_MAGIC_SIZE) {
        fclose(fin);
        return -1;
    }
    
    if (memcmp(magic, ARCFOUR_MAGIC, ARCFOUR_MAGIC_SIZE) != 0) {
        fclose(fin);
        return -1;
    }
    
    uint8_t nonce[ARCFOUR_NONCE_SIZE];
    if (fread(nonce, 1, ARCFOUR_NONCE_SIZE, fin) != ARCFOUR_NONCE_SIZE) {
        fclose(fin);
        return -1;
    }
    
    uint8_t tag[ARCFOUR_TAG_SIZE];
    if (fread(tag, 1, ARCFOUR_TAG_SIZE, fin) != ARCFOUR_TAG_SIZE) {
        fclose(fin);
        return -1;
    }
    
    size_t total_size = 0;
    if (arcfour_file_size(input_path, &total_size) != 0) {
        fclose(fin);
        return -1;
    }
    
    /* Check for integer overflow in size calculation */
    size_t header_size = ARCFOUR_MAGIC_SIZE + ARCFOUR_NONCE_SIZE + ARCFOUR_TAG_SIZE;
    if (total_size < header_size) {
        fclose(fin);
        return -1;
    }
    size_t cipher_size = total_size - header_size;
    
    uint8_t* ciphertext = malloc(cipher_size);
    if (!ciphertext) {
        fclose(fin);
        return -1;
    }
    
    if (fread(ciphertext, 1, cipher_size, fin) != cipher_size) {
        free(ciphertext);
        fclose(fin);
        return -1;
    }
    fclose(fin);
    
    uint8_t aad[8] = "FILEAEAD";
    size_t aad_len = 8;
    
    size_t plain_len = 0;
    uint8_t* plaintext = malloc(cipher_size);
    
    if (!plaintext) {
        free(ciphertext);
        return -1;
    }
    
    int ret = aead_rc4_decrypt(key, key_len, nonce, ARCFOUR_NONCE_SIZE,
                               aad, aad_len, ciphertext, cipher_size,
                               tag, plaintext, &plain_len);
    
    free(ciphertext);
    
    if (ret != 0) {
        free(plaintext);
        return -1;
    }
    
    FILE* fout = fopen(output_path, "wb");
    if (!fout) {
        free(plaintext);
        return -1;
    }
    
    if (fwrite(plaintext, 1, plain_len, fout) != plain_len) {
        free(plaintext);
        fclose(fout);
        return -1;
    }
    
    free(plaintext);
    fclose(fout);
    
    return 0;
}