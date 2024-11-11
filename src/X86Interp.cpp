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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>

// user incl
#include "X86Interp.h"
#include "file.h"
#include "util.h"

#ifndef NO_MEM_TRACKING
#include "mem_tracking.h"
#endif

// x86 registers
const X86_REG_MAP x86_registers[] {
	{ X86_REG_EAX, "eax" },
	{ X86_REG_EAX, "ecx" },
	{ X86_REG_EDX, "edx" },
	{ X86_REG_EBX, "ebx" },
	{ X86_REG_ESP, "esp" },
	{ X86_REG_EBP, "ebp" },
	{ X86_REG_ESI, "esi" },
	{ X86_REG_EDI, "edi" },
	{ X86_REG_CR0, "cr0" },
	{ X86_REG_CR3, "cr3" },
	{ X86_SEG_DS, "ds" },
	{ X86_SEG_ES, "es" },
	{ X86_SEG_SS, "ss" },
	{ X86_SEG_FS, "fs" },
	{ X86_SEG_GS, "gs" },
};

// x86 mnemonics
const X86_MNEMONIC_MAP x86_mnemonics[] {
	{ X86_INSTR_INC, "inc" },
	{ X86_INSTR_DEC, "dec" },
	{ X86_INSTR_SHR, "shr" },
	{ X86_INSTR_SHL, "shl" },
	{ X86_INSTR_MOV_REG, "mov"},
	{ X86_INSTR_MOV_NUM, "mov"},
	{ X86_INSTR_MOV_PTR, "mov"},
	{ X86_INSTR_XOR, "xor" },
	{ X86_INSTR_AND, "and" },
	{ X86_INSTR_OR, "or" },
	{ X86_INSTR_ADD, "add" },
	{ X86_INSTR_SUB, "sub" },
	{ X86_INSTR_MUL, "mul" },
	{ X86_INSTR_DIV, "div" },
	{ X86_INSTR_JMP_REG, "jmp" },
	{ X86_INSTR_JMP_DIRECT, "jmp" },
	{ X86_INSTR_JMP_INDIRECT, "jmp" },
	{ X86_INSTR_CALL_REG, "call" },
	{ X86_INSTR_CALL_DIRECT, "call" },
	{ X86_INSTR_CALL_INDIRECT, "call" },
	{ X86_INSTR_PUSH_REG, "push" },
	{ X86_INSTR_PUSH_DIRECT, "push" },
	{ X86_INSTR_POP_REG, "pop" },
	{ X86_INSTR_REP_MOVSD, "rep movsd" },
	{ X86_INSTR_REP_STOSD, "rep stosd" },
	{ X86_INSTR_WRMSR, "wrmsr" },
	{ X86_INSTR_WBINVD, "wbinvd" },
	{ X86_INSTR_PUSHA, "pusha" },
	{ X86_INSTR_POPA, "popa" },
	{ X86_INSTR_NOP, "nop" },
	{ X86_INSTR_CLI, "cli" },
	{ X86_INSTR_CLD, "cld" },
	{ X86_INSTR_STI, "sti" },
	{ X86_INSTR_STD, "std" },
	{ X86_INSTR_INT3, "int 3" },
	{ X86_INSTR_HLT, "hlt" },
	{ X86_INSTR_LIDTD_EAX_PTR, "lidtd [eax]" },
	{ X86_INSTR_LGDTD_EAX_PTR, "lgdtd [eax]" },
	{ X86_INSTR_LLDT_AX, "lldt ax" },
	{ X86_INSTR_CMP, "cmp" },	
	{ X86_INSTR_JO, "jo" },
	{ X86_INSTR_JNO, "jno" },
	{ X86_INSTR_JB, "jb" },
	{ X86_INSTR_JAE, "jae" },
	{ X86_INSTR_JE, "je" },
	{ X86_INSTR_JNE, "jne" },
	{ X86_INSTR_JBE, "jbe" },
	{ X86_INSTR_JA, "ja" },
	{ X86_INSTR_JS, "js" },
	{ X86_INSTR_JNS, "jns" },
	{ X86_INSTR_JP, "jp" },
	{ X86_INSTR_JNP, "jnp" },
	{ X86_INSTR_JL, "jl" },
	{ X86_INSTR_JGE, "jge" },
	{ X86_INSTR_JLE, "jle" },
	{ X86_INSTR_JG, "jg" },
};

