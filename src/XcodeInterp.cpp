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

// user incl
#include "XcodeInterp.h"
#include "file.h"
#include "util.h"

#define IS_NEG_TRUE(x, y) (y < 0 ? true : x == y)

#define XC_WRITE_MASK(i, xc_cmd, xc_addr, addr_mask, xc_data) (((UCHAR*)(_ptr+i)) >= _data) && ((_ptr+i)->opcode == xc_cmd && ((_ptr+i)->addr & addr_mask) == xc_addr && IS_NEG_TRUE((_ptr+i)->data, xc_data))
#define XC_WRITE(i, xc_cmd, xc_addr, xc_data) XC_WRITE_MASK(i, xc_cmd, xc_addr, 0xFFFFFFFF, xc_data)

// write comment base on current xcode and address mask.
#define XC_WRITE_COMMENT_MASK(xc_cmd, xc_addr, addr_mask, xc_data, comment) \
	if (XC_WRITE_MASK(0, xc_cmd, xc_addr, addr_mask, xc_data)) \
	{ \
		strcat(str, comment); \
		return 0; \
	}


// write comment base on current xcode.
#define XC_WRITE_STATEMENT(xc_cmd, xc_addr, xc_data, statement) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data)) \
	{ \
		statement; \
		return 0; \
	}

// write comment base on current xcode.
#define XC_WRITE_COMMENT(xc_cmd, xc_addr, xc_data, comment) XC_WRITE_STATEMENT(xc_cmd, xc_addr, xc_data, strcat(str, comment))

	// write comment base on current and previous xcode.
#define XC_WRITE_COMMENT_LAST_XCODE(xc_cmd, xc_addr, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_data, comment) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data) && \
		XC_WRITE(-1, xc_prev_cmd, xc_prev_addr, xc_prev_data)) \
		{ \
			strcat(str, comment); \
			return 0; \
		}

// write comment base on current and previous xcode.
#define XC_WRITE_COMMENT_MASK_LAST_XCODE(xc_cmd, xc_addr, xc_addr_mask, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data, comment) \
	if (XC_WRITE_MASK(0, xc_cmd, xc_addr, xc_addr_mask, xc_data) && \
		XC_WRITE_MASK(-1, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data)) \
		{ \
			strcat(str, comment); \
			return 0; \
		}

// write comment base on current and previous xcode.
#define XC_WRITE_COMMENT_LAST_2_XCODE(xc_cmd, xc_addr, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_data, xc_prev1_cmd, xc_prev1_addr, xc_prev1_data, comment) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data) && \
		XC_WRITE(-1, xc_prev_cmd, xc_prev_addr, xc_prev_data) && \
		XC_WRITE(-2, xc_prev1_cmd, xc_prev1_addr, xc_prev1_data)) \
		{ \
			strcat(str, comment); \
			return 0; \
		}

// write comment base on current and next xcode.
#define XC_WRITE_COMMENT_NEXT_XCODE(xc_cmd, xc_addr, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_data, comment) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data) && \
		XC_WRITE(1, xc_prev_cmd, xc_prev_addr, xc_prev_data)) \
		{ \
			strcat(str, comment); \
			return 0; \
		}


// write comment base on current and next xcode.
#define XC_WRITE_COMMENT_MASK_NEXT_XCODE(xc_cmd, xc_addr, xc_addr_mask, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data, comment) \
	if (XC_WRITE_COMMENT_MASK(0, xc_cmd, xc_addr, xc_addr_mask, xc_data) && \
		XC_WRITE_COMMENT_MASK(1, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data)) \
		{ \
			strcat(str, comment); \
			return 0; \
		}

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

