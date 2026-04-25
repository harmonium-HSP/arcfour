#include "aead_rc4.h"
#include "arcfour.h"
#include <string.h>
#include <stdint.h>

typedef struct {
    uint32_t r[4];
    uint32_t s[4];
} poly1305_ctx;

static void poly1305_init(poly1305_ctx* ctx, const uint8_t* key) {
    ctx->r[0] = (uint32_t)key[0] | ((uint32_t)key[1] << 8) | ((uint32_t)key[2] << 16) | ((uint32_t)key[3] << 24);
    ctx->r[1] = ((uint32_t)key[4] | ((uint32_t)key[5] << 8) | ((uint32_t)key[6] << 16) | ((uint32_t)key[7] << 24)) & 0x0ffffff;
    ctx->r[2] = ((uint32_t)key[8] | ((uint32_t)key[9] << 8) | ((uint32_t)key[10] << 16) | ((uint32_t)key[11] << 24)) & 0x0ffffff;
    ctx->r[3] = ((uint32_t)key[12] | ((uint32_t)key[13] << 8) | ((uint32_t)key[14] << 16) | ((uint32_t)key[15] << 24)) & 0x0ffffff;
    
    ctx->s[0] = (uint32_t)key[16] | ((uint32_t)key[17] << 8) | ((uint32_t)key[18] << 16) | ((uint32_t)key[19] << 24);
    ctx->s[1] = (uint32_t)key[20] | ((uint32_t)key[21] << 8) | ((uint32_t)key[22] << 16) | ((uint32_t)key[23] << 24);
    ctx->s[2] = (uint32_t)key[24] | ((uint32_t)key[25] << 8) | ((uint32_t)key[26] << 16) | ((uint32_t)key[27] << 24);
    ctx->s[3] = (uint32_t)key[28] | ((uint32_t)key[29] << 8) | ((uint32_t)key[30] << 16) | ((uint32_t)key[31] << 24);
}

static void poly1305_add_block(poly1305_ctx* ctx, const uint8_t* m, size_t len) {
    uint32_t t0 = 0, t1 = 0, t2 = 0, t3 = 0;
    
    for (size_t i = 0; i < len; i++) {
        uint32_t shift = (i % 4) * 8;
        switch (i / 4) {
            case 0: t0 |= (uint32_t)m[i] << shift; break;
            case 1: t1 |= (uint32_t)m[i] << shift; break;
            case 2: t2 |= (uint32_t)m[i] << shift; break;
            case 3: t3 |= (uint32_t)m[i] << shift; break;
        }
    }
    
    if (len % 16 != 0) {
        t3 |= 1UL << ((len % 16) * 8);
    }
    
    uint32_t r0 = ctx->r[0], r1 = ctx->r[1], r2 = ctx->r[2], r3 = ctx->r[3];
    uint32_t s0 = ctx->s[0], s1 = ctx->s[1], s2 = ctx->s[2], s3 = ctx->s[3];
    
    uint64_t c = s0 + t0; s0 = (uint32_t)c; c = c >> 32;
    c += s1 + t1; s1 = (uint32_t)c; c = c >> 32;
    c += s2 + t2; s2 = (uint32_t)c; c = c >> 32;
    c += s3 + t3; s3 = (uint32_t)c; c = c >> 32;
    s0 += (uint32_t)c;
    
    uint64_t u0 = (uint64_t)s0 * r0;
    uint64_t u1 = (uint64_t)s0 * r1 + (uint64_t)s1 * r0;
    uint64_t u2 = (uint64_t)s0 * r2 + (uint64_t)s1 * r1 + (uint64_t)s2 * r0;
    uint64_t u3 = (uint64_t)s0 * r3 + (uint64_t)s1 * r2 + (uint64_t)s2 * r1 + (uint64_t)s3 * r0;
    uint64_t u4 = (uint64_t)s1 * r3 + (uint64_t)s2 * r2 + (uint64_t)s3 * r1;
    
    u3 += (u4 >> 2);
    u4 = (u4 & 0x3ffffff) * 5;
    
    uint32_t carry = (uint32_t)((u0 >> 24) + (u1 >> 24) + (u2 >> 24) + (u3 >> 24) + (u4 >> 24));
    u0 = (u0 & 0xffffff) + (carry << 24);
    
    carry = u0 >> 24; u0 &= 0xffffff;
    u1 += carry; carry = u1 >> 24; u1 &= 0xffffff;
    u2 += carry; carry = u2 >> 24; u2 &= 0xffffff;
    u3 += carry; carry = u3 >> 24; u3 &= 0xffffff;
    u4 += carry; u4 &= 0xffffff;
    
    u0 = (uint32_t)((uint64_t)u0 * 5);
    u1 = (uint32_t)((uint64_t)u1 * 5);
    u2 = (uint32_t)((uint64_t)u2 * 5);
    u3 = (uint32_t)((uint64_t)u3 * 5);
    u4 = (uint32_t)((uint64_t)u4 * 5);
    
    carry = (u0 >> 24) + (u1 >> 24) + (u2 >> 24) + (u3 >> 24) + (u4 >> 24);
    u0 = (u0 & 0xffffff) + (carry << 24);
    
    carry = u0 >> 24; u0 &= 0xffffff;
    u1 += carry; u1 &= 0xffffff;
    
    ctx->s[0] = u0;
    ctx->s[1] = u1;
    ctx->s[2] = (uint32_t)u2;
    ctx->s[3] = (uint32_t)u3;
}

