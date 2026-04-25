#ifndef ARCFOUR_ARM_H
#define ARCFOUR_ARM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ARM-optimized RC4 functions */

/**
 * @brief ARM-optimized Key Scheduling Algorithm (KSA)
 * 
 * @param S The 256-byte state array
 * @param key The key
 * @param key_len Key length in bytes
 */
void arcfour_ksa_arm(uint8_t* S, const uint8_t* key, size_t key_len);

/**
 * @brief ARM-optimized PRGA - generate single byte
 * 
 * @param S The 256-byte state array
 * @param i Pointer to i state variable
 * @param j Pointer to j state variable
 * @return Next byte of keystream
 */
uint8_t arcfour_byte_arm(uint8_t* S, uint8_t* i, uint8_t* j);

/**
 * @brief ARM-optimized PRGA - process 16 bytes at once
 * 
 * @param in Input data
 * @param out Output data (can be same as input for in-place)
 * @param S The 256-byte state array
 * @param i Pointer to i state variable
 * @param j Pointer to j state variable
 */
void arcfour_xor_16_arm(const uint8_t* in, uint8_t* out,
                         uint8_t* S, uint8_t* i, uint8_t* j);

/**
 * @brief ARM-optimized PRGA - process 4 bytes at once
 * 
 * @param in Input data
 * @param out Output data (can be same as input for in-place)
 * @param S The 256-byte state array
 * @param i Pointer to i state variable
 * @param j Pointer to j state variable
 */
void arcfour_xor_4_arm(const uint8_t* in, uint8_t* out,
                        uint8_t* S, uint8_t* i, uint8_t* j);

/**
 * @brief ARM-optimized bulk encryption
 * 
 * @param S The 256-byte state array
 * @param i Pointer to i state variable  
 * @param j Pointer to j state variable
 * @param in Input data
 * @param out Output data
 * @param len Length in bytes
 */
void arcfour_encrypt_arm(uint8_t* S, uint8_t* i, uint8_t* j,
                         const uint8_t* in, uint8_t* out, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ARCFOUR_ARM_H */