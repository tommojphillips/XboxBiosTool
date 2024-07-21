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

const UINT XCODE_BASE = sizeof(INIT_TBL);

enum OPCODE : UCHAR
{
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
    XC_EXIT =        0XEE,

    XC_NOP =         0X00,
    XC_NOP_F5 =      0XF5,
    XC_NOP_80 =      0X80
};

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

    ~_OPCODE_VERSION_INFO() {
        deconstruct();
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

struct DECODE_SETTINGS
{
    char* format_str;
    char* jmp_str;
    char* no_operand_str;
    char* num_str;
    char* num_str_format;

    bool label_on_new_line;
    
    OPCODE_VERSION_INFO opcodes[15];

    DECODE_SETTINGS() : opcodes{}, format_str(NULL), jmp_str(NULL), no_operand_str(NULL), num_str(NULL), num_str_format(NULL), label_on_new_line(false) { }

    ~DECODE_SETTINGS() {
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
};

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

struct DECODE_CONTEXT
{
    XCODE*& xcode;
    FILE* stream;
    DECODE_SETTINGS settings;
    std::list<LABEL*> labels;

    DECODE_CONTEXT(XCODE*& xcode, FILE* stream) : xcode(xcode), stream(stream), settings(), labels() { }

    ~DECODE_CONTEXT() { 
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
};

// XcodeInterp class
class XcodeInterp
{
public:
    XcodeInterp() {
		_size = 0;
        reset();
        _data = NULL;
    };
    ~XcodeInterp() {
        deconstruct();
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

    int simulateXcodes();

    // decode the xcode, writes it to the stream. returns 0 if success.
    int decodeXcode(DECODE_CONTEXT& context);

    // load settings from the settings file.
    int loadini(DECODE_SETTINGS& settings) const;

private:
    UCHAR* _data;           // XCODE data
    UINT _size;		        // size of the XCODE data  
    XCODE* _ptr;            // current position in the XCODE data
    UINT _offset;           // offset from the start of the data to the end of the current XCODE (offset to the next XCODE)
    INTERP_STATUS _status;  // status of the xcode interpreter
    
    // opcode map
    OPCODE_VERSION_INFO opcodeMap[15] = {
        { "nop",        XC_NOP },
        { "mem_read",	XC_MEM_READ },
		{ "mem_write",	XC_MEM_WRITE },
		{ "pci_write",	XC_PCI_WRITE },
		{ "pci_read",	XC_PCI_READ },
		{ "and_or",		XC_AND_OR },
		{ "use_rslt",	XC_USE_RESULT },
		{ "jne",		XC_JNE },
		{ "jmp",		XC_JMP },
		{ "accum",		XC_ACCUM },
		{ "io_write",	XC_IO_WRITE },
		{ "io_read",	XC_IO_READ },
		{ "nop_80",		XC_NOP_80 },
		{ "exit",		XC_EXIT },
		{ "nop_f5",		XC_NOP_F5 }
    };

};

#endif // !XB_BIOS_XCODE_INTERP_H