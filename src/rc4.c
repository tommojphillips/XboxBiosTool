// rc4.c : defines functions for symmetric encryption and decryption using the rc4 algorithm

/* Copyright(C) 2024 tommojphillips
 *
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < https://www.gnu.org/licenses/>.
*/

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#include "rc4.h"

inline void swap_byte(uint8_t* a, uint8_t* b) {
    uint8_t tmp = *a;
    *a = *b;
    *b = tmp;
}

void rc4_key(RC4_CONTEXT* context, const uint8_t* key, const size_t len) {
    int i = 0;
    uint8_t j = 0;
    uint8_t k[256] = { 0 };

    context->k = 0;
    context->j = 0;
    context->t = 0;

    for (i = 0; i < 256; ++i) {
        context->s[i] = (uint8_t)i;
        k[i] = key[i % len];
    }

    for (i = 0; i < 256; ++i) {
        j = j + context->s[i] + k[i];
        swap_byte(&context->s[i], &context->s[j]);
    }
}

void rc4(RC4_CONTEXT* c, uint8_t* data, const size_t size) {
    for (size_t i = 0; i < size; ++i) {
        c->k = c->k + 1;
        c->j = c->j + c->s[c->k];
        swap_byte(&c->s[c->k], &c->s[c->j]);
        c->t = c->s[c->k] + c->s[c->j];
        if (data != NULL)
            data[i] ^= c->s[c->t];
    }
}

void symmetricEncDec(uint8_t* data, const size_t size, const uint8_t* key, const size_t key_len) {
    RC4_CONTEXT context = { 0 };
    rc4_key(&context, key, key_len);
    rc4(&context, data, size);
}
