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

#include <stdint.h>

#define XB_KEY_SIZE 16

typedef struct _DRW_SLW_COUNTS {
    uint8_t countA;
    uint8_t countB;
} DRV_SLW_COUNTS;

typedef struct _DRV_SLW_CAL_PARAMS {
    uint16_t max_m_clk;
    DRV_SLW_COUNTS slowest;
    DRV_SLW_COUNTS slow;
    DRV_SLW_COUNTS standard;
    DRV_SLW_COUNTS fast;
    DRV_SLW_COUNTS fastest;
} DRV_SLW_CAL_PARAMS;

typedef struct _DRV_SLW_PAD_PARAMS {
    uint8_t adr_drv_fall;
    uint8_t adr_drv_rise;
    uint8_t adr_slw_fall;
    uint8_t adr_slw_rise;
    uint8_t clk_drv_fall;
    uint8_t clk_drv_rise;
    uint8_t clk_slw_fall;
    uint8_t clk_slw_rise;
    uint8_t dat_drv_fall;
    uint8_t dat_drv_rise;
    uint8_t dat_slw_fall;
    uint8_t dat_slw_rise;
    uint8_t dqs_drv_fall;
    uint8_t dqs_drv_rise;
    uint8_t dqs_slw_fall;
    uint8_t dqs_slw_rise;
    uint8_t dat_inb_delay;
    uint8_t clk_ic_delay;
    uint8_t dqs_inb_delay;
} DRV_SLW_PAD_PARAMS;

// init table structure.
typedef struct _INIT_TBL {
    uint32_t ptr1;
    uint32_t ptr2;
    uint32_t header;
    uint32_t val1;
    uint32_t val2;
    uint32_t val3;
    uint32_t val4;
    uint32_t val5;
    uint32_t val6;
    uint32_t val7;
    uint32_t val8;
    uint32_t val9;
    uint32_t val10;
    uint32_t vals[14];
    uint16_t revision;
    uint16_t val11;
    uint32_t val12;
    uint32_t val13;
    uint16_t init_tbl_identifier;
    uint16_t kernel_ver;
    uint32_t data_tbl_offset;
} INIT_TBL;

// rom table structure.
typedef struct _ROM_DATA_TBL {
    DRV_SLW_CAL_PARAMS cal;
    DRV_SLW_PAD_PARAMS samsung[5];
    DRV_SLW_PAD_PARAMS micron[5];
} ROM_DATA_TBL;

// xcode structure.
#pragma pack(push, 1)
typedef struct _XCODE {
    uint8_t opcode;
    uint32_t addr;
    uint32_t data;
} XCODE;
#pragma pack(pop)

// loader parameters structure.
typedef struct _BOOT_LDR_PARAMS {
    uint32_t bldrEntryPoint;
    char cli[64];
} BOOT_LDR_PARAM;

// boot params structure.
typedef struct _BOOT_PARAMS {
    uint32_t krnlDataSize;
    uint32_t inittblSize;
    uint32_t signature;
    uint32_t krnlSize;
    uint8_t digest[20];
} BOOT_PARAMS;

// 2BL keys structure
typedef struct _BLDR_KEYS {
    uint8_t eepromKey[XB_KEY_SIZE];
    uint8_t certKey[XB_KEY_SIZE];
    uint8_t krnlKey[XB_KEY_SIZE];
} BLDR_KEYS;

// 2BL entry structure
typedef struct _BLDR_ENTRY {
    uint32_t keysPtr;
    uint32_t bfmEntryPoint;
} BLDR_ENTRY;

// Preldr parameters structure
#pragma pack(push, 1)
typedef struct _PRELDR_PARAMS {
    uint8_t jmpOpcode;
    uint32_t jmpOffset;
    uint8_t pad[3];
    uint32_t funcBlockPtr;
} PRELDR_PARAMS;
#pragma pack(pop)

// Preldr pointers structure
typedef struct _PRELDR_FUNC_PTRS {
    uint32_t pubKeyPtr;
    uint32_t funcBlockPtr;
} PRELDR_FUNC_PTRS;

// Preldr function block structure
typedef struct _PRELDR_FUNC_BLOCK {
    uint32_t shaInitFuncPtr;
    uint32_t shaUpdateFuncPtr;
    uint32_t shaResultFuncPtr;
    uint32_t verifyDigestFuncPtr;
} PRELDR_FUNC_BLOCK;

// Preldr entry structure
typedef struct _PRELDR_ENTRY {
    uint32_t bldrEntryOffset;
    uint32_t bfmEntryPoint;
} PRELDR_ENTRY;

#endif // !XB_BLDR_H
