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
#include <memory.h>

// user incl
#include "XcodeInterp.h"
#include "file.h"
#include "type_defs.h"
#include "util.h"
#include "xbmem.h"

#define IS_NEG_TRUE(x, y) (y < 0 ? true : x == y)

#define XC_WRITE_MASK(i, xc_cmd, xc_addr, addr_mask, xc_data, data_mask) \
	(((UCHAR*)(_ptr+i)) >= _data) && ((_ptr+i)->opcode == xc_cmd && \
	((_ptr+i)->addr & addr_mask) == xc_addr && \
	IS_NEG_TRUE(((_ptr+i)->data  & data_mask), xc_data))

#define XC_WRITE(i, xc_cmd, xc_addr, xc_data) XC_WRITE_MASK(i, xc_cmd, xc_addr, 0xFFFFFFFF, xc_data, 0xFFFFFFFF)

#define XC_WRITE_COMMENT_MASK(comment, xc_cmd, xc_addr, addr_mask, xc_data) \
	if (XC_WRITE_MASK(0, xc_cmd, xc_addr, addr_mask, xc_data, 0xFFFFFFFF)) \
		{ strcat(str, comment); return 0; }
	
#define XC_WRITE_STATEMENT(xc_cmd, xc_addr, xc_data, statement) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data)) \
		{ statement; return 0; }

#define XC_WRITE_COMMENT(comment, xc_cmd, xc_addr, xc_data) XC_WRITE_STATEMENT(xc_cmd, xc_addr, xc_data, strcat(str, comment))

#define XC_WRITE_COMMENT_LAST_XCODE(comment, xc_cmd, xc_addr, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_data) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data) && XC_WRITE(-1, xc_prev_cmd, xc_prev_addr, xc_prev_data)) \
		{ strcat(str, comment); return 0; }

#define XC_WRITE_COMMENT_NEXT_XCODE(comment, xc_cmd, xc_addr, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_data) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data) && XC_WRITE(1, xc_prev_cmd, xc_prev_addr, xc_prev_data)) \
		{ strcat(str, comment); return 0; }

#define XC_WRITE_COMMENT_LAST_2_XCODE(comment, xc_cmd, xc_addr, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_data, xc_prev1_cmd, xc_prev1_addr, xc_prev1_data) \
	if (XC_WRITE(0, xc_cmd, xc_addr, xc_data) && XC_WRITE(-1, xc_prev_cmd, xc_prev_addr, xc_prev_data) && XC_WRITE(-2, xc_prev1_cmd, xc_prev1_addr, xc_prev1_data)) \
		{ strcat(str, comment); return 0; }

#define XC_WRITE_COMMENT_MASK_LAST_XCODE(comment, xc_cmd, xc_addr, xc_addr_mask, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data) \
	if (XC_WRITE_MASK(0, xc_cmd, xc_addr, xc_addr_mask, xc_data, 0xFFFFFFFF) && XC_WRITE_MASK(-1, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data, 0xFFFFFFFF)) \
		{ strcat(str, comment); return 0; }

#define XC_WRITE_COMMENT_MASK_NEXT_XCODE(comment, xc_cmd, xc_addr, xc_addr_mask, xc_data, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data) \
	if (XC_WRITE_COMMENT_MASK(0, xc_cmd, xc_addr, xc_addr_mask, xc_data) && XC_WRITE_COMMENT_MASK(1, xc_prev_cmd, xc_prev_addr, xc_prev_addr_mask, xc_prev_data)) \
		{ strcat(str, comment); return 0; }

int XcodeInterp::load(UCHAR* data, UINT size)
{
	if (_data != NULL)
	{
		return 1;
	}

	_data = (UCHAR*)xb_alloc(size);
	if (_data == NULL)
	{
		return ERROR_OUT_OF_MEMORY;
	}

	memcpy(_data, data, size);
	_size = size;
	
	reset();

	return 0;
}

void XcodeInterp::reset()
{
	_offset = 0;
	_status = DATA_OK;
	_ptr = (XCODE*)_data;
}