// x86 instructions
const X86_INSTR_MAP x86_instructions[] = {

	// 3 byte opcodes
	{ X86_INSTR_MOV_REG, X86_REG_EAX, X86_REG_CR0, X86_OPCODE_MOV_EAX_CR0, 3, 0,0 },
	{ X86_INSTR_MOV_REG, X86_REG_CR0, X86_REG_EAX, X86_OPCODE_MOV_CR0_EAX, 3, 0,0 },

	{ X86_INSTR_LIDTD_EAX_PTR, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_LIDTD_EAX_PTR, 3, 0,0 },
	{ X86_INSTR_LGDTD_EAX_PTR, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_LGDTD_EAX_PTR, 3, 0,0 },
	{ X86_INSTR_LLDT_AX, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_LLDT_AX_PTR, 3, 0,0 },

	{ X86_INSTR_MOV_REG, X86_REG_EAX, X86_REG_CR3, X86_OPCODE_MOV_EAX_CR3, 3, 0,0 },
	{ X86_INSTR_MOV_REG, X86_REG_CR3, X86_REG_EAX, X86_OPCODE_MOV_CR3_EAX, 3, 0,0 },

	// 2 byte opcodes
	{ X86_INSTR_MOV_PTR, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_MOV_ECX_PTR, 2, 4,0 },
	{ X86_INSTR_MOV_PTR, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_MOV_EBX_PTR, 2, 4,0 },
	{ X86_INSTR_MOV_PTR, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_MOV_EDX_PTR, 2, 4,0 },

	{ X86_INSTR_REP_MOVSD, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_REP_MOVSD, 2, 0,0 },
	{ X86_INSTR_REP_STOSD, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_REP_STOSD, 2, 0,0 },

	{ X86_INSTR_SHR, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_SHR_EAX, 2, 1,0 },
	{ X86_INSTR_SHR, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_SHR_ECX, 2, 1,0 },
	{ X86_INSTR_SHR, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_SHR_EDX, 2, 1,0 },
	{ X86_INSTR_SHR, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_SHR_EBX, 2, 1,0 },
	{ X86_INSTR_SHR, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_SHR_ESP, 2, 1,0 },
	{ X86_INSTR_SHR, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_SHR_EBP, 2, 1,0 },
	{ X86_INSTR_SHR, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_SHR_ESI, 2, 1,0 },
	{ X86_INSTR_SHR, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_SHR_EDI, 2, 1,0 },

	{ X86_INSTR_SHL, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_SHL_EAX, 2, 1,0 },
	{ X86_INSTR_SHL, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_SHL_ECX, 2, 1,0 },
	{ X86_INSTR_SHL, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_SHL_EDX, 2, 1,0 },
	{ X86_INSTR_SHL, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_SHL_EBX, 2, 1,0 },
	{ X86_INSTR_SHL, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_SHL_ESP, 2, 1,0 },
	{ X86_INSTR_SHL, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_SHL_EBP, 2, 1,0 },
	{ X86_INSTR_SHL, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_SHL_ESI, 2, 1,0 },
	{ X86_INSTR_SHL, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_SHL_EDI, 2, 1,0 },
	
	{ X86_INSTR_MOV_REG, X86_SEG_DS, X86_REG_EAX, X86_OPCODE_MOV_DS_EAX, 2, 0,0 },
	{ X86_INSTR_MOV_REG, X86_SEG_ES, X86_REG_EAX, X86_OPCODE_MOV_ES_EAX, 2, 0,0 },
	{ X86_INSTR_MOV_REG, X86_SEG_SS, X86_REG_EAX, X86_OPCODE_MOV_SS_EAX, 2, 0,0 },
	{ X86_INSTR_MOV_REG, X86_SEG_FS, X86_REG_EAX, X86_OPCODE_MOV_FS_EAX, 2, 0,0 },
	{ X86_INSTR_MOV_REG, X86_SEG_GS, X86_REG_EAX, X86_OPCODE_MOV_GS_EAX, 2, 0,0 },

	{ X86_INSTR_MOV_REG, X86_REG_EAX, X86_REG_EAX, X86_OPCODE_MOV_EAX_EAX, 2, 0,0 },
	{ X86_INSTR_MOV_REG, X86_REG_ECX, X86_REG_EAX, X86_OPCODE_MOV_ECX_EAX, 2, 0,0 },
	{ X86_INSTR_MOV_REG, X86_REG_EDX, X86_REG_EAX, X86_OPCODE_MOV_EDX_EAX, 2, 0,0 },
	{ X86_INSTR_MOV_REG, X86_REG_EBX, X86_REG_EAX, X86_OPCODE_MOV_EBX_EAX, 2, 0,0 },
	{ X86_INSTR_MOV_REG, X86_REG_ESP, X86_REG_EAX, X86_OPCODE_MOV_ESP_EAX, 2, 0,0 },
	{ X86_INSTR_MOV_REG, X86_REG_EBP, X86_REG_EAX, X86_OPCODE_MOV_EBP_EAX, 2, 0,0 },
	{ X86_INSTR_MOV_REG, X86_REG_ESI, X86_REG_EAX, X86_OPCODE_MOV_ESI_EAX, 2, 0,0 },
	{ X86_INSTR_MOV_REG, X86_REG_EDI, X86_REG_EAX, X86_OPCODE_MOV_EDI_EAX, 2, 0,0 },

	////
	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_EAX, X86_OPCODE_XOR_EAX_EAX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_EAX, X86_OPCODE_XOR_ECX_EAX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_EAX, X86_OPCODE_XOR_EDX_EAX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_EAX, X86_OPCODE_XOR_EBX_EAX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ESP, X86_REG_EAX, X86_OPCODE_XOR_ESP_EAX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBP, X86_REG_EAX, X86_OPCODE_XOR_EBP_EAX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ESI, X86_REG_EAX, X86_OPCODE_XOR_ESI_EAX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDI, X86_REG_EAX, X86_OPCODE_XOR_EDI_EAX1, 2, 0,0 },

	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_ECX, X86_OPCODE_XOR_EAX_ECX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_ECX, X86_OPCODE_XOR_ECX_ECX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_ECX, X86_OPCODE_XOR_EDX_ECX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_ECX, X86_OPCODE_XOR_EBX_ECX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ESP, X86_REG_ECX, X86_OPCODE_XOR_ESP_ECX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBP, X86_REG_ECX, X86_OPCODE_XOR_EBP_ECX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ESI, X86_REG_ECX, X86_OPCODE_XOR_ESI_ECX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDI, X86_REG_ECX, X86_OPCODE_XOR_EDI_ECX1, 2, 0,0 },

	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_EDX, X86_OPCODE_XOR_EAX_EDX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_EDX, X86_OPCODE_XOR_ECX_EDX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_EDX, X86_OPCODE_XOR_EDX_EDX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_EDX, X86_OPCODE_XOR_EBX_EDX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ESP, X86_REG_EDX, X86_OPCODE_XOR_ESP_EDX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBP, X86_REG_EDX, X86_OPCODE_XOR_EBP_EDX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ESI, X86_REG_EDX, X86_OPCODE_XOR_ESI_EDX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDI, X86_REG_EDX, X86_OPCODE_XOR_EDI_EDX1, 2, 0,0 },

	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_EBX, X86_OPCODE_XOR_EAX_EBX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_EBX, X86_OPCODE_XOR_ECX_EBX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_EBX, X86_OPCODE_XOR_EDX_EBX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_EBX, X86_OPCODE_XOR_EBX_EBX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ESP, X86_REG_EBX, X86_OPCODE_XOR_ESP_EBX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBP, X86_REG_EBX, X86_OPCODE_XOR_EBP_EBX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ESI, X86_REG_EBX, X86_OPCODE_XOR_ESI_EBX1, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDI, X86_REG_EBX, X86_OPCODE_XOR_EDI_EBX1, 2, 0,0 },

	///
	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_EAX, X86_OPCODE_XOR_EAX_EAX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_ECX, X86_OPCODE_XOR_EAX_ECX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_EDX, X86_OPCODE_XOR_EAX_EDX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_EBX, X86_OPCODE_XOR_EAX_EBX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_ESP, X86_OPCODE_XOR_EAX_ESP3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_EBP, X86_OPCODE_XOR_EAX_EBP3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_ESI, X86_OPCODE_XOR_EAX_ESI3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EAX, X86_REG_EDI, X86_OPCODE_XOR_EAX_EDI3, 2, 0,0 },

	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_EAX, X86_OPCODE_XOR_ECX_EAX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_ECX, X86_OPCODE_XOR_ECX_ECX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_EDX, X86_OPCODE_XOR_ECX_EDX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_EBX, X86_OPCODE_XOR_ECX_EBX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_ESP, X86_OPCODE_XOR_ECX_ESP3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_EBP, X86_OPCODE_XOR_ECX_EBP3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_ESI, X86_OPCODE_XOR_ECX_ESI3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_ECX, X86_REG_EDI, X86_OPCODE_XOR_ECX_EDI3, 2, 0,0 },

	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_EAX, X86_OPCODE_XOR_EDX_EAX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_ECX, X86_OPCODE_XOR_EDX_ECX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_EDX, X86_OPCODE_XOR_EDX_EDX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_EBX, X86_OPCODE_XOR_EDX_EBX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_ESP, X86_OPCODE_XOR_EDX_ESP3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_EBP, X86_OPCODE_XOR_EDX_EBP3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_ESI, X86_OPCODE_XOR_EDX_ESI3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EDX, X86_REG_EDI, X86_OPCODE_XOR_EDX_EDI3, 2, 0,0 },

	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_EAX, X86_OPCODE_XOR_EBX_EAX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_ECX, X86_OPCODE_XOR_EBX_ECX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_EDX, X86_OPCODE_XOR_EBX_EDX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_EBX, X86_OPCODE_XOR_EBX_EBX3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_ESP, X86_OPCODE_XOR_EBX_ESP3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_EBP, X86_OPCODE_XOR_EBX_EBP3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_ESI, X86_OPCODE_XOR_EBX_ESI3, 2, 0,0 },
	{ X86_INSTR_XOR, X86_REG_EBX, X86_REG_EDI, X86_OPCODE_XOR_EBX_EDI3, 2, 0,0 },

	{ X86_INSTR_WRMSR, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_WRMSR, 2, 0,0 },
	{ X86_INSTR_WBINVD, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_WBINVD, 2, 0,0 },

	{ X86_INSTR_OR, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_OR_ECX, 2, 4,0 },
	{ X86_INSTR_OR, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_OR_EDX, 2, 4,0 },
	{ X86_INSTR_OR, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_OR_EBX, 2, 4,0 },
	{ X86_INSTR_OR, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_OR_ESP, 2, 4,0 },
	{ X86_INSTR_OR, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_OR_EBP, 2, 4,0 },
	{ X86_INSTR_OR, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_OR_ESI, 2, 4,0 },
	{ X86_INSTR_OR, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_OR_EDI, 2, 4,0 },

	{ X86_INSTR_AND, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_AND_ECX, 2, 4,0 },
	{ X86_INSTR_AND, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_AND_EDX, 2, 4,0 },
	{ X86_INSTR_AND, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_AND_EBX, 2, 4,0 },
	{ X86_INSTR_AND, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_AND_ESP, 2, 4,0 },
	{ X86_INSTR_AND, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_AND_EBP, 2, 4,0 },
	{ X86_INSTR_AND, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_AND_ESI, 2, 4,0 },
	{ X86_INSTR_AND, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_AND_EDI, 2, 4,0 },

	{ X86_INSTR_ADD, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_ADD_EAX_BYTE, 2, 1,0 },
	{ X86_INSTR_ADD, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_ADD_ECX_BYTE, 2, 1,0 },
	{ X86_INSTR_ADD, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_ADD_EDX_BYTE, 2, 1,0 },
	{ X86_INSTR_ADD, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_ADD_EBX_BYTE, 2, 1,0 },
	{ X86_INSTR_ADD, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_ADD_ESP_BYTE, 2, 1,0 },
	{ X86_INSTR_ADD, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_ADD_EBP_BYTE, 2, 1,0 },
	{ X86_INSTR_ADD, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_ADD_ESI_BYTE, 2, 1,0 },
	{ X86_INSTR_ADD, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_ADD_EDI_BYTE, 2, 1,0 },

	{ X86_INSTR_ADD, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_ADD_ECX, 2, 4,0 },
	{ X86_INSTR_ADD, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_ADD_EDX, 2, 4,0 },
	{ X86_INSTR_ADD, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_ADD_EBX, 2, 4,0 },
	{ X86_INSTR_ADD, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_ADD_ESP, 2, 4,0 },
	{ X86_INSTR_ADD, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_ADD_EBP, 2, 4,0 },
	{ X86_INSTR_ADD, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_ADD_ESI, 2, 4,0 },
	{ X86_INSTR_ADD, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_ADD_EDI, 2, 4,0 },

	{ X86_INSTR_SUB, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_SUB_ECX, 2, 4,0 },
	{ X86_INSTR_SUB, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_SUB_EDX, 2, 4,0 },
	{ X86_INSTR_SUB, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_SUB_EBX, 2, 4,0 },
	{ X86_INSTR_SUB, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_SUB_ESP, 2, 4,0 },
	{ X86_INSTR_SUB, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_SUB_EBP, 2, 4,0 },
	{ X86_INSTR_SUB, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_SUB_ESI, 2, 4,0 },
	{ X86_INSTR_SUB, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_SUB_EDI, 2, 4,0 },

	{ X86_INSTR_JMP_REG, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_JMP_EAX, 2, 0,0 },
	{ X86_INSTR_JMP_REG, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_JMP_ECX, 2, 0,0 },
	{ X86_INSTR_JMP_REG, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_JMP_EBX, 2, 0,0 },
	{ X86_INSTR_JMP_REG, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_JMP_EDX, 2, 0,0 },
	{ X86_INSTR_JMP_REG, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_JMP_ESP, 2, 0,0 },
	{ X86_INSTR_JMP_REG, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_JMP_EBP, 2, 0,0 },
	{ X86_INSTR_JMP_REG, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_JMP_ESI, 2, 0,0 },
	{ X86_INSTR_JMP_REG, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_JMP_EDI, 2, 0,0 },

	{ X86_INSTR_CMP, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_CMP_EAX_BYTE, 2, 1,0 },
	{ X86_INSTR_CMP, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_CMP_ECX_BYTE, 2, 1,0 },
	{ X86_INSTR_CMP, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_CMP_EDX_BYTE, 2, 1,0 },
	{ X86_INSTR_CMP, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_CMP_EBX_BYTE, 2, 1,0 },
	{ X86_INSTR_CMP, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_CMP_ESP_BYTE, 2, 1,0 },
	{ X86_INSTR_CMP, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_CMP_EBP_BYTE, 2, 1,0 },
	{ X86_INSTR_CMP, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_CMP_ESI_BYTE, 2, 1,0 },
	{ X86_INSTR_CMP, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_CMP_EDI_BYTE, 2, 1,0 },

	{ X86_INSTR_CMP, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_CMP_EAX_WORD, 2, 4,0 },
	{ X86_INSTR_CMP, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_CMP_ECX_WORD, 2, 4,0 },
	{ X86_INSTR_CMP, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_CMP_EDX_WORD, 2, 4,0 },
	{ X86_INSTR_CMP, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_CMP_EBX_WORD, 2, 4,0 },
	{ X86_INSTR_CMP, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_CMP_ESP_WORD, 2, 4,0 },
	{ X86_INSTR_CMP, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_CMP_EBP_WORD, 2, 4,0 },
	{ X86_INSTR_CMP, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_CMP_ESI_WORD, 2, 4,0 },
	{ X86_INSTR_CMP, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_CMP_EDI_WORD, 2, 4,0 },

	// 1 byte opcodes
	{ X86_INSTR_JO, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JO_BYTE, 1, 1,0 },
	{ X86_INSTR_JNO, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JNO_BYTE, 1, 1,0 },
	{ X86_INSTR_JB, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JB_BYTE, 1, 1,0 },
	{ X86_INSTR_JAE, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JAE_BYTE, 1, 1,0 },
	{ X86_INSTR_JE, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JE_BYTE, 1, 1,0 },
	{ X86_INSTR_JNE, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JNE_BYTE, 1, 1,0 },		
	{ X86_INSTR_JBE, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JBE_BYTE, 1, 1,0 },
	{ X86_INSTR_JA, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JA_BYTE, 1, 1,0 },
	{ X86_INSTR_JS, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JS_BYTE, 1, 1,0 },
	{ X86_INSTR_JNS, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JNS_BYTE, 1, 1,0 },
	{ X86_INSTR_JP, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JP_BYTE, 1, 1,0 },
	{ X86_INSTR_JNP, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JNP_BYTE, 1, 1,0 },
	{ X86_INSTR_JL, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JL_BYTE, 1, 1,0 },
	{ X86_INSTR_JGE, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JGE_BYTE, 1, 1,0 },
	{ X86_INSTR_JLE, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JLE_BYTE, 1, 1,0 },
	{ X86_INSTR_JG, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JG_BYTE, 1, 1,0 },

	{ X86_INSTR_CALL_DIRECT, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_CALL_FAR, 1, 4,2 }, // Ap
	{ X86_INSTR_CALL_INDIRECT, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_CALL_REL, 1, 4,0}, // Jz

	{ X86_INSTR_JMP_DIRECT, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JMP_FAR, 1, 4,2 }, // Ap
	{ X86_INSTR_JMP_INDIRECT, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JMP_REL, 1, 4,0}, // Jz
	{ X86_INSTR_JMP_INDIRECT, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_JMP_REL_BYTE, 1, 1,0 }, // Jb
	
	{ X86_INSTR_MOV_NUM, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_MOV_EAX_NUM, 1, 4,0 },
	{ X86_INSTR_MOV_NUM, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_MOV_ECX_NUM, 1, 4,0 },
	{ X86_INSTR_MOV_NUM, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_MOV_EDX_NUM, 1, 4,0 },
	{ X86_INSTR_MOV_NUM, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_MOV_EBX_NUM, 1, 4,0 },
	{ X86_INSTR_MOV_NUM, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_MOV_ESP_NUM, 1, 4,0 },
	{ X86_INSTR_MOV_NUM, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_MOV_EBP_NUM, 1, 4,0 },
	{ X86_INSTR_MOV_NUM, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_MOV_ESI_NUM, 1, 4,0 },
	{ X86_INSTR_MOV_NUM, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_MOV_EDI_NUM, 1, 4,0 },

	{ X86_INSTR_MOV_PTR, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_MOV_EAX_PTR, 1, 4,0 }, // eax ptr reg is 1 byte optimized instead of 2.
	
	{ X86_INSTR_AND, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_AND_EAX, 1, 4,0 },
	{ X86_INSTR_ADD, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_ADD_EAX, 1, 4,0 },
	{ X86_INSTR_SUB, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_SUB_EAX, 1, 4,0 },
	{ X86_INSTR_OR, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_OR_EAX, 1, 4,0 },
		
	{ X86_INSTR_INC, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_INC_EAX, 1, 0,0 },
	{ X86_INSTR_INC, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_INC_ECX, 1, 0,0 },
	{ X86_INSTR_INC, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_INC_EDX, 1, 0,0 },
	{ X86_INSTR_INC, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_INC_EBX, 1, 0,0 },
	{ X86_INSTR_INC, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_INC_ESP, 1, 0,0 },
	{ X86_INSTR_INC, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_INC_EBP, 1, 0,0 },
	{ X86_INSTR_INC, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_INC_ESI, 1, 0,0 },
	{ X86_INSTR_INC, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_INC_EDI, 1, 0,0 },

	{ X86_INSTR_DEC, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_DEC_EAX, 1, 0,0 },
	{ X86_INSTR_DEC, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_DEC_ECX, 1, 0,0 },
	{ X86_INSTR_DEC, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_DEC_EDX, 1, 0,0 },
	{ X86_INSTR_DEC, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_DEC_EBX, 1, 0,0 },
	{ X86_INSTR_DEC, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_DEC_ESP, 1, 0,0 },
	{ X86_INSTR_DEC, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_DEC_EBP, 1, 0,0 },
	{ X86_INSTR_DEC, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_DEC_ESI, 1, 0,0 },
	{ X86_INSTR_DEC, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_DEC_EDI, 1, 0,0 },

	{ X86_INSTR_PUSH_REG, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_PUSH_EAX, 1, 0,0 },
	{ X86_INSTR_PUSH_REG, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_PUSH_ECX, 1, 0,0 },
	{ X86_INSTR_PUSH_REG, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_PUSH_EDX, 1, 0,0 },
	{ X86_INSTR_PUSH_REG, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_PUSH_EBX, 1, 0,0 },
	{ X86_INSTR_PUSH_REG, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_PUSH_ESP, 1, 0,0 },
	{ X86_INSTR_PUSH_REG, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_PUSH_EBP, 1, 0,0 },
	{ X86_INSTR_PUSH_REG, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_PUSH_ESI, 1, 0,0 },
	{ X86_INSTR_PUSH_REG, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_PUSH_EDI, 1, 0,0 },
	{ X86_INSTR_PUSH_DIRECT, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_PUSH_DIRECT, 1, 4,0 },

	{ X86_INSTR_POP_REG, X86_REG_EAX, X86_REG_NONE, X86_OPCODE_POP_EAX, 1, 0,0 },
	{ X86_INSTR_POP_REG, X86_REG_ECX, X86_REG_NONE, X86_OPCODE_POP_ECX, 1, 0,0 },
	{ X86_INSTR_POP_REG, X86_REG_EDX, X86_REG_NONE, X86_OPCODE_POP_EDX, 1, 0,0 },
	{ X86_INSTR_POP_REG, X86_REG_EBX, X86_REG_NONE, X86_OPCODE_POP_EBX, 1, 0,0 },
	{ X86_INSTR_POP_REG, X86_REG_ESP, X86_REG_NONE, X86_OPCODE_POP_ESP, 1, 0,0 },
	{ X86_INSTR_POP_REG, X86_REG_EBP, X86_REG_NONE, X86_OPCODE_POP_EBP, 1, 0,0 },
	{ X86_INSTR_POP_REG, X86_REG_ESI, X86_REG_NONE, X86_OPCODE_POP_ESI, 1, 0,0 },
	{ X86_INSTR_POP_REG, X86_REG_EDI, X86_REG_NONE, X86_OPCODE_POP_EDI, 1, 0,0 },

	{ X86_INSTR_PUSHA, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_PUSHA, 1, 0,0 },
	{ X86_INSTR_POPA, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_POPA, 1, 0,0 },
	{ X86_INSTR_NOP, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_NOP, 1, 0,0 },
	{ X86_INSTR_CLI, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_CLI, 1, 0,0 },
	{ X86_INSTR_CLD, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_CLD, 1, 0,0 },
	{ X86_INSTR_STI, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_STI, 1, 0,0 },
	{ X86_INSTR_STD, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_STD, 1, 0,0 },
	{ X86_INSTR_INT3, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_INT3, 1, 0,0 },
	{ X86_INSTR_HLT, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_HLT, 1, 0,0 },
};

