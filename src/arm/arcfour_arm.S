/*
 * RC4 ARM assembly optimization for ARM Cortex-M
 * 
 * This file contains hand-optimized ARM assembly functions
 * for the RC4 stream cipher.
 */

    .syntax unified
    .cpu cortex-m4
    .thumb

    .global arcfour_byte_asm
    .global arcfour_encrypt_block_asm

/*
 * arcfour_byte_asm - generate single RC4 keystream byte
 * 
 * r0 = S-box pointer
 * r1 = i pointer
 * r2 = j pointer
 * Returns: r0 = keystream byte
 */
    .thumb_func
arcfour_byte_asm:
    ldrb    r3, [r1]        @ r3 = *i
    adds    r3, #1          @ i++
    strb    r3, [r1]        @ store back
    uxtb    r3, r3          @ zero extend
    ldrb    r4, [r0, r3]    @ S[i]
    ldrb    r5, [r2]        @ j
    adds    r5, r4          @ j += S[i]
    strb    r5, [r2]        @ store back
    uxtb    r5, r5
    ldrb    r6, [r0, r5]    @ S[j]
    strb    r6, [r0, r3]    @ swap S[i] = S[j]
    strb    r4, [r0, r5]    @ S[j] = old S[i]
    adds    r3, r5          @ t = i + j
    uxtb    r3, r3
    ldrb    r0, [r0, r3]    @ return S[t]
    bx      lr

/*
 * arcfour_encrypt_block_asm - encrypt 64-byte block
 * 
 * r0 = input
 * r1 = output
 * r2 = S-box
 * r3 = i pointer
 * stack: j pointer
 */
    .thumb_func
arcfour_encrypt_block_asm:
    push    {r4-r11, lr}
    ldr     r4, [sp, #36]   @ j pointer
    ldrb    r5, [r3]        @ i
    ldrb    r6, [r4]        @ j
    mov     r7, #64         @ counter

encrypt_loop:
    adds    r5, #1
    uxtb    r5, r5
    ldrb    r8, [r2, r5]
    adds    r6, r8
    uxtb    r6, r6
    ldrb    r9, [r2, r6]
    strb    r9, [r2, r5]
    strb    r8, [r2, r6]
    adds    r8, r9
    uxtb    r8, r8
    ldrb    r8, [r2, r8]
    ldrb    r10, [r0], #1
    eor     r8, r10
    strb    r8, [r1], #1
    subs    r7, #1
    bne     encrypt_loop

    strb    r5, [r3]
    strb    r6, [r4]
    pop     {r4-r11, pc}