void XcodeInterp::deconstruct()
{
	_size = 0;
	reset();

	if (_data != NULL)
	{
		xb_free(_data);
		_data = NULL;
	}

	for (int i = 0; i < (sizeof(opcodeMap) / sizeof(OPCODE_VERSION_INFO)); i++)
	{
		opcodeMap[i].deconstruct();
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
	if (_data == NULL)
	{
		_status = INTERP_STATUS::DATA_ERROR;
		return 1;
	}

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

int XcodeInterp::getDefOpcodeStr(const UCHAR opcode, char*& opcode_str) const
{
	for (int i = 0; i < sizeof(opcodeMap) / sizeof(OPCODE_VERSION_INFO); i++)
	{
		if (opcodeMap[i].opcode == opcode)
		{
			opcode_str = opcodeMap[i].str;
			return 0;
		}
	}
	opcode_str = NULL;
	return 1;
}

int XcodeInterp::getAddressStr(char* str, const char* format_str)
{
	if (_ptr == NULL)
	{
		_status = INTERP_STATUS::DATA_ERROR;
		return 1;
	}

	const UCHAR opcode = _ptr->opcode;
	char* opcode_str;

	// opcodes that dont use addr field
	switch (opcode)
	{
		case XC_JMP:
			str[0] = '\0';
			return 1;
		case XC_USE_RESULT:
			if (getDefOpcodeStr(_ptr->addr, opcode_str) == 0)
			{
				sprintf(str, "%s", opcode_str);
				break;
			}
			// fall through
		default:
		format(str, format_str, _ptr->addr);
			break;
	}


	return 0;
}
int XcodeInterp::getDataStr(char* str, const char* format_str)
{
	if (_ptr == NULL)
	{
		_status = INTERP_STATUS::DATA_ERROR;
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
			format(str, format_str, _ptr->data);
			break;
	}

	return 0;
}

int XcodeInterp::getCommentStr(char* str)
{
	if (_ptr == NULL)
	{
		_status = INTERP_STATUS::DATA_ERROR;
		return 1;
	}

	// -1 = dont care about field

	XC_WRITE_COMMENT(XC_IO_READ, SMB_BASE + 0x00, -1, "smbus read status");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x00, 0x10, "smbus clear status");

	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE+0x08, 0x01, "smbus read revision register");

	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE+0x08, -1, "smbus set cmd");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE+0x06, -1, "smbus set val");

	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x02, 0x0A, "smbus kickoff");

	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x04, 0x20, "smc slave write addr");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x04, 0x21, "smc slave read addr");

	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x04, 0x8A, "871 encoder slave addr");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x04, 0xD4, "focus encoder slave addr");
	XC_WRITE_COMMENT(XC_IO_WRITE, SMB_BASE + 0x04, 0xE1, "xcalibur encoder slave addr");

	XC_WRITE_COMMENT(XC_PCI_READ, MCPX_1_0_IO_BAR, -1, "read io bar (B02) MCPX v1.0");
	XC_WRITE_COMMENT(XC_PCI_WRITE, MCPX_1_0_IO_BAR, MCPX_IO_BAR_VAL, "set io bar (B02) MCPX v1.0");
	XC_WRITE_COMMENT_LAST_XCODE(XC_JNE, MCPX_IO_BAR_VAL, -1, XC_PCI_READ, MCPX_1_1_IO_BAR, -1, "jmp if (B02) MCPX v1.0");

	XC_WRITE_COMMENT(XC_PCI_READ, MCPX_1_1_IO_BAR, -1, "read io bar (C03) MCPX v1.1");
	XC_WRITE_COMMENT(XC_PCI_WRITE, MCPX_1_1_IO_BAR, MCPX_IO_BAR_VAL, "set io bar (C03) MCPX v1.1");
	XC_WRITE_COMMENT_LAST_XCODE(XC_JNE, MCPX_IO_BAR_VAL, -1, XC_PCI_READ, MCPX_1_0_IO_BAR, -1, "jmp if (C03) MCPX v1.1");
	
	XC_WRITE_COMMENT_LAST_XCODE(XC_JNE, 0x10, _offset-18U, XC_IO_READ, SMB_BASE + 0x00, -1, "spin until smbus is ready");
	
	XC_WRITE_COMMENT(XC_IO_WRITE, 0x8049, 0x08, "disable the tco timer");
	XC_WRITE_COMMENT(XC_IO_WRITE, 0x80D9, 0, "KBDRSTIN# in gpio mode");
	XC_WRITE_COMMENT(XC_IO_WRITE, 0x8026, 0x01, "disable PWRBTN#");

	XC_WRITE_COMMENT_MASK(XC_PCI_WRITE, 0x80000880, 0xF000FFFF, 0x2, "turn off secret rom");
	
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x80000804, 0x03, "enable io space");

	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000F04C, 0x01, "enable internal graphics");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000F018, 0x10100, "setup secondary bus 1");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000036C, 0x1000000, "smbus is bad, flatline clks");

	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x80010010, NV2A_BASE, "set nv reg base");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000F020, 0xFDF0FD00, "reload nv reg base");
	
	// nv clk
	XC_WRITE_COMMENT(XC_MEM_WRITE, NV2A_BASE + NV_CLK_REG, 0x11701, "set nv clk 155 MHz ( rev == A1 < DVT4)");
	XC_WRITE_STATEMENT(XC_MEM_WRITE, NV2A_BASE + NV_CLK_REG, -1,
		UINT base = 16667;
		UINT nvclk = base * ((_ptr->data & 0xFF00) >> 8);
		nvclk /= 1 << ((_ptr->data & 0x70000) >> 16);
		nvclk /= _ptr->data & 0xFF; nvclk /= 1000;
		sprintf(str+strlen(str), "set nv clk %dMHz (@ %.3fMHz)", nvclk, (float)(base / 1000.00f)));

	XC_WRITE_COMMENT_NEXT_XCODE(XC_MEM_READ, NV2A_BASE, -1, XC_AND_OR, 0xFF, 0, "get nv rev");
	XC_WRITE_COMMENT_LAST_2_XCODE(XC_JNE, 0xA1, -1, XC_AND_OR, 0xFF, 0, XC_MEM_READ, NV2A_BASE, -1, "jmp if nv rev >= A2 ( >= DVT4 )");
	XC_WRITE_COMMENT_LAST_2_XCODE(XC_JNE, 0xA2, -1, XC_AND_OR, 0xFF, 0, XC_MEM_READ, NV2A_BASE, -1, "jmp if nv rev < A2 ( < DVT4 )");

	XC_WRITE_COMMENT_NEXT_XCODE(XC_MEM_WRITE, NV2A_BASE+0x1214, 0x28282828, XC_MEM_WRITE, NV2A_BASE+0x122C, 0x88888888, "configure pad for micron");
	XC_WRITE_COMMENT_NEXT_XCODE(XC_MEM_WRITE, NV2A_BASE+0x1214, 0x09090909, XC_MEM_WRITE, NV2A_BASE+0x122C, 0xAAAAAAAA, "configure pad for samsung");

	XC_WRITE_STATEMENT(XC_PCI_WRITE, 0x80000084, -1, sprintf(str+strlen(str), "set memory size %d Mb", (_ptr->data+1)/1024/1024));

	XC_WRITE_COMMENT(XC_MEM_WRITE, NV2A_BASE+0x100200, 0x03070103, " set extbank bit (00000F00)");
	XC_WRITE_COMMENT(XC_MEM_WRITE, NV2A_BASE+0x100200, 0x03070003, " clear extbank bit (00000F00)");

	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000103C, 0, "clear scratch pad (mem type)");
	XC_WRITE_COMMENT(XC_PCI_WRITE, 0x8000183C, 0, "clear scratch pad (mem result)");

	XC_WRITE_COMMENT_MASK(XC_MEM_WRITE, 0x00555508, 0x00FFFF0F, MEMTEST_PATTERN1, "mem test pattern 1");
	XC_WRITE_COMMENT_MASK(XC_MEM_WRITE, 0x00555508, 0x00FFFF0F, MEMTEST_PATTERN2, "mem test pattern 2");
	XC_WRITE_COMMENT_MASK(XC_MEM_WRITE, 0x00555508, 0x00FFFF0F, MEMTEST_PATTERN3, "mem test pattern 3");

	XC_WRITE_COMMENT_MASK_LAST_XCODE(XC_JNE, MEMTEST_PATTERN2, 0xFFFFFFFF, -1, XC_MEM_READ, 0x00555508, 0x00FFFF0F, 0, "does dram exist?");

	XC_WRITE_COMMENT_NEXT_XCODE(XC_JMP, 0, _offset, XC_JMP, 0, _offset+9U, "15ns delay by performing jmps");

	XC_WRITE_COMMENT_LAST_2_XCODE(XC_USE_RESULT, 0x04, MCP_CFG_LEG_24, XC_AND_OR, 0xFFFFFFFF, 0x400, XC_PCI_READ, MCP_CFG_LEG_24, 0, "don't gen INIT# on powercycle");

	XC_WRITE_COMMENT(XC_MEM_WRITE, 0, -1, "visor");

	XC_WRITE_COMMENT(XC_EXIT, 0x806, -1, "quit xcodes");

	return 0;
}

