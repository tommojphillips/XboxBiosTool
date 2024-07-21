// X86Interp.h

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

#ifndef XB_X86_INTERP_H
#define XB_X86_INTERP_H

// std incl
#include <stdio.h>

// user incl
#include "type_defs.h"

enum X86_INSTR_TYPE {
	X86_INSTR_TYPE_1BYTEOP,
	X86_INSTR_TYPE_1BYTEOP_NUM,
	X86_INSTR_TYPE_1BYTEOP_PTR,

	X86_INSTR_TYPE_2BYTEOP,
	X86_INSTR_TYPE_2BYTEOP_NUM,
	X86_INSTR_TYPE_2BYTEOP_PTR,

	X86_INSTR_TYPE_JMP_FAR,
};

int parseInstruction(char* buf, UCHAR* data, UINT& offset);
int decodeX86(UCHAR* data, UINT size, FILE* stream);

#endif // XB_X86_INTERP_H
