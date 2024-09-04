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
#include <cstdio>
#include <memory.h>

// user incl
#include "X86Interp.h"
#include "file.h"
#include "type_defs.h"
#include "util.h"

// x86 instruction map; sort by opcode length; largest first
const X86_INSTR_MAP instrs[] = {
	{ X86_INSTR_REG_PTR, 0x1D8B, "mov ebx", 2, 4 }, // mov ebx, ptr [123]
	{ X86_INSTR_REG_PTR, 0x0D8B, "mov ecx", 2, 4 },
	{ X86_INSTR_REG_PTR, 0x158B, "mov edx", 2, 4 },

	{ X86_INSTR_OP, 0xE0FF, "jmp eax", 2, 0 }, // jmp eax
	{ X86_INSTR_OP, 0xE1FF, "jmp ecx", 2, 0 },
	{ X86_INSTR_OP, 0xE2FF, "jmp edx", 2, 0 },
	{ X86_INSTR_OP, 0xE3FF, "jmp ebx", 2, 0 },
	{ X86_INSTR_OP, 0xE4FF, "jmp esp", 2, 0 },
	{ X86_INSTR_OP, 0xE5FF, "jmp ebp", 2, 0 },
	{ X86_INSTR_OP, 0xE6FF, "jmp esi", 2, 0 },
	{ X86_INSTR_OP, 0xE7FF, "jmp edi", 2, 0 },

	{ X86_INSTR_OP, 0xA5F3, "rep movsd", 2, 0 },

	{ X86_INSTR_JMP_FAR, 0xEA, "jmp", 1, 2+4 }, // jmp seg:offset

	{ X86_INSTR_JMP_REL, 0xE9, "jmp", 1, 4 }, // jmp offset+operand+operand_size
	{ X86_INSTR_JMP_REL, 0xEB, "jmp", 1, 1 },
	{ X86_INSTR_JMP_REL, 0x70, "jo", 1, 1 },
	{ X86_INSTR_JMP_REL, 0x71, "jno", 1, 1 },
	{ X86_INSTR_JMP_REL, 0x72, "jb", 1, 1 },
	{ X86_INSTR_JMP_REL, 0x73, "jae", 1, 1 },
	{ X86_INSTR_JMP_REL, 0x74, "je", 1, 1 },
	{ X86_INSTR_JMP_REL, 0x75, "jne", 1, 1 },

	{ X86_INSTR_REG_NUM, 0xB8, "mov eax", 1, 4 }, // mov eax, 123
	{ X86_INSTR_REG_NUM, 0xB9, "mov ecx", 1, 4 },
	{ X86_INSTR_REG_NUM, 0xBA, "mov edx", 1, 4 },
	{ X86_INSTR_REG_NUM, 0xBB, "mov ebx", 1, 4 },
	{ X86_INSTR_REG_NUM, 0xBC, "mov esp", 1, 4 },
	{ X86_INSTR_REG_NUM, 0xBD, "mov ebp", 1, 4 },
	{ X86_INSTR_REG_NUM, 0xBE, "mov esi", 1, 4 },
	{ X86_INSTR_REG_NUM, 0xBF, "mov edi", 1, 4 },
	
	{ X86_INSTR_REG_PTR, 0xA1, "mov eax", 1, 4 }, // mov eax, ptr [123]

	{ X86_INSTR_OP, 0x90, "nop", 1, 0 },
	{ X86_INSTR_OP, 0xFA, "cli", 1, 0 },
	{ X86_INSTR_OP, 0xFC, "cld", 1, 0 },
	{ X86_INSTR_OP, 0xCC, "int 3", 1, 0 },
	{ X86_INSTR_OP, 0xf, "hlt", 1, 0 },
};

int decodeX86(UCHAR* data, UINT size, FILE* stream, UINT* codeSize)
{
	UINT i;
	const UINT MAX_INSTR_SIZE = 6;
	const UCHAR zero_mem[MAX_INSTR_SIZE] = { 0 };

	char str_instr[128] = { 0 };

	int result = 0;

	printf("\nx86 instructions:\n");

	for (i = 0; i < size;)
	{
		// check if the next [max_instr_size] bytes are zero, if so we are done.
		if (memcmp((data + i), zero_mem, (i < size - MAX_INSTR_SIZE ? MAX_INSTR_SIZE : size - i)) == 0)
			break;

		if (parseInstruction(str_instr, data, i) != 0)
		{
			printf("Unknown x86 instruction at offset %04x, INSTR: %02X\n", i, data[i]);
			i++;
			result = ERROR_INVALID_DATA;
			continue;
		}

		fprintf(stream, "%s\n", str_instr);
	}

	if (codeSize != NULL)
	{
		*codeSize = i;
	}

	return result;
}

const X86_INSTR_MAP* getInstruction(const UCHAR* data)
{
	// iterate through the x86 instructions
	for (int i = 0; i < sizeof(instrs) / sizeof(X86_INSTR_MAP); i++)
	{
		if (memcmp(data, (UCHAR*)&instrs[i].opcode, instrs[i].opcode_len) == 0)
		{
			return &instrs[i];
		}
	}
	return NULL;
}

int parseInstruction(char* buf, UCHAR* data, UINT& offset)
{
	const X86_INSTR_MAP* instr = NULL;
	UINT operand2 = 0;
	
	instr = getInstruction(data + offset);
	if (instr == NULL)
	{
		return ERROR_INVALID_DATA;
	}

	memcpy(&operand2, data + offset + instr->opcode_len, instr->operrand_len > 4 ? 4 : instr->operrand_len);

	const char* operand2_type;
	switch (instr->operrand_len)
	{
		case 1:
			operand2_type = "byte ";
			break;
		case 2:
			operand2_type = "word ";
			break;
		case 4:
			operand2_type = "dword ";
			break;
		default:
			operand2_type = "";
			break;
	}

	switch (instr->type)
	{
		case X86_INSTR_OP:
			sprintf(buf, "%s", instr->asm_instr);
			break;
		case X86_INSTR_REG_PTR:
			sprintf(buf, "%s, %sptr [0x%x]", instr->asm_instr, operand2_type, operand2);
			break;
		case X86_INSTR_REG_NUM:
			sprintf(buf, "%s, %s0x%x", instr->asm_instr, operand2_type, operand2);
			break;			
		case X86_INSTR_JMP_FAR:
			sprintf(buf, "%s 0x%x:0x%x ", instr->asm_instr, operand2, *(USHORT*)(data + offset + 5));
			break;
		case X86_INSTR_JMP_REL: // base + offset?
			sprintf(buf, "%s 0x%x", instr->asm_instr, offset + instr->opcode_len + instr->operrand_len + operand2);
			break;
	}
	offset += instr->opcode_len + instr->operrand_len;

	return 0;
}