static void poly1305_finish(poly1305_ctx* ctx, uint8_t* mac) {
    uint32_t s0 = ctx->s[0], s1 = ctx->s[1], s2 = ctx->s[2], s3 = ctx->s[3];
    
    uint32_t g0 = s0 + 5;
    uint32_t c = g0 >> 24;
    g0 &= 0xffffff;
    
    uint32_t g1 = s1 + c;
    c = g1 >> 24;
    g1 &= 0xffffff;
    
    uint32_t g2 = s2 + c;
    c = g2 >> 24;
    g2 &= 0xffffff;
    
    uint32_t g3 = s3 + c;
    c = g3 >> 24;
    g3 &= 0xffffff;
    
    g0 += c;
    g0 &= 0xffffff;
    
    uint32_t mask = (g0 - 5) >> 31;
    s0 = (g0 & ~mask) | (s0 & mask);
    s1 = (g1 & ~mask) | (s1 & mask);
    s2 = (g2 & ~mask) | (s2 & mask);
    s3 = (g3 & ~mask) | (s3 & mask);
    
    uint32_t h0 = ((s0 >> 24) | ((s0 >> 8) & 0xff00) | ((s0 << 8) & 0xff0000) | ((s0 << 24) & 0xff000000));
    uint32_t h1 = ((s1 >> 24) | ((s1 >> 8) & 0xff00) | ((s1 << 8) & 0xff0000) | ((s1 << 24) & 0xff000000));
    uint32_t h2 = ((s2 >> 24) | ((s2 >> 8) & 0xff00) | ((s2 << 8) & 0xff0000) | ((s2 << 24) & 0xff000000));
    uint32_t h3 = ((s3 >> 24) | ((s3 >> 8) & 0xff00) | ((s3 << 8) & 0xff0000) | ((s3 << 24) & 0xff000000));
    
    mac[0] = (h0 >> 24) & 0xff;
    mac[1] = (h0 >> 16) & 0xff;
    mac[2] = (h0 >> 8) & 0xff;
    mac[3] = h0 & 0xff;
    mac[4] = (h1 >> 24) & 0xff;
    mac[5] = (h1 >> 16) & 0xff;
    mac[6] = (h1 >> 8) & 0xff;
    mac[7] = h1 & 0xff;
    mac[8] = (h2 >> 24) & 0xff;
    mac[9] = (h2 >> 16) & 0xff;
    mac[10] = (h2 >> 8) & 0xff;
    mac[11] = h2 & 0xff;
    mac[12] = (h3 >> 24) & 0xff;
    mac[13] = (h3 >> 16) & 0xff;
    mac[14] = (h3 >> 8) & 0xff;
    mac[15] = h3 & 0xff;
}

