#ifndef ARCFOUR_UTILS_H
#define ARCFOUR_UTILS_H

#include <stdint.h>
#include <stddef.h>

#ifdef ARCFOUR_UTILS_STATIC
#  define ARCFOUR_UTILS_API
#elif defined(_WIN32)
#  ifdef ARCFOUR_UTILS_EXPORTS
#    define ARCFOUR_UTILS_API __declspec(dllexport)
#  else
#    define ARCFOUR_UTILS_API __declspec(dllimport)
#  endif
#else
#  define ARCFOUR_UTILS_API __attribute__((visibility("default")))
#endif

#define ARCFOUR_MAGIC "ARCF"
#define ARCFOUR_MAGIC_SIZE 4
#define ARCFOUR_NONCE_SIZE 12
#define ARCFOUR_TAG_SIZE 16
#define ARCFOUR_CHUNK_SIZE (64 * 1024)

typedef void (*arcfour_progress_callback)(size_t bytes_processed, size_t total_bytes, void* user_data);

ARCFOUR_UTILS_API int arcfour_encrypt_file(
    const char* input_path,
    const char* output_path,
    const uint8_t* key,
    size_t key_len,
    arcfour_progress_callback progress_cb,
    void* user_data
);

ARCFOUR_UTILS_API int arcfour_decrypt_file(
    const char* input_path,
    const char* output_path,
    const uint8_t* key,
    size_t key_len,
    arcfour_progress_callback progress_cb,
    void* user_data
);

ARCFOUR_UTILS_API int arcfour_file_size(const char* path, size_t* size);

#endif