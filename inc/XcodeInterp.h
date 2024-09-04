// XcodeInterp.h: Implements an xcode interpreter.

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

#ifndef XCODE_INTERP_H
#define XCODE_INTERP_H

// user incl
#include "bldr.h"
#include "type_defs.h"
#include "loadini.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

const UINT SMB_BASE = 0xC000;
const UINT NV2A_BASE = 0x0F000000;
const UINT NV2A_BASE_KERNEL = 0x0FD00000;
const UINT MCPX_1_0_IO_BAR = 0x80000810;
const UINT MCPX_1_1_IO_BAR = 0x80000884;
const UINT MCPX_IO_BAR_VAL = 0x8001;
const UINT MCPX_LEG_24 = 0x80000860;
const UINT NV_CLK_REG = 0x680500;
const UINT MEMTEST_PATTERN1 = 0xAAAAAAAA;
const UINT MEMTEST_PATTERN2 = 0x5A5A5A5A;
const UINT MEMTEST_PATTERN3 = 0x55555555;

#define XCODE_OPCODE_COUNT 15

typedef enum : UCHAR {
    XC_RESERVED =    0X01,
    XC_MEM_READ =    0X02,
    XC_MEM_WRITE =   0X03,
    XC_PCI_WRITE =   0X04,
    XC_PCI_READ =    0X05,
    XC_AND_OR =      0X06,
    XC_USE_RESULT =  0X07,
    XC_JNE =         0X08,
    XC_JMP =         0X09,
    XC_ACCUM =       0X10,
    XC_IO_WRITE =    0X11,
    XC_IO_READ =     0X12,
    XC_NOP_80 =      0X80,
    XC_EXIT =        0XEE,
    XC_NOP_F5 =      0XF5,
} OPCODE;

typedef struct {
    UINT offset;
    char name[20];
} LABEL;

typedef struct {
    const char* str;
    UCHAR field;
} FIELD_MAP;

// Xcode interpreter

static const FIELD_MAP opcodeMap[] = {
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

class XcodeInterp
{
public:
    XcodeInterp() {
		_size = 0;
        reset();
        _data = NULL;
    };
    ~XcodeInterp() {
        if (_data != NULL)
        {
            free(_data);
            _data = NULL;
        }
	};

    enum INTERP_STATUS : int { DATA_OK = 0, EXIT_OP_FOUND, DATA_ERROR };

    int load(UCHAR* data, UINT size);
    void reset();
    int interpretNext(XCODE*& xcode);
        
    XCODE* getPtr() const { return _ptr; };
    UCHAR* getData() const { return _data; };
    UINT getOffset() const { return _offset; };
    INTERP_STATUS getStatus() const { return _status; };

private:
    UCHAR* _data;           // XCODE data
    UINT _size;		        // size of the XCODE data  
    XCODE* _ptr;            // current position in the XCODE data
    UINT _offset;           // offset from the start of the data to the end of the current XCODE (offset to the next XCODE)
    INTERP_STATUS _status;  // status of the xcode interpreter
};

int encodeX86AsMemWrites(UCHAR* data, UINT size, UCHAR*& buffer, UINT* xcodeSize);

int getOpcodeStr(const FIELD_MAP* opcodes, UCHAR opcode, const char*& str);
const LOADINI_RETURN_MAP  getDecodeSettingsMap();
inline const FIELD_MAP* getOpcodeMap() { return opcodeMap; };

#endif // !XCODE_INTERP_H