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
#include <stdint.h>
#include <stdio.h>

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#endif

// X86 instruction types;
enum X86_INSTR_TYPE : uint8_t {
	X86_INSTR_INC = 0,
	X86_INSTR_DEC,
	X86_INSTR_SHR,
	X86_INSTR_SHL,
	X86_INSTR_MOV_REG,
	X86_INSTR_MOV_NUM,
	X86_INSTR_MOV_PTR, // reg, [ptr]
	X86_INSTR_XOR,
	X86_INSTR_AND,
	X86_INSTR_OR,
	X86_INSTR_ADD,
	X86_INSTR_SUB,
	X86_INSTR_MUL,
	X86_INSTR_DIV,
	X86_INSTR_JMP_REG,
	X86_INSTR_JMP_DIRECT,
	X86_INSTR_JMP_INDIRECT,
	X86_INSTR_CALL_REG,
	X86_INSTR_CALL_DIRECT,
	X86_INSTR_CALL_INDIRECT,
	X86_INSTR_PUSH_DIRECT,
	X86_INSTR_PUSH_REG,
	X86_INSTR_POP_REG,
	X86_INSTR_REP_MOVSD,
	X86_INSTR_REP_STOSD,
	X86_INSTR_WRMSR,
	X86_INSTR_WBINVD,
	X86_INSTR_PUSHA,
	X86_INSTR_POPA,
	X86_INSTR_NOP,
	X86_INSTR_CLI,
	X86_INSTR_CLD,
	X86_INSTR_STI,
	X86_INSTR_STD,
	X86_INSTR_INT3,
	X86_INSTR_HLT,
	X86_INSTR_LIDTD_EAX_PTR,
	X86_INSTR_LGDTD_EAX_PTR,
	X86_INSTR_LLDT_AX,
	X86_INSTR_CMP,
	X86_INSTR_JO, // OF=1
	X86_INSTR_JNO, // OF=0
	X86_INSTR_JB, // CF=1
	X86_INSTR_JAE, // CF=0 or ZF=0
	X86_INSTR_JE, // ZF=1	
	X86_INSTR_JNE, // ZF=0
	X86_INSTR_JBE, // CF=1 or ZF=1
	X86_INSTR_JA, // CF=0 and ZF=0
	X86_INSTR_JS, // SF=1
	X86_INSTR_JNS, // SF=0
	X86_INSTR_JP, // PF=1
	X86_INSTR_JNP, // PF=0
	X86_INSTR_JL, // SF != OF
	X86_INSTR_JGE, // SF=OF or ZF=1
	X86_INSTR_JLE, // SF != OF or ZF=1
	X86_INSTR_JG, // SF=OF and ZF=0

	X86_INSTR_NONE, // no instruction
};

// x86 opcodes
enum X86_OPCODE : uint32_t {
	X86_OPCODE_NONE = 0,
	X86_OPCODE_NOP = 0x90,
	X86_OPCODE_CLI = 0xFA,
	X86_OPCODE_STI = 0xFB,
	X86_OPCODE_CLD = 0xFC,
	X86_OPCODE_STD = 0xFD,
	X86_OPCODE_INT3 = 0xCC,
	X86_OPCODE_HLT = 0xF4,

	X86_OPCODE_ADD_EAX = 0x05,
	X86_OPCODE_SUB_EAX = 0x2d,
	X86_OPCODE_OR_EAX = 0x0d,
	X86_OPCODE_AND_EAX = 0x25,

	X86_OPCODE_ADD_ECX = 0xc181,
	X86_OPCODE_ADD_EDX = 0xc281,
	X86_OPCODE_ADD_EBX = 0xc381,
	X86_OPCODE_ADD_ESP = 0xc481,
	X86_OPCODE_ADD_EBP = 0xc581,
	X86_OPCODE_ADD_ESI = 0xc681,
	X86_OPCODE_ADD_EDI = 0xc781,

	X86_OPCODE_OR_ECX = 0xc981,
	X86_OPCODE_OR_EDX = 0xca81,
	X86_OPCODE_OR_EBX = 0xcb81,
	X86_OPCODE_OR_ESP = 0xcc81,
	X86_OPCODE_OR_EBP = 0xcd81,
	X86_OPCODE_OR_ESI = 0xce81,
	X86_OPCODE_OR_EDI = 0xcf81,

