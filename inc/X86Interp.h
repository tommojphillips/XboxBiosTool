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
#include <cstdio>

// user incl
#include "type_defs.h"

enum X86_INSTR_TYPE : UCHAR {
	X86_INSTR_OP,
	X86_INSTR_REG_NUM,
	X86_INSTR_REG_PTR,
	X86_INSTR_JMP_FAR,
	X86_INSTR_JMP_REL,
};
struct X86_INSTR_MAP {
	X86_INSTR_TYPE type;
	USHORT opcode;
	const char* asm_instr;
	USHORT opcode_len;
	USHORT operrand_len;
};

int parseInstruction(char* buf, UCHAR* data, UINT& offset);
int decodeX86(UCHAR* data, UINT size, FILE* stream);

#endif // XB_X86_INTERP_H
