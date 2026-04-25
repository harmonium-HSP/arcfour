#include "arcfour.h"
#include <string.h>

void arcfour_ksa_generic(uint8_t* S, const uint8_t* key, size_t key_len) {
    // Initialize S-box
    for (int i = 0; i < 256; i++) {
        S[i] = (uint8_t)i;
    }
    
    // KSA swapping phase
    uint8_t j = 0;
    for (int i = 0; i < 256; i++) {
        j += S[i] + key[i % key_len];
        
        uint8_t temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }
}

uint8_t arcfour_byte_generic(uint8_t* S, uint8_t* i, uint8_t* j) {
    *i = (*i + 1) & 0xFF;
    *j = (*j + S[*i]) & 0xFF;
    
    uint8_t temp = S[*i];
    S[*i] = S[*j];
    S[*j] = temp;
    
    uint8_t t = (S[*i] + S[*j]) & 0xFF;
    return S[t];
}

void arcfour_encrypt_generic(uint8_t* S, uint8_t* i, uint8_t* j,
                             const uint8_t* in, uint8_t* out, size_t len) {
    for (size_t k = 0; k < len; k++) {
        *i = (*i + 1) & 0xFF;
        *j = (*j + S[*i]) & 0xFF;
        
        uint8_t temp = S[*i];
        S[*i] = S[*j];
        S[*j] = temp;
        
        uint8_t t = (S[*i] + S[*j]) & 0xFF;
        out[k] = in[k] ^ S[t];
    }
}

void arcfour_decrypt_generic(uint8_t* S, uint8_t* i, uint8_t* j,
                             const uint8_t* in, uint8_t* out, size_t len) {
    // RC4 encryption and decryption are identical
    arcfour_encrypt_generic(S, i, j, in, out, len);
}