const X86_INSTR_MAP x86_unknown_instruction = { X86_INSTR_NONE, X86_REG_NONE, X86_REG_NONE, X86_OPCODE_NONE, 0, 0,0 };

const uint32_t MAX_INSTR_SIZE = 6; // 2 + 4

int decodeX86(uint8_t* data, uint32_t size, uint32_t base, FILE* stream, uint32_t* codeSize)
{
	int result;
	uint32_t i;
	char str_instr[128] = { 0 };
	X86_INSTR* instructionTree = NULL;
	uint32_t instructionTreeSize;

	result = generateX86InstructionTree(data+base, size-base, &instructionTree, &instructionTreeSize);
	if (result > 1) {
		printf("failed to generate instruction tree.\n");
		if (instructionTree != NULL) {
			free(instructionTree);
			instructionTree = NULL;
		}
		return 1;
	}

	if (instructionTreeSize == 0) {
		printf("No instructions found.\n");
	}
	else {
		printf("\nx86 instructions:\n");

		uint32_t offset = 0;
		for (i = 0; i < instructionTreeSize / sizeof(X86_INSTR); ++i) {
			getX86Mnemonic(&instructionTree[i], str_instr);
			fprintf(stream, "\t %s\n", str_instr);

			offset += (instructionTree[i].map->opcode_len + instructionTree[i].map->operand1_len + instructionTree[i].map->operand2_len);
		}

		if (codeSize != NULL) {
			*codeSize = offset;
		}
	}

	free(instructionTree);
	instructionTree = NULL;

	return 0;
}

