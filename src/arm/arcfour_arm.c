#include "arcfour_arm.h"
#include "arcfour_port.h"

void arcfour_ksa_arm(uint8_t* S, const uint8_t* key, size_t key_len) {
    uint32_t* S32 = (uint32_t*)S;
    
    // Initialize S-box with 4-byte writes (64 iterations instead of 256)
    for (int i = 0; i < 64; i++) {
        S32[i] = (uint32_t)(i * 4) | 
                 ((uint32_t)(i * 4 + 1) << 8) |
                 ((uint32_t)(i * 4 + 2) << 16) |
                 ((uint32_t)(i * 4 + 3) << 24);
    }
    
    // KSA swapping phase
    uint8_t j = 0;
    for (int i = 0; i < 256; i++) {
        j += S[i] + key[i % key_len];
        
        // Swap bytes
        uint8_t temp = S[i];
        S[i] = S[j];
        S[j] = temp;
    }
}

uint8_t arcfour_byte_arm(uint8_t* S, uint8_t* i, uint8_t* j) {
    uint8_t ii = *i + 1;
    uint8_t jj = *j + S[ii];
    uint8_t Si, Sj, t;
    
    Si = S[ii];
    Sj = S[jj];
    
    // Swap
    S[ii] = Sj;
    S[jj] = Si;
    
    t = Si + Sj;
    
    *i = ii;
    *j = jj;
    
    return S[t];
}

void arcfour_xor_4_arm(const uint8_t* in, uint8_t* out,
                        uint8_t* S, uint8_t* i, uint8_t* j) {
    uint8_t ii = *i;
    uint8_t jj = *j;
    uint8_t Si, Sj, t;
    uint32_t result = 0;
    
    // Process 4 bytes
    for (int k = 0; k < 4; k++) {
        ii++;
        jj += S[ii];
        
        Si = S[ii];
        Sj = S[jj];
        
        S[ii] = Sj;
        S[jj] = Si;
        
        t = Si + Sj;
        
        result = (result << 8) | (S[t] ^ in[k]);
    }
    
    *i = ii;
    *j = jj;
    
    // Store result in little-endian
    out[0] = (uint8_t)result;
    out[1] = (uint8_t)(result >> 8);
    out[2] = (uint8_t)(result >> 16);
    out[3] = (uint8_t)(result >> 24);
}

void arcfour_xor_16_arm(const uint8_t* in, uint8_t* out,
                         uint8_t* S, uint8_t* i, uint8_t* j) {
    uint8_t ii = *i;
    uint8_t jj = *j;
    
    // Process 16 bytes in 4-byte chunks
    for (int k = 0; k < 16; k += 4) {
        uint8_t Si0, Sj0, t0;
        uint8_t Si1, Sj1, t1;
        uint8_t Si2, Sj2, t2;
        uint8_t Si3, Sj3, t3;
        
        // Byte 0
        ii++; jj += S[ii]; Si0 = S[ii]; Sj0 = S[jj];
        S[ii] = Sj0; S[jj] = Si0; t0 = Si0 + Sj0;
        
        // Byte 1
        ii++; jj += S[ii]; Si1 = S[ii]; Sj1 = S[jj];
        S[ii] = Sj1; S[jj] = Si1; t1 = Si1 + Sj1;
        
        // Byte 2
        ii++; jj += S[ii]; Si2 = S[ii]; Sj2 = S[jj];
        S[ii] = Sj2; S[jj] = Si2; t2 = Si2 + Sj2;
        
        // Byte 3
        ii++; jj += S[ii]; Si3 = S[ii]; Sj3 = S[jj];
        S[ii] = Sj3; S[jj] = Si3; t3 = Si3 + Sj3;
        
        // XOR and store
        out[k]     = S[t0] ^ in[k];
        out[k + 1] = S[t1] ^ in[k + 1];
        out[k + 2] = S[t2] ^ in[k + 2];
        out[k + 3] = S[t3] ^ in[k + 3];
    }
    
    *i = ii;
    *j = jj;
}

void arcfour_encrypt_arm(uint8_t* S, uint8_t* i, uint8_t* j,
                         const uint8_t* in, uint8_t* out, size_t len) {
    size_t remaining = len;
    
    // Process 16-byte chunks
    while (remaining >= 16) {
        arcfour_xor_16_arm(in, out, S, i, j);
        in += 16;
        out += 16;
        remaining -= 16;
    }
    
    // Process 4-byte chunks
    while (remaining >= 4) {
        arcfour_xor_4_arm(in, out, S, i, j);
        in += 4;
        out += 4;
        remaining -= 4;
    }
    
    // Process remaining bytes
    while (remaining > 0) {
        *out++ = arcfour_byte_arm(S, i, j) ^ *in++;
        remaining--;
    }
}