// load ini file helper function
static int ll(char* output, char* str, UINT i, UINT& j, UINT len)
{
	if (len < 1)
		return -1;

 	if (str[i] != '{')
		return 1;

	// go find matching '}'
	j = i + 1;
	while (j < len && str[j] != '}')
	{
		j++;
	}
	if (str[j] != '}')
	{
		error("Error: missing closing bracket\n");
		return -1;
	}
	// found matching '}'

	// cpy str to output ; null terminate
	xb_cpy(output, str + i, j - i + 1);
	output[j - i + 1] = '\0';
	xb_mov(str + i + 3, str + j + 1, len - j);

	return 0;
}
// set setting value helper function
static void setSettingValue(char** setting, const char* value, UINT len)
{
	if (*setting != NULL)  // setting already set, skip
		return;

	*setting = (char*)xb_alloc(len + 1);
	if (*setting == NULL)
		return;

	strcpy(*setting, value);
}

int XcodeInterp::loadini(DECODE_SETTINGS& settings) const
{
	const char* filename = "decode.ini";
	FILE* stream = fopen(filename, "r");
	if (stream == NULL)
	{
		error("Error: Failed to open %s\n", filename);
		return 1;
	}

	const char delim[] = "=";

	char buf[128] = { 0 }; // buffer for reading the file
	char* line_ptr = NULL;
	char* key = NULL;
	char* value = NULL;

	UINT len = 0;
	UINT i, j;
	UCHAR k;

	int result = 0;

	const int ERROR_INVALID_KEY = 3;
	const int ERROR_INVALID_VALUE = 4;
	const int ERROR_OUT_OF_MEMORY = 5;

	// parse next line
	while (fgets(buf, sizeof(buf), stream) != NULL)
	{		
		line_ptr = buf;
		ltrim(line_ptr);

		// line-format: key=value

		// ignore invalid characters
		if (line_ptr[0] == ';')
				continue;

		// get the key.
		key = strtok(line_ptr, delim);
		if (key == NULL)
			continue;

		// get the value
		value = strtok(NULL, delim);
		if (value == NULL)
			continue;
		len = strlen(value);
		ltrim(value);
		if (value[len - 1] == '\n')
			value[len - 1] = '\0';
		rtrim(value);

		// cmp key and set values.

		strcpy(buf, key);

		if (strcmp(buf, "format_str") == 0)
		{
			setSettingValue(&settings.format_str, value, len);
		}
		else if (strcmp(buf, "jmp_str") == 0)
		{
			setSettingValue(&settings.jmp_str, value, len);
		}
		else if (strcmp(buf, "no_operand_str") == 0)
		{
			setSettingValue(&settings.no_operand_str, value, len);
		}
		else if (strcmp(buf, "num_str") == 0)
		{
			setSettingValue(&settings.num_str, value, len);
		}
		else if (strcmp(buf, "xc_nop") == 0)
		{
			// nop opcode
			settings.opcodes[0].opcode = XC_NOP;
			setSettingValue(&settings.opcodes[0].str, value, len);
		}
		else if (strcmp(buf, "xc_mem_read") == 0)
		{
			// mem_read opcode
			settings.opcodes[1].opcode = XC_MEM_READ;
			setSettingValue(&settings.opcodes[1].str, value, len);
		}
		else if (strcmp(buf, "xc_mem_write") == 0)
		{
			// mem_write opcode
			settings.opcodes[2].opcode = XC_MEM_WRITE;
			setSettingValue(&settings.opcodes[2].str, value, len);
		}
		else if (strcmp(buf, "xc_pci_write") == 0)
		{
			// pci_write opcode
			settings.opcodes[3].opcode = XC_PCI_WRITE;
			setSettingValue(&settings.opcodes[3].str, value, len);
		}
		else if (strcmp(buf, "xc_pci_read") == 0)
		{
			// pci_read opcode
			settings.opcodes[4].opcode = XC_PCI_READ;
			setSettingValue(&settings.opcodes[4].str, value, len);
		}
		else if (strcmp(buf, "xc_and_or") == 0)
		{
			// and_or opcode
			settings.opcodes[5].opcode = XC_AND_OR;
			setSettingValue(&settings.opcodes[5].str, value, len);
		}
		else if (strcmp(buf, "xc_use_result") == 0)
		{
			// use_result opcode
			settings.opcodes[6].opcode = XC_USE_RESULT;
			setSettingValue(&settings.opcodes[6].str, value, len);
		}
		else if (strcmp(buf, "xc_jne") == 0)
		{
			// jne opcode
			settings.opcodes[7].opcode = XC_JNE;
			setSettingValue(&settings.opcodes[7].str, value, len);
		}
		else if (strcmp(buf, "xc_jmp") == 0)
		{
			// jmp opcode
			settings.opcodes[8].opcode = XC_JMP;
			setSettingValue(&settings.opcodes[8].str, value, len);
		}
		else if (strcmp(buf, "xc_accum") == 0)
		{
			// accum opcode
			settings.opcodes[9].opcode = XC_ACCUM;
			setSettingValue(&settings.opcodes[9].str, value, len);
		}
		else if (strcmp(buf, "xc_io_write") == 0)
		{
			// io_write opcode
			settings.opcodes[10].opcode = XC_IO_WRITE;
			setSettingValue(&settings.opcodes[10].str, value, len);
		}
		else if (strcmp(buf, "xc_io_read") == 0)
		{
			// io_read opcode
			settings.opcodes[11].opcode = XC_IO_READ;
			setSettingValue(&settings.opcodes[11].str, value, len);
		}
		else if (strcmp(buf, "xc_nop_f5") == 0)
		{
			// nop_f5 opcode
			settings.opcodes[12].opcode = XC_NOP_F5;
			setSettingValue(&settings.opcodes[12].str, value, len);
		}
		else if (strcmp(buf, "xc_exit") == 0)
		{
			// exit opcode
			settings.opcodes[13].opcode = XC_EXIT;
			setSettingValue(&settings.opcodes[13].str, value, len);
		}		 
		else if (strcmp(buf, "xc_nop_80") == 0)
		{
			// nop_80 opcode
			settings.opcodes[14].opcode = XC_NOP_80;
			setSettingValue(&settings.opcodes[14].str, value, len);
		}		
		else if (strcmp(buf, "label_on_new_line") == 0)
		{
			// set the label_on_new_line flag
			if (strcmp(value, "true") == 0)
			{
				settings.label_on_new_line = true;
			}
			else if (strcmp(value, "false") == 0)
			{
				settings.label_on_new_line = false;
			}
			else
			{				
				result = ERROR_INVALID_VALUE;
				goto Cleanup;
			}
		}
		else
		{
			result = ERROR_INVALID_KEY;
			goto Cleanup;
		}
	}

	// verify settings

	// format_str 
	if (settings.format_str != NULL)
	{
		// format_str may contain '{offset}' '{op}' '{addr}' '{data}' '{comment}'
		
		const char* repl = NULL;
		len = strlen(settings.format_str);
		for (i = 0; i < len; i++)
		{
			result = ll(buf, settings.format_str, i, j, len);
			if (result == -1) // parse error
			{
				result = 1;
				goto Cleanup;
			}
			else if (result != 0) // no '{' found, skip
			{
				result = 0;
				continue;
			}

			if (strcmp(buf, "{op}") == 0)
			{
				repl = "{2}";
			}
			else if (strcmp(buf, "{addr}") == 0)
			{
				repl = "{3}";
			}
			else if (strcmp(buf, "{data}") == 0)
			{
				repl = "{4}";
			}
			else if (strcmp(buf, "{comment}") == 0)
			{
				repl = "{5}";
			}
			else if (strcmp(buf, "{offset}") == 0)
			{
				repl = "{1}";
			}
			else
			{
				result = ERROR_INVALID_KEY;
				goto Cleanup;
			}

			xb_cpy(settings.format_str + i, repl, 3);
		}		
	}
	else
	{
		settings.format_str = (char*)xb_alloc(22);
		if (settings.format_str == NULL)
		{
			result = ERROR_OUT_OF_MEMORY;
			goto Cleanup;
		}
		strcpy(settings.format_str, "{1}: {2} {3} {4} {5}");
	}

	// jmp_str
	if (settings.jmp_str != NULL)
	{
		// jmp_str may contain '{label}' or '{offset}'

		len = strlen(settings.jmp_str);

		for (i = 0; i < len; i++)
		{	
			result = ll(buf, settings.jmp_str, i, j, len);
			if (result == -1) // parse error
			{
				result = 1;
				goto Cleanup;
			}
			else if (result != 0) // no '{' found, skip
			{
				result = 0;
				continue;
			}

			if (strcmp(buf, "{label}") == 0 || strcmp(buf, "{offset}") == 0)
			{
				xb_cpy(settings.jmp_str + i, "{1}", 3);
			}
			else
			{
				result = ERROR_INVALID_KEY;
				goto Cleanup;
			}
		}
	}
	else
	{
		settings.jmp_str = (char*)xb_alloc(6);
		if (settings.jmp_str == NULL)
		{
			result = ERROR_OUT_OF_MEMORY;
			goto Cleanup;
		}
		strcpy(settings.jmp_str, "{1}:");
	}
	
	// num_str
	if (settings.num_str != NULL)
	{
		// num_str may contain '{num}'

		len = strlen(settings.num_str);
		for (i = 0; i < len; i++)
		{
			result = ll(buf, settings.num_str, i, j, len);
			if (result == -1) // parse error
			{
				result = 1;
				goto Cleanup;
			}
			else if (result != 0) // no '{' found
			{
				result = 0;
				continue;
			}

			if (strcmp(buf, "{num}") == 0)
			{
				settings.num_str_format = (char*)xb_alloc(3);
				if (settings.num_str_format == NULL)
				{
					result = ERROR_OUT_OF_MEMORY;
					goto Cleanup;
				}
				xb_cpy(settings.num_str_format, "%d", 2);
			} 
			else if (strcmp(buf, "{hex}") == 0)
			{
				// replace {num}, {hex} with {1}
				settings.num_str_format = (char*)xb_alloc(5);
				if (settings.num_str_format == NULL)
				{
					result = ERROR_OUT_OF_MEMORY;
					goto Cleanup;
				}
				xb_cpy(settings.num_str_format, "%08X", 4);
			}
			else if (buf[1] == '%')
			{
				settings.num_str_format = (char*)xb_alloc(j);
				if (settings.num_str_format == NULL)
				{
					result = ERROR_OUT_OF_MEMORY;
					goto Cleanup;
				}
				xb_cpy(settings.num_str_format, buf+1, j-2); // cpy chars between '{' and '}'
			}
			else
			{
				result = ERROR_INVALID_KEY;
				goto Cleanup;
			}

			xb_cpy(settings.num_str + i, "{1}", 3);
		}
	}
	else
	{
		settings.num_str = (char*)xb_alloc(8);
		if (settings.num_str == NULL)
		{
			result = ERROR_OUT_OF_MEMORY;
			goto Cleanup;
		}
		strcpy(settings.num_str, "0x%08x");
	}

	// set default num_str_format
	if (settings.num_str_format == NULL)
	{
		settings.num_str_format = (char*)xb_alloc(5);
		if (settings.num_str_format == NULL)
		{
			result = ERROR_OUT_OF_MEMORY;
			goto Cleanup;
		}
		strcpy(settings.num_str_format, "%08X");
	}

	// default opcodes
	k = 0;
	for (i = 0; i < (sizeof(settings.opcodes) / sizeof(OPCODE_VERSION_INFO)); i++)
	{
		if (settings.opcodes[i].str == NULL)
		{
			// find next valid opcode
			UCHAR opcode;
			char* str = NULL;

			while ((opcode = k) < 255 && getDefOpcodeStr(k++, str) != 0); // find next valid opcode (range 0-255)

			if (str == NULL)
			{
				error("Error: failed to find default opcode\n");
				result = 1;
				goto Cleanup;
			}

			settings.opcodes[i].str = (char*)xb_alloc(strlen(str) + 1);
			if (settings.opcodes[i].str == NULL)
			{
				result = ERROR_OUT_OF_MEMORY;
				goto Cleanup;
			}
			strcpy(settings.opcodes[i].str, str);
			xb_cpy(&settings.opcodes[i].opcode, &opcode, sizeof(OPCODE));
		}
	}

Cleanup:
	switch (result)
	{
		case ERROR_INVALID_KEY:
			error("Error: Invalid key '%s'\n", buf);
			break;
		case ERROR_OUT_OF_MEMORY:
			error("Error: Out of memory\n");
			break;
		case ERROR_INVALID_VALUE:
			error("Error: Invalid value '%s'\n", value);
			break;
	}

	// close the file
	fclose(stream);

	return result;
}

