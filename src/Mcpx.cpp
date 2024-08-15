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
#include <memory>

// user incl
#include "Mcpx.h"
#include "bldr.h"
#include "file.h"
#include "xbmem.h"

int MCPX_ROM::load(const char* filename)
{
	if (filename == NULL || data != NULL)
		return 1;

	data = readFile(filename, NULL, MCPX_BLOCK_SIZE);
	if (data == NULL)
		return 1;

	int result = verifyMCPX();

	printf("mcpx file: %s ", filename);
	switch (version)
	{
	case MCPX_V1_0:
		printf("( v1.0 )\n");
		break;
	case MCPX_V1_1:
		printf("( v1.1 )\n");
		break;
	default:
		printf("\nError: MCPX ROM is invalid\n");
		break;
	}

	return result;
}
int MCPX_ROM::verifyMCPX()
{
	// verify we got a valid mcpx rom. readFile() has already checked the size.
	// create some constants to check against.

	if (data == NULL)
		return 1;

	const UINT MCPX_CONST_ONE = 280018995;
	const UINT MCPX_CONST_TWO = 3230587022;
	const UINT MCPX_CONST_THREE = 3993153540;
	const UINT MCPX_CONST_FOUR = 2160066559;

	if (memcmp(data, &MCPX_CONST_ONE, 4) != 0 ||
		memcmp(data + 4, &MCPX_CONST_TWO, 4) != 0 ||
		memcmp(data + MCPX_BLOCK_SIZE - 4, &MCPX_CONST_THREE, 4) != 0)
	{
		return 1;
	}

	// valid mcpx rom

	// mcpx v1.0 has a hardcoded signature at offset 0x187.
	if (memcmp(data + 0x187, &BOOT_PARAMS_SIGNATURE, 4) == 0)
	{
		version = MCPX_ROM::MCPX_V1_0;
		sbkey = (data + 0x1A5);
		return 0;
	}

	// verify the mcpx rom is v1.1
	if (memcmp(data + 0xe0, &MCPX_CONST_FOUR, 4) == 0)
	{
		version = MCPX_ROM::MCPX_V1_1;
		sbkey = (data + MCPX_BLOCK_SIZE - 0x64);
		return 0;
	}
	return 1;
}