int XcodeInterp::interpretNext(XCODE*& xcode)
{
	if (_data == NULL)
	{
		_status = INTERP_STATUS::DATA_ERROR;
		return ERROR_INVALID_DATA;
	}

	if (_status == INTERP_STATUS::EXIT_OP_FOUND) // last xcode was an exit. dont interpret anymore xcodes after this
		return 1;

	if (_offset + sizeof(XCODE) > _size) // check if offset is within bounds
	{
		_status = INTERP_STATUS::DATA_ERROR;		
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

int XcodeInterp::getAddressStr(char* str, const OPCODE_VERSION_INFO* map, const char* num_str)
{
	const char* opcode_str;

	// opcodes that dont use addr field
	switch (_ptr->opcode)
	{
		case XC_JMP:
			str[0] = '\0';
			return 1;
		case XC_USE_RESULT:
			if (getOpcodeStr(map, (UCHAR)_ptr->addr, opcode_str) == 0)
			{
				strcpy(str, opcode_str);
				break;
			}
			// fall through
		default:			
			sprintf(str, num_str, _ptr->addr);
			break;
	}


	return 0;
}
int XcodeInterp::getDataStr(char* str, const char* num_str)
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
			sprintf(str, num_str, _ptr->data);
			break;
	}

	return 0;
}
int XcodeInterp::getCommentStr(char* str)
{
	// -1 = dont care about field

	XC_WRITE_COMMENT("smbus read status", XC_IO_READ, SMB_BASE + 0x00, -1);
	XC_WRITE_COMMENT("smbus clear status", XC_IO_WRITE, SMB_BASE + 0x00, 0x10);

	XC_WRITE_COMMENT("smbus read revision register", XC_IO_WRITE, SMB_BASE+0x08, 0x01);

	XC_WRITE_COMMENT("smbus set cmd", XC_IO_WRITE, SMB_BASE+0x08, -1);
	XC_WRITE_COMMENT("smbus set val", XC_IO_WRITE, SMB_BASE+0x06, -1);

	XC_WRITE_COMMENT("smbus kickoff", XC_IO_WRITE, SMB_BASE + 0x02, 0x0A);

	XC_WRITE_COMMENT("smc slave write addr", XC_IO_WRITE, SMB_BASE + 0x04, 0x20);
	XC_WRITE_COMMENT("smc slave read addr", XC_IO_WRITE, SMB_BASE + 0x04, 0x21);

	XC_WRITE_COMMENT("871 encoder slave addr", XC_IO_WRITE, SMB_BASE + 0x04, 0x8A);
	XC_WRITE_COMMENT("focus encoder slave addr", XC_IO_WRITE, SMB_BASE + 0x04, 0xD4);
	XC_WRITE_COMMENT("xcalibur encoder slave addr", XC_IO_WRITE, SMB_BASE + 0x04, 0xE1);

	XC_WRITE_COMMENT("read io bar (B02) MCPX v1.0", XC_PCI_READ, MCPX_1_0_IO_BAR, 0);
	XC_WRITE_COMMENT("set io bar (B02) MCPX v1.0", XC_PCI_WRITE, MCPX_1_0_IO_BAR, MCPX_IO_BAR_VAL);

	XC_WRITE_COMMENT_LAST_XCODE("jmp if (B02) MCPX v1.0", 
		XC_JNE, MCPX_IO_BAR_VAL, -1, 
		XC_PCI_READ, MCPX_1_1_IO_BAR, 0);

	XC_WRITE_COMMENT("read io bar (C03) MCPX v1.1", XC_PCI_READ, MCPX_1_1_IO_BAR, 0);
	XC_WRITE_COMMENT("set io bar (C03) MCPX v1.1", XC_PCI_WRITE, MCPX_1_1_IO_BAR, MCPX_IO_BAR_VAL);
	XC_WRITE_COMMENT_LAST_XCODE("jmp if (C03) MCPX v1.1",
		XC_JNE, MCPX_IO_BAR_VAL, -1,
		XC_PCI_READ, MCPX_1_0_IO_BAR, 0);
	
	XC_WRITE_COMMENT_LAST_XCODE("spin until smbus is ready",
		XC_JNE, 0x10, _offset-18U,
		XC_IO_READ, SMB_BASE + 0x00, 0);
	
	XC_WRITE_COMMENT("disable the tco timer", XC_IO_WRITE, 0x8049, 0x08);
	XC_WRITE_COMMENT("KBDRSTIN# in gpio mode", XC_IO_WRITE, 0x80D9, 0);
	XC_WRITE_COMMENT("disable PWRBTN#", XC_IO_WRITE, 0x8026, 0x01);

	XC_WRITE_COMMENT_MASK("turn off secret rom", XC_PCI_WRITE, 0x80000880, 0xF000FFFF, 0x2);
	
	XC_WRITE_COMMENT("enable io space", XC_PCI_WRITE, 0x80000804, 0x03);

	XC_WRITE_COMMENT("enable internal graphics", XC_PCI_WRITE, 0x8000F04C, 0x01);
	XC_WRITE_COMMENT("setup secondary bus 1", XC_PCI_WRITE, 0x8000F018, 0x10100);
	XC_WRITE_COMMENT("smbus is bad, flatline clks", XC_PCI_WRITE, 0x8000036C, 0x1000000);

	XC_WRITE_COMMENT("set nv reg base", XC_PCI_WRITE, 0x80010010, NV2A_BASE);
	XC_WRITE_COMMENT("reload nv reg base", XC_PCI_WRITE, 0x8000F020, 0xFDF0FD00);
	
	// nv clk
	XC_WRITE_COMMENT("set nv clk 155 MHz ( rev == A1 )", XC_MEM_WRITE, NV2A_BASE + NV_CLK_REG, 0x11701);
	
	XC_WRITE_STATEMENT(XC_MEM_WRITE, NV2A_BASE + NV_CLK_REG, -1,
		UINT base = 16667;
		UINT nvclk = base * ((_ptr->data & 0xFF00) >> 8);
		nvclk /= 1 << ((_ptr->data & 0x70000) >> 16);
		nvclk /= _ptr->data & 0xFF; nvclk /= 1000;
		sprintf(str+strlen(str), "set nv clk %dMHz (@ %.3fMHz)", nvclk, (float)(base / 1000.00f)) );

	XC_WRITE_COMMENT_NEXT_XCODE("get nv rev", 
		XC_MEM_READ, NV2A_BASE, 0, 
		XC_AND_OR, 0xFF, 0);
	
	XC_WRITE_COMMENT_LAST_2_XCODE("jmp if nv rev >= A2 ( >= DVT4 )",
		XC_JNE, 0xA1, -1,
		XC_AND_OR, 0xFF, 0,
		XC_MEM_READ, NV2A_BASE, 0);
	
	XC_WRITE_COMMENT_LAST_2_XCODE("jmp if nv rev < A2 ( < DVT4 )",
		XC_JNE, 0xA2, -1, 
		XC_AND_OR, 0xFF, 0, 
		XC_MEM_READ, NV2A_BASE, 0);

	XC_WRITE_COMMENT_NEXT_XCODE("configure pad for micron",
		XC_MEM_WRITE, NV2A_BASE+0x1214, 0x28282828, 
		XC_MEM_WRITE, NV2A_BASE+0x122C, 0x88888888);

	XC_WRITE_COMMENT_NEXT_XCODE("configure pad for samsung",
		XC_MEM_WRITE, NV2A_BASE+0x1214, 0x09090909,
		XC_MEM_WRITE, NV2A_BASE+0x122C, 0xAAAAAAAA);

	XC_WRITE_STATEMENT(XC_PCI_WRITE, 0x80000084, -1, sprintf(str+strlen(str), "set memory size %d Mb", (_ptr->data+1)/1024/1024) );

	XC_WRITE_COMMENT("set extbank bit (00000F00)", XC_MEM_WRITE, NV2A_BASE+0x100200, 0x03070103);
	XC_WRITE_COMMENT("clear extbank bit (00000F00)", XC_MEM_WRITE, NV2A_BASE+0x100200, 0x03070003);

	XC_WRITE_COMMENT("clear scratch pad (mem type)", XC_PCI_WRITE, 0x8000103C, 0);
	XC_WRITE_COMMENT("clear scratch pad (mem result)", XC_PCI_WRITE, 0x8000183C, 0);

	XC_WRITE_COMMENT_MASK("mem test pattern 1", XC_MEM_WRITE, 0x00555508, 0x00FFFF0F, MEMTEST_PATTERN1);
	XC_WRITE_COMMENT_MASK("mem test pattern 2", XC_MEM_WRITE, 0x00555508, 0x00FFFF0F, MEMTEST_PATTERN2);
	XC_WRITE_COMMENT_MASK("mem test pattern 3", XC_MEM_WRITE, 0x00555508, 0x00FFFF0F, MEMTEST_PATTERN3);

	XC_WRITE_COMMENT_MASK_LAST_XCODE("does dram exist?",
		XC_JNE, MEMTEST_PATTERN2, 0xFFFFFFFF, -1,
		XC_MEM_READ, 0x00555508, 0x00FFFF0F, 0);

	XC_WRITE_COMMENT_NEXT_XCODE("15ns delay by performing jmps",
		XC_JMP, 0, _offset, 
		XC_JMP, 0, _offset+9U);

	XC_WRITE_COMMENT_LAST_2_XCODE("don't gen INIT# on powercycle",
		XC_USE_RESULT, 0x04, MCPX_LEG_24,
		XC_AND_OR, 0xFFFFFFFF, 0x400,
		XC_PCI_READ, MCPX_LEG_24, 0);

	XC_WRITE_COMMENT("visor", XC_MEM_WRITE, 0, -1);

	XC_WRITE_COMMENT("quit xcodes", XC_EXIT, 0x806, 0);

	return 0;
}