int getX86Instruction(const uint8_t* data, uint32_t size, X86_INSTR* instruction)
{
	if (instruction == NULL)
		return X86_ERROR;

	initX86Instruction(instruction);
	
	// iterate through the x86 instructions
	for (uint32_t i = 0; i < sizeof(x86_instructions) / sizeof(X86_INSTR_MAP); i++) {
		if (x86_instructions[i].opcode_len > size)
			continue;
		if (memcmp(data, (uint8_t*)&x86_instructions[i].opcode, x86_instructions[i].opcode_len) == 0) {
			instruction->map = &x86_instructions[i];
		}
	}

	if (instruction->map->type == X86_INSTR_NONE) {
		memcpy(&instruction->opcode, data, 1);
		return X86_ERROR_UNKOWN_OPCODE;
	}

	memcpy(&instruction->opcode, data, instruction->map->opcode_len);

	if (instruction->map->operand1_len > 0) {
		memcpy(&instruction->operand1, data + instruction->map->opcode_len, instruction->map->operand1_len);
	}
	
	if (instruction->map->operand2_len > 0) {
		memcpy(&instruction->operand2, data + instruction->map->opcode_len + instruction->map->operand1_len, instruction->map->operand2_len);
	}

	//instruction->len = instruction->map->opcode_len + instruction->map->operand1_len + instruction->map->operand2_len;
	
	return X86_ERROR_SUCCESS;
}