	X86_OPCODE_AND_ECX = 0xe181,
	X86_OPCODE_AND_EDX = 0xe281,
	X86_OPCODE_AND_EBX = 0xe381,
	X86_OPCODE_AND_ESP = 0xe481,
	X86_OPCODE_AND_EBP = 0xe581,
	X86_OPCODE_AND_ESI = 0xe681,
	X86_OPCODE_AND_EDI = 0xe781,

	X86_OPCODE_SUB_ECX = 0xe881,
	X86_OPCODE_SUB_EDX = 0xea81,
	X86_OPCODE_SUB_EBX = 0xeb81,
	X86_OPCODE_SUB_ESP = 0xec81,
	X86_OPCODE_SUB_EBP = 0xed81,
	X86_OPCODE_SUB_ESI = 0xee81,
	X86_OPCODE_SUB_EDI = 0xef81,

	X86_OPCODE_XOR_EAX_EAX1 = 0xc031,
	X86_OPCODE_XOR_ECX_EAX1 = 0xc131,
	X86_OPCODE_XOR_EDX_EAX1 = 0xc231,
	X86_OPCODE_XOR_EBX_EAX1 = 0xc331,
	X86_OPCODE_XOR_ESP_EAX1 = 0xc431,
	X86_OPCODE_XOR_EBP_EAX1 = 0xc531,
	X86_OPCODE_XOR_ESI_EAX1 = 0xc631,
	X86_OPCODE_XOR_EDI_EAX1 = 0xc731,

	X86_OPCODE_XOR_EAX_ECX1 = 0xc831,
	X86_OPCODE_XOR_ECX_ECX1 = 0xc931,
	X86_OPCODE_XOR_EDX_ECX1 = 0xcA31,
	X86_OPCODE_XOR_EBX_ECX1 = 0xcB31,
	X86_OPCODE_XOR_ESP_ECX1 = 0xcc31,
	X86_OPCODE_XOR_EBP_ECX1 = 0xcd31,
	X86_OPCODE_XOR_ESI_ECX1 = 0xce31,
	X86_OPCODE_XOR_EDI_ECX1 = 0xcf31,

	X86_OPCODE_XOR_EAX_EDX1 = 0xd031,
	X86_OPCODE_XOR_ECX_EDX1 = 0xd131,
	X86_OPCODE_XOR_EDX_EDX1 = 0xd231,
	X86_OPCODE_XOR_EBX_EDX1 = 0xd331,
	X86_OPCODE_XOR_ESP_EDX1 = 0xd431,
	X86_OPCODE_XOR_EBP_EDX1 = 0xd531,
	X86_OPCODE_XOR_ESI_EDX1 = 0xd631,
	X86_OPCODE_XOR_EDI_EDX1 = 0xd731,

	X86_OPCODE_XOR_EAX_EBX1 = 0xd831,
	X86_OPCODE_XOR_ECX_EBX1 = 0xd931,
	X86_OPCODE_XOR_EDX_EBX1 = 0xdA31,
	X86_OPCODE_XOR_EBX_EBX1 = 0xdB31,
	X86_OPCODE_XOR_ESP_EBX1 = 0xdc31,
	X86_OPCODE_XOR_EBP_EBX1 = 0xdd31,
	X86_OPCODE_XOR_ESI_EBX1 = 0xde31,
	X86_OPCODE_XOR_EDI_EBX1 = 0xdf31,

	X86_OPCODE_XOR_EAX_EAX3 = 0xc033,
	X86_OPCODE_XOR_EAX_ECX3 = 0xc133,
	X86_OPCODE_XOR_EAX_EDX3 = 0xc233,
	X86_OPCODE_XOR_EAX_EBX3 = 0xc333,
	X86_OPCODE_XOR_EAX_ESP3 = 0xc433,
	X86_OPCODE_XOR_EAX_EBP3 = 0xc533,
	X86_OPCODE_XOR_EAX_ESI3 = 0xc633,
	X86_OPCODE_XOR_EAX_EDI3 = 0xc733,

