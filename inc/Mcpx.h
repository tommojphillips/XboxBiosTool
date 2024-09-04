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

#ifndef XB_BIOS_MCPX_ROM_H
#define XB_BIOS_MCPX_ROM_H

#include "memory"

// user incl
#include "type_defs.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

const UINT MCPX_BLOCK_SIZE = 512; // mcpx southbridge chip rom size in bytes

// sha1 hashes of the mcpx roms.
const UCHAR MCPX_V1_0_SHA1_HASH[20] { 
	0x5d, 0x27, 0x06, 0x75, 0xb5, 
	0x4e, 0xb8, 0x07, 0x1b, 0x48, 
	0x0e, 0x42, 0xd2, 0x2a, 0x30,
	0x15, 0xac, 0x21, 0x1c, 0xef 
};
const UCHAR MCPX_V1_1_SHA1_HASH[20] {
	0x6c, 0x87, 0x5f, 0x17, 
	0xf7, 0x73, 0xaa, 0xec, 
	0x51, 0xeb, 0x43, 0x40, 
	0x68, 0xbb, 0x6c, 0x65, 
	0x7c, 0x43, 0x43, 0xc0 
};
const UCHAR MOUSE_REV0_SHA1_HASH[20] {
	0x2a, 0xf8, 0x46, 0xd4, 
	0x29, 0xaa, 0xa1, 0x09, 
	0x26, 0x68, 0x09, 0x00, 
	0x3d, 0xc9, 0x06, 0x02,
	0xa7, 0x2f, 0x76, 0xc1 
};
const UCHAR MOUSE_REV1_SHA1_HASH[20] {
	0x15, 0x13, 0xab, 0xcb, 
	0x6b, 0x97, 0x9f, 0x79, 
	0x53, 0x6f, 0xcf, 0x0e, 
	0xd9, 0x67, 0xf3, 0x77, 
	0x55, 0xe0, 0x7f, 0x9b
};

class Mcpx
{
public:
	Mcpx() {
		version = UNK;
		data = NULL;
		sbkey = NULL;
		memset(&hash, 0, 20);
	};
	~Mcpx() {
		sbkey = NULL;
		version = Mcpx::UNK;

		if (data != NULL) {
			free(data);
			data = NULL;
		}
	};

	enum MCPX_ROM_VERSION { UNK, MCPX_V1_0, MCPX_V1_1, MOUSE_V1_0, MOUSE_V1_1 };

	MCPX_ROM_VERSION version;	// version of the mcpx rom.
	UCHAR* data;				// allocated mcpx data.
	UCHAR* sbkey;				// ptr to the sb key.
	UCHAR hash[20];				// sha1 hash of the mcpx rom.

	// sets the mcpx rom data. takes ownership of the data.
	// verify we got a valid mcpx rom.
	// mcpxData: ptr to the mcpx rom data.
	// returns 0 on success, 1 on failure.
	int load(UCHAR* mcpxData);
};

#endif // !XB_BIOS_MCPX_ROM_H
