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

#ifndef X86_INTERP_H
#define X86_INTERP_H

// std incl
#include <cstdio>

// user incl
#include "type_defs.h"

// X86 instruction types;
enum X86_INSTR_TYPE : UCHAR {
	X86_INSTR_OP,		// single instruction opcode; op
	X86_INSTR_REG_NUM,  // register instruction with number; reg [num]
	X86_INSTR_REG_PTR,	// register instruction with pointer; reg [ptr]
	X86_INSTR_JMP_FAR,  // far jump instruction; seg:offset
	X86_INSTR_JMP_REL,	// relative jump instruction; offset
};

// X86 instruction map; opcode, asm instruction, opcode length, operand length
struct X86_INSTR_MAP {
	X86_INSTR_TYPE type;
	USHORT opcode;
	const char* asm_instr;
	USHORT opcode_len;
	USHORT operrand_len;
};

// get the instruction from the data
// data: the input buffer;
// returns the instruction if found, otherwise NULL
const X86_INSTR_MAP* getInstruction(const UCHAR* data);

// parse instruction from data+offset, and copy asm instruction to buf
// buf: the output buffer;
// data: the input buffer;
// offset: the offset of the instruction to parse in data; once decoded; &offset will be updated with the size of the instruction. (points to the next instruction)
// returns 0 on success otherwise, error code
int parseInstruction(char* buf, UCHAR* data, UINT& offset);

// decode X86 instructions from data, and write to stream
// data: the input buffer;
// size: the size of the input buffer;
// stream: the output stream;
// codeSize: the size of the decoded instructions; if NULL, the size will not be returned
int decodeX86(UCHAR* data, UINT size, FILE* stream, UINT* codeSize);

#endif // XB_X86_INTERP_H
