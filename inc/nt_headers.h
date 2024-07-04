// nt_headers.h: contains the definition of the IMAGE_DOS_HEADER, IMAGE_FILE_HEADER, IMAGE_OPTIONAL_HEADER, and IMAGE_NT_HEADER structures. 
// It also contains the print_image_dos_header, print_image_file_header, print_optional_header, and print_nt_header functions.
// These functions are used to print the contents of the IMAGE_DOS_HEADER, IMAGE_FILE_HEADER, IMAGE_OPTIONAL_HEADER, and IMAGE_NT_HEADER structures,
// respectively. The print function is used to print the contents of the structures to the console.

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_BIOS_TOOL_NT_IMAGE_H
#define XB_BIOS_TOOL_NT_IMAGE_H

#include "type_defs.h"

#define IMAGE_DOS_SIGNATURE 0x5A4D      // MZ
#define IMAGE_NT_SIGNATURE  0x00004550  // PE00

typedef struct _IMAGE_DOS_HEADER {        // DOS .EXE header
    USHORT   e_magic;                     // Magic number
    USHORT   e_cblp;                      // Bytes on last page of file
    USHORT   e_cp;                        // Pages in file
    USHORT   e_crlc;                      // Relocations
    USHORT   e_cparhdr;                   // Size of header in paragraphs
    USHORT   e_minalloc;                  // Minimum extra paragraphs needed
    USHORT   e_maxalloc;                  // Maximum extra paragraphs needed
    USHORT   e_ss;                        // Initial (relative) SS value
    USHORT   e_sp;                        // Initial SP value
    USHORT   e_csum;                      // Checksum
    USHORT   e_ip;                        // Initial IP value
    USHORT   e_cs;                        // Initial (relative) CS value
    USHORT   e_lfarlc;                    // File address of relocation table
    USHORT   e_ovno;                      // Overlay number
    USHORT   e_res[4];                    // Reserved words
    USHORT   e_oemid;                     // OEM identifier (for e_oeminfo)
    USHORT   e_oeminfo;                   // OEM information; e_oemid specific
    USHORT   e_res2[10];                  // Reserved words
    long   e_lfanew;                      // File address of new exe header
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

static void print_image_dos_header(IMAGE_DOS_HEADER* dos_header)
{
	print("e_magic: %x\n" \
		"e_cblp: %x\n" \
		"e_cp: %x\n" \
		"e_crlc: %x\n" \
		"e_cparhdr: %x\n" \
		"e_minalloc: %x\n" \
		"e_maxalloc: %x\n" \
		"e_ss: %x\n" \
		"e_sp: %x\n" \
		"e_csum: %x\n" \
		"e_ip: %x\n" \
		"e_cs: %x\n" \
		"e_lfarlc: %x\n" \
		"e_ovno: %x\n" \
		"e_oemid: %x\n" \
		"e_oeminfo: %x\n" \
		"e_lfanew: %x\n", \
		dos_header->e_magic, dos_header->e_cblp, dos_header->e_cp, dos_header->e_crlc, dos_header->e_cparhdr,
        dos_header->e_minalloc, dos_header->e_maxalloc, dos_header->e_ss, dos_header->e_sp, dos_header->e_csum,
        dos_header->e_ip, dos_header->e_cs, dos_header->e_lfarlc, dos_header->e_ovno, dos_header->e_oemid,
        dos_header->e_oeminfo, dos_header->e_lfanew);
}
static void print_image_file_header_basic(IMAGE_FILE_HEADER* file_header)
{
    char datetime[28] = { 0 };
    char machine_str[64] = { 0 };

    switch (file_header->machine)
    {
        case 0x014C: strcpy(machine_str, "X86 "); break;
        default: format(machine_str, "Unk (%04X) ", file_header->machine); break;
    }

    getTimestamp(file_header->datetimeStamp, datetime);

    print("Machine:\t%s\nTimestamp:\t%s UTC\n", machine_str, datetime);
}
static void print_image_file_header(IMAGE_FILE_HEADER* file_header)
{
    print_image_file_header_basic(file_header);

    print("NumberOfSections:\t%d\nSize of New header:\t0x%X\n", file_header->numSections, file_header->sizeOfOptionalHeader);

    if (file_header->symbolTablePtr != 0)
    {
        print("PointerToSymbolTable:\t0x%08X\n" \
            "NumberOfSymbols:\t0x%08X\n", file_header->symbolTablePtr, file_header->numSymbols);
    }
}
static void print_optional_header_basic(IMAGE_OPTIONAL_HEADER* optional_header)
{
    char magic_str[32] = { 0 };

    switch (optional_header->magic)
    {
        case 0x010B: strcpy(magic_str, "EXE"); break;
        case 0x0107: strcpy(magic_str, "ROM"); break;
    }

    print("Magic:\t\t%s (%04X)\n" \
        "Headers Size:\t%d\nCode Size:\t%d\nData Size:\t%d\nImage Size:\t%d\n" \
        "\nEntry Point:\t0x%08X\nCode Base:\t0x%08X\nData Base:\t0x%08X\nImage Base:\t0x%08X\n",
        magic_str, optional_header->magic, optional_header->headersSize, optional_header->codeSize,
        optional_header->initializedDataSize + optional_header->uninitializedDataSize,
        optional_header->imageSize, optional_header->addressOfEntryPoint, optional_header->baseOfCode,
        optional_header->baseOfData, optional_header->imageBase);
}
static void print_optional_header(IMAGE_OPTIONAL_HEADER* optional_header)
{    
    print_optional_header_basic(optional_header);

    print("Section Alignment:\t0x%08X\nFile Alignment:\t0x%08X\n" \
        "\nImage Version:\t%d.%d\nSubsystem:\t%d\n" \
        "\nStack Reserve:\t0x%08X\nStack Commit:\t0x%08X\nHeap Reserve:\t0x%08X\nHeap Commit:\t0x%08X\n\n",
        optional_header->sectionAlignment, optional_header->fileAlignment, optional_header->majorImageVersion, 
        optional_header->minorImageVersion, optional_header->subsystem, optional_header->stackReserveSize, 
        optional_header->stackCommitSize, optional_header->heapReserveSize, optional_header->heapCommitSize);
}
static void print_nt_header(IMAGE_NT_HEADER* nt_header)
{
    print("NT header signature:\t%s\n", &nt_header->signature);
    print_image_file_header(&nt_header->file_header);
    print_optional_header(&nt_header->optional_header);
}
static void print_nt_header_basic(IMAGE_NT_HEADER* nt_header)
{
    print("Signature:\t%s\n", &nt_header->signature);
    print_image_file_header_basic(&nt_header->file_header);
    print_optional_header_basic(&nt_header->optional_header);
    print("\n");
}

#endif
