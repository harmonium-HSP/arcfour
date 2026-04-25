/*
 * RC4 Thumb-2 assembly optimization for ARM Cortex-M
 * 
 * This file contains hand-optimized Thumb-2 assembly functions
 * for the RC4 stream cipher, providing maximum performance on
 * Cortex-M4/M7/M33 processors.
 */

    .syntax unified
    .cpu cortex-m4
    .thumb

    .global arcfour_byte_thumb2
    .global arcfour_xor_16_thumb2
    .global arcfour_ksa_thumb2

/*
 * arcfour_byte_thumb2 - generate single RC4 keystream byte
 * 
 * Inputs:
 *   r0 = pointer to S-box (uint8_t*)
 *   r1 = pointer to i (uint8_t*)
 *   r2 = pointer to j (uint8_t*)
 * 
 * Output:
 *   r0 = keystream byte
 */
    .thumb_func
arcfour_byte_thumb2:
    ldrb    r3, [r1]        @ r3 = *i
    adds    r3, #1          @ r3 = i + 1 (sets flags)
    strb    r3, [r1]        @ *i = r3
    uxtb    r3, r3          @ zero-extend to 32 bits
    ldrb    r4, [r0, r3]    @ r4 = S[i]
    ldrb    r5, [r2]        @ r5 = *j
    adds    r5, r4          @ r5 = j + S[i]
    strb    r5, [r2]        @ *j = r5
    uxtb    r5, r5          @ zero-extend
    ldrb    r6, [r0, r5]    @ r6 = S[j]
    strb    r6, [r0, r3]    @ S[i] = S[j]
    strb    r4, [r0, r5]    @ S[j] = old S[i]
    adds    r3, r5          @ r3 = i + j
    uxtb    r3, r3          @ zero-extend
    ldrb    r0, [r0, r3]    @ r0 = S[i+j]
    bx      lr

/*
 * arcfour_xor_16_thumb2 - process 16 bytes at once
 * 
 * Inputs:
 *   r0 = input pointer (const uint8_t*)
 *   r1 = output pointer (uint8_t*)
 *   r2 = S-box pointer (uint8_t*)
 *   r3 = pointer to i (uint8_t*)
 *   [sp] = pointer to j (uint8_t*)
 * 
 * Clobbers: r0-r7
 */
    .thumb_func
arcfour_xor_16_thumb2:
    push    {r4-r7, lr}
    ldr     r4, [sp, #20]   @ r4 = pointer to j
    ldrb    r5, [r3]        @ r5 = i
    ldrb    r6, [r4]        @ r6 = j

    /* Process 16 bytes in 4-byte blocks */
    mov     r7, #4          @ loop counter

arcfour_xor_16_loop:
    /* Byte 0 */
    adds    r5, #1
    ldrb    r0, [r2, r5]
    adds    r6, r0
    ldrb    r1, [r2, r6]
    strb    r1, [r2, r5]
    strb    r0, [r2, r6]
    adds    r0, r1
    ldrb    r0, [r2, r0]
    ldrb    r3, [r7, #-4]   @ load input byte (adjust offset)
    eor     r0, r3
    strb    r0, [r1, #-4]   @ store output byte
    
    subs    r7, #1
    bne     arcfour_xor_16_loop

    strb    r5, [r3]        @ save i
    strb    r6, [r4]        @ save j
    pop     {r4-r7, pc}

/*
 * arcfour_ksa_thumb2 - Key Scheduling Algorithm (optimized)
 * 
 * Inputs:
 *   r0 = S-box pointer (uint8_t*)
 *   r1 = key pointer (const uint8_t*)
 *   r2 = key length (size_t)
 */
    .thumb_func
arcfour_ksa_thumb2:
    push    {r4-r7, lr}
    
    /* Initialize S-box: S[i] = i */
    mov     r3, #0          @ i = 0

ksa_init_loop:
    strb    r3, [r0, r3]    @ S[i] = i
    adds    r3, #1
    bne     ksa_init_loop   @ loop until r3 wraps to 0

    /* KSA swapping phase */
    mov     r3, #0          @ i = 0
    mov     r4, #0          @ j = 0

ksa_swap_loop:
    ldrb    r5, [r0, r3]    @ r5 = S[i]
    udiv    r6, r3, r2      @ r6 = i / key_len
    mls     r6, r6, r2, r3  @ r6 = i % key_len
    ldrb    r6, [r1, r6]    @ r6 = key[i % key_len]
    adds    r4, r5          @ j += S[i]
    adds    r4, r6          @ j += key[...]
    ldrb    r5, [r0, r4]    @ r5 = S[j]
    strb    r5, [r0, r3]    @ S[i] = S[j]
    strb    r3, [r0, r4]    @ S[j] = old S[i] (which was r3 before increment)
    adds    r3, #1
    bne     ksa_swap_loop

    pop     {r4-r7, pc}