// bldr.h: Implements Original Xbox Bootloader structures and constants.

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

#ifndef XB_BLDR_H
#define XB_BLDR_H

#include "type_defs.h"

const UINT KEY_SIZE = 16;
const UINT BOOT_PARAMS_SIGNATURE = MAKE_4BYTE_SIGNATURE('J', 'y', 'T', 'x');

// The init table structure.
typedef struct {
    UINT ptr1;
    UINT ptr2;
    UINT header;
    UINT val1;
    UINT val2;
    UINT val3;
    UINT val4;
    UINT val5;
    UINT val6;
    UINT val7;
    UINT val8;
    UINT val9;
    UINT val10;
    UINT vals[14];
    USHORT revision;
    USHORT val11;
    UINT val12;
    UINT val13;
    USHORT init_tbl_identifier;
    USHORT kernel_ver;
    UINT data_tbl_offset;
} INIT_TBL; static_assert(sizeof(INIT_TBL) == 128, "INIT_TBL struct size is not 128 bytes");

#pragma pack(push, 1)
// XCODE operand struct.
typedef struct {
    UCHAR opcode;   // al
    UINT addr;      // ebx
    UINT data;      // ecx
} XCODE; static_assert(sizeof(XCODE) == 9, "XCODE struct size is not 9 bytes");
#pragma pack(pop)

// The boot parameters structure.
typedef struct {
    UINT krnlDataSize;  // size of the kernel data section (uncompressed)
    UINT inittblSize;   // size of the init table	
    UINT signature;
    UINT krnlSize;      // size of the kernel (compressed)
    UCHAR digest[20];   // the ROM digest.
} BOOT_PARAMS;

// The loader parameters structure.
typedef struct {
    UINT bldrEntryPoint;    // 32bit entry point
    char cli[64];           // command line
} BOOT_LDR_PARAM;

// drive / slew calibration parameters
typedef struct {
    USHORT max_m_clk;

    UCHAR slow_count_ext;
    UCHAR slow_countB_ext;

    UCHAR slow_count_avg;
    UCHAR slow_countB_avg;
    
    UCHAR typi_count;
    UCHAR typi_countB;

    UCHAR fast_count_avg;
    UCHAR fast_countB_avg;
    
    UCHAR fast_count_ext;
    UCHAR fast_countB_ext;

} DRV_SLW_CAL_PARAMS;

// drive / slew pad parameters
typedef struct {
    UCHAR adr_drv_fall;
    UCHAR adr_drv_rise;

    UCHAR adr_slw_fall;
    UCHAR adr_slw_rise;

    UCHAR clk_drv_fall;
    UCHAR clk_drv_rise;

    UCHAR clk_slw_fall;
    UCHAR clk_slw_rise;

    UCHAR dat_drv_fall;
    UCHAR dat_drv_rise;

    UCHAR dat_slw_fall;
    UCHAR dat_slw_rise;

    UCHAR dqs_drv_fall;
    UCHAR dqs_drv_rise;

    UCHAR dqs_slw_fall;
    UCHAR dqs_slw_rise;

    UCHAR dat_inb_delay;
    UCHAR clk_ic_delay;
    UCHAR dqs_inb_delay;

} DRV_SLW_PAD_PARAMS;

// data table as found in the bios
typedef struct {
    DRV_SLW_CAL_PARAMS cal;         // calibration parameters
    DRV_SLW_PAD_PARAMS samsung[5];  // samsung memory parameters
    DRV_SLW_PAD_PARAMS micron[5];   // micron memory parameters
} ROM_DATA_TBL;

// The bldr keys structure as found in the bios
typedef struct {
    UCHAR bfmKey[KEY_SIZE];     // bfm key; this is used for re-encryption of the 2bl to restore it's original state
    UCHAR eepromKey[KEY_SIZE];  // eeprom key
    UCHAR certKey[KEY_SIZE];    // certificate key
    UCHAR krnlKey[KEY_SIZE];    // kernel key
} BLDR_KEYS;

// The bldr entry structure as found in the bios
typedef struct {
    UINT keysPtr;          // rc4 crypto keys pointer
    UINT bfmEntryPoint;    // bldr entry point
} BLDR_ENTRY;

typedef struct {
    UINT jmpInstr;		// jmp opcode + rel offset
    UINT reserved;
    UINT funcBlockPtr;	// address to sha func block. should equal PRELDR_ENTRY::funcBlockPtr
    UINT pubKeyPtr;		// always 0.
} PRELDR_PARAMS;

typedef struct {
    UINT pubKeyPtr;		// address to the encrypted public key in the preldr	
    UINT funcBlockPtr;	// address to sha func block. should equal PRELDR_PARAMS::funcBlockPtr
} PRELDR_ENTRY;

#endif // !XBT_BLDR_H
