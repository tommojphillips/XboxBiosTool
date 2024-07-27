// XcodeInterp.h

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

#ifndef XB_XCODE_INTERP_H
#define XB_XCODE_INTERP_H

// std incl
#include<list>

// user incl
#include "bldr.h"
#include "type_defs.h"
#include "xbmem.h"

const UINT SMB_BASE = 0xC000;
const UINT NV2A_BASE = 0x0F000000;
const UINT NV2A_BASE_KERNEL = 0x0FD00000;
const UINT MCPX_1_0_IO_BAR = 0x80000810;
const UINT MCPX_1_1_IO_BAR = 0x80000884;
const UINT MCPX_IO_BAR_VAL = 0x8001;
const UINT MCP_CFG_LEG_24 = 0x80000860;
const UINT NV_CLK_REG = 0x680500;
const UINT MEMTEST_PATTERN1 = 0xAAAAAAAA;
const UINT MEMTEST_PATTERN2 = 0x5A5A5A5A;
const UINT MEMTEST_PATTERN3 = 0x55555555;

typedef enum : UCHAR
{
    XC_NOP =         0X00,
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

typedef struct _OPCODE_VERSION_INFO
{
    char* str;
    UCHAR opcode;

    _OPCODE_VERSION_INFO() : str(NULL), opcode(0) { }
    
    _OPCODE_VERSION_INFO(const char* s, UCHAR opcode) {
        if (s != NULL)
        {
            str = (char*)xb_alloc(strlen(s) + 1);
            if (str != NULL)
            {
				strcpy(str, s);
            }
		}
        else
        {
            str = NULL;
        }

        this->opcode = opcode;
	}

    void deconstruct()
    {
        if (str != NULL)
        {
            xb_free(str);
            str = NULL;
        }
    }

} OPCODE_VERSION_INFO;

typedef struct {
    const char* field;
    const char* value;
} DECODE_SETTING_MAP;

typedef struct _DECODE_SETTINGS {
    char* format_str;
    char* jmp_str;
    char* no_operand_str;
    char* num_str;
    char* num_str_format;

    bool label_on_new_line;
    
    OPCODE_VERSION_INFO opcodes[15];

    _DECODE_SETTINGS() : opcodes{}, format_str(NULL), jmp_str(NULL), no_operand_str(NULL),
        num_str(NULL), num_str_format(NULL), label_on_new_line(false) { }

    ~_DECODE_SETTINGS() {
        deconstruct();
    }

    void deconstruct()
    {
        if (format_str != NULL)
        {
            xb_free(format_str);
            format_str = NULL;
        }
        if (jmp_str != NULL)
        {
            xb_free(jmp_str);
            jmp_str = NULL;
        }
        if (no_operand_str != NULL)
        {
            xb_free(no_operand_str);
            no_operand_str = NULL;
        }
        if (num_str != NULL)
        {
            xb_free(num_str);
            num_str = NULL;
        }
        if (num_str_format != NULL)
        {
			xb_free(num_str_format);
			num_str_format = NULL;
		}

        for (int i = 0; i < (sizeof(opcodes) / sizeof(OPCODE_VERSION_INFO)); i++)
        {
            opcodes[i].deconstruct();
        }
    }

    int getOpcodeStr(const UCHAR opcode, char*& opcode_str) const
    {
        for (int i = 0; i < (sizeof(opcodes) / sizeof(OPCODE_VERSION_INFO)); i++)
        {
            if (opcodes[i].opcode == opcode)
            {
				opcode_str = opcodes[i].str;
				return 0;
			}
		}
		return 1;
	}
} DECODE_SETTINGS;

typedef struct _LABEL {
    UINT offset = 0;
    char name[10] = { 0 };

    void load(UINT offset, const char* label)
    {
        this->offset = offset;
        if (label == NULL)
            return;
        
        if (strlen(label) < sizeof(name))
        {
            strcpy(name, label);
        }
        else
        {
            xb_cpy(name, label, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
        }        
    }

} LABEL;

typedef struct _DECODE_CONTEXT {
    XCODE* xcode;
    FILE* stream;
    DECODE_SETTINGS settings;
    std::list<LABEL*> labels;
    UINT base;

    _DECODE_CONTEXT(FILE* stream) : xcode(NULL), stream(stream), settings(), labels(), base(0) { }

    ~_DECODE_CONTEXT() {
        deconstruct();
    }

    void deconstruct()
    {
        // free labels
        for (LABEL* label : labels)
        {
            if (label != NULL)
            {
                xb_free(label);
                label = NULL;
            }
        }
        labels.clear();

        // free settings
        settings.deconstruct();
	}

    int getOpcodeStr(char*& opcode_str) const
    {
		return settings.getOpcodeStr(xcode->opcode, opcode_str);
	}
} DECODE_CONTEXT;

// XcodeInterp class
class XcodeInterp
{
public:
    XcodeInterp() {
		_size = 0;
        reset();
        _data = NULL;
    };

    // xcode interpreter status
    enum INTERP_STATUS : int {
        UNK =          -1, // unknown status
        DATA_OK =       0, // data ok. more data to interpret.
	    EXIT_OP_FOUND = 1, // exit opcode found
        DATA_ERROR =    2  // data error. end of data reached.
    };

    void deconstruct();                 // deconstruct the XCODE interpreter, free memory etc.

    int load(UCHAR* data, UINT size);   // load the XCODE data ( inittbl ) from data. returns 0 if success.    
    void reset();                       // reset the XCODE interpreter, set the status to UNK, set the offset to 0, set the ptr to NULL.
    
    int interpretNext(XCODE*& xcode);   // interpret the next XCODE. Stores it in xcode and cosumes it returns 0 if success.

    int getAddressStr(char* str, const char* format_str);   // get the address string. Stores the string in str.
    int getDataStr(char* str, const char* format_str);      // get the data string. Stores the string in str.
    int getCommentStr(char* str);                           // get the comment string. Stores the string in str.
    
    XCODE* getPtr() const { return _ptr; };                 // get the current position in the XCODE data.
    UINT getOffset() const { return _offset; };             // get the offset from the start of the data to the end of the current XCODE (offset to the next XCODE)
    INTERP_STATUS getStatus() const { return _status; };    // get the status of the xcode interpreter.
        
    // get default xcode opcode str from the provided opcode, stores it in opcode_str. returns 0 if success.
    int getDefOpcodeStr(const UCHAR opcode, char*& opcode_str) const;

    // decode the xcode, writes it to the stream. returns 0 if success.
    int decodeXcode(DECODE_CONTEXT& context);

    // load settings from the settings file.
    int loadDecodeSettings(DECODE_SETTINGS& settings) const;

private:
    UCHAR* _data;           // XCODE data
    UINT _size;		        // size of the XCODE data  
    XCODE* _ptr;            // current position in the XCODE data
    UINT _offset;           // offset from the start of the data to the end of the current XCODE (offset to the next XCODE)
    INTERP_STATUS _status;  // status of the xcode interpreter
    
    // opcode map
    OPCODE_VERSION_INFO opcodeMap[15] = {
        { "xc_nop", XC_NOP },
        { "xc_mem_read", XC_MEM_READ },
		{ "xc_mem_write", XC_MEM_WRITE },
		{ "xc_pci_write", XC_PCI_WRITE },
		{ "xc_pci_read", XC_PCI_READ },
		{ "xc_and_or", XC_AND_OR },
		{ "xc_use_result", XC_USE_RESULT },
		{ "xc_jne", XC_JNE },
		{ "xc_jmp",	 XC_JMP },
		{ "xc_accum", XC_ACCUM },
		{ "xc_io_write", XC_IO_WRITE },
		{ "xc_io_read",	XC_IO_READ },
		{ "xc_nop_80", XC_NOP_80 },
		{ "xc_exit", XC_EXIT },
		{ "xc_nop_f5", XC_NOP_F5 }
    };
};

// encode x86 code as xcode mem writes
int encodeX86AsMemWrites(UCHAR* data, UINT size, UCHAR*& buffer, UINT* bufferSize);

#endif // !XB_BIOS_XCODE_INTERP_H