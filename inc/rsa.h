// rsa.h:

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

#ifndef RSA_H
#define RSA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RSA_ERROR_SUCCESS		0
#define RSA_ERROR               1
#define RSA_ERROR_INVALID_DATA	2

#define RSA_MOD_SIZE(rsa_header) ((rsa_header)->bits / 8U)
#define RSA_MOD_BUFFER_SIZE(rsa_header) ((rsa_header)->mod_size)
#define RSA_PUBKEY_SIZE(rsa_header) (sizeof(RSA_HEADER) + RSA_MOD_SIZE((RSA_HEADER*)rsa_header))
#define RSA_PUBKEY_BUFFER_SIZE(rsa_header) (sizeof(RSA_HEADER) + RSA_MOD_BUFFER_SIZE(rsa_header))

// The rsa header structure
typedef struct _RSA_HEADER {
    char magic[4];     // (RSA1/RSA2) rsa magic
    uint32_t mod_size;  // modulus buffer size in bytes
    uint32_t bits;      // modulus size in bits
    uint32_t max_bytes; // max bytes to be encrypted
    uint32_t exponent;  // public exponent
} RSA_HEADER;

// The public key structure
typedef struct _PUBLIC_KEY {
    RSA_HEADER header;  // rsa header structure
    uint8_t* modulus;   // pointer to the modulus.
} PUBLIC_KEY;

// verify rsa1 public key at offset
// data: input buffer
// size: size of the buffer
// offset: the offset to verify the pub key at.
// pubkey: output pub key (points to the public key in data)
int rsa_verifyPublicKey(uint8_t* data, uint32_t size, uint32_t offset, PUBLIC_KEY** pubkey);

// find a rsa1 public key in the buffer.
// data: input buffer
// size: size of the buffer
// pubkey: output pub key (points to the public key in data)
// offset: output offset where the pub key was found.
int rsa_findPublicKey(uint8_t* data, uint32_t size, PUBLIC_KEY** pubkey, uint32_t* offset);

#ifdef __cplusplus
};
#endif

#endif // !RSA_H