int getX86Mnemonic(X86_INSTR* instruction, char* str)
{
	uint32_t i = 0;
	uint32_t operand1 = instruction->operand1;
	const X86_INSTR_MAP map = *instruction->map;

	if (map.type == X86_INSTR_NONE) {
		sprintf(str + i, "unk 0x%02x", instruction->opcode);
		return X86_ERROR_UNKOWN_OPCODE;
	}

	strcpy(str, x86_mnemonics[map.type].str);
	i = strlen(str);

	if (map.reg != X86_REG_NONE) {
		sprintf(str + i, " %s", x86_registers[map.reg].str);
		i = strlen(str);
	}
	
	if (map.reg2 != X86_REG_NONE) {
		sprintf(str + i, ", %s", x86_registers[map.reg2].str);
		i = strlen(str);
	}
		
	if (instruction->map->operand1_len == 0)
		return X86_ERROR_SUCCESS;

	if (map.reg != X86_REG_NONE) {
		strcpy(str + i, ",");
		i++;
	}

	switch (map.type) {
		case X86_INSTR_MOV_PTR:
			sprintf(str + i, " [0x%02x]", operand1);
			break;

		case X86_INSTR_JMP_INDIRECT:
		case X86_INSTR_CALL_INDIRECT:
			operand1 += (map.opcode_len + map.operand1_len + map.operand1_len);
			sprintf(str + i, " 0x%02x", operand1);
			break;

		case X86_INSTR_JMP_DIRECT:
		case X86_INSTR_CALL_DIRECT:
			sprintf(str + i, " 0x%x:0x%02x ", operand1, instruction->operand2);
			break;

		default:
			sprintf(str + i, " 0x%02x", operand1);
			break;
	}

	return X86_ERROR_SUCCESS;
}

