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
#include <cstring>

// user incl
#include "XcodeInterp.h"
#include "XbTool.h"
#include "file.h"
#include "util.h"

#define XC_WRITE_COMMENT(xc_cmd, xc_addr, xc_data, comment) \
{ \
	if ((_ptr->opcode == xc_cmd) && (_ptr->addr == xc_addr) && (xc_data == NULL ? true : (xc_data == _ptr->data))) \
	{ \
		strcat(str, comment); \
		return 0; \
	} \
}

extern Parameters& params;

int XcodeInterp::load(UCHAR* data, UINT size)
{
	if (_data != NULL)
	{
		error("Error: XCODE data is already loaded\n");
		return 1;
	}

	_data = (UCHAR*)xb_alloc(size);
	if (_data == NULL)
	{
		return 1;
	}

	xb_cpy(_data, data, size);

	_size = size;

	reset();

	return 0;
}

void XcodeInterp::unload()
{
	_size = 0;
	reset();

	if (_data != NULL)
	{
		xb_free(_data);
		_data = NULL;
	}
}

void XcodeInterp::reset()
{
	_offset = 0;
	_status = INTERP_STATUS::UNK;
	_ptr = (XCODE*)_data;
}

int XcodeInterp::interpretNext(XCODE*& xcode)
{
	if (_status == INTERP_STATUS::EXIT_OP_FOUND) // last xcode was an exit. dont interpret anymore xcodes after this
		return 1;

	if (_offset + sizeof(XCODE) > _size) // check if offset is within bounds
	{
		_status = INTERP_STATUS::DATA_ERROR;
		error("Error: EXIT opcode '%X' not found. End of data reached\n", XC_EXIT);
		return 1;
	}

	_ptr = (XCODE*)(_data + _offset);
	
	xcode = _ptr;

	if (xcode->opcode == XC_EXIT)
	{
		_status = INTERP_STATUS::EXIT_OP_FOUND;
	}
	else
	{
		_status = INTERP_STATUS::DATA_OK;
	}
	
	_offset += sizeof(XCODE);

	return 0;
}

const char* INTERP_ERROR_XCODE_PTR_NULL = "Error: XCODE pointer is NULL\n";

int XcodeInterp::getOpcodeStr(char* str)
{
	if (_ptr == NULL)
	{
		error(INTERP_ERROR_XCODE_PTR_NULL);
		return 1;
	}

	const UCHAR opcode = _ptr->opcode;

	switch (opcode)
	{
	case XC_MEM_READ:
		strcpy(str, "mem_read");
		break;
	case XC_MEM_WRITE:
		strcpy(str, "mem_write");
		break;
	case XC_PCI_WRITE:
		strcpy(str, "pci_write");
		break;
	case XC_PCI_READ:
		strcpy(str, "pci_read");
		break;
	case XC_AND_OR:
		strcpy(str, "and_or");
		break;
	case XC_USE_RESULT:
		strcpy(str, "use_rslt");
		break;						
	case XC_JNE:
		strcpy(str, "jne");
		break;
	case XC_JMP:
		strcpy(str, "jmp");
		break;
	case XC_ACCUM:
		strcpy(str, "accum");
		break;
	case XC_IO_WRITE:
		strcpy(str, "io_write");
		break;
	case XC_IO_READ:
		strcpy(str, "io_read");
		break;
	case XC_EXIT:
		strcpy(str, "exit");
		break;

	case XC_NOP:
	default:
		format(str, "nop_%X", opcode);
		break;
	}

	return 0;
}
int XcodeInterp::getAddressStr(char* str)
{
	if (_ptr == NULL)
	{
		error(INTERP_ERROR_XCODE_PTR_NULL);
		return 1;
	}

	const UCHAR opcode = _ptr->opcode;

	// opcodes that dont use addr field
	switch (opcode)
	{
		case XC_JMP:
			str[0] = '\0';
			return 1;
	}

	format(str, "0x%X", _ptr->addr);

	return 0;
}
int XcodeInterp::getDataStr(char* str)
{
	if (_ptr == NULL)
	{
		error(INTERP_ERROR_XCODE_PTR_NULL);
		return 1;
	}

	switch (_ptr->opcode)
	{
		// opcodes that dont use data field
		case XC_MEM_READ:
		case XC_IO_READ:
		case XC_PCI_READ:
		case XC_EXIT:
			str[0] = '\0';
			break;

		case XC_JNE:
		case XC_JMP:
			break;

		default:
			format(str, "0x%X", _ptr->data);
			break;
	}

	return 0;
}
int XcodeInterp::getCommentStr(char* str)
{
	if (_ptr == NULL)
	{
		error(INTERP_ERROR_XCODE_PTR_NULL);
		return 1;
	}

	XC_WRITE_COMMENT(XC_JNE, 0x10, 0xFFFFFFEE, "\t ; spin until smbus is ready");

	XC_WRITE_COMMENT(XC_IO_READ, SMB_BASE + 0x00, NULL, "smbus read status");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x00, 0x10, "smbus clear status");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x08, NULL, "smbus set cmd");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x06, NULL, "smbus set val");

	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x02, 0x0A, "smbus kickoff");

	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x04, 0x20, "smc slave write addr");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x04, 0x21, "smc slave read addr");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x08, 0x01, "smc read revision register");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x04, 0x8A, "871 slave addr");

	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x80000884, 0x8001, "setup io bar (rev >= C03)");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x80000810, 0x8001, "setup io bar (rev < C03)");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x80000804, 0x03, "enable io space");
	XC_WRITE_COMMENT(XC_IO_WRITE, 0x8049, 0x08, "disable the tco timer");

	XC_WRITE_COMMENT(XC_IO_WRITE, 0x80D9, 0x00, "KBDRSTIN# in gpio mode");
	XC_WRITE_COMMENT(XC_IO_WRITE, 0x8026, 0x01, "disable PWRBTN#");

	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000F04C, 0x01, "enable internal graphics");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000F018, 0x10100, "setup secondary bus 1");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000036C, 0x1000000, "smbus is bad, flatline clks");

	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x80010010, NV2A_BASE, "set NV20 register base");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000F020, 0xFDF0FD00, "reload NV20 register base");

	XC_WRITE_COMMENT(XC_MEM_READ, NV2A_BASE, NULL, "PCM_BOOT_0");

	XC_WRITE_COMMENT(XC_MEM_WRITE, NV2A_BASE+0x101000, 0x803D4401, "PEXTDEV_BOOT_0");
	XC_WRITE_COMMENT(XC_MEM_READ, NV2A_BASE + 0x101000, NULL, "PEXTDEV_BOOT_0");

	XC_WRITE_COMMENT(XC_MEM_WRITE, NV2A_BASE + 0x1214, 0x28282828, "configure for micron");
	XC_WRITE_COMMENT(XC_MEM_WRITE, NV2A_BASE + 0x1214, 0x09090909, "configure for samsung");

	XC_WRITE_COMMENT(XC_EXIT, 0x806, NULL, "end of xcodes");

	return 0;
}

