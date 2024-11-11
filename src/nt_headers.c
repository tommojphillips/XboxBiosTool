// nt_headers.cpp: 

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

// std incl
#include <stdint.h>
#include <stdio.h>
#include <memory.h>

// user incl
#include "nt_headers.h"
#include "util.h"

void print_image_dos_header(IMAGE_DOS_HEADER* dos_header)
{
    char magic[3] = { 0 };
    memcpy(magic, &dos_header->e_magic, 2);
    
    printf("Magic:\t\t%s ( %02X )\n" \
        "cblp:\t\t0x%02x\n" \
        "cp:\t\t0x%02x\n" \
        "crlc:\t\t0x%02x\n" \
        "cparhdr:\t0x%02x\n" \
        "alloc:\t\t0x%02x - 0x%02x\n" \
        "\nss:\t\t0x%02x\n" \
        "sp:\t\t0x%02x\n" \
        "ip:\t\t0x%02x\n" \
        "cs:\t\t0x%02x\n" \
        
        "\nchecksum:\t0x%02x\n" \
        "lfarlc:\t\t0x%02x\n" \
        "ovno:\t\t0x%02x\n" \
        
        "oem id:\t\t0x%02x\n" \
        "oem info:\t0x%02x\n" \
        
        "nt header ptr:\t0x%02x\n", \
        &magic, dos_header->e_magic, dos_header->e_cblp, dos_header->e_cp, dos_header->e_crlc, dos_header->e_cparhdr,
        dos_header->e_minalloc, dos_header->e_maxalloc, dos_header->e_ss, dos_header->e_sp,
        dos_header->e_ip, dos_header->e_cs, dos_header->e_csum, dos_header->e_lfarlc, dos_header->e_ovno, dos_header->e_oemid,
        dos_header->e_oeminfo, dos_header->e_lfanew);

    printf("reserved 1:\t");
    for (int i = 0; i < 4; i++)
    {
		printf("0x%x ", dos_header->e_res[i]);
	}
    printf("\n");

    printf("reserved 2:\t");
    for (int i = 0; i < 10; i++)
    {
        printf("0x%x ", dos_header->e_res2[i]);
    }
    printf("\n");
}
void print_krnl_data_section_header(IMAGE_DOS_HEADER* dos_header)
{
    DATA_SECTION_HEADER* data_section = (DATA_SECTION_HEADER*)&dos_header->e_res2;
	printf("\nData Section Header:\n" \
            "uninitialized data:\t%d\n" \
    		"initialized data:\t%d\n" \
    		"data ptr:\t\t0x%04X\n" \
    		"virtual address:\t0x%04X\n",
        data_section->uninitializedDataSize, data_section->initializedDataSize, data_section->rawDataPtr, data_section->virtualAddr);
}

void print_image_file_header(COFF_FILE_HEADER* file_header, bool basic)
{ 
    char datetime[28] = { 0 };
    const char* machine_str = { 0 };

    switch (file_header->machine)
    {
    case 0x014C: machine_str = "X86 "; break;
    default: machine_str = "unk"; break;
    }

    util_getTimestampStr(file_header->datetimeStamp, datetime);

    printf("\nFile Header:\nMachine:\t%s ( %X )\nTimestamp:\t%s UTC\n", machine_str, file_header->machine, datetime);

    if (basic)
        return;

    printf("Num sections:\t%d\nHeader size: %X\n", file_header->numSections, file_header->sizeOfOptionalHeader);

    if (file_header->symbolTablePtr != 0)
    {
        printf("Symbol table ptr:\t0x%08X\n" \
            "Num symbols:\t0x%08X\n", file_header->symbolTablePtr, file_header->numSymbols);
    }
}

void print_image_optional_header(IMAGE_OPTIONAL_HEADER* optional_header, bool basic)
{
    const char* magic_str = NULL;
    const char* subsys_str = NULL;

    switch (optional_header->std.magic)
    {
    case 0x010B: magic_str = "PE32"; break;
    case 0x020B: magic_str = "PE32+"; break;
    default: magic_str = "unk"; break;
    }

    switch (optional_header->subsystem)
	{
		case 0xE: subsys_str = "xbox"; break;
		case 0x3: subsys_str = "windows console"; break;
        default: subsys_str = "unk"; break;
	}

    printf("\nOptional Header:\nMagic:\t\t%s ( %X )\n" \
        "Headers size:\t%d\nCode size:\t%d\nData size:\t%d\nImage size:\t%d\n" \
        "\nEntry point:\t0x%X ( 0x%X ) main\nCode base:\t0x%X\nData base:\t0x%X\nImage base:\t0x%X\n",
        magic_str, optional_header->std.magic, optional_header->headersSize, optional_header->std.codeSize,
        optional_header->std.initializedDataSize + optional_header->std.uninitializedDataSize,
        optional_header->imageSize, optional_header->std.addressOfEntryPoint, optional_header->imageBase + optional_header->std.addressOfEntryPoint, optional_header->std.baseOfCode,
        optional_header->std.baseOfData, optional_header->imageBase);

    if (basic)
        return;

    printf("Sect alignment:\t0x%x\nFile alignment:\t0x%x\n" \
        "\nImage version:\t%d.%d\nOperating sys:\t%d.%d\nSubsystem:\t%s ( %X )\n" \
        "\nStack reserve:\t0x%X\nStack commit:\t0x%X\nHeap reserve:\t0x%X\nHeap commit:\t0x%X\n",
        optional_header->sectionAlignment, optional_header->fileAlignment, optional_header->majorImageVersion,
        optional_header->minorImageVersion, optional_header->majorOperatingSystemVersion, optional_header->minorOperatingSystemVersion, subsys_str, optional_header->subsystem, optional_header->stackReserveSize,
        optional_header->stackCommitSize, optional_header->heapReserveSize, optional_header->heapCommitSize);
}

void print_nt_headers(IMAGE_NT_HEADER* nt_header, bool basic)
{
    print_image_file_header(&nt_header->file_header, basic);
    print_image_optional_header(&nt_header->optional_header, basic);
}

int dump_nt_headers(uint8_t* data, uint32_t size, bool basic)
{
    IMAGE_NT_HEADER* nt = verify_nt_headers(data, size);
    if (nt == NULL)
	{
		return 1;
	}

    printf("PE signature found\n");

    print_nt_headers(nt, basic);

    return 0;
}

IMAGE_DOS_HEADER* verify_dos_header(uint8_t* data, uint32_t size)
{
    if (data == NULL)
    {
        printf("Error: Invalid data\n");
        return NULL;
    }

    IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)data;
    if (IN_BOUNDS(dosHeader, data, size) == false)
    {
        printf("Error: DOS header out of bounds\n");
        return NULL;
    }

    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        printf("Error: Invalid DOS signature\n");
        return NULL;
    }

    return dosHeader;
}
IMAGE_NT_HEADER* verify_nt_headers(uint8_t* data, uint32_t size)
{
    IMAGE_DOS_HEADER* dos = NULL;
    IMAGE_NT_HEADER* nt = NULL;

    if (data == NULL)
    {
        printf("Error: Invalid data\n");
        return NULL;
    }

    dos = verify_dos_header(data, size);
    if (dos == NULL)
    {
        return NULL;
    }

    nt = (IMAGE_NT_HEADER*)(data + dos->e_lfanew);
    if (IN_BOUNDS(nt, data, size) == false)
    {
        printf("Error: NT headers out of bounds\n");
        return NULL;
    }

    if (nt->signature != IMAGE_NT_SIGNATURE)
    {
        printf("Error: Invalid PE signature\n");
        return NULL;
    }

    return nt;
}