int executeX86Instruction(X86_CPU* cpu)
{
	const X86_INSTR_MAP* map;
	uint32_t operand;
	X86_REG_TYPE reg;
	uint8_t* instructionPtr;
	int result;
	
	if (cpu->hlt)
		return X86_ERROR_HLT;
	
	instructionPtr = (uint8_t*)getX86CPUMemoryPtr(cpu, cpu->eip);
	if (instructionPtr == NULL)
		return X86_ERROR;

	result = getX86Instruction(instructionPtr, cpu->memory.ram_end, &cpu->instruction);
	if (result != 0)
		return result;
		
	operand = cpu->instruction.operand1;
	map = cpu->instruction.map;
	reg = map->reg;

	cpu->eip += (map->opcode_len + map->operand1_len + map->operand2_len);

	if (map->reg2 != X86_REG_NONE)
		operand = cpu->registers[map->reg2];

	switch (map->type) {

	case X86_INSTR_MOV_REG:
		cpu->registers[reg] = operand;
		break;

	case X86_INSTR_MOV_NUM:
		cpu->registers[reg] = operand;
		break;

	case X86_INSTR_MOV_PTR:	{
		uint32_t* ptr = (uint32_t*)getX86CPUMemoryBlock(cpu, operand, sizeof(uint32_t));
		if (ptr == NULL)
			return X86_ERROR;
		cpu->registers[reg] = *ptr;
	} break;
	
	case X86_INSTR_XOR:
	case X86_INSTR_AND:
	case X86_INSTR_OR:
	case X86_INSTR_ADD:
	case X86_INSTR_SUB:
	case X86_INSTR_MUL:
	case X86_INSTR_DIV:
	case X86_INSTR_SHR:
	case X86_INSTR_SHL:
	case X86_INSTR_DEC:
	case X86_INSTR_INC:
	case X86_INSTR_CMP:
		ALU(cpu, map->type, cpu->registers[reg], operand, &cpu->registers[reg]);
		break;

	case X86_INSTR_JMP_REG:
	case X86_INSTR_JMP_DIRECT:
	case X86_INSTR_JMP_INDIRECT:
	case X86_INSTR_JE:
	case X86_INSTR_JNE:
	case X86_INSTR_JG:
	case X86_INSTR_JGE: 
	case X86_INSTR_JA:
	case X86_INSTR_JAE:
	case X86_INSTR_JL:
	case X86_INSTR_JLE:
	case X86_INSTR_JS:
	case X86_INSTR_JNS:
	case X86_INSTR_JB:
	case X86_INSTR_JBE:
	case X86_INSTR_JO:
	case X86_INSTR_JNO:
	case X86_INSTR_JP:
	case X86_INSTR_JNP:
		JLU(cpu);
		break;

	case X86_INSTR_HLT:
		cpu->hlt = true;
		return X86_ERROR_HLT;

	case X86_INSTR_REP_MOVSD:
		copyFromToX86CPUMemory(cpu, cpu->registers[X86_REG_ESI], cpu->registers[X86_REG_EDI], cpu->registers[X86_REG_ECX] * 4);
		break;

	case X86_INSTR_REP_STOSD:
		void* src = getX86CPUMemoryBlock(cpu, cpu->registers[X86_REG_EDI], cpu->registers[X86_REG_ECX] * 4);
		if (src != NULL)
			memset(src, cpu->registers[X86_REG_EAX], cpu->registers[X86_REG_ECX] * 4);
		break;
	}

	return X86_ERROR_SUCCESS;
}