int XcodeInterp::decodeXcode(FILE* stream, XCODE*& xcode, std::list<LABEL>& labels) // decode xcode and print to stream
{
	bool found = false;
	UINT len = 0;
	UINT offset = _offset - sizeof(XCODE);

	char str_label[10 + 1] = { 0 };
	char str_opcode[10 + 1] = { 0 };
	char str_addr[10 + 1] = { 0 };
	char str_data[10 + 1] = { 0 };
	char str_comment[64] = { 0 };

	// check if offset should be a label
	for (LABEL& label : labels)
	{
		if (label.offset == offset)
		{
			format(str_label, "%s:", label.name);
			len = strlen(str_label);
			break;
		}
	}
	xb_set(str_label + len, ' ', sizeof(str_label) - len - 1);

	// get the opcode string
	getOpcodeStr(str_opcode);
	len = strlen(str_opcode);
	xb_set(str_opcode + len, ' ', sizeof(str_opcode) - len - 1);

	// get the address string
	getAddressStr(str_addr);
	len = strlen(str_addr);
	xb_set(str_addr + len, ' ', sizeof(str_addr) - len - 1);

	// get the data string
	switch (xcode->opcode)
	{
	case XC_JMP:
	case XC_JNE:
		found = false;
		for (LABEL& label : labels)
		{
			if (label.offset == xcode->data)
			{
				found = true;
				if (xcode->opcode == XC_JNE)
				{
					format(str_data, "%s:", label.name);
				}
				else
				{
					// jmp does not use addr, so set it to the label name
					format(str_addr, "%s:", label.name);
				}
				break;
			}
		}
		if (!found)
		{
			error("Error: Label not found for offset %04x\n", xcode->data);
		}
		break;

	default:
		getDataStr(str_data);
		break;
	}

	// format the data string
	len = strlen(str_data);
	xb_set(str_data + len, ' ', sizeof(str_data) - len - 1);

	// get the comment string
	xb_zero(str_comment, sizeof(str_comment));
	getCommentStr(str_comment + 2);
	if (str_comment[2] != '\0')
	{
		xb_cpy(str_comment, "; ", 2);
	}

	if (stream == NULL)
		return 1;

	// print the xcode to the stream
	print(stream, "%s %04x: %s %s %s %s\n", &str_label, (XCODE_BASE + offset), &str_opcode, &str_addr, &str_data, &str_comment);

	return 0;
}

