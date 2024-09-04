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
#include <cstdlib>
#include <cstring>

// user incl
#include "XcodeInterp.h"
#include "file.h"
#include "type_defs.h"
#include "str_util.h"

#include "util.h" // error codes. MOVE

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

int XcodeInterp::load(UCHAR* data, UINT size)
{
	if (_data != NULL)
	{
		return 1;
	}

	_data = (UCHAR*)malloc(size);
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

int encodeX86AsMemWrites(UCHAR* data, UINT size, UCHAR*& buffer, UINT* xcodeSize)
{
	const UINT allocSize = sizeof(XCODE) * 10;

	buffer = (UCHAR*)malloc(allocSize);
	if (buffer == NULL)
		return ERROR_OUT_OF_MEMORY;

	UINT nSize = allocSize;
	UINT offset = 0;
	XCODE xcode = { XC_MEM_WRITE, 0, 0 };

	for (UINT i = 0; i < size; i+=4)
	{
		xcode.addr = i;
		xcode.data = *(UINT*)(data + i);

		if (offset + sizeof(XCODE) > nSize)
		{
			UCHAR* new_buffer = (UCHAR*)realloc(buffer, nSize + allocSize);
			if (new_buffer == NULL)
			{
				return ERROR_OUT_OF_MEMORY;
			}
			buffer = new_buffer;
			nSize += allocSize;
		}

		memcpy(buffer + offset, &xcode, sizeof(XCODE));
		offset += sizeof(XCODE);
	}
	
	if (xcodeSize != NULL) 
	{
		*xcodeSize = offset;
	}

	return 0;
}

int getOpcodeStr(const FIELD_MAP* opcodes, UCHAR opcode, const char*& str)
{
	for (UINT i = 0; i < XCODE_OPCODE_COUNT; i++)
	{
		if (opcodes[i].field == opcode)
		{
			str = opcodes[i].str;
			return 0;
		}
	}
	return 1;
}
