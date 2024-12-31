// tea.c
// https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
// 
// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include <stdint.h>

#include "tea.h"

#define TEA_DELTA 0x9E3779B9
#define TEA_DECRYPT_SUM 0xC6EF3720

void tea_encrypt(uint32_t* v, const uint32_t* k) {
    uint32_t sum = 0;
    for (uint32_t i = 0; i < 32; i++) {
        sum += TEA_DELTA;
        v[0] += ((v[1] << 4) + k[0]) ^ (v[1] + sum) ^ ((v[1] >> 5) + k[1]);
        v[1] += ((v[0] << 4) + k[2]) ^ (v[0] + sum) ^ ((v[0] >> 5) + k[3]);
    }
}
void tea_decrypt(uint32_t* v, const uint32_t* k) {
    uint32_t sum = TEA_DECRYPT_SUM;
    for (uint32_t i = 0; i < 32; i++) {
        v[1] -= ((v[0] << 4) + k[2]) ^ (v[0] + sum) ^ ((v[0] >> 5) + k[3]);
        v[0] -= ((v[1] << 4) + k[0]) ^ (v[1] + sum) ^ ((v[1] >> 5) + k[1]);
        sum -= TEA_DELTA;
    }
}