int XcodeInterp::decodeXcodes()
{
	XCODE* xcode;
	std::list<LABEL> labels;

	// decode phase 1
	while (SUCCESS(interpretNext(xcode)))
	{
		// go through the xcodes and fix up jmp offsets.

		if (xcode->opcode != XC_JMP && xcode->opcode != XC_JNE)
			continue;

		// calculate the offset and set the data to the offset.

		bool isLabel = false;
		UINT labelOffset = _offset + xcode->data;

		// Fix up the xcode data to the offset for phase 2 decoding
		xcode->data = labelOffset;

		// check if offset is already a label
		for (LABEL& label : labels)
		{
			if (label.offset == labelOffset)
			{
				isLabel = true;
				break;
			}
		}

		// label does not exist. create one.
		if (!isLabel)
		{
			char str[16] = { 0 };
			format(str, "lb_%02d", labels.size());

			labels.push_back(LABEL(labelOffset, str));
		}
	}

	if (_status == XcodeInterp::DATA_ERROR)
	{
		error("Error: Failed to decode xcodes\n");
		return 1;
	}
	
	// decode phase 2
	
	FILE* stream = stdout;

	// if -d flag is set, dump the xcodes to a file, otherwise print the xcodes
	if ((params.sw_flag & SW_DMP) != 0)
	{
		const char* filename = params.outFile;
		if (filename == NULL)
		{
			filename = "xcodes.txt";
		}

		// del file if it exists
		//if (fileExists(filename))
		{
			deleteFile(filename);
		}

		print("Writing decoded xcodes to %s (text format)\n", filename, _offset);
		stream = fopen(filename, "w");
		if (stream == NULL)
		{
			error("Error: Failed to open file %s\n", filename);
			return 1;
		}
	}

	print(stream, "xcodes: %d ( %ld bytes )\n", _offset / sizeof(XCODE), _offset);

	// print the xcodes to the stream.
	reset();
	while (interpretNext(xcode) == 0)
	{
		decodeXcode(stream, xcode, labels);
	}

	if ((params.sw_flag & SW_DMP) != 0)
	{
		fclose(stream);
		print("Done\n");
	}

	labels.clear();

	return 0;
}

