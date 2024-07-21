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

// user incl
#include "type_defs.h"

const UINT MCPX_BLOCK_SIZE = 512; // mcpx southbridge chip rom size in bytes

class MCPX_ROM
{
public:
	enum MCPX_ROM_VERSION { MCPX_UNK, MCPX_V1_0, MCPX_V1_1 };

	MCPX_ROM_VERSION version;	// version of the mcpx rom.
	UCHAR* data;				// allocated mcpx data.
	UCHAR* sbkey;				// ptr to the sb key.

	MCPX_ROM() : version(MCPX_UNK), data(NULL), sbkey(NULL) { };

	int load(const char* filename);

	void deconstruct();

	int verifyMCPX();
};

#endif // !XB_BIOS_MCPX_ROM_H