////////////////////////////

// XCODE DECODER

////////////////////////////

#include "loadini.h"

int ll(char* output, char* str, UINT i, UINT* j, UINT len, UINT m);
int ll2(char* output, char* str, UINT i, UINT& j, UINT len);

int XcodeInterp::loadDecodeSettings(const char* ini, DECODE_SETTINGS& settings) const
{
	char* value = NULL;

	UINT len = 0;
	UINT i = 0;
	UINT j = 0;
	UCHAR k = 0;

	int result = 0;

	FILE* stream = NULL;

	char buf[128] = {};

	static const LOADINI_SETTING_MAP settings_map[] = {
		{ "format_str", LOADINI_SETTING_TYPE::STR, &settings.format_str },
		{ "jmp_str", LOADINI_SETTING_TYPE::STR, &settings.jmp_str },
		{ "no_operand_str", LOADINI_SETTING_TYPE::STR, &settings.no_operand_str },
		{ "num_str", LOADINI_SETTING_TYPE::STR, &settings.num_str },
		{ "label_on_new_line", LOADINI_SETTING_TYPE::BOOL, &settings.label_on_new_line },

		{ opcodeMap[0].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[0].str },
		{ opcodeMap[1].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[1].str },
		{ opcodeMap[2].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[2].str },
		{ opcodeMap[3].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[3].str },
		{ opcodeMap[4].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[4].str },
		{ opcodeMap[5].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[5].str },
		{ opcodeMap[6].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[6].str },
		{ opcodeMap[7].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[7].str },
		{ opcodeMap[8].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[8].str },
		{ opcodeMap[9].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[9].str },
		{ opcodeMap[10].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[10].str },
		{ opcodeMap[11].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[11].str },
		{ opcodeMap[12].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[12].str },
		{ opcodeMap[13].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[13].str },
		{ opcodeMap[14].str, LOADINI_SETTING_TYPE::STR, &settings.opcodes[14].str }
	};	

	static const DECODE_SETTING_MAP num_str_fields[] = {
		{ "{num}", "%d" },
		{ "{hex}", "%08x" }
	};

	for (i = 0; i < sizeof(settings.format_map) / sizeof(DECODE_STR_SETTING_MAP); i++)
	{
		settings.format_map[i].field = NULL;
		settings.format_map[i].seq = 0;
		settings.format_map[i].str = NULL;
	}

	settings.format_map[0].field = (char*)xb_alloc(9);
	if (settings.format_map[0].field == NULL)
	{
		result = ERROR_OUT_OF_MEMORY;
		goto Cleanup;
	}
	strcpy(settings.format_map[0].field, "{offset}");

	settings.format_map[1].field = (char*)xb_alloc(5);
	if (settings.format_map[1].field == NULL)
	{
		result = ERROR_OUT_OF_MEMORY;
		goto Cleanup;
	}
	strcpy(settings.format_map[1].field, "{op}");

	settings.format_map[2].field = (char*)xb_alloc(7);
	if (settings.format_map[2].field == NULL)
	{
		result = ERROR_OUT_OF_MEMORY;
		goto Cleanup;
	}
	strcpy(settings.format_map[2].field, "{addr}");

	settings.format_map[3].field = (char*)xb_alloc(7);
	if (settings.format_map[3].field == NULL)
	{
		result = ERROR_OUT_OF_MEMORY;
		goto Cleanup;
	}
	strcpy(settings.format_map[3].field, "{data}");

	settings.format_map[4].field = (char*)xb_alloc(10);
	if (settings.format_map[4].field == NULL)
	{
		result = ERROR_OUT_OF_MEMORY;
		goto Cleanup;
	}
	strcpy(settings.format_map[4].field, "{comment}");

	if (ini != NULL)
	{
		stream = fopen(ini, "r");
		if (stream != NULL) // only load ini file if it exists
		{
			printf("settings file: %s\n", ini);
			result = loadini(stream, settings_map, sizeof(settings_map));
			fclose(stream);
			if (result != 0) // convert to error code
			{				
				result = 1;
				goto Cleanup;
			}
		}
		else
		{
			printf("settings: default\n");
		}
	}

	// verify and parse settings / set defaults

	// format_str 
	
	// default format_str
	if (settings.format_str == NULL)
	{
		static const char* default_format_str = "{offset}: {op} {addr} {data} {comment}";

		settings.format_str = (char*)xb_alloc(strlen(default_format_str) + 1);
		if (settings.format_str == NULL)
		{
			result = ERROR_OUT_OF_MEMORY;
			goto Cleanup;
		}
		strcpy(settings.format_str, default_format_str);
	}

	// format_str may contain '{offset}' '{op}' '{addr}' '{data}' '{comment}'

	k = 1;
	len = strlen(settings.format_str);
	for (i = 0; i < len; i++)
	{
		result = ll2(buf, settings.format_str, i, j, len);
		if (result == ERROR_INVALID_DATA)
		{
			goto Cleanup;
		}
		else if (result != 0)
		{
			if (i != 0)
				continue;

			if (j > 1)
			{
				settings.prefix_str = (char*)xb_alloc(j + 1);
				if (settings.prefix_str == NULL)
				{
					result = ERROR_OUT_OF_MEMORY;
					goto Cleanup;
				}
				strcpy(settings.prefix_str, buf);
				i = j - 1;
			}
			else
			{
				settings.prefix_str = NULL;
			}
			
			continue;
		}
			
		for (j = 0; j < sizeof(settings.format_map) / sizeof(DECODE_STR_SETTING_MAP); j++)
		{
			if (strstr(buf, settings.format_map[j].field) != NULL)
			{
				result = ll(NULL, buf, 0, NULL, strlen(buf), 2);
				memcpy(buf, "%s", 2);

				settings.format_map[j].str = (char*)xb_alloc(strlen(buf) + 1);
				if (settings.format_map[j].str == NULL)
				{
					result = ERROR_OUT_OF_MEMORY;
					goto Cleanup;
				}

				strcpy(settings.format_map[j].str, buf);
				settings.format_map[j].seq = k++;
				i += strlen(buf) - 1;
				break;
			}
		}
		if (j == sizeof(settings.format_map) / sizeof(DECODE_STR_SETTING_MAP))
		{
			result = ERROR_INVALID_DATA;
			goto Cleanup;
		}
	}

#if 1
	//testing
	buf[0] = '\0';
	printf("format_str: '");
	if (settings.prefix_str != NULL)
	{
		printf(settings.prefix_str);
	}
	for (k = 1; k < sizeof(settings.format_map) / sizeof(DECODE_STR_SETTING_MAP); k++)
	{
		for (int t = 0; t < sizeof(settings.format_map) / sizeof(DECODE_STR_SETTING_MAP); t++)
		{
			if (settings.format_map[t].seq == k)
			{
				if (settings.format_map[t].str != NULL)
				{
					sprintf(buf, "%s", settings.format_map[t].str);
					printf(buf, settings.format_map[t].field);
				}
				break;
			}
		}
	}
	printf("'\n");
#endif

	// jmp_str
	if (settings.jmp_str != NULL)
	{
		len = strlen(settings.jmp_str);
		for (i = 0; i < len; i++)
		{	
			result = ll(buf, settings.jmp_str, i, &j, len, 2);
			if (result == ERROR_INVALID_DATA) // parse error
			{
				goto Cleanup;
			}
			else if (result != 0) // no '{' found, skip
			{
				result = 0;
				continue;
			}

			if (strcmp(buf, "{label}") == 0)
			{
				memcpy(settings.jmp_str + i, "%s", 2);
			}
			else
			{
				result = ERROR_INVALID_DATA;
				goto Cleanup;
			}
		}

		printf("jmp_str: '%s'\n", settings.jmp_str);
	}
	else
	{
		settings.jmp_str = (char*)xb_alloc(6);
		if (settings.jmp_str == NULL)
		{
			result = ERROR_OUT_OF_MEMORY;
			goto Cleanup;
		}
		strcpy(settings.jmp_str, "%s:");
	}
	
	// num_str
	if (settings.num_str != NULL)
	{		
		len = strlen(settings.num_str);
		for (i = 0; i < len; i++)
		{
			result = ll(buf, settings.num_str, i, &j, len, 2);
			if (result == ERROR_INVALID_DATA) // parse error
			{
				goto Cleanup;
			}
			else if (result != 0) // no '{' found
			{
				result = 0;
				continue;
			}

			for (j = 0; j < sizeof(num_str_fields) / sizeof(DECODE_SETTING_MAP); j++)
			{
				if (strcmp(buf, num_str_fields[j].field) == 0)
				{
					settings.num_str_format = (char*)xb_alloc(strlen(num_str_fields[j].value) + 1);
					if (settings.num_str_format == NULL)
					{
						result = ERROR_OUT_OF_MEMORY;
						goto Cleanup;
					}
					strcpy(settings.num_str_format, num_str_fields[j].value);
					break;
				}
			}

			if (j == sizeof(num_str_fields) / sizeof(DECODE_SETTING_MAP))
			{
				result = ERROR_INVALID_DATA;
				goto Cleanup;
			}

			memcpy(settings.num_str + i, "%s", 2);	
		}

		printf("num_str: '%s'\n", settings.num_str);
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
			
	// fill in default opcodes
	k = 0;
	settings.opcodeMaxLen = 0;
	for (i = 0; i < XCODE_OPCODE_COUNT; i++)
	{
		UCHAR opcode = 0;
		const char* str = NULL;

		// find next valid opcode (range 0-255)
		while ((opcode = k) < 255 && getOpcodeStr(opcodeMap, k++, str) != 0);

		if (str == NULL)
		{
			result = 1;
			goto Cleanup;
		}

		if (settings.opcodes[i].str == NULL) // set default opcode string
		{
			settings.opcodes[i].str = (char*)xb_alloc(strlen(str) + 1);
			if (settings.opcodes[i].str == NULL)
			{
				result = ERROR_OUT_OF_MEMORY;
				goto Cleanup;
			}
			strcpy((char*)settings.opcodes[i].str, str);
		}

		memcpy(&settings.opcodes[i].opcode, &opcode, sizeof(OPCODE));

		len = strlen(settings.opcodes[i].str);
		if (len > settings.opcodeMaxLen)
		{
			settings.opcodeMaxLen = len;
		}

		printf("opcode: '%s'\n", settings.opcodes[i].str);
	}

Cleanup:

	if (result == ERROR_INVALID_DATA)
	{
		printf("Error: Key '%s' has invalid value '%s'\n", buf, value);
	}

	return result;
}

int XcodeInterp::initDecoder(const char* ini, DECODE_CONTEXT& context)
{
	if (loadDecodeSettings(ini, context.settings) != 0)
		return 1;
	
	bool isLabel = false;
	static const char* label_format = "lb_%02d";

	reset();
	while (interpretNext(context.xcode) == 0)
	{
		// go through the xcodes and fix up jmp offsets.

		if (context.xcode->opcode != XC_JMP && context.xcode->opcode != XC_JNE)
			continue;

		isLabel = false;

		// Fix up the xcode data to the offset for phase 2 decoding
		context.xcode->data = _offset + context.xcode->data;

		// check if offset is already a label
		for (LABEL* label : context.labels)
		{
			if (label->offset == context.xcode->data)
			{
				isLabel = true;
				break;
			}
		}

		// label does not exist. create one.
		if (!isLabel)
		{
			LABEL* label = (LABEL*)xb_alloc(sizeof(LABEL) + 20);
			if (label == NULL)
				return 1;

			label->name = ((char*)label + sizeof(LABEL));
			sprintf(label->name, label_format, context.labels.size());

			UINT len = strlen(label->name);
			if (context.settings.labelMaxLen < len)
				context.settings.labelMaxLen = len;

			label->offset = context.xcode->data;

			context.labels.push_back(label);
		}
	}

	return 0;
}

#define NO_FORMAT 1
#undef NO_FORMAT

int XcodeInterp::decodeXcode(DECODE_CONTEXT& context)
{
	int i, j;
	UINT len;
	UINT fmt_len;
	UINT op_len = 0;
	UINT operand_len = 0;
	UINT no_operand_len = 0;

	char str_decode[128] = {};
	char str_num[64] = {};
	char str[64] = {};
	char str_tmp[64] = {};
			
	sprintf(str_num, context.settings.num_str, context.settings.num_str_format);

	// label
	for (LABEL* label : context.labels)
	{
		if (label == NULL)
			continue;
		if (label->offset == _offset - sizeof(XCODE))
		{
			sprintf(str, "%s:", label->name);
			if (context.settings.label_on_new_line)
				strcat(str, "\n");

			break;
		}
	}
	if (!context.settings.label_on_new_line)
		rpad(str, context.settings.labelMaxLen + 2 + 1, ' '); // 2 for ': '
	else
		strcat(str, "\t");
	strcat(str_decode, str);
	str[0] = '\0';

	// prefix
	if (context.settings.prefix_str != NULL)
	{
		strcat(str_decode, context.settings.prefix_str);
	}

	for (i = 1; i < 5; i++)
	{
		for (j = 0; j < 5; j++)
		{
			if (context.settings.format_map[j].seq != i)
				continue;
			if (context.settings.format_map[j].str == NULL)
				break;

			len = strlen(str_decode);
			fmt_len = strlen(context.settings.format_map[j].str) - 2;
			if (context.settings.no_operand_str != NULL)
			{
				no_operand_len = strlen(context.settings.no_operand_str) + fmt_len + 1;
			}
			op_len = context.settings.opcodeMaxLen + fmt_len + 1;
			operand_len = strlen(str_num) - strlen(context.settings.num_str_format) + 8 + fmt_len + 1;

			str[0] = '\0';
			memset(str_tmp, 0, sizeof(str_tmp));

			if (strcmp(context.settings.format_map[j].field, "{offset}") == 0)
			{
				sprintf(str, "%04x: ", context.base + _offset - sizeof(XCODE));
			}
			else if (strcmp(context.settings.format_map[j].field, "{op}") == 0)
			{
				const char* str_opcode;
				if (getOpcodeStr(context.settings.opcodes, context.xcode->opcode, str_opcode) != 0)
					return 1;

				sprintf(str, context.settings.format_map[j].str, str_opcode);
				rpad(str, op_len, ' ');
			}
			else if (strcmp(context.settings.format_map[j].field, "{addr}") == 0)
			{
				getAddressStr(str_tmp, context.settings.opcodes, str_num);
				if (str_tmp[0] == '\0')
				{
					if (context.settings.no_operand_str != NULL)
					{
						strcpy(str_tmp, context.settings.no_operand_str);
					}
				}

				sprintf(str, context.settings.format_map[j].str, str_tmp);	
				if (operand_len < op_len)
					operand_len = op_len;
				if (operand_len < no_operand_len)
					operand_len = no_operand_len;
				rpad(str, operand_len, ' ');
			}
			else if (strcmp(context.settings.format_map[j].field, "{data}") == 0)
			{
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
							sprintf(str_tmp, context.settings.jmp_str, label->name);
							break;
						}
					}
					break;

				default:
					getDataStr(str_tmp, str_num);
					break;
				}

				if (str_tmp[0] == '\0')
				{
					if (context.settings.no_operand_str != NULL)
					{
						strcpy(str_tmp, context.settings.no_operand_str);
					}
				}
				
				sprintf(str, context.settings.format_map[j].str, str_tmp);				
				UINT jmp_len = context.settings.labelMaxLen + strlen(context.settings.jmp_str) - 3 + fmt_len + 1;
				if (operand_len < jmp_len)
					operand_len = jmp_len;
				if (operand_len < no_operand_len)
					operand_len = no_operand_len;
				rpad(str, operand_len, ' ');
			}
			else if (strcmp(context.settings.format_map[j].field, "{comment}") == 0)
			{
				getCommentStr(str_tmp + 2);
				if (str_tmp[2] != '\0')
				{
					memcpy(str_tmp, "; ", 2);
				}
				sprintf(str, context.settings.format_map[j].str, str_tmp);
			}

			strcpy(str_decode + len, str);
			break;
		}		
	}

	fprintf(context.stream, "%s\n", str_decode);

	return 0;
}