	X86_OPCODE_XOR_ECX_EAX3 = 0xc833,
	X86_OPCODE_XOR_ECX_ECX3 = 0xc933,
	X86_OPCODE_XOR_ECX_EDX3 = 0xcA33,
	X86_OPCODE_XOR_ECX_EBX3 = 0xcB33,
	X86_OPCODE_XOR_ECX_ESP3 = 0xcc33,
	X86_OPCODE_XOR_ECX_EBP3 = 0xcd33,
	X86_OPCODE_XOR_ECX_ESI3 = 0xce33,
	X86_OPCODE_XOR_ECX_EDI3 = 0xcf33,

	X86_OPCODE_XOR_EDX_EAX3 = 0xd033,
	X86_OPCODE_XOR_EDX_ECX3 = 0xd133,
	X86_OPCODE_XOR_EDX_EDX3 = 0xd233,
	X86_OPCODE_XOR_EDX_EBX3 = 0xd333,
	X86_OPCODE_XOR_EDX_ESP3 = 0xd433,
	X86_OPCODE_XOR_EDX_EBP3 = 0xd533,
	X86_OPCODE_XOR_EDX_ESI3 = 0xd633,
	X86_OPCODE_XOR_EDX_EDI3 = 0xd733,

	X86_OPCODE_XOR_EBX_EAX3 = 0xd833,
	X86_OPCODE_XOR_EBX_ECX3 = 0xd933,
	X86_OPCODE_XOR_EBX_EDX3 = 0xdA33,
	X86_OPCODE_XOR_EBX_EBX3 = 0xdB33,
	X86_OPCODE_XOR_EBX_ESP3 = 0xdc33,
	X86_OPCODE_XOR_EBX_EBP3 = 0xdd33,
	X86_OPCODE_XOR_EBX_ESI3 = 0xde33,
	X86_OPCODE_XOR_EBX_EDI3 = 0xdf33,

	X86_OPCODE_WRMSR = 0x300f,
	X86_OPCODE_WBINVD = 0x090f,

	X86_OPCODE_MOV_EAX_CR0 = 0xc0200f,
	X86_OPCODE_MOV_CR0_EAX = 0xc0220f,

	X86_OPCODE_MOV_EAX_CR3 = 0xd8200f,
	X86_OPCODE_MOV_CR3_EAX = 0xd8220f,

	X86_OPCODE_ADD_EAX_BYTE = 0xc083,
	X86_OPCODE_ADD_ECX_BYTE = 0xc183,
	X86_OPCODE_ADD_EDX_BYTE = 0xc283,
	X86_OPCODE_ADD_EBX_BYTE = 0xc383,
	X86_OPCODE_ADD_ESP_BYTE = 0xc483,
	X86_OPCODE_ADD_EBP_BYTE = 0xc583,
	X86_OPCODE_ADD_ESI_BYTE = 0xc683,
	X86_OPCODE_ADD_EDI_BYTE = 0xc783,

	X86_OPCODE_MOV_EAX_EAX = 0xc089,
	X86_OPCODE_MOV_ECX_EAX = 0xc189,
	X86_OPCODE_MOV_EDX_EAX = 0xc289,
	X86_OPCODE_MOV_EBX_EAX = 0xc389,
	X86_OPCODE_MOV_ESP_EAX = 0xc489,
	X86_OPCODE_MOV_EBP_EAX = 0xc589,
	X86_OPCODE_MOV_ESI_EAX = 0xc689,
	X86_OPCODE_MOV_EDI_EAX = 0xc789,

	X86_OPCODE_INC_EAX = 0x40,
	X86_OPCODE_INC_ECX = 0x41,
	X86_OPCODE_INC_EDX = 0x42,
	X86_OPCODE_INC_EBX = 0x43,
	X86_OPCODE_INC_ESP = 0x44,
	X86_OPCODE_INC_EBP = 0x45,
	X86_OPCODE_INC_ESI = 0x46,
	X86_OPCODE_INC_EDI = 0x47,

	X86_OPCODE_DEC_EAX = 0x48,
	X86_OPCODE_DEC_ECX = 0x49,
	X86_OPCODE_DEC_EDX = 0x4A,
	X86_OPCODE_DEC_EBX = 0x4B,
	X86_OPCODE_DEC_ESP = 0x4C,
	X86_OPCODE_DEC_EBP = 0x4D,
	X86_OPCODE_DEC_ESI = 0x4E,
	X86_OPCODE_DEC_EDI = 0x4F,

