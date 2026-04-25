from cffi import FFI

ffi = FFI()

ffi_def = """
typedef struct arcfour_ctx arcfour_ctx;

arcfour_ctx* arcfour_init(const uint8_t* key, size_t key_len);
void arcfour_uninit(arcfour_ctx* ctx);
void arcfour_encrypt(arcfour_ctx* ctx, const uint8_t* plaintext, uint8_t* ciphertext, size_t len);
void arcfour_decrypt(arcfour_ctx* ctx, const uint8_t* ciphertext, uint8_t* plaintext, size_t len);

int aead_rc4_encrypt(
    const uint8_t* key, size_t key_len,
    const uint8_t* nonce, size_t nonce_len,
    const uint8_t* aad, size_t aad_len,
    const uint8_t* plaintext, size_t plaintext_len,
    uint8_t* ciphertext, size_t* ciphertext_len,
    uint8_t* tag
);

int aead_rc4_decrypt(
    const uint8_t* key, size_t key_len,
    const uint8_t* nonce, size_t nonce_len,
    const uint8_t* aad, size_t aad_len,
    const uint8_t* ciphertext, size_t ciphertext_len,
    const uint8_t* tag,
    uint8_t* plaintext, size_t* plaintext_len
);
"""

ffi.cdef(ffi_def)

import os
import sys

lib_path = None
if sys.platform.startswith('win'):
    lib_path = os.path.join(os.path.dirname(__file__), '..', '..', 'build', 'libarcfour.dll')
elif sys.platform.startswith('linux'):
    lib_path = os.path.join(os.path.dirname(__file__), '..', '..', 'build', 'libarcfour.so')
elif sys.platform.startswith('darwin'):
    lib_path = os.path.join(os.path.dirname(__file__), '..', '..', 'build', 'libarcfour.dylib')

if lib_path and os.path.exists(lib_path):
    lib = ffi.dlopen(lib_path)
else:
    try:
        lib = ffi.dlopen('arcfour')
    except:
        raise RuntimeError("Could not load arcfour library. Please build the library first.")