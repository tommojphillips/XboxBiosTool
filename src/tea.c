// tea.c
// https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
// 
// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <stdint.h>

#include "tea.h"

#define TEA_DELTA 0x9E3779B9
#define TEA_DECRYPT_SUM 0xC6EF3720

void tea_encrypt(uint32_t v[2], const uint32_t k[4]) {
    uint32_t sum = 0;
    uint32_t h0 = v[0], h1 = v[1];
    uint32_t k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3];

    for (uint32_t i = 0; i < 32; ++i) {
        sum += TEA_DELTA;
        h0 += ((h1 << 4) + k0) ^ (h1 + sum) ^ ((h1 >> 5) + k1);
        h1 += ((h0 << 4) + k2) ^ (h0 + sum) ^ ((h0 >> 5) + k3);
    }

    v[0] = h0;
    v[1] = h1;
}

void tea_decrypt(uint32_t v[2], const uint32_t k[4]) {
    uint32_t sum = TEA_DECRYPT_SUM;
    uint32_t h0 = v[0], h1 = v[1];
    uint32_t k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3];

    for (uint32_t i = 0; i < 32; ++i) {
        h1 -= ((h0 << 4) + k2) ^ (h0 + sum) ^ ((h0 >> 5) + k3);
        h0 -= ((h1 << 4) + k0) ^ (h1 + sum) ^ ((h1 >> 5) + k1);
        sum -= TEA_DELTA;
    }
}
