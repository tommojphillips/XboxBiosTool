#include "nt_headers.h"
#include "util.h"

void print_image_dos_header(IMAGE_DOS_HEADER* dos_header)
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

    print("NumberOfSections:\t%d\nSize of New header:\t0x%X\n", file_header->numSections, file_header->sizeOfOptionalHeader);

    if (file_header->symbolTablePtr != 0)
    {
        print("PointerToSymbolTable:\t0x%08X\n" \
            "NumberOfSymbols:\t0x%08X\n", file_header->symbolTablePtr, file_header->numSymbols);
    }
}
void print_optional_header_basic(IMAGE_OPTIONAL_HEADER* optional_header)
{
    const char* magic_str = NULL;

    switch (optional_header->magic)
    {
        case 0x010B: magic_str = "EXE"; break;
        default: magic_str = "unk"; break;
    }

    print("Magic:\t\t%s\n" \
        "Headers Size:\t%d\nCode Size:\t%d\nData Size:\t%d\nImage Size:\t%d\n" \
        "\nEntry Point:\t0x%08X\nCode Base:\t0x%08X\nData Base:\t0x%08X\nImage Base:\t0x%08X\n",
        magic_str, optional_header->headersSize, optional_header->codeSize,
        optional_header->initializedDataSize + optional_header->uninitializedDataSize,
        optional_header->imageSize, optional_header->addressOfEntryPoint, optional_header->baseOfCode,
        optional_header->baseOfData, optional_header->imageBase);
}
void print_optional_header(IMAGE_OPTIONAL_HEADER* optional_header)
{
    print_optional_header_basic(optional_header);

    print("Section Alignment:\t0x%08X\nFile Alignment:\t0x%08X\n" \
        "\nImage Version:\t%d.%d\nSubsystem:\t%d\n" \
        "\nStack Reserve:\t0x%08X\nStack Commit:\t0x%08X\nHeap Reserve:\t0x%08X\nHeap Commit:\t0x%08X\n\n",
        optional_header->sectionAlignment, optional_header->fileAlignment, optional_header->majorImageVersion,
        optional_header->minorImageVersion, optional_header->subsystem, optional_header->stackReserveSize,
        optional_header->stackCommitSize, optional_header->heapReserveSize, optional_header->heapCommitSize);
}
void print_nt_header(IMAGE_NT_HEADER* nt_header)
{
    print("NT header signature:\t%s\n", &nt_header->signature);
    print_image_file_header(&nt_header->file_header);
    print_optional_header(&nt_header->optional_header);
}
void print_nt_header_basic(IMAGE_NT_HEADER* nt_header)
{
    print("Signature:\t%s\n", &nt_header->signature);
    print_image_file_header_basic(&nt_header->file_header);
    print_optional_header_basic(&nt_header->optional_header);
    print("\n");
}
