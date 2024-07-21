// X86Interp.cpp: X86 interpreter implementation

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
#include "X86Interp.h"
#include "file.h"
#include "type_defs.h"
#include "util.h"
#include "xbmem.h"

typedef struct X86_INSTR_MAP
{
	X86_INSTR_TYPE type;
	USHORT opcode;
	const char* asm_instr;
	UINT opcode_len;
	UINT operrand_len;
} X86_INSTR_MAP;

X86_INSTR_MAP instrs[] = {
	{ X86_INSTR_TYPE_2BYTEOP_PTR, 0x1D8B, "mov ebx", 2, 4 },
	{ X86_INSTR_TYPE_2BYTEOP_PTR, 0x0D8B, "mov ecx", 2, 4 },
	{ X86_INSTR_TYPE_2BYTEOP_PTR, 0x158B, "mov edx", 2, 4 },

	{ X86_INSTR_TYPE_2BYTEOP, 0xE0FF, "jmp eax", 2, 0 },
	{ X86_INSTR_TYPE_2BYTEOP, 0xE1FF, "jmp ecx", 2, 0 },
	{ X86_INSTR_TYPE_2BYTEOP, 0xE2FF, "jmp edx", 2, 0 },
	{ X86_INSTR_TYPE_2BYTEOP, 0xE3FF, "jmp ebx", 2, 0 },
	{ X86_INSTR_TYPE_2BYTEOP, 0xE4FF, "jmp esp", 2, 0 },
	{ X86_INSTR_TYPE_2BYTEOP, 0xE5FF, "jmp ebp", 2, 0 },
	{ X86_INSTR_TYPE_2BYTEOP, 0xE6FF, "jmp esi", 2, 0 },
	{ X86_INSTR_TYPE_2BYTEOP, 0xE7FF, "jmp edi", 2, 0 },

	{ X86_INSTR_TYPE_2BYTEOP, 0xA5F3, "rep movsd", 2, 0 },

	{ X86_INSTR_TYPE_1BYTEOP_NUM, 0xB8, "mov eax", 1, 4 },
	{ X86_INSTR_TYPE_1BYTEOP_NUM, 0xB9, "mov ecx", 1, 4 },
	{ X86_INSTR_TYPE_1BYTEOP_NUM, 0xBA, "mov edx", 1, 4 },
	{ X86_INSTR_TYPE_1BYTEOP_NUM, 0xBB, "mov ebx", 1, 4 },
	{ X86_INSTR_TYPE_1BYTEOP_NUM, 0xBC, "mov esp", 1, 4 },
	{ X86_INSTR_TYPE_1BYTEOP_NUM, 0xBD, "mov ebp", 1, 4 },
	{ X86_INSTR_TYPE_1BYTEOP_NUM, 0xBE, "mov esi", 1, 4 },
	{ X86_INSTR_TYPE_1BYTEOP_NUM, 0xBF, "mov edi", 1, 4 },
	{ X86_INSTR_TYPE_1BYTEOP_PTR, 0xA1, "mov eax", 1, 4 },

	{ X86_INSTR_TYPE_JMP_FAR, 0xEA, "jmp far", 1, 5 },
	{ X86_INSTR_TYPE_1BYTEOP, 0x90, "nop", 1, 0 },
	{ X86_INSTR_TYPE_1BYTEOP, 0xFC, "cld", 1, 0 },
};

int decodeX86(UCHAR* data, UINT size, FILE* stream)
{
	UINT i;
	const UINT MAX_INSTR_SIZE = 6;
	const UCHAR zero_mem[MAX_INSTR_SIZE] = { 0 };

	char str_instr[128] = { 0 };

	bool unkInstrs = false;

	print("\nx86 32-bit instructions:\n");

	for (i = 0; i < size;)
	{
		// check if the next [max_instr_size] bytes are zero, if so we are done.
		if (xb_cmp((data + i), zero_mem, (i < size - MAX_INSTR_SIZE ? MAX_INSTR_SIZE : size - i)) == 0)
			break;

		if (parseInstruction(str_instr, data, i) != 0)
		{
			print("Unknown x86 instruction at offset %04x, INSTR: %02X\n", i, data[i]);
			i++;
			unkInstrs = true;
			continue;
		}

		print(stream, "%s\n", str_instr);
	}	

	if (unkInstrs)
		return 1;
	return 0;
}

int parseInstruction(char* buf, UCHAR* data, UINT& offset)
{
	X86_INSTR_MAP* instr = NULL;

	// iterate through the x86 instructions
	for (int i = 0; i < sizeof(instrs) / sizeof(X86_INSTR_MAP); i++)
	{
		if (xb_cmp(data + offset, (UCHAR*)&instrs[i].opcode, instrs[i].opcode_len) == 0)
		{
			instr = &instrs[i];
			break;
		}
	}

	if (instr == NULL)
	{
		return 1;
	}

	switch (instr->type)
	{
		case X86_INSTR_TYPE_1BYTEOP:
		case X86_INSTR_TYPE_2BYTEOP:
			sprintf(buf, "%s", instr->asm_instr);
			break;
		case X86_INSTR_TYPE_1BYTEOP_PTR:
		case X86_INSTR_TYPE_2BYTEOP_PTR:
			sprintf(buf, "%s, [0x%08x]", instr->asm_instr, *(UINT*)(data + offset + instr->opcode_len));
			break;
		case X86_INSTR_TYPE_1BYTEOP_NUM:
		case X86_INSTR_TYPE_2BYTEOP_NUM:
			sprintf(buf, "%s, 0x%08x", instr->asm_instr, *(UINT*)(data + offset + instr->opcode_len));
			break;			
		case X86_INSTR_TYPE_JMP_FAR:
			sprintf(buf, "%s, 0x%08x 0x%x", instr->asm_instr, *(UINT*)(data + offset + instr->opcode_len), *(USHORT*)(data + offset + 5));
			break;
	}
	offset += instr->opcode_len + instr->operrand_len;

	return 0;
}
