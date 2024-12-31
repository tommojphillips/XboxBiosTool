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

typedef enum { 
	MCPX_VERSION_UNK,
	MCPX_VERSION_MCPX_V1_0,
	MCPX_VERSION_MCPX_V1_1,
	MCPX_VERSION_MOUSE_V1_0, 
	MCPX_VERSION_MOUSE_V1_1 
} MCPX_VERSION;

typedef struct {
	MCPX_VERSION version;
	uint8_t* data;
	uint8_t* sbkey;
	uint8_t hash[20];
} Mcpx;

#ifdef __cplusplus
extern "C" {
#endif

void mcpx_init(Mcpx* mcpx);
void mcpx_free(Mcpx* mcpx);
int mcpx_load(Mcpx* mcpx, uint8_t* data);

#ifdef __cplusplus
};
#endif

#endif // !MCPX_ROM_H