////////////////////////////

// HELPER FUNCTIONS

////////////////////////////

int encodeX86AsMemWrites(UCHAR* data, UINT size, UCHAR*& buffer, UINT* xcodeSize)
{
	const UINT allocSize = sizeof(XCODE) * 10;

	buffer = (UCHAR*)xb_alloc(allocSize);
	if (buffer == NULL)
	{
		return ERROR_OUT_OF_MEMORY;
	}
	UINT nSize = allocSize;

	XCODE xcode = {0};
	xcode.opcode = XC_MEM_WRITE;

	// encode x86 as mem writes
	UINT j = 0;
	for (UINT i = 0; i < size; i+=4)
	{
		xcode.addr = i;
		xcode.data = *(UINT*)(data + i);

		// realloc if needed
		if (j + sizeof(XCODE) > nSize)
		{
			UCHAR* new_buffer = (UCHAR*)xb_realloc(buffer, nSize + allocSize);
			if (new_buffer == NULL)
			{
				return ERROR_OUT_OF_MEMORY;
			}
			buffer = new_buffer;
			nSize += allocSize;
		}

		memcpy(buffer+j, &xcode, sizeof(XCODE));
		j += sizeof(XCODE);
	}

	if (xcodeSize != NULL) // update buffer size
	{
		*xcodeSize = j;
	}

	return 0;
}

