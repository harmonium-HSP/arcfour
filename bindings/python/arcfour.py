import os
from .arcfour_cffi import ffi, lib

class ArcFour:
    def __init__(self, key):
        if not isinstance(key, bytes):
            raise TypeError("key must be bytes")
        
        self._ctx = lib.arcfour_init(key, len(key))
        if self._ctx == ffi.NULL:
            raise ValueError("Failed to initialize RC4 context")
    
    def encrypt(self, plaintext):
        if not isinstance(plaintext, bytes):
            raise TypeError("plaintext must be bytes")
        
        ciphertext = ffi.new("uint8_t[]", len(plaintext))
        lib.arcfour_encrypt(self._ctx, plaintext, ciphertext, len(plaintext))
        return bytes(ffi.buffer(ciphertext, len(plaintext)))
    
    def decrypt(self, ciphertext):
        return self.encrypt(ciphertext)
    
    def close(self):
        if self._ctx != ffi.NULL:
            lib.arcfour_uninit(self._ctx)
            self._ctx = ffi.NULL
    
    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
    
    def __del__(self):
        self.close()

class ArcFourAEAD:
    def __init__(self, key):
        if not isinstance(key, bytes):
            raise TypeError("key must be bytes")
        if len(key) not in (16, 24, 32):
            raise ValueError("key length must be 16, 24, or 32 bytes")
        self._key = key
    
    def encrypt(self, plaintext, aad=b""):
        if not isinstance(plaintext, bytes):
            raise TypeError("plaintext must be bytes")
        if not isinstance(aad, bytes):
            raise TypeError("aad must be bytes")
        
        nonce = self.generate_nonce()
        ciphertext = ffi.new("uint8_t[]", len(plaintext))
        cipher_len = ffi.new("size_t*")
        tag = ffi.new("uint8_t[16]")
        
        result = lib.aead_rc4_encrypt(
            self._key, len(self._key),
            nonce, len(nonce),
            aad, len(aad),
            plaintext, len(plaintext),
            ciphertext, cipher_len,
            tag
        )
        
        if result != 0:
            raise RuntimeError("Encryption failed")
        
        return bytes(ffi.buffer(nonce, 12)), bytes(ffi.buffer(ciphertext, cipher_len[0])), bytes(ffi.buffer(tag, 16))
    
    def decrypt(self, nonce, ciphertext, tag, aad=b""):
        if not isinstance(nonce, bytes) or len(nonce) != 12:
            raise ValueError("nonce must be 12 bytes")
        if not isinstance(ciphertext, bytes):
            raise TypeError("ciphertext must be bytes")
        if not isinstance(tag, bytes) or len(tag) != 16:
            raise ValueError("tag must be 16 bytes")
        if not isinstance(aad, bytes):
            raise TypeError("aad must be bytes")
        
        plaintext = ffi.new("uint8_t[]", len(ciphertext))
        plain_len = ffi.new("size_t*")
        
        result = lib.aead_rc4_decrypt(
            self._key, len(self._key),
            nonce, len(nonce),
            aad, len(aad),
            ciphertext, len(ciphertext),
            tag,
            plaintext, plain_len
        )
        
        if result != 0:
            raise ValueError("Decryption failed: tag verification failed")
        
        return bytes(ffi.buffer(plaintext, plain_len[0]))
    
    @staticmethod
    def generate_key(key_len=32):
        if key_len not in (16, 24, 32):
            raise ValueError("key_len must be 16, 24, or 32")
        return os.urandom(key_len)
    
    @staticmethod
    def generate_nonce():
        return os.urandom(12)