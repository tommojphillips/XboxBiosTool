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

#ifndef NO_MEM_TRACKING
#include "mem_tracking.h"
#endif

void initMcpx(Mcpx* mcpx)
{
	mcpx->version = Mcpx::UNK;
	mcpx->data = NULL;
	mcpx->sbkey = NULL;
	memset(mcpx->hash, 0, SHA1_DIGEST_LEN);
}
void freeMcpx(Mcpx* mcpx)
{
	if (mcpx->data != NULL) 
	{
		free(mcpx->data);
		mcpx->data = NULL;
	}

	mcpx->sbkey = NULL;
	mcpx->version = Mcpx::UNK;
	memset(mcpx->hash, 0, SHA1_DIGEST_LEN);
}
int loadMcpx(Mcpx* mcpx, uint8_t* mcpxData) 
{
	mcpx->data = mcpxData;

	// hash the mcpx
	SHA1Context context;
	SHA1Reset(&context);
	SHA1Input(&context, mcpx->data, MCPX_BLOCK_SIZE);
	SHA1Result(&context, mcpx->hash);

	// compare precomputed hashes

	// mcpx v1.0 
	if (memcmp(mcpx->hash, MCPX_V1_0_SHA1_HASH, SHA1_DIGEST_LEN) == 0)
	{
		mcpx->version = Mcpx::MCPX_V1_0;
		mcpx->sbkey = (mcpx->data + 0x1A5);
		return 0;
	}

	// mcpx v1.1
	if (memcmp(mcpx->hash, MCPX_V1_1_SHA1_HASH, SHA1_DIGEST_LEN) == 0)
	{
		mcpx->version = Mcpx::MCPX_V1_1;
		mcpx->sbkey = (mcpx->data + 0x19C);
		return 0;
	}

	// mouse rev.0 
	if (memcmp(mcpx->hash, MOUSE_REV0_SHA1_HASH, SHA1_DIGEST_LEN) == 0)
	{
		mcpx->version = Mcpx::MOUSE_V1_0;
		mcpx->sbkey = (mcpx->data + 0x19C);
		return 0;
	}

	// mouse rev.1
	if (memcmp(mcpx->hash, MOUSE_REV1_SHA1_HASH, SHA1_DIGEST_LEN) == 0)
	{
		mcpx->version = Mcpx::MOUSE_V1_1;
		mcpx->sbkey = (mcpx->data + 0x19C);
		return 0;
	}
	return 0;
}