static void poly1305_update(poly1305_ctx* ctx, const uint8_t* data, size_t len) {
    size_t remaining = len;
    const uint8_t* ptr = data;
    
    while (remaining >= 16) {
        poly1305_add_block(ctx, ptr, 16);
        ptr += 16;
        remaining -= 16;
    }
    
    if (remaining > 0) {
        uint8_t block[16] = {0};
        memcpy(block, ptr, remaining);
        poly1305_add_block(ctx, block, remaining);
    }
}

AEAD_RC4_API int aead_secure_memcmp(const uint8_t* a, const uint8_t* b, size_t len) {
    volatile uint8_t result = 0;
    for (size_t i = 0; i < len; i++) {
        result |= a[i] ^ b[i];
    }
    return result == 0 ? 0 : -1;
}

AEAD_RC4_API int aead_rc4_encrypt(
    const uint8_t* key, size_t key_len,
    const uint8_t* nonce, size_t nonce_len,
    const uint8_t* aad, size_t aad_len,
    const uint8_t* plaintext, size_t plaintext_len,
    uint8_t* ciphertext, size_t* ciphertext_len,
    uint8_t* tag
) {
    if (!key || !nonce || !ciphertext || !ciphertext_len || !tag) {
        return -1;
    }
    
    if (plaintext_len > 0 && (!plaintext || !ciphertext)) {
        return -1;
    }
    
    if (key_len != 16 && key_len != 24 && key_len != 32) {
        return -1;
    }
    
    if (nonce_len == 0 || nonce_len > 16) {
        return -1;
    }
    
    size_t total_len = key_len + nonce_len;
    uint8_t combined[48] = {0};
    memcpy(combined, key, key_len);
    memcpy(combined + key_len, nonce, nonce_len);
    
#ifdef ARCFOUR_STATIC_ONLY
    arcfour_ctx_t ctx;
    arcfour_init_static(&ctx, combined, total_len);
#else
    arcfour_ctx* ctx = arcfour_init(combined, total_len);
    if (!ctx) {
        return -1;
    }
#endif
    
    uint8_t poly_key[32];
#ifdef ARCFOUR_STATIC_ONLY
    arcfour_encrypt_static(&ctx, NULL, poly_key, 32);
#else
    arcfour_encrypt(ctx, NULL, poly_key, 32);
#endif
    
    uint8_t dummy[32] = {0};
#ifdef ARCFOUR_STATIC_ONLY
    arcfour_encrypt_static(&ctx, dummy, dummy, 32);
    if (plaintext_len > 0) {
        arcfour_encrypt_static(&ctx, plaintext, ciphertext, plaintext_len);
    }
#else
    arcfour_encrypt(ctx, dummy, dummy, 32);
    if (plaintext_len > 0) {
        arcfour_encrypt(ctx, plaintext, ciphertext, plaintext_len);
    }
#endif
    *ciphertext_len = plaintext_len;
    
    poly1305_ctx poly;
    poly1305_init(&poly, poly_key);
    
    if (aad_len > 0) {
        poly1305_update(&poly, aad, aad_len);
        if (aad_len % 16 != 0) {
            uint8_t pad[16] = {0};
            poly1305_add_block(&poly, pad, 16 - (aad_len % 16));
        }
    }
    
    poly1305_update(&poly, ciphertext, plaintext_len);
    if (plaintext_len % 16 != 0) {
        uint8_t pad[16] = {0};
        poly1305_add_block(&poly, pad, 16 - (plaintext_len % 16));
    }
    
    uint8_t len_block[16];
    memset(len_block, 0, 16);
    len_block[12] = (aad_len >> 24) & 0xff;
    len_block[13] = (aad_len >> 16) & 0xff;
    len_block[14] = (aad_len >> 8) & 0xff;
    len_block[15] = aad_len & 0xff;
    poly1305_add_block(&poly, len_block, 16);
    
    memset(len_block, 0, 16);
    len_block[12] = (plaintext_len >> 24) & 0xff;
    len_block[13] = (plaintext_len >> 16) & 0xff;
    len_block[14] = (plaintext_len >> 8) & 0xff;
    len_block[15] = plaintext_len & 0xff;
    poly1305_add_block(&poly, len_block, 16);
    
    poly1305_finish(&poly, tag);
    
#ifndef ARCFOUR_STATIC_ONLY
    arcfour_uninit(ctx);
#endif
    
    volatile uint8_t* p = (volatile uint8_t*)poly_key;
    for (size_t i = 0; i < 32; i++) {
        p[i] = 0;
    }
    
    return 0;
}

