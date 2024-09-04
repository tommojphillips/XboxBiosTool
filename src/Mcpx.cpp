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

// user incl
#include "Mcpx.h"
#include "file.h"
#include "sha1.h"

int Mcpx::load(UCHAR* mcpxData)
{
	data = mcpxData;

	SHA1Context context;
	SHA1Reset(&context);
	SHA1Input(&context, data, MCPX_BLOCK_SIZE);
	SHA1Result(&context, hash);
	
	// compare precomputed hashes

	// mcpx v1.0 
	if (memcmp(hash, MCPX_V1_0_SHA1_HASH, 20) == 0)
	{
		version = Mcpx::MCPX_V1_0;
		sbkey = (data + 0x1A5);
		return 0;
	}

	// mcpx v1.1
	if (memcmp(hash, MCPX_V1_1_SHA1_HASH, 20) == 0)
	{
		version = Mcpx::MCPX_V1_1;
		sbkey = (data + 0x19C);
		return 0;
	}

	// mouse rev.0 
	if (memcmp(hash, MOUSE_REV0_SHA1_HASH, 20) == 0)
	{
		version = Mcpx::MOUSE_V1_0;
		sbkey = (data + 0x19C);
		return 0;
	}

	// mouse rev.1
	if (memcmp(hash, MOUSE_REV1_SHA1_HASH, 20) == 0)
	{
		version = Mcpx::MOUSE_V1_1;
		sbkey = (data + 0x19C);
		return 0;
	}

	return 1;
}
