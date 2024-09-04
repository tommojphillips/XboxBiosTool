// XbTool.h

/* Copyright(C) 2024 tommojphillips
 *
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < https://www.gnu.org/licenses/>.
*/

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_BIOS_TOOL_H
#define XB_BIOS_TOOL_H

// user incl
#include "Bios.h"
#include "Mcpx.h"
#include "cli_tbl.h"
#include "type_defs.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

int listBios();
int extractBios();
int buildBios();
int splitBios();
int combineBios();
int simulateXcodes();
int decodeXcodes();
int decompressKrnl();
int extractPubKey(UCHAR* data, UINT size);
int encodeX86();
int dumpImg(UCHAR* data, UINT size);
void info();

int readKeys();
int readMCPX();

#endif // !XB_BIOS_TOOL_H