int ALU(X86_CPU* cpu, X86_INSTR_TYPE type, uint32_t operand1, uint32_t operand2, uint32_t* result)
{	
	switch (type) {
		case X86_INSTR_ADD:
			*result = (operand1 + operand2);
			break;
		case X86_INSTR_SUB:
			*result = (operand1 - operand2);
			break;
		case X86_INSTR_DIV:
			*result = (operand1 / operand2);
			break;
		case X86_INSTR_MUL:
			*result = (operand1 * operand2);
			break;
		case X86_INSTR_XOR:
			*result = (operand1 ^ operand2);
			break;
		case X86_INSTR_OR:
			*result = (operand1 | operand2);
			break;
		case X86_INSTR_AND:
			*result = (operand1 & operand2);
			break;
		case X86_INSTR_SHR:
			*result = (operand1 >> operand2);
			break;
		case X86_INSTR_SHL:
			*result = (operand1 << operand2);
			break;
		case X86_INSTR_DEC:
			*result = (operand1 - 1);
			break;
		case X86_INSTR_INC:
			*result = (operand1 + 1);
			break;
		
		case X86_INSTR_CMP:
			ALU_setflags(&cpu->eflags, operand1, operand2, (operand1 - operand2));
			return 0;

		default:
			return 0;
	}
	
	ALU_setflags(&cpu->eflags, operand1, operand2, *result);

	return 0;
}
int ALU_setflags(X86_EFLAGS* eflags, uint32_t operand1, uint32_t operand2, uint32_t result)
{
	#define bitset(flags, n) ((flags & (1 << (n))) != 0)

	// overflow flag; compare sign bit
	eflags->OF = ((bitset(operand1, 31) == bitset(operand2, 31)) && (bitset(operand1, 31) != bitset(result, 31)));

	// zero flag
	eflags->ZF = (result == 0);

	// sign flag
	eflags->SF = bitset(result, 32 - 1);

	// carry flag
	eflags->CF = eflags->OF;

	return 0;
}

