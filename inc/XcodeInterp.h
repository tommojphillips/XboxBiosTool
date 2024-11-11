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

#include <stdint.h>

// user incl
#include "bldr.h"
#include "loadini.h"

#ifndef NO_MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

#define SMB_BASE            0xC000
#define SMB_CMD_REGISTER    (SMB_BASE + 0x08)
#define SMB_VAL_REGISTER    (SMB_BASE + 0x06)

#define NV2A_BASE           0x0F000000
#define NV2A_BASE_KERNEL    0x0FD00000
#define MCPX_1_0_IO_BAR     0x80000810
#define MCPX_1_1_IO_BAR     0x80000884
#define MCPX_IO_BAR_VAL     0x8001
#define MCPX_LEG_24         0x80000860
#define NV_CLK_REG          0x680500

#define MEMTEST_PATTERN1 0xAAAAAAAA
#define MEMTEST_PATTERN2 0x5A5A5A5A
#define MEMTEST_PATTERN3 0x55555555

#define XC_RESERVED     0X01
#define XC_MEM_READ     0X02
#define XC_MEM_WRITE    0X03
#define XC_PCI_WRITE    0X04
#define XC_PCI_READ     0X05
#define XC_AND_OR       0X06
#define XC_USE_RESULT   0X07
#define XC_JNE          0X08
#define XC_JMP          0X09
#define XC_ACCUM        0X10
#define XC_IO_WRITE     0X11
#define XC_IO_READ      0X12
#define XC_NOP_80       0X80
#define XC_EXIT         0XEE
#define XC_NOP_F5       0XF5

#define XC_OPCODE_COUNT 15

typedef uint8_t OPCODE;

typedef struct _LABEL {
    uint32_t offset;        // the offset of the label
    uint32_t references;    // how many times the label is jumped to ( xcodes that use this label )
    bool defined;           // if we have come across this label definition. ( the xcode that this label points to )
    char name[20];
} LABEL;

typedef struct _FIELD_MAP {
    const char* str;
    uint8_t field;
} FIELD_MAP;

// Xcode interpreter

extern const FIELD_MAP xcode_opcode_map[];

class XcodeInterp {
public:
    XcodeInterp() {
		size = 0;
        reset();
        data = NULL;
    };
    ~XcodeInterp() {
        if (data != NULL) {
            free(data);
            data = NULL;
        }
	};

    enum INTERP_STATUS : int { DATA_OK = 0, EXIT_OP_FOUND, DATA_ERROR };

    int load(uint8_t* data, uint32_t size);
    void reset();
    int interpretNext(XCODE*& xcode);
    
    uint8_t* data;         // XCODE data
    uint32_t size;         // size of the XCODE data  
    XCODE* ptr;            // current position in the XCODE data
    uint32_t offset;       // offset from the start of the data to the end of the current XCODE (offset to the next XCODE)
    INTERP_STATUS status;  // status of the xcode interpreter
private:
};

int encodeX86AsMemWrites(uint8_t* data, uint32_t size, uint32_t base, uint8_t*& buffer, uint32_t* xcodeSize);

int getOpcodeStr(const FIELD_MAP* opcodes, uint8_t opcode, const char*& str);
const LOADINI_RETURN_MAP  getDecodeSettingsMap();

#endif // !XCODE_INTERP_H