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

	_ptr = (XCODE*)((UCHAR*)_data + _offset);
	
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

int XcodeInterp::isOpcodeValid(UCHAR opcode)
{
	return (opcode >= XC_MEM_READ && opcode <= XC_EXIT) == 0;
}

int XcodeInterp::getOpcodeStr(char* str)
{
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

void XcodeInterp::reset()
{
	_offset = 0;
	_status = INTERP_STATUS::UNK;
	_ptr = NULL;
}

int XcodeInterp::decodeXcode(XCODE*& xcode, std::list<LABEL>& labels)
{
	const UINT BASE_OFFSET = sizeof(INIT_TBL);

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

	// print the xcode
	print("%s %04x: %s %s %s %s\n", &str_label, (BASE_OFFSET + offset), &str_opcode, &str_addr, &str_data, &str_comment);

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

		// if the xcode is a jmp, then calculate the offset and set the data to the offset.
		if (xcode->opcode == XC_JMP || xcode->opcode == XC_JNE)
		{
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
	}

	if (_status == XcodeInterp::DATA_ERROR)
	{
		error("Error: Failed to decode xcodes\n");
		return 1;
	}

	print("xcodes: %d ( %ld bytes )\n", _offset / sizeof(XCODE), _offset);

	// decode phase 2
		
	reset(); // reset the interpreter to the start of the xcodes

	while (interpretNext(xcode) == 0)
	{
		decodeXcode(xcode, labels);		
	}

	labels.clear();

	return 0;
}

int XcodeInterp::simulateXcodes()
{
	// load the inittbl file

	// 6 byte encoding
	const USHORT MOV_PTR_INSTRS_2_BYTE[] = { 0x1D8B,		0x0D8B,		0x158B };
	const char* MOV_PTR_NAMES_2_BYTE[] = { "mov ebx",	"mov ecx",	"mov edx" };

	// 5 byte encoding
	const UCHAR MOV_INSTRS[] = { 0xB8,		0xB9,		0xBA,		0xBB,		0xBC,		0xBD,		0xBE,		0xBF };
	const char* MOV_NAMES[] = { "mov eax", "mov ecx",	"mov edx",	"mov ebx",	"mov esp",	"mov ebp",	"mov esi",	"mov edi" };

	// 5 byte encoding
	const UCHAR MOV_EAX_PTR_INSTR = 0xA1;
	const char* MOV_EAX_PTR_NAME = "mov eax";

	// 2 byte encoding
	const USHORT JMP_INSTRS[] = { 0xE0FF,		0xE1FF,		0xE2FF,		0xE3FF,		0xE4FF,		0xE5FF,		0xE6FF,		0xE7FF };
	const char* JMP_NAMES[] = { "jmp eax",	"jmp ecx",	"jmp edx",	"jmp ebx",	"jmp esp",	"jmp ebp",	"jmp esi",	"jmp edi" };

	// 2 byte encoding
	const USHORT REP_INSTRS[] = { 0xA5F3 };
	const char* REP_NAMES[] = { "rep movsd" };

	// 1 byte encoding
	const UCHAR FAR_JMP_INSTR = 0xEA;
	const char* FAR_JMP_NAME = "jmp far";

	// 1 byte encoding
	const UCHAR NOP_INSTR = 0x90;
	const char* NOP_NAME = "nop";

	// 1 byte encoding
	const UCHAR CLD_INSTR = 0xFC;
	const char* CLD_NAME = "cld";

	const UINT DEFAULT_SIM_SIZE = 128; // 32KB
	const UINT SIM_SIZE = (params.simSize > 0) ? params.simSize : DEFAULT_SIM_SIZE;

	const UINT MAX_INSTR_SIZE = 6;
	const UCHAR zero_mem[MAX_INSTR_SIZE] = { 0 };
	char str_opcode[16] = { 0 };

	int result = 0;

	bool hasMemChanges_total = false;	
	bool found = false;
	bool unkInstrs = false;

	UCHAR* mem_sim = NULL;
	XCODE* xcode = NULL;
	

	UINT i, j, offset, dumpTo;
	
	print("mem space: %d bytes\n\n", SIM_SIZE);

	mem_sim = (UCHAR*)xb_alloc(SIM_SIZE);
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
		// skip this xcode. only care about xcodes that write to RAM
		if (xcode->opcode != XC_MEM_WRITE)
			continue;

		// sanity check of addr.
		if (xcode->addr < 0 || xcode->addr >= SIM_SIZE)
			continue;

		hasMemChanges_total = true;

		// write the data to simulated memory
		xb_cpy(mem_sim + xcode->addr, (UCHAR*)&xcode->data, 4);

		offset = _offset + sizeof(INIT_TBL) - sizeof(XCODE);

		// print the xcode
		xb_zero(str_opcode, sizeof(str_opcode));
		getOpcodeStr(str_opcode);

		print("%04x: %s 0x%02x, 0x%08X\n", offset, str_opcode, xcode->addr, xcode->data);
	}

	if (_status != XcodeInterp::EXIT_OP_FOUND)
	{
		result = 1;
		goto Cleanup;
	}

	if (!hasMemChanges_total)
	{
		print("No Memory changes in range 0x0 - 0x%x\n", SIM_SIZE);
		goto Cleanup;
	}

	// after successful simulation, look for some x86 32-bit instructions.

	print("\nx86 32-bit instructions:\n");

	// IMPORTANT: sort by opcode len (2, 1); to prevent a 1 byte instru from being parsed as a 2 byte instru.
	for (i = 0; i < SIM_SIZE;)
	{
		// check if the next [max_instr_size] bytes are zero, if so we are done.
		if (xb_cmp(mem_sim + i, zero_mem, (i < SIM_SIZE - MAX_INSTR_SIZE ? MAX_INSTR_SIZE : SIM_SIZE - i)) == 0)
			break;

		found = false;

		// check for 2 byte mov ptr instructions
		for (j = 0; j < sizeof(MOV_PTR_INSTRS_2_BYTE) / sizeof(USHORT); j++)
		{
			if (xb_cmp(mem_sim + i, MOV_PTR_INSTRS_2_BYTE + j, 2) == 0)
			{
				print("%04x: %s, [0x%08x]\n", i, MOV_PTR_NAMES_2_BYTE[j], *((UINT*)(mem_sim + i + 2)));
				i += 6; // 2 byte opcode + 4 byte data
				found = true;
				break;
			}
		}
		if (found)
			continue;

		// check for 2 byte jmp instructions
		for (j = 0; j < sizeof(JMP_INSTRS) / sizeof(USHORT); j++)
		{
			if (xb_cmp(mem_sim + i, JMP_INSTRS + j, 2) == 0)
			{
				print("%04x: %s\n", i, JMP_NAMES[j]);
				i += 2; // 2 byte opcode
				found = true;
				break;
			}
		}
		if (found)
			continue;

		// check for 2 byte rep instructions
		for (j = 0; j < sizeof(REP_INSTRS) / sizeof(USHORT); j++)
		{
			if (xb_cmp(mem_sim + i, REP_INSTRS + j, 2) == 0)
			{
				print("%04x: %s\n", i, REP_NAMES[j]);
				i += 2; // 2 byte opcode
				found = true;
				break;
			}
		}
		if (found)
			continue;

		// check for 1 byte far jmp instructions
		if (mem_sim[i] == FAR_JMP_INSTR)
		{
			print("%04x: %s 0x%x:0x%08x\n", i, FAR_JMP_NAME, *((USHORT*)(mem_sim + i + 5)), *((UINT*)(mem_sim + i + 1)));
			i += 6; // 1 byte opcode + 2 byte for seg + 4 byte for offset
			continue;
		}

		// check for 1 byte mov ptr instructions
		if (mem_sim[i] == MOV_EAX_PTR_INSTR)
		{
			print("%04x: %s, [0x%8x]\n", i, MOV_EAX_PTR_NAME, *((UINT*)(mem_sim + i + 1)));
			i += 5; // 1 byte opcode + 4 byte data
			continue;
		}

		// check for 1 byte mov instructions
		for (j = 0; j < sizeof(MOV_INSTRS); j++)
		{
			if (xb_cmp(mem_sim + i, MOV_INSTRS + j, 1) == 0)
			{
				print("%04x: %s, 0x%x\n", i, MOV_NAMES[j], *((UINT*)(mem_sim + i + 1)));
				i += 5; // 1 byte opcode + 4 byte data
				found = true;
				break;
			}
		}
		if (found)
			continue;

		if (mem_sim[i] == NOP_INSTR)
		{
			print("%04x: %s\n", i, NOP_NAME);
			i++; // 1 byte opcode
			continue;
		}

		if (mem_sim[i] == CLD_INSTR)
		{
			print("%04x: %s\n", i, CLD_NAME);
			i++; // 1 byte opcode
			continue;
		}

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

		print("\nWriting memory dump to %s (%d bytes)\n", filename, SIM_SIZE);
		result = writeFile(filename, mem_sim, SIM_SIZE);
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