	X86_OPCODE_PUSH_EAX = 0x50,
	X86_OPCODE_PUSH_ECX = 0x51,
	X86_OPCODE_PUSH_EDX = 0x52,
	X86_OPCODE_PUSH_EBX = 0x53,
	X86_OPCODE_PUSH_ESP = 0x54,
	X86_OPCODE_PUSH_EBP = 0x55,
	X86_OPCODE_PUSH_ESI = 0x56,
	X86_OPCODE_PUSH_EDI = 0x57,

	X86_OPCODE_POP_EAX = 0x58,
	X86_OPCODE_POP_ECX = 0x59,
	X86_OPCODE_POP_EDX = 0x5A,
	X86_OPCODE_POP_EBX = 0x5B,
	X86_OPCODE_POP_ESP = 0x5C,
	X86_OPCODE_POP_EBP = 0x5D,
	X86_OPCODE_POP_ESI = 0x5E,
	X86_OPCODE_POP_EDI = 0x5F,

	X86_OPCODE_PUSHA = 0x60,
	X86_OPCODE_POPA = 0x61,

	X86_OPCODE_PUSH_DIRECT = 0x68,

	X86_OPCODE_MOV_EAX_PTR = 0xA1,
	X86_OPCODE_MOV_ECX_PTR = 0x0D8B,
	X86_OPCODE_MOV_EDX_PTR = 0x158B,
	X86_OPCODE_MOV_EBX_PTR = 0x1D8B,

	X86_OPCODE_JMP_EAX = 0xE0FF,
	X86_OPCODE_JMP_ECX = 0xE1FF,
	X86_OPCODE_JMP_EDX = 0xE2FF,
	X86_OPCODE_JMP_EBX = 0xE3FF,
	X86_OPCODE_JMP_ESP = 0xE4FF,
	X86_OPCODE_JMP_EBP = 0xE5FF,
	X86_OPCODE_JMP_ESI = 0xE6FF,
	X86_OPCODE_JMP_EDI = 0xE7FF,

	X86_OPCODE_CALL_REL = 0xE8,
	X86_OPCODE_CALL_FAR = 0x9A,

	X86_OPCODE_JMP_REL = 0xE9,
	X86_OPCODE_JMP_REL_BYTE = 0xEB,
	X86_OPCODE_JMP_FAR = 0xEA,

	X86_OPCODE_JO_BYTE = 0x70,
	X86_OPCODE_JNO_BYTE = 0x71,
	X86_OPCODE_JB_BYTE = 0x72, // JB == JC
	X86_OPCODE_JAE_BYTE = 0x73, // JAE == JNC
	X86_OPCODE_JE_BYTE = 0x74, // JE == JZ
	X86_OPCODE_JNE_BYTE = 0x75, // JNE == JNZ
	X86_OPCODE_JBE_BYTE = 0x76,
	X86_OPCODE_JA_BYTE = 0x77,
	X86_OPCODE_JS_BYTE = 0x78,
	X86_OPCODE_JNS_BYTE = 0x79,
	X86_OPCODE_JP_BYTE = 0x7a, // JP == JPE
	X86_OPCODE_JNP_BYTE = 0x7b, // JNP == JPO
	X86_OPCODE_JL_BYTE = 0x7c,
	X86_OPCODE_JGE_BYTE = 0x7d,
	X86_OPCODE_JLE_BYTE = 0x7e,
	X86_OPCODE_JG_BYTE = 0x7f,

	X86_OPCODE_REP_MOVSD = 0xA5F3,
	X86_OPCODE_REP_STOSD = 0xAAF3,

	X86_OPCODE_SHL_EAX = 0xe0c1,
	X86_OPCODE_SHL_ECX = 0xe1c1,
	X86_OPCODE_SHL_EDX = 0xe2c1,
	X86_OPCODE_SHL_EBX = 0xe3c1,
	X86_OPCODE_SHL_ESP = 0xe4c1,
	X86_OPCODE_SHL_EBP = 0xe5c1,
	X86_OPCODE_SHL_ESI = 0xe6c1,
	X86_OPCODE_SHL_EDI = 0xe7c1,

