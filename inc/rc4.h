// rc4.h

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

#ifndef RC4_H
#define RC4_H

#include <stddef.h>
#include <stdint.h>

typedef struct _RC4_CONTEXT {
    uint8_t k;
    uint8_t j;
    uint8_t t;
    uint8_t s[256];
} RC4_CONTEXT;

#ifdef __cplusplus
extern "C" {
#endif

void rc4_key(RC4_CONTEXT* context, const uint8_t* key, const size_t len);
void rc4(RC4_CONTEXT* context, uint8_t* data, const size_t size);
void rc4_symmetric_enc_dec(uint8_t* data, const size_t size, const uint8_t* key, const size_t key_len);

#ifdef __cplusplus
};
#endif

#endif // _RC4_H
