// nt_headers.h:

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_BIOS_TOOL_NT_IMAGE_H
#define XB_BIOS_TOOL_NT_IMAGE_H

#include <stdint.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#define IMAGE_DOS_SIGNATURE 0x5A4D      // MZ
#define IMAGE_NT_SIGNATURE  0x00004550  // PE00

typedef struct {
    uint16_t e_magic;
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    uint32_t e_lfanew;
} IMAGE_DOS_HEADER;
typedef struct {
    uint16_t machine;
    uint16_t numSections;
    uint32_t datetimeStamp;
    uint32_t symbolTablePtr;
    uint32_t numSymbols;
    uint16_t sizeOfOptionalHeader;
    uint16_t characteristics;
} COFF_FILE_HEADER;
typedef struct {
    uint32_t address;
    uint32_t size;
} IMAGE_DATA_DIRECTORY;
typedef struct {
    uint16_t magic;
    uint8_t majorLinkerVersion;
    uint8_t minorLinkerVersion;
    uint32_t codeSize;
    uint32_t initializedDataSize;
    uint32_t uninitializedDataSize;
    uint32_t addressOfEntryPoint;
    uint32_t baseOfCode;
    uint32_t baseOfData;
} IMAGE_OPTIONAL_HEADER_STD_FIELDS;
typedef struct {
    IMAGE_OPTIONAL_HEADER_STD_FIELDS std;
    uint32_t imageBase;
    uint32_t sectionAlignment;
    uint32_t fileAlignment;
    uint16_t majorOperatingSystemVersion;
    uint16_t minorOperatingSystemVersion;
    uint16_t majorImageVersion;
    uint16_t minorImageVersion;
    uint16_t majorSubsystemVersion;
    uint16_t minorSubsystemVersion;
    uint32_t win32VersionValue;
    uint32_t imageSize;
    uint32_t headersSize;
    uint32_t checksum;
    uint16_t subsystem;
    uint16_t characteristics;
    uint32_t stackReserveSize;
    uint32_t stackCommitSize;
    uint32_t heapReserveSize;
    uint32_t heapCommitSize;
    uint32_t loaderFlags;
    uint32_t numRvaAndSizes;
    IMAGE_DATA_DIRECTORY* dataDirectory;
} IMAGE_OPTIONAL_HEADER;
typedef struct {
    uint32_t signature;
    COFF_FILE_HEADER file_header;
    IMAGE_OPTIONAL_HEADER optional_header;
} IMAGE_NT_HEADER;
typedef struct {
    uint8_t name[8];
    uint32_t virtualSize;
    uint32_t virtualAddress;
    uint32_t rawDataSize;
    uint32_t pointerToRawData;
    uint32_t pointerToRelocations;
    uint32_t pointerToLineNumbers;
    uint16_t numRelocations;
    uint16_t numLineNumbers;
    uint32_t characteristics;
} IMAGE_SECTION_HEADER;
typedef struct {
    uint32_t uninitializedDataSize;
    uint32_t initializedDataSize;
    uint32_t rawDataPtr;
    uint32_t virtualAddr;
} DATA_SECTION_HEADER;

#ifdef __cplusplus
extern "C" {
#endif

void print_krnl_data_section_header(IMAGE_DOS_HEADER* dos_header);

void print_image_dos_header(IMAGE_DOS_HEADER* dos_header);
void print_image_file_header(COFF_FILE_HEADER* file_header, bool basic);
void print_image_optional_header(IMAGE_OPTIONAL_HEADER* optional_header, bool basic);
void print_nt_headers(IMAGE_NT_HEADER* nt_header, bool basic);

IMAGE_DOS_HEADER* verify_dos_header(uint8_t* data, uint32_t size);
IMAGE_NT_HEADER* verify_nt_headers(uint8_t* data, uint32_t size);

int dump_nt_headers(uint8_t* data, uint32_t size, bool basic);

#ifdef __cplusplus
};
#endif

#endif