	X86_OPCODE_SHR_EAX = 0xe8c1,
	X86_OPCODE_SHR_ECX = 0xe9c1,
	X86_OPCODE_SHR_EDX = 0xeac1,
	X86_OPCODE_SHR_EBX = 0xebc1,
	X86_OPCODE_SHR_ESP = 0xecc1,
	X86_OPCODE_SHR_EBP = 0xedc1,
	X86_OPCODE_SHR_ESI = 0xeec1,
	X86_OPCODE_SHR_EDI = 0xefc1,

	X86_OPCODE_MOV_EAX_NUM = 0xB8,
	X86_OPCODE_MOV_ECX_NUM = 0xB9,
	X86_OPCODE_MOV_EDX_NUM = 0xBA,
	X86_OPCODE_MOV_EBX_NUM = 0xBB,
	X86_OPCODE_MOV_ESP_NUM = 0xBC,
	X86_OPCODE_MOV_EBP_NUM = 0xBD,
	X86_OPCODE_MOV_ESI_NUM = 0xBE,
	X86_OPCODE_MOV_EDI_NUM = 0xBF,

	X86_OPCODE_LIDTD_EAX_PTR = 0x18010f,
	X86_OPCODE_LGDTD_EAX_PTR = 0x10010f,
	X86_OPCODE_LLDT_AX_PTR = 0xd0000f,
	
	X86_OPCODE_CMP_EAX_BYTE = 0xf883,
	X86_OPCODE_CMP_ECX_BYTE = 0xf983,
	X86_OPCODE_CMP_EDX_BYTE = 0xfa83,
	X86_OPCODE_CMP_EBX_BYTE = 0xfb83,
	X86_OPCODE_CMP_ESP_BYTE = 0xfc83,
	X86_OPCODE_CMP_EBP_BYTE = 0xfd83,
	X86_OPCODE_CMP_ESI_BYTE = 0xfe83,
	X86_OPCODE_CMP_EDI_BYTE = 0xff83,

	X86_OPCODE_CMP_EAX_WORD = 0xf881,
	X86_OPCODE_CMP_ECX_WORD = 0xf981,
	X86_OPCODE_CMP_EDX_WORD = 0xfa81,
	X86_OPCODE_CMP_EBX_WORD = 0xfb81,
	X86_OPCODE_CMP_ESP_WORD = 0xfc81,
	X86_OPCODE_CMP_EBP_WORD = 0xfd81,
	X86_OPCODE_CMP_ESI_WORD = 0xfe81,
	X86_OPCODE_CMP_EDI_WORD = 0xff81,

	X86_OPCODE_MOV_DS_EAX = 0xd88e,
	X86_OPCODE_MOV_ES_EAX = 0xc08e,
	X86_OPCODE_MOV_SS_EAX = 0xd08e,
	X86_OPCODE_MOV_FS_EAX = 0xe08e,
	X86_OPCODE_MOV_GS_EAX = 0xe88e,

};

// x86 registers
enum X86_REG_TYPE : uint8_t {
	X86_REG_EAX = 0,
	X86_REG_ECX,
	X86_REG_EDX,
	X86_REG_EBX,
	X86_REG_ESP,
	X86_REG_EBP,
	X86_REG_ESI,
	X86_REG_EDI,
	X86_REG_CR0,
	X86_REG_CR3,
	X86_SEG_DS,
	X86_SEG_ES,
	X86_SEG_SS,
	X86_SEG_FS,
	X86_SEG_GS,

	X86_REG_COUNT,
	X86_REG_NONE,
};

// x86 error codes
enum X86_ERROR_CODES {
	X86_ERROR_SUCCESS = 0,
	X86_ERROR = 1,
	X86_ERROR_HLT,
	X86_ERROR_UNKOWN_OPCODE,
	X86_ERROR_MEMORY_OVERRUN,
};

// X86 instruction map
typedef struct {
	X86_INSTR_TYPE type;
	X86_REG_TYPE reg;
	X86_REG_TYPE reg2;
	X86_OPCODE opcode;
	uint8_t opcode_len;
	uint8_t operand1_len;
	uint8_t operand2_len;
} X86_INSTR_MAP;

// X86 instruction
typedef struct {
	const X86_INSTR_MAP* map;
	X86_OPCODE opcode;
	uint32_t operand1;
	uint32_t operand2;
} X86_INSTR;

