// nt_headers.h:

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_BIOS_TOOL_NT_IMAGE_H
#define XB_BIOS_TOOL_NT_IMAGE_H

#include "type_defs.h"

#define IMAGE_DOS_SIGNATURE 0x5A4D      // MZ
#define IMAGE_NT_SIGNATURE  0x00004550  // PE00

typedef struct {            // DOS .EXE header
    USHORT   e_magic;       // Magic number
    USHORT   e_cblp;        // Bytes on last page of file
    USHORT   e_cp;          // Pages in file
    USHORT   e_crlc;        // Relocations
    USHORT   e_cparhdr;     // Size of header in paragraphs
    USHORT   e_minalloc;    // Minimum extra paragraphs needed
    USHORT   e_maxalloc;    // Maximum extra paragraphs needed
    USHORT   e_ss;          // Initial (relative) SS value
    USHORT   e_sp;          // Initial SP value
    USHORT   e_csum;        // Checksum
    USHORT   e_ip;          // Initial IP value
    USHORT   e_cs;          // Initial (relative) CS value
    USHORT   e_lfarlc;      // File address of relocation table
    USHORT   e_ovno;        // Overlay number
    USHORT   e_res[4];      // Reserved words
    USHORT   e_oemid;       // OEM identifier (for e_oeminfo)
    USHORT   e_oeminfo;     // OEM information; e_oemid specific
    USHORT   e_res2[10];    // Reserved words
    long   e_lfanew;        // File address of new exe header
} IMAGE_DOS_HEADER;
typedef struct
{
    USHORT  machine;
    USHORT  numSections;
    ULONG   datetimeStamp;
    ULONG   symbolTablePtr;
    ULONG   numSymbols;
    USHORT  sizeOfOptionalHeader;
    USHORT  characteristics;
} IMAGE_FILE_HEADER;
typedef struct
{
    ULONG address;
    ULONG size;
} IMAGE_DATA_DIRECTORY;
typedef struct
{
    USHORT  magic;
    UCHAR   majorLinkerVersion;
    UCHAR   minorLinkerVersion;
    ULONG   codeSize;
    ULONG   initializedDataSize;
    ULONG   uninitializedDataSize;
    ULONG   addressOfEntryPoint;
    ULONG   baseOfCode;
    ULONG   baseOfData;
    ULONG   imageBase;
    ULONG   sectionAlignment;
    ULONG   fileAlignment;
    USHORT  majorOperatingSystemVersion;
    USHORT  minorOperatingSystemVersion;
    USHORT  majorImageVersion;
    USHORT  minorImageVersion;
    USHORT  majorSubsystemVersion;
    USHORT  minorSubsystemVersion;
    ULONG   win32VersionValue;
    ULONG   imageSize;
    ULONG   headersSize;
    ULONG   checksum;
    USHORT  subsystem;
    USHORT  characteristics;
    ULONG   stackReserveSize;
    ULONG   stackCommitSize;
    ULONG   heapReserveSize;
    ULONG   heapCommitSize;
    ULONG   loaderFlags;
    ULONG   numRvaAndSizes;
    IMAGE_DATA_DIRECTORY dataDirectory[16];
} IMAGE_OPTIONAL_HEADER;
typedef struct
{
    ULONG signature;
    IMAGE_FILE_HEADER file_header;
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

void print_image_dos_header(IMAGE_DOS_HEADER* dos_header);
void print_image_file_header_basic(IMAGE_FILE_HEADER* file_header);
void print_image_file_header(IMAGE_FILE_HEADER* file_header);
void print_optional_header_basic(IMAGE_OPTIONAL_HEADER* optional_header);
void print_optional_header(IMAGE_OPTIONAL_HEADER* optional_header);
void print_nt_header_basic(IMAGE_NT_HEADER* nt_header);
void print_nt_header(IMAGE_NT_HEADER* nt_header);

#endif
