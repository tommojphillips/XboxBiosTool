// Mcpx.cpp : Implements the MCPX_ROM class for handling MCPX ROMs.

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

// std incl
#include <stdio.h>
#include <string.h>
#include <malloc.h>

// user incl
#include "Mcpx.h"
#include "sha1.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#endif

// SHA1 hashes of accepted mcpx roms. 
// Used to verify the integrity of the mcpx rom

// MCPX V1.0
const uint8_t MCPX_V1_0_SHA1_HASH[SHA1_DIGEST_LEN] = {
	0x5d, 0x27, 0x06, 0x75,
	0xb5, 0x4e, 0xb8, 0x07,
	0x1b, 0x48, 0x0e, 0x42,
	0xd2, 0x2a, 0x30, 0x15,
	0xac, 0x21, 0x1c, 0xef
};

// MCPX V1.1
const uint8_t MCPX_V1_1_SHA1_HASH[SHA1_DIGEST_LEN] = {
	0x6c, 0x87, 0x5f, 0x17,
	0xf7, 0x73, 0xaa, 0xec,
	0x51, 0xeb, 0x43, 0x40,
	0x68, 0xbb, 0x6c, 0x65,
	0x7c, 0x43, 0x43, 0xc0
};

// MOUSE rev 0
const uint8_t MOUSE_REV0_SHA1_HASH[SHA1_DIGEST_LEN] = {
	0x2a, 0xf8, 0x46, 0xd4,
	0x29, 0xaa, 0xa1, 0x09,
	0x26, 0x68, 0x09, 0x00,
	0x3d, 0xc9, 0x06, 0x02,
	0xa7, 0x2f, 0x76, 0xc1
};

// MOUSE rev 1
const uint8_t MOUSE_REV1_SHA1_HASH[SHA1_DIGEST_LEN] = {
	0x15, 0x13, 0xab, 0xcb,
	0x6b, 0x97, 0x9f, 0x79,
	0x53, 0x6f, 0xcf, 0x0e,
	0xd9, 0x67, 0xf3, 0x77,
	0x55, 0xe0, 0x7f, 0x9b
};

void mcpx_init(Mcpx* mcpx) {
	mcpx->version = MCPX_VERSION_UNK;
	mcpx->data = NULL;
	mcpx->sbkey = NULL;
	memset(mcpx->hash, 0, SHA1_DIGEST_LEN);
}
void mcpx_free(Mcpx* mcpx) {
	if (mcpx->data != NULL) {
		free(mcpx->data);
		mcpx->data = NULL;
	}

	mcpx->sbkey = NULL;
	mcpx->version = MCPX_VERSION_UNK;
	memset(mcpx->hash, 0, SHA1_DIGEST_LEN);
}
int mcpx_load(Mcpx* mcpx, uint8_t* data) {

	mcpx->data = data;

	// hash the mcpx
	SHA1Context context;
	SHA1Reset(&context);
	SHA1Input(&context, mcpx->data, MCPX_BLOCK_SIZE);
	SHA1Result(&context, mcpx->hash);

	// compare precomputed hashes

	// mcpx v1.0 
	if (memcmp(mcpx->hash, MCPX_V1_0_SHA1_HASH, SHA1_DIGEST_LEN) == 0)
	{
		mcpx->version = MCPX_VERSION_MCPX_V1_0;
		mcpx->sbkey = (mcpx->data + 0x1A5);
		return 0;
	}

	// mcpx v1.1
	if (memcmp(mcpx->hash, MCPX_V1_1_SHA1_HASH, SHA1_DIGEST_LEN) == 0)
	{
		mcpx->version = MCPX_VERSION_MCPX_V1_1;
		mcpx->sbkey = (mcpx->data + 0x19C);
		return 0;
	}

	// mouse rev.0 
	if (memcmp(mcpx->hash, MOUSE_REV0_SHA1_HASH, SHA1_DIGEST_LEN) == 0)
	{
		mcpx->version = MCPX_VERSION_MOUSE_V1_0;
		mcpx->sbkey = (mcpx->data + 0x19C);
		return 0;
	}

	// mouse rev.1
	if (memcmp(mcpx->hash, MOUSE_REV1_SHA1_HASH, SHA1_DIGEST_LEN) == 0)
	{
		mcpx->version = MCPX_VERSION_MOUSE_V1_1;
		mcpx->sbkey = (mcpx->data + 0x19C);
		return 0;
	}
	return 0;
}