// x86 e flags
typedef struct  {
	bool CF;
	bool PF;
	bool AF;
	bool ZF;
	bool SF;
	bool OF;
} X86_EFLAGS;

// x86 memory map
typedef struct {
	uint8_t* ram;
	uint32_t ram_start;
	uint32_t ram_end;
	uint32_t ram_size;
	uint8_t* rom;
	uint32_t rom_start;
	uint32_t rom_end;
	uint32_t rom_size;
} X86_MEMORY;

// x86 cpu state
typedef struct {
	uint32_t registers[X86_REG_COUNT];
	X86_EFLAGS eflags;
	uint32_t eip;
	X86_MEMORY memory;
	X86_INSTR instruction;
	bool hlt;
} X86_CPU;

// X86 register map
typedef struct {
	X86_REG_TYPE reg;
	const char* str;
} X86_REG_MAP;

// X86 mnemonic map
typedef struct {
	X86_INSTR_TYPE type;
	const char* str;
} X86_MNEMONIC_MAP;

extern const X86_REG_MAP x86_registers[];
extern const X86_MNEMONIC_MAP x86_mnemonics[];
extern const X86_INSTR_MAP x86_instructions[];
extern const X86_INSTR_MAP x86_unknown_instruction;

// initialize x86 instruction.
inline void initX86Instruction(X86_INSTR* mem) {
	mem->map = &x86_unknown_instruction;
	mem->opcode = X86_OPCODE_NONE;
	mem->operand1 = 0;
	mem->operand2 = 0;
};

// initialize x86 cpu memory
inline int initX86CPUMemory(X86_MEMORY* memory, uint32_t ramStart, uint32_t ramEnd, uint32_t romStart, uint32_t romEnd) {
	// ram
	memory->ram_start = ramStart;
	memory->ram_end = ramEnd;
	memory->ram_size = (ramEnd - ramStart + 1);
	memory->ram = (uint8_t*)malloc(memory->ram_size);

	// rom
	memory->rom_start = romStart;
	memory->rom_end = romEnd;
	memory->rom_size = (romEnd - romStart + 1);
	memory->rom = (uint8_t*)malloc(memory->rom_size);

	if (memory->ram == NULL || memory->rom == NULL)
		return 1;

	return 0;
};

// destroy free x86 cpu memory
inline void freeX86CPUMemory(X86_MEMORY* memory) {
	if (memory->ram != NULL)
	{
		free(memory->ram);
		memory->ram = NULL;
	}
	if (memory->rom != NULL)
	{
		free(memory->rom);
		memory->rom = NULL;
	}
};

// reset cpu state
inline void resetX86CPU(X86_CPU* cpu) {
	for (int i = 0; i < X86_REG_COUNT; ++i)
		cpu->registers[i] = 0;

	cpu->eflags.OF = false;
	cpu->eflags.PF = false;
	cpu->eflags.AF = false;
	cpu->eflags.ZF = false;
	cpu->eflags.SF = false;
	cpu->eflags.CF = false;
	cpu->eip = 0;
	cpu->hlt = false;

	initX86Instruction(&cpu->instruction);
};

// clear cpu memory
inline void clearX86CPUMemory(X86_CPU* cpu) {
	memset(cpu->memory.ram, 0, cpu->memory.ram_size);
	memset(cpu->memory.rom, 0, cpu->memory.rom_size);
};

// initialize cpu.
inline int initX86CPU(X86_CPU* cpu, uint32_t ramStart, uint32_t ramEnd, uint32_t romStart, uint32_t romEnd) {
	if (initX86CPUMemory(&cpu->memory, ramStart, ramEnd, romStart, romEnd) != 0)
		return 1;

	resetX86CPU(cpu);
	clearX86CPUMemory(cpu);

	return 0;
};

// destroy and free cpu.
inline void freeX86CPU(X86_CPU* cpu) {
	resetX86CPU(cpu);
	freeX86CPUMemory(&cpu->memory);
};