int XcodeInterp::decodeXcode(DECODE_CONTEXT& context)
{
	UINT len = 0;
	UINT offset = _offset - sizeof(XCODE); // after a call to interpretNext(), _offset always points to the next xcode

	char* str_opcode = NULL;

	char str_label[16] = { 0 };
	char str_addr[15 + 1] = { 0 };
	char str_data[15 + 1] = { 0 };
	char str_comment[64] = { 0 };
	char str_offset[6 + 1] = { 0 };
	char str_decode[128] = { 0 };
	char str_num[15 + 1] = { 0 };

	// check if offset should be a label
	for (LABEL* label : context.labels)
	{
		if (label == NULL)
			continue;
		if (label->offset == offset)
		{
			format(str_label, "%s:", label->name);
			
			if (context.settings.label_on_new_line)
				strcat(str_label, "\n");
			len = strlen(str_label);
			break;
		}
	}
	if (!context.settings.label_on_new_line)
		xb_set(str_label + len, ' ', 8 - len - 1);
	else
		strcat(str_label, "\t");

	// get the format string for the number
	print_f(str_num, sizeof(str_num), context.settings.num_str, context.settings.num_str_format);

	// get the opcode string
	for (int i = 0; i < (sizeof(context.settings.opcodes) / sizeof(OPCODE_VERSION_INFO)); i++)
	{
		if (context.settings.opcodes[i].opcode == context.xcode->opcode)
		{
			str_opcode = context.settings.opcodes[i].str; // point to the opcode string
			break;
		}
	}

	// get the address string
	getAddressStr(str_addr, str_num);
	if (str_addr[0] == '\0')
	{
		// no address string
		if (context.settings.no_operand_str != NULL)
		{
			strcpy(str_addr, context.settings.no_operand_str);
		}
	}

	// get the data string
	switch (context.xcode->opcode)
	{
	case XC_JMP:
	case XC_JNE:
		for (LABEL* label : context.labels)
		{
			if (label == NULL)
				continue;
			if (label->offset == context.xcode->data)
			{
				print_f(str_data, sizeof(str_data), context.settings.jmp_str, label->name);
				break;
			}
		}		
		break;

	default:
		getDataStr(str_data, str_num);
		break;
	}

	// format the data string
	if (str_data[0] == '\0')
	{
		// no data string
		if (context.settings.no_operand_str != NULL)
		{
			strcpy(str_data, context.settings.no_operand_str);
		}
	}

	// get the comment string
	str_comment[0] = '\0';
	getCommentStr(str_comment + 2);
	if (str_comment[2] != '\0')
	{
		xb_cpy(str_comment, "; ", 2);
	}

	if (context.stream == NULL)
		return 1;

	// format the decode string
	format(str_offset, "%04x", (XCODE_BASE + offset));
	print_f(str_decode, sizeof(str_decode), context.settings.format_str, &str_offset, str_opcode, &str_addr, &str_data, &str_comment);
	print(context.stream, "%s%s\n", &str_label, &str_decode);

	return 0;
}
