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

#include<list>

#include "bldr.h"
#include "type_defs.h"
#include "xbmem.h"

const UINT SMB_BASE = 0xC000;
const UINT NV2A_BASE = 0x0F000000;
const UINT NV2A_BASE_KERNEL = 0x0FD00000;
const UINT XCODE_BASE = sizeof(INIT_TBL);

// mcpx v1.1 xcode opcodes
enum OPCODE : UCHAR
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
    XC_EXIT =        0XEE,
};

typedef struct _LABEL {
    UINT offset = 0;
    char name[16] = { 0 };

    _LABEL(UINT offset, const char* label)
    {
        this->offset = offset;
        xb_cpy(this->name, label, sizeof(this->name));
    }

} LABEL;

// XcodeInterp class
class XcodeInterp
{
public:
    XcodeInterp() 
    {
		_size = 0;
        reset();
        _data = NULL;
    };
    ~XcodeInterp() 
    {
        unload();
	};

    // xcode interpreter status
    enum INTERP_STATUS : int
    {
        UNK =          -1,  // unknown status
        DATA_OK =       0,   // data ok. more data to interpret.
	    EXIT_OP_FOUND = 1,  // exit opcode found
        DATA_ERROR =    2    // data error. end of data reached.
    };

    int load(UCHAR* data, UINT size);               // load the XCODE data ( inittbl ) from data. returns 0 if success.    
    void unload();                                  // unload the XCODE data. returns 0 if success.
    int interpretNext(XCODE*& xcode);               // interpret the next XCODE. Stores it in xcode and cosumes it returns 0 if success.
    void reset();                                   // reset the XCODE interpreter.

    int decodeXcode(XCODE*& xcode, std::list<LABEL>& labels);
   
    int isOpcodeValid(UCHAR opcode);                        // check if the opcode is valid. returns 0 if the opcode does something.
    int getOpcodeStr(char* str);                            // get the opcode string. Stores the string in str.
    int getAddressStr(char* str);                           // get the address string. Stores the string in str.
    int getDataStr(char* str);                              // get the data string. Stores the string in str.
    int getCommentStr(char* str);                           // get the comment string. Stores the string in str.
    
    XCODE* getPtr() const { return _ptr; };                 // get the ptr. current position in the XCODE data.
    UINT getOffset() const { return _offset; };             // get the current offset from the start of the XCODE data.
    INTERP_STATUS getStatus() const { return _status; };    // get the status of the xcode interpreter.

    int decodeXcodes();
    int simulateXcodes();
private:
    UCHAR* _data;           // XCODE data
    UINT _size;		        // size of the XCODE data  
    XCODE* _ptr;            // current position in the XCODE data
    UINT _offset;           // offset from the start of the XCODE data.
    INTERP_STATUS _status;  // status of the xcode interpreter
};

#endif // !XB_BIOS_XCODE_INTERP_H