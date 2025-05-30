// XcodeInterp.cpp: defines functions for interpreting XCODE instructions

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
#include <cstdint>
#include <memory.h>
#if !__APPLE__
#include <malloc.h>
#else
#include <cstdlib>
#endif

// user incl
#include "XcodeInterp.h"
#include "str_util.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#endif

const FIELD_MAP xcode_opcode_map[] = {
	{ "xc_reserved",	XC_RESERVED },
	{ "xc_mem_read",	XC_MEM_READ },
	{ "xc_mem_write",	XC_MEM_WRITE },
	{ "xc_pci_write",	XC_PCI_WRITE },
	{ "xc_pci_read",	XC_PCI_READ },
	{ "xc_and_or",		XC_AND_OR },
	{ "xc_result",      XC_USE_RESULT },
	{ "xc_jne",			XC_JNE },
	{ "xc_jmp",			XC_JMP },
	{ "xc_accum",		XC_ACCUM },
	{ "xc_io_write",	XC_IO_WRITE },
	{ "xc_io_read",		XC_IO_READ },
	{ "xc_nop_f5",		XC_NOP_F5 },
	{ "xc_exit",		XC_EXIT },
	{ "xc_nop_80",		XC_NOP_80 }
};

int XcodeInterp::load(uint8_t* in_data, uint32_t in_size) {
	if (data != NULL) {
		return 1;
	}

	data = (uint8_t*)malloc(in_size);
	if (data == NULL) {
		return XC_INTERP_ERROR_OUT_OF_MEMORY;
	}

	memcpy(data, in_data, in_size);
	size = in_size;
	
	reset();

	return 0;
}
void XcodeInterp::reset() {
	offset = 0;
	status = DATA_OK;
	ptr = (XCODE*)data;
}
void XcodeInterp::unload() {
	if (data != NULL) {
		free(data);
		data = NULL;
	}
}
int XcodeInterp::interpretNext(XCODE*& xcode) {
	if (data == NULL) {
		status = INTERP_STATUS::DATA_ERROR;
		return XC_INTERP_ERROR_INVALID_DATA;
	}

	// last xcode was an exit. dont interpret anymore xcodes after this
	if (status == INTERP_STATUS::EXIT_OP_FOUND) 
		return 1;

	// check if offset is within bounds
	if (offset + sizeof(XCODE) > size) {
		status = INTERP_STATUS::DATA_ERROR;		
		return 1;
	}

	ptr = (XCODE*)(data + offset);	
	xcode = ptr;

	if (xcode->opcode == XC_EXIT) {
		status = INTERP_STATUS::EXIT_OP_FOUND;
	}
	else {
		status = INTERP_STATUS::DATA_OK;
	}
	
	offset += sizeof(XCODE);

	return 0;
}

int encodeX86AsMemWrites(uint8_t* data, uint32_t size, uint32_t base, uint8_t*& buffer, uint32_t* xcodeSize) {
	const uint32_t allocSize = sizeof(XCODE) * 10;

	buffer = (uint8_t*)malloc(allocSize);
	if (buffer == NULL)
		return XC_INTERP_ERROR_OUT_OF_MEMORY;

	uint32_t nSize = allocSize;
	uint32_t offset = 0;
	XCODE xcode = { XC_MEM_WRITE, 0, 0 };
	uint32_t xc_d;
	
	for (uint32_t i = 0; i < size; i+=4) {
		if (i + 4 > size) {
			xc_d = 0;
			memcpy(&xc_d, data + i, size - i);
		}
		else {
			xc_d = *(uint32_t*)(data + i);
		}

		xcode.addr = base + i;
		xcode.data = xc_d;

		if (offset + sizeof(XCODE) > nSize) {
			uint8_t* new_buffer = (uint8_t*)realloc(buffer, nSize + allocSize);
			if (new_buffer == NULL) {
				return XC_INTERP_ERROR_OUT_OF_MEMORY;
			}
			buffer = new_buffer;
			nSize += allocSize;
		}

		memcpy(buffer + offset, &xcode, sizeof(XCODE));
		offset += sizeof(XCODE);
	}
	
	if (xcodeSize != NULL) {
		*xcodeSize = offset;
	}

	return 0;
}
int getOpcodeStr(const FIELD_MAP* opcodes, uint8_t opcode, const char*& str) {
	for (uint32_t i = 0; i < XC_OPCODE_COUNT; ++i) {
		if (opcodes[i].field == opcode) {
			str = opcodes[i].str;
			return 0;
		}
	}
	return 1;
}
