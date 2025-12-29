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

#define MCPX_BLOCK_SIZE 512

typedef enum {
	MCPX_REV_UNK,
	MCPX_REV_0,
	MCPX_REV_1,
} MCPX_REV;

typedef enum {
	MCPX_FLAVOR_UNK,
	MCPX_FLAVOR_AUTH,
	MCPX_FLAVOR_MOUSE,
} MCPX_FLAVOR;

typedef struct {	
	uint8_t* data;
	uint8_t* sbkey;
	uint8_t* teahash;
	MCPX_REV rev;
	MCPX_FLAVOR flavor;
	uint8_t hash[20];
} MCPX;

#ifdef __cplusplus
extern "C" {
#endif

void mcpx_init(MCPX* mcpx);
void mcpx_free(MCPX* mcpx);
int mcpx_load(MCPX* mcpx, uint8_t* data);

#ifdef __cplusplus
};
#endif

#endif // !MCPX_ROM_H