int ll(char* output, char* str, UINT i, UINT* j, UINT len, UINT m)
{
	if (str[i] != '{')
		return 1;

	UINT k = i + 1;
	while (k < len && str[k] != '}')
	{
		k++;
	}
	if (j != NULL)
	{
		*j = k;
	}

	if (str[k] != '}')
	{
		return ERROR_INVALID_DATA;
	}

	if (output != NULL)
	{
		memcpy(output, str + i, k - i + 1);
		output[k - i + 1] = '\0';
	}

	if (m > 0)
	{
		memmove(str + i + m, str + k + 1, len - k);
	}

	return 0;
}

int getOpcodeStr(const OPCODE_VERSION_INFO* opcodes, UCHAR opcode, const char*& str)
{
	for (UINT i = 0; i < XCODE_OPCODE_COUNT; i++)
	{
		if (opcodes[i].opcode == opcode)
		{
			str = opcodes[i].str;
			return 0;
		}
	}
	return 1;
}

int ll2(char* output, char* str, UINT i, UINT& j, UINT len)
{
	if (str[i] != '{')
	{
		j = i;
		while (j < len && str[j] != '{')
		{
			j++;
		}
		memcpy(output, str + i, j - i);
		output[j - i] = '\0';

		return 1;
	}
	
	j = i;
	while (j < len && str[j] != '}')
	{
		j++;
	}
	if (str[j] != '}')
	{
		return ERROR_INVALID_DATA;
	}
	UINT k = j;
	while (k + 1 < len && str[k + 1] != '{')
	{
		k++;
	}
	if (k + 1 < len)
	{
		j = k;
	}

	memcpy(output, str + i, j - i + 1);
	output[j - i + 1] = '\0';

	return 0;
}