// get a cpu memory ptr
// cpu: pointer to cpu state.
// address: the address to get a pointer to. address can reside in rom or ram.
inline void* getX86CPUMemoryPtr(X86_CPU* cpu, uint32_t address) {
	if (address >= cpu->memory.ram_start && address < cpu->memory.ram_end) {
			return cpu->memory.ram + address - cpu->memory.ram_start;
		}
		if (address >= cpu->memory.rom_start && address < cpu->memory.rom_end) {
			return cpu->memory.rom + address - cpu->memory.rom_start;
		}
	return NULL;
};

// get a cpu memory ptr. checks sizes is within the cpu memory.
// cpu: pointer to cpu state.
inline void* getX86CPUMemoryBlock(X86_CPU* cpu, uint32_t address, uint32_t size) {
		if (address >= cpu->memory.ram_start && address + size < cpu->memory.ram_end) {
			return cpu->memory.ram + address - cpu->memory.ram_start;
		}
		if (address >= cpu->memory.rom_start && address + size < cpu->memory.rom_end) {
			return cpu->memory.rom + address - cpu->memory.rom_start;
		}
	return NULL;
};

// copy cpu memory from one address to another. fromAddress and toAddress must reside in cpu memory. (rom or ram)
// cpu: pointer to cpu state.
inline int copyFromToX86CPUMemory(X86_CPU* cpu, uint32_t fromAddress, uint32_t toAddress, uint32_t size) {
	void* src = getX86CPUMemoryBlock(cpu, fromAddress, size);
	void* dest = getX86CPUMemoryBlock(cpu, toAddress, size);
	if (src == NULL || dest == NULL)
		return 1;
	memcpy(dest, src, size);
	return 0;
};

// copy from an external src into cpu memory. toAddress must reside in cpu memory. (rom or ram)
// cpu: pointer to cpu state.
// toAddress: the cpu memory address copy the external data to.
// data: the external data to copy into cpu memory.
// size: the external data size.
inline int copyIntoX86CPUMemory(X86_CPU* cpu, uint32_t toAddress, uint8_t* data, uint32_t size) {
	void* dest = getX86CPUMemoryBlock(cpu, toAddress, size);
	if (data == NULL || dest == NULL)
		return 1;
	memcpy(dest, data, size);
	return 0;
};

// print cpu state to console.
// cpu: pointer to cpu state.
inline void printX86CPUState(const X86_CPU* cpu) {
	for (int i = 0; i < X86_REG_COUNT; ++i)	{
		printf("%s: %x\n", x86_registers[i].str, cpu->registers[i]);
	}

	printf("eflags: OF: %d, ZF: %d, SF: %d, CF: %d\n", 
		cpu->eflags.OF, cpu->eflags.ZF, cpu->eflags.SF, cpu->eflags.CF);

	printf("eip: %x\n", cpu->eip);
};

// arithmetic logic unit
int ALU(X86_CPU* cpu, X86_INSTR_TYPE type, uint32_t operand1, uint32_t operand2, uint32_t* result);

// set status flags based on operands and result.
int ALU_setflags(X86_EFLAGS* eflags, uint32_t operand1, uint32_t operand2, uint32_t result);

// jump logic unit
int JLU(X86_CPU* cpu);

// execute instruction.
int executeX86Instruction(X86_CPU* cpu);

// decode X86 instructions from data, and write to stream
// data: the input buffer.
// size: the size of the input buffer.
// stream: the output stream;
// codeSize: the output code size
int decodeX86(uint8_t* data, uint32_t size, uint32_t base, FILE* stream, uint32_t* codeSize);

// get mnemonic from instruction.
// instruction: the input instruction.
// str: the output mnemonic.
// returns 0 if successful, otherwise 1 if instruction mnemonic not found.
int getX86Mnemonic(X86_INSTR* instruction, char* str);

// generate a instruction tree.
// data: the input buffer.
// size: the size of the input buffer.
// instructionTree: the output instruction tree.
// instructionTreeSize: the instruction tree size.
int generateX86InstructionTree(uint8_t* data, uint32_t size, X86_INSTR** instructionTree, uint32_t* instructionTreeSize);

// get the instruction from the data
// data: the start of the input buffer;
// offset: the offset from the start of the input buffer.
// instruction: the output instruction;
// returns 0 if successful, 1 if instruction not found, otherwise error code.
int getX86Instruction(const uint8_t* data, uint32_t size, X86_INSTR* instruction);

#endif // X86_INTERP_H
