// nt_headers.h:

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_BIOS_TOOL_NT_IMAGE_H
#define XB_BIOS_TOOL_NT_IMAGE_H

#include "type_defs.h"

#define IMAGE_DOS_SIGNATURE 0x5A4D      // MZ
#define IMAGE_NT_SIGNATURE  0x00004550  // PE00

typedef struct {
    USHORT e_magic;
    USHORT e_cblp;
    USHORT e_cp;
    USHORT e_crlc;
    USHORT e_cparhdr;
    USHORT e_minalloc;
    USHORT e_maxalloc;
    USHORT e_ss;
    USHORT e_sp;
    USHORT e_csum;
    USHORT e_ip;
    USHORT e_cs;
    USHORT e_lfarlc;
    USHORT e_ovno;
    USHORT e_res[4];
    USHORT e_oemid;
    USHORT e_oeminfo;
    USHORT e_res2[10];
    UINT e_lfanew;
} IMAGE_DOS_HEADER;
typedef struct {
    USHORT machine;
    USHORT numSections;
    UINT datetimeStamp;
    UINT symbolTablePtr;
    UINT numSymbols;
    USHORT sizeOfOptionalHeader;
    USHORT characteristics;
} COFF_FILE_HEADER;
typedef struct {
    UINT address;
    UINT size;
} IMAGE_DATA_DIRECTORY;
typedef struct {
    USHORT magic;
    UCHAR majorLinkerVersion;
    UCHAR minorLinkerVersion;
    UINT codeSize;
    UINT initializedDataSize;
    UINT uninitializedDataSize;
    UINT addressOfEntryPoint;
    UINT baseOfCode;
    UINT baseOfData;
} IMAGE_OPTIONAL_HEADER_STD_FIELDS;
typedef struct {
    IMAGE_OPTIONAL_HEADER_STD_FIELDS std;
    UINT imageBase;
    UINT sectionAlignment;
    UINT fileAlignment;
    USHORT majorOperatingSystemVersion;
    USHORT minorOperatingSystemVersion;
    USHORT majorImageVersion;
    USHORT minorImageVersion;
    USHORT majorSubsystemVersion;
    USHORT minorSubsystemVersion;
    UINT win32VersionValue;
    UINT imageSize;
    UINT headersSize;
    UINT checksum;
    USHORT subsystem;
    USHORT characteristics;
    UINT stackReserveSize;
    UINT stackCommitSize;
    UINT heapReserveSize;
    UINT heapCommitSize;
    UINT loaderFlags;
    UINT numRvaAndSizes;
    IMAGE_DATA_DIRECTORY* dataDirectory;
} IMAGE_OPTIONAL_HEADER;
typedef struct {
    UINT signature;
    COFF_FILE_HEADER file_header;
    IMAGE_OPTIONAL_HEADER optional_header;
} IMAGE_NT_HEADER;
typedef struct {
    UCHAR name[8];
    UINT virtualSize;
    UINT virtualAddress;
    UINT rawDataSize;
    UINT pointerToRawData;
    UINT pointerToRelocations;
    UINT pointerToLineNumbers;
    USHORT numRelocations;
    USHORT numLineNumbers;
    UINT characteristics;
} IMAGE_SECTION_HEADER;
typedef struct {
    UINT uninitializedDataSize;
    UINT initializedDataSize;
    UINT rawDataPtr;
    UINT virtualAddr;
} DATA_SECTION_HEADER;

void print_krnl_data_section_header(IMAGE_DOS_HEADER* dos_header);

void print_image_dos_header(IMAGE_DOS_HEADER* dos_header);
void print_image_file_header(COFF_FILE_HEADER* file_header, bool basic);
void print_image_optional_header(IMAGE_OPTIONAL_HEADER* optional_header, bool basic);
void print_nt_headers(IMAGE_NT_HEADER* nt_header, bool basic);

IMAGE_DOS_HEADER* verify_dos_header(UCHAR* data, UINT size);
IMAGE_NT_HEADER* verify_nt_headers(UCHAR* data, UINT size);

int dump_nt_headers(UCHAR* data, UINT size, bool basic);

#endif