AEAD_RC4_API int aead_rc4_decrypt(
    const uint8_t* key, size_t key_len,
    const uint8_t* nonce, size_t nonce_len,
    const uint8_t* aad, size_t aad_len,
    const uint8_t* ciphertext, size_t ciphertext_len,
    const uint8_t* tag,
    uint8_t* plaintext, size_t* plaintext_len
) {
    if (!key || !nonce || !tag || !plaintext_len) {
        return -1;
    }
    
    if (ciphertext_len > 0 && (!ciphertext || !plaintext)) {
        return -1;
    }
    
    if (key_len != 16 && key_len != 24 && key_len != 32) {
        return -1;
    }
    
    if (nonce_len == 0 || nonce_len > 16) {
        return -1;
    }
    
    size_t total_len = key_len + nonce_len;
    uint8_t combined[48] = {0};
    memcpy(combined, key, key_len);
    memcpy(combined + key_len, nonce, nonce_len);
    
#ifdef ARCFOUR_STATIC_ONLY
    arcfour_ctx_t ctx;
    arcfour_init_static(&ctx, combined, total_len);
#else
    arcfour_ctx* ctx = arcfour_init(combined, total_len);
    if (!ctx) {
        return -1;
    }
#endif
    
    uint8_t poly_key[32];
#ifdef ARCFOUR_STATIC_ONLY
    arcfour_encrypt_static(&ctx, NULL, poly_key, 32);
#else
    arcfour_encrypt(ctx, NULL, poly_key, 32);
#endif
    
    uint8_t dummy[32] = {0};
#ifdef ARCFOUR_STATIC_ONLY
    arcfour_encrypt_static(&ctx, dummy, dummy, 32);
#else
    arcfour_encrypt(ctx, dummy, dummy, 32);
#endif
    
    poly1305_ctx poly;
    poly1305_init(&poly, poly_key);
    
    if (aad_len > 0) {
        poly1305_update(&poly, aad, aad_len);
        if (aad_len % 16 != 0) {
            uint8_t pad[16] = {0};
            poly1305_add_block(&poly, pad, 16 - (aad_len % 16));
        }
    }
    
    poly1305_update(&poly, ciphertext, ciphertext_len);
    if (ciphertext_len % 16 != 0) {
        uint8_t pad[16] = {0};
        poly1305_add_block(&poly, pad, 16 - (ciphertext_len % 16));
    }
    
    uint8_t len_block[16];
    memset(len_block, 0, 16);
    len_block[12] = (aad_len >> 24) & 0xff;
    len_block[13] = (aad_len >> 16) & 0xff;
    len_block[14] = (aad_len >> 8) & 0xff;
    len_block[15] = aad_len & 0xff;
    poly1305_add_block(&poly, len_block, 16);
    
    memset(len_block, 0, 16);
    len_block[12] = (ciphertext_len >> 24) & 0xff;
    len_block[13] = (ciphertext_len >> 16) & 0xff;
    len_block[14] = (ciphertext_len >> 8) & 0xff;
    len_block[15] = ciphertext_len & 0xff;
    poly1305_add_block(&poly, len_block, 16);
    
    uint8_t computed_tag[16];
    poly1305_finish(&poly, computed_tag);
    
    if (aead_secure_memcmp(computed_tag, tag, 16) != 0) {
#ifndef ARCFOUR_STATIC_ONLY
        arcfour_uninit(ctx);
#endif
        return -1;
    }
    
    if (ciphertext_len > 0) {
#ifdef ARCFOUR_STATIC_ONLY
        arcfour_encrypt_static(&ctx, ciphertext, plaintext, ciphertext_len);
#else
        arcfour_encrypt(ctx, ciphertext, plaintext, ciphertext_len);
#endif
    }
    *plaintext_len = ciphertext_len;
    
#ifndef ARCFOUR_STATIC_ONLY
    arcfour_uninit(ctx);
#endif
    
    volatile uint8_t* p = (volatile uint8_t*)poly_key;
    for (size_t i = 0; i < 32; i++) {
        p[i] = 0;
    }
    
    return 0;
}