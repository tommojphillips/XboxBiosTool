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

#include "type_defs.h"

// The rsa header structure
typedef struct {
    char magic[4];
    UINT modSize;
    UINT bits;
    UINT maxBytes;
    UINT exponent;
} RSA_HEADER;

// The public key structure
typedef struct {
    RSA_HEADER header;
    UCHAR modulus[264];
} PUBLIC_KEY;

PUBLIC_KEY* rsaVerifyPublicKey(UCHAR* data);
int rsaPrintPublicKey(PUBLIC_KEY* pubkey);

#endif // !RSA_H