int XcodeInterp::simulateXcodes()
{
	// load the inittbl file

	typedef struct X86_INSTR
	{
		USHORT opcode;
		const char* asm_instr;
		UINT opcode_len;
		UINT instr_len;
		bool uses_operand = false;
		const char* data_str = NULL;
	} X86_INSTR;

	const char* PTR_DATA_STR = ", [0x%08x]";
	const char* NUM_DATA_STR = ", 0x%08x";

	const X86_INSTR instrs[] = {
		{ 0x1D8B, "mov ebx", 2, 6, true, PTR_DATA_STR },
		{ 0x0D8B, "mov ecx", 2, 6, true, PTR_DATA_STR },
		{ 0x158B, "mov edx", 2, 6, true, PTR_DATA_STR },

		{ 0xE0FF, "jmp eax", 2, 2 },
		{ 0xE1FF, "jmp ecx", 2, 2 },
		{ 0xE2FF, "jmp edx", 2, 2 },
		{ 0xE3FF, "jmp ebx", 2, 2 },
		{ 0xE4FF, "jmp esp", 2, 2 },
		{ 0xE5FF, "jmp ebp", 2, 2 },
		{ 0xE6FF, "jmp esi", 2, 2 },
		{ 0xE7FF, "jmp edi", 2, 2 },

		{ 0xA5F3, "rep movsd", 2, 2 },

		{ 0xB8, "mov eax", 1, 5, true },
		{ 0xB9, "mov ecx", 1, 5, true },
		{ 0xBA, "mov edx", 1, 5, true },
		{ 0xBB, "mov ebx", 1, 5, true },
		{ 0xBC, "mov esp", 1, 5, true },
		{ 0xBD, "mov ebp", 1, 5, true },
		{ 0xBE, "mov esi", 1, 5, true },
		{ 0xBF, "mov edi", 1, 5, true },
		{ 0xA1, "mov eax", 1, 5, true, PTR_DATA_STR },

		{ 0xEA, "jmp far", 1, 6, true },
		{ 0x90, "nop", 1, 1 },
		{ 0xFC, "cld", 1, 1 }
	};

	const UINT memSize = (params.simSize > 0) ? params.simSize : 32;
	const UINT MAX_INSTR_SIZE = 6;

	const UCHAR zero_mem[MAX_INSTR_SIZE] = { 0 };
	char str_opcode[16] = { 0 };
	char str_operand[16] = { 0 };
	char str_instr[32] = { 0 };

	UINT operand = 0;
	UINT operandLen = 0;

	int result = 0;

	bool hasMemChanges_total = false;	
	bool found = false;
	bool unkInstrs = false;

	UCHAR* mem_sim = NULL;
	XCODE* xcode = NULL;
	

	UINT i, j, dumpTo;
	
	print("mem space: %d bytes\n\n", memSize);

	mem_sim = (UCHAR*)xb_alloc(memSize);
	if (mem_sim == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// simulate memory output
	hasMemChanges_total = false;
	print("xcode sim:\n");
	while (SUCCESS(interpretNext(xcode)))
	{
		// only care about xcodes that write to RAM
		if (xcode->opcode != XC_MEM_WRITE)
			continue;

		// sanity check of addr.
		if (xcode->addr < 0 || xcode->addr >= memSize)
		{
			continue;
		}
		hasMemChanges_total = true;

		// write the data to simulated memory
		xb_cpy(mem_sim + xcode->addr, (UCHAR*)&xcode->data, 4);

		// print the xcode
		xb_zero(str_opcode, sizeof(str_opcode));
		getOpcodeStr(str_opcode);

		print("%04x: %s 0x%02x, 0x%08X\n", (XCODE_BASE + _offset - sizeof(XCODE)), str_opcode, xcode->addr, xcode->data);
	}

	if (_status != XcodeInterp::EXIT_OP_FOUND)
	{
		result = 1;
		goto Cleanup;
	}

	if (!hasMemChanges_total)
	{
		print("No Memory changes in range 0x0 - 0x%x\n", memSize);
		goto Cleanup;
	}

	// after successful simulation, look for some x86 32-bit instructions.

	print("\nx86 32-bit instructions:\n");

	// IMPORTANT: sort by opcode len (2, 1); to prevent a 1 byte instru from being parsed as a 2 byte instru.
	for (i = 0; i < memSize;)
	{
		// check if the next [max_instr_size] bytes are zero, if so we are done.
		if (xb_cmp(mem_sim + i, zero_mem, (i < memSize - MAX_INSTR_SIZE ? MAX_INSTR_SIZE : memSize - i)) == 0)
			break;

		// iterate through the x86 instructions
		for (j = 0; j < sizeof(instrs) / sizeof(X86_INSTR); j++)
		{
			if (xb_cmp(mem_sim + i, (UCHAR*)&instrs[j].opcode, instrs[j].opcode_len) == 0)
			{
				xb_zero(str_instr, sizeof(str_instr));
				format(str_instr, "%04x: %s", i, instrs[j].asm_instr);

				if (instrs[j].uses_operand)
				{
					operand = 0;
					operandLen = instrs[j].instr_len - instrs[j].opcode_len;
					if (operandLen > 4) // UINT is 4 bytes
						operandLen = 4;

					// to-do: fix jmp far operand. currently only supports 4 byte operand single operand instructions.

					xb_cpy(&operand, (UCHAR*)(mem_sim + i + instrs[j].opcode_len), operandLen);
					format(str_operand, instrs[j].data_str == NULL ? NUM_DATA_STR : instrs[j].data_str, operand);
					strcat(str_instr, str_operand);
				}
				strcat(str_instr, "\n");

				print(str_instr);

				i += instrs[j].instr_len;
				found = true;
				break;
			}
		}
		if (found)
			continue;

		// opcode not found
		error("Error: Unknown instruction at offset %04x, INSTR: %02X\n", i, mem_sim[i]);
		i++;
		unkInstrs = true;
	}
	dumpTo = i; // save the offset to dump to

	print("size of code: %d bytes\n", dumpTo);

	// if -d flag is set, dump the memory to a file, otherwise print the memory dump
	if ((params.sw_flag & SW_DMP) != 0)
	{
		const char* filename = params.outFile;
		if (filename == NULL)
		{
			filename = "mem_sim.bin";
		}

		print("\nWriting memory dump to %s (%d bytes)\n", filename, memSize);
		result = writeFile(filename, mem_sim, memSize);
		if (!SUCCESS(result))
		{
			error("Error: Failed to write memory dump to %s\n", filename);
		}
	}
	else
	{
		// print memory dump
		print("\nmem dump:\n");
		for (i = 0; i < dumpTo; i += 8)
		{
			print("%04x: ", i);
			printData(mem_sim + i, 8);
		}

		if (unkInstrs)
		{
			print("\nUnknown instructions found. Dump to file ( -d ) to decompile with a x86 32-bit decompiler.\n" \
				"Only a handful of opcodes have been implemented. Better off " \
				"dumping to a file and analyzing with a x86 32-bit compatible decompiler.\n");
		}
	}

Cleanup:

	if (mem_sim != NULL)
	{
		xb_free(mem_sim);
	}

	return result;
}
