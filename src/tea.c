// tea.c

// https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <stdint.h>

#define TEA_DELTA 0x9E3779B9

void tea_encrypt(uint32_t v[2], const uint32_t k[4])
{
    uint32_t v0 = v[0];                                     /* set up */
    uint32_t v1 = v[1];
    uint32_t sum = 0;
    uint32_t delta = TEA_DELTA;                             /* a key schedule constant */
    uint32_t k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3];    /* cache key */

    for (uint32_t i = 0; i < 32; i++) {
        sum += delta;
        v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        v1 += ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
    }

    v[0] = v0;
    v[1] = v1;
}

void tea_decrypt(uint32_t v[2], const uint32_t k[4])
{
    uint32_t v0 = v[0];                                     /* set up */
    uint32_t v1 = v[1];
    uint32_t sum = 0xC6EF3720;
    uint32_t delta = TEA_DELTA;                             /* a key schedule constant */
    uint32_t k0 = k[0], k1 = k[1], k2 = k[2], k3 = k[3];    /* cache key */

    for (uint32_t i = 0; i < 32; i++) {
        v1 -= ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
        v0 -= ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        sum -= delta;
    }
    
    v[0] = v0; 
    v[1] = v1;
}
