#include "nt_headers.h"
#include "util.h"
#include "xbmem.h"

void print_image_file_header_basic(IMAGE_FILE_HEADER* file_header);
void print_image_file_header(IMAGE_FILE_HEADER* file_header);
void print_image_optional_header_basic(IMAGE_OPTIONAL_HEADER* optional_header);
void print_image_optional_header(IMAGE_OPTIONAL_HEADER* optional_header);

void print_image_dos_header(IMAGE_DOS_HEADER* dos_header)
{
    char magic[3] = { 0 };
    xb_cpy(magic, &dos_header->e_magic, 2);
    
    print("Magic:\t\t%s ( %02X )\n" \
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

    print("reserved 1:\t");
    for (int i = 0; i < 4; i++)
    {
		print("0x%x ", dos_header->e_res[i]);
	}
    print("\n");

    print("reserved 2:\t");
    for (int i = 0; i < 10; i++)
    {
        print("0x%x ", dos_header->e_res2[i]);
    }
    print("\n");
}
void print_krnl_data_section_header(IMAGE_DOS_HEADER* dos_header)
{
    DATA_SECTION_HEADER* data_section = (DATA_SECTION_HEADER*)&dos_header->e_res2;
	print("\nData Section Header:\n" \
            "uninitialized data:\t%d\n" \
    		"initialized data:\t%d\n" \
    		"data ptr:\t\t0x%04X\n" \
    		"virtual address:\t0x%04X\n",
        data_section->uninitializedDataSize, data_section->initializedDataSize, data_section->rawDataPtr, data_section->virtualAddr);
}
void print_image_file_header_basic(IMAGE_FILE_HEADER* file_header)
{
    char datetime[28] = { 0 };
    const char* machine_str = { 0 };

    switch (file_header->machine)
    {
        case 0x014C: machine_str = "X86 "; break;
        default: machine_str = "unk"; break;
    }

    getTimestamp(file_header->datetimeStamp, datetime);

    print("Machine:\t%s\nTimestamp:\t%s UTC\n", machine_str, datetime);
}
void print_image_file_header(IMAGE_FILE_HEADER* file_header)
{ 
    print_image_file_header_basic(file_header);

    print("Num sections:\t%d\nHeader size:\t%d\n", file_header->numSections, file_header->sizeOfOptionalHeader);

    if (file_header->symbolTablePtr != 0)
    {
        print("Symbol table ptr:\t0x%08X\n" \
            "Num symbols:\t0x%08X\n", file_header->symbolTablePtr, file_header->numSymbols);
    }
}
void print_image_optional_header_basic(IMAGE_OPTIONAL_HEADER* optional_header)
{
    const char* magic_str = NULL;

    switch (optional_header->magic)
    {
        case 0x010B: magic_str = "EXE"; break;
        default: magic_str = "unk"; break;
    }

    print("Magic:\t\t%s\n" \
        "Headers size:\t%d\nCode size:\t%d\nData size:\t%d\nImage size:\t%d\n" \
        "\nEntry point:\t0x%08X\nCode base:\t0x%08X\nData base:\t0x%08X\nImage base:\t0x%08X\n",
        magic_str, optional_header->headersSize, optional_header->codeSize,
        optional_header->initializedDataSize + optional_header->uninitializedDataSize,
        optional_header->imageSize, optional_header->addressOfEntryPoint, optional_header->baseOfCode,
        optional_header->baseOfData, optional_header->imageBase);
}
void print_image_optional_header(IMAGE_OPTIONAL_HEADER* optional_header)
{
    print_image_optional_header_basic(optional_header);

    print("Sect alignment:\t%d\nFile alignment:\t%d\n" \
        "\nImage version:\t%d.%d\nSubsystem:\t%d\n" \
        "\nStack reserve:\t%d bytes\nStack commit:\t%d bytes\nHeap reserve:\t%d bytes\nHeap commit:\t%d bytes\n",
        optional_header->sectionAlignment, optional_header->fileAlignment, optional_header->majorImageVersion,
        optional_header->minorImageVersion, optional_header->subsystem, optional_header->stackReserveSize,
        optional_header->stackCommitSize, optional_header->heapReserveSize, optional_header->heapCommitSize);
}
void print_nt_header(IMAGE_NT_HEADER* nt_header, bool basic)
{
    char magic[5] = { 0 };
    xb_cpy(magic, &nt_header->signature, 4);

    print("Magic:\t\t%s ( %04x )\n", magic, nt_header->signature);
    if (basic)
    {
		print_image_file_header_basic(&nt_header->file_header);
		print_image_optional_header_basic(&nt_header->optional_header);
	}
    else
    {
		print_image_file_header(&nt_header->file_header);
		print_image_optional_header(&nt_header->optional_header);
    }
}
