// Mcpx.h:

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

#ifndef MCPX_ROM_H
#define MCPX_ROM_H

#include <stdint.h>

const uint32_t MCPX_BLOCK_SIZE = 512;

// SHA1 hashes of accepted mcpx roms. 
// Used to verify the integrity of the mcpx rom

// MCPX V1.0
const uint8_t MCPX_V1_0_SHA1_HASH[20] { 
	0x5d, 0x27, 0x06, 0x75,
	0xb5, 0x4e, 0xb8, 0x07,
	0x1b, 0x48, 0x0e, 0x42,
	0xd2, 0x2a, 0x30, 0x15,
	0xac, 0x21, 0x1c, 0xef
};

// MCPX V1.1
const uint8_t MCPX_V1_1_SHA1_HASH[20] {
	0x6c, 0x87, 0x5f, 0x17,
	0xf7, 0x73, 0xaa, 0xec,
	0x51, 0xeb, 0x43, 0x40,
	0x68, 0xbb, 0x6c, 0x65,
	0x7c, 0x43, 0x43, 0xc0
};

// MOUSE rev 0
const uint8_t MOUSE_REV0_SHA1_HASH[20] {
	0x2a, 0xf8, 0x46, 0xd4,
	0x29, 0xaa, 0xa1, 0x09,
	0x26, 0x68, 0x09, 0x00,
	0x3d, 0xc9, 0x06, 0x02,
	0xa7, 0x2f, 0x76, 0xc1 
};

// MOUSE rev 1
const uint8_t MOUSE_REV1_SHA1_HASH[20] {
	0x15, 0x13, 0xab, 0xcb,
	0x6b, 0x97, 0x9f, 0x79,
	0x53, 0x6f, 0xcf, 0x0e,
	0xd9, 0x67, 0xf3, 0x77,
	0x55, 0xe0, 0x7f, 0x9b
};


typedef struct {
	enum McpxVersion { UNK, MCPX_V1_0, MCPX_V1_1, MOUSE_V1_0, MOUSE_V1_1 };
	McpxVersion version;
	uint8_t* data;
	uint8_t* sbkey;
	uint8_t hash[20];
} Mcpx;

void initMcpx(Mcpx* mcpx);
void freeMcpx(Mcpx* mcpx);
int loadMcpx(Mcpx* mcpx, uint8_t* data);

#endif // !MCPX_ROM_H
