// bldr.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_BLDR_H
#define XB_BLDR_H

#include "type_defs.h"

#define KEY_SIZE	            16      // rc4 key size in bytes
#define DIGEST_LEN              20

#define MCPX_BLOCK_SIZE         512     // mcpx southbridge chip rom size in bytes
#define BLDR_BLOCK_SIZE         24576   // bldr block size in bytes
#define PRELDR_BLOCK_SIZE       10752   // preldr dev, mcpx v1.1 block size in bytes
#define PRELDR_XM3_BLOCK_SIZE   2048    // preldr mcpx v1.0 block size in bytes

#define DEFAULT_ROM_SIZE	    256     // default rom size in KB

const UCHAR BOOT_PARAMS_SIGNATURE[4] = { 'J', 'y', 'T', 'x' }; // hash of "JyTx". MCPX checks this to verify the bldr.
const UINT BLDR_RELOC = 0x00400000;
const UINT BLDR_BASE =  0x00090000;

// The init table structure.
typedef struct
{
    // Northbridge data for NV2A starts at offset+0
    UINT ptr1;          // 0:0. nv2a init table ptr 1. ptr to the ROM tbl
    UINT ptr2;          // 0:4. nv2a init table ptr 2. ptr to the ROM tbl
    UINT header;        // 0:8. northbridge boot header

    UINT val1;          // 0:12.
    UINT val2;          // 0:16.
    UINT val3;          // 0:20.
    UINT val4;          // 0:24.
    UINT val5;          // 0:28.
    UINT val6;          // 0:32.
    UINT val7;          // 0:36.
    UINT val8;          // 0:40.
    UINT val9;          // 0:44.
    UINT val10;         // 0:48.
    UINT vals[14];      // 0:52 - 0:104.

    USHORT revision;   // 0:108. initializtion table revision

    USHORT val11;      // 0:110.
    UINT val12;        // 0:112. MCP_ROM_BOOT_FREQ_STRAP
    UINT val13;        // 0:116. changes depending if dvt4, retail, or dvt6

    USHORT init_tbl_identifier; // 0:120. init table identifier code
    USHORT kernel_ver;          // 0:122. kernel version
    USHORT data_tbl_offset;     // 0:124. offset from ROM base to the data tbl
    USHORT val14;               // 0:126. fill value
} INIT_TBL; static_assert(sizeof(INIT_TBL) == 128, "INIT_TBL struct size is not 128 bytes");

#pragma pack(push, 1)
// XCODE operand struct.
typedef struct _XCODE
{
    UCHAR opcode;   // al
    UINT addr;      // ebx
    UINT data;      // ecx
} XCODE; static_assert((sizeof(XCODE) == 9), "XCODE struct size is not 9 bytes");
#pragma pack(pop)

// The boot parameters structure.
typedef struct
{
    ULONG krnlDataSize;         // size of the kernel data (uncompressed)
    ULONG inittblSize;          // size of the init table	
    UCHAR signature[4];         // the bldr signature. mcpx checks this to verify the bldr
    ULONG krnlSize;             // size of the kernel (compressed)
    UCHAR digest[DIGEST_LEN];   // the ROM digest.
} BOOT_PARAMS;

// The loader parameters structure.
typedef struct
{
    ULONG bldrEntryPoint;   // 32bit entry point
    char cli[64];           // command line
} BOOT_LDR_PARAM;

// drive / slew calibration parameters
typedef struct
{
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
typedef struct
{
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
typedef struct
{
    DRV_SLW_CAL_PARAMS cal;         // calibration parameters
    DRV_SLW_PAD_PARAMS samsung[5];  // samsung memory parameters
    DRV_SLW_PAD_PARAMS micron[5];   // micron memory parameters
} ROM_DATA_TBL;

// The bldr keys structure as found in the bios
typedef struct
{
    UCHAR eepromKey[KEY_SIZE];  // eeprom key
    UCHAR certKey[KEY_SIZE];    // certificate key
    UCHAR krnlKey[KEY_SIZE];    // kernel key
} BLDR_KEYS;

// The bldr entry structure as found in the bios
typedef struct
{
    ULONG keysPtr;          // rc4 crypto keys pointer
    ULONG bfmEntryPoint;    // bldr entry point
} BLDR_ENTRY;

// The rsa header structure
typedef struct
{
    char magic[4];
    UINT mod_size;
    UINT bits;
    UINT max_bytes;
    UINT exponent;
} RSA_HEADER;

// The public key structure
typedef struct
{
    RSA_HEADER header;
    UCHAR modulus[264];
} PUBLIC_KEY;

typedef struct {
    UINT jmpInstr;		// jmp opcode + rel offset
    UINT reserved;
    UINT funcBlockPtr;	// address to sha func block. should equal PRELDR_ENTRY::shaFuncBlockPtr
    UINT pubKeyPtr;		// always 0.
} PRELDR_PARAMS;

typedef struct {
    UINT pubKeyPtr;		// address to the encrypted public key in the preldr	
    UINT funcBlockPtr;	// address to sha func block. should equal PRELDR_PARAMS::shaFuncBlockPtr
} PRELDR_ENTRY;

#endif // !XBT_BLDR_H