int JLU(X86_CPU* cpu)
{
	X86_INSTR_TYPE type = cpu->instruction.map->type;
	int operand = 99999;

	if (type == X86_INSTR_JMP_REG) {
		cpu->eip = cpu->registers[cpu->instruction.map->reg];
		return 0;
	}

	if (type == X86_INSTR_JMP_DIRECT) {
		cpu->eip = cpu->instruction.operand1;
		return 0;
	}

	switch (cpu->instruction.map->operand1_len)	{
		case 1:
			operand = (char)(cpu->instruction.operand1);
			break;
		case 2:
			operand = (short)cpu->instruction.operand1;
			break;
		case 4:
			operand = (int)cpu->instruction.operand1;
			break;
	}

	switch (type) {
		case X86_INSTR_JMP_INDIRECT:
			cpu->eip += operand;
			break;
		case X86_INSTR_JE: // if equal
			if (cpu->eflags.ZF) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JNE: // if not equal
			if (cpu->eflags.ZF == false) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JG: // if greater
			if (cpu->eflags.SF == cpu->eflags.OF && cpu->eflags.ZF == false) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JGE: // if greater or equal
			if (cpu->eflags.SF == cpu->eflags.OF || cpu->eflags.ZF == false) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JA: // if above (unsigned comparison)
			if (cpu->eflags.CF == false && cpu->eflags.ZF == false) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JAE: // if above or equal (unsigned comparison)
			if (cpu->eflags.CF == false || cpu->eflags.ZF) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JL: // if lesser
			if (cpu->eflags.SF != cpu->eflags.OF) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JLE: // if lesser or equal
			if (cpu->eflags.SF != cpu->eflags.OF || cpu->eflags.ZF) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JS: // if signed
			if (cpu->eflags.SF) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JNS: // if not signed
			if (cpu->eflags.SF == false) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JB: // if below
			if (cpu->eflags.CF) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JBE: // if below or equal
			if (cpu->eflags.CF || cpu->eflags.ZF) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JO:
			if (cpu->eflags.OF) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JNO:
			if (cpu->eflags.OF == false) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JP:
			if (cpu->eflags.PF) {
				cpu->eip += operand;
			}
			break;
		case X86_INSTR_JNP:
			if (cpu->eflags.PF == false) {
				cpu->eip += operand;
			}
			break;
	}

	return 0;
}

int generateX86InstructionTree(uint8_t* data, uint32_t size, X86_INSTR** instructionTree, uint32_t* instructionTreeSize)
{
	const uint32_t ALLOC_SIZE = sizeof(X86_INSTR) * 10;
	const uint8_t zero_mem[MAX_INSTR_SIZE] = { 0 };

	int result;
	uint32_t offset;
	uint32_t treeSize;
	uint32_t treeIndex;
	X86_INSTR* tree;

	tree = (X86_INSTR*)malloc(ALLOC_SIZE);
	if (tree == NULL)
		return ERROR_OUT_OF_MEMORY;
	treeSize = ALLOC_SIZE;
	
	treeIndex = 0;
	result = 1;

	for (offset = 0; offset < size - 1;) {
		if (size > MAX_INSTR_SIZE && memcmp((data + offset), zero_mem, (offset < size - MAX_INSTR_SIZE ? MAX_INSTR_SIZE : size - offset)) == 0)
			break;

		if (treeIndex >= treeSize / sizeof(X86_INSTR)) {
			X86_INSTR* newTree = (X86_INSTR*)realloc(tree, treeSize + ALLOC_SIZE);
			if (newTree == NULL) {
				if (tree != NULL) {
					free(tree);
					tree = NULL;
				}

				return ERROR_OUT_OF_MEMORY;
			}

			tree = newTree;
			treeSize += ALLOC_SIZE;
		}

		result = getX86Instruction(data + offset, size - offset, &tree[treeIndex]);
		if (result != 0) {
			offset++;
			result = 0;
			break;
		}

		offset += (tree[treeIndex].map->opcode_len + tree[treeIndex].map->operand1_len + tree[treeIndex].map->operand2_len);
		treeIndex++;
	}

	if (instructionTree != NULL) {
		*instructionTree = tree;
	}

	if (instructionTreeSize != NULL) {
		*instructionTreeSize = treeIndex * sizeof(X86_INSTR);
	}

	return result;
}
