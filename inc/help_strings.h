// help_strings.h

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

#ifndef XB_HELP_STR_H
#define XB_HELP_STR_H

// All lines are limited to 80 characters.

const char HELP_USAGE_STR[] = "Usage: xbios <command> [switches]";
const char HELP_STR_LIST[] = "Dump BIOS infomation. params, sizes, signatures, keys, tables, etc.";

const char HELP_STR_BUILD[] = "Build a BIOS from a preldr, 2BL, kernel, section data, init table.\n" \
"* The romsize dictates the amount of space available for the BIOS image.\n" \
"* The BIOS image is replicated upto the binsize";

const char HELP_STR_SPLIT[] = "Split a BIOS into banks. Eg: bios.bin 1mb -> bios.bin 4x 256kb.\n" \
"* Use -romsize <size> to split the BIOS into 256kb or 512kb banks";

const char HELP_STR_COMBINE[] = "Combine BIOSes into a single BIOS. Eg: bios.bin x4 256kb -> bios.bin 1mb.\n" \
"* Provide atleast 2 BIOSes to combine.";

const char HELP_STR_X86_ENCODE[] = "Encode x86 instructions as mem write xcodes. (visor)\n" \
"* input file must be a binary file containing x86 instructions.";

const char HELP_STR_XCODE_DECODE[] = "Decode xcodes from a BIOS or init tbl.\n" \
"* Supports retail opcodes\n" \
"* Supports custom decode format via config file";

const char HELP_STR_EXTR_ALL[] = "Extract the preldr, 2BL, kernel, section data, init table.";
const char HELP_STR_XCODE_SIM[] = "Simulate mem-write xcodes and parse x86 instructions. (visor sim)";
const char HELP_STR_DUMP_NT_IMG[] = "Dump COFF/PE image infomation. headers, sections etc.";
const char HELP_STR_INFO[] = "Display licence, version and author information.";
const char HELP_STR_REPLICATE[] = "Replicate a BIOS image upto a specified size.";
const char HELP_STR_COMPRESS_FILE[] = "Compress a file using the lzx algorithm.";
const char HELP_STR_DECOMPRESS_FILE[] = "Decompress a file using the lzx algorithm.";
const char HELP_STR_DISASM[] = "Disasm x86 instructions from a file.";

const char HELP_STR_VALID_ROM_SIZES[] = "valid opts: 256, 512, 1024.";

const char HELP_STR_MCPX_ROM[] = "-mcpx <path>\t\t- path to the mcpx file. Should be 512 bytes\n" \
"\t\t\t- Used for en/decrypting the 2BL.\n" \
"\t\t\t- Use mcpx 1.0 for BIOS versions <= 4627\n\t\t\t- Use mcpx 1.1 for BIOS versions >= 4817.";

const char HELP_STR_RC4_KEY[] = " <path> " \
"\t- path to the %s key file. Should be 16 bytes";

const char HELP_STR_RC4_ENC[] = "\t\t- dont encrypt/decrypt the %s";

const char HELP_STR_PARAM_EEPROM_KEY[] =	"-eepromkey <path>  eeprom key file";
const char HELP_STR_PARAM_BLDR[] =			"-bldr <path>     - 2BL file";
const char HELP_STR_PARAM_PRELDR[] =		"-preldr <path>   - FBL file";
const char HELP_STR_PARAM_KRNL[] =			"-krnl <path>	  - kernel file";
const char HELP_STR_PARAM_KRNL_DATA[] =		"-krnldata <path> - kernel data section file";
const char HELP_STR_PARAM_INITTBL[] =		"-inittbl <path>  - init table file";
const char HELP_STR_PARAM_CERT_KEY[] =		"-certkey <path>  - cert key file";
const char HELP_STR_PARAM_PUB_KEY[] =		"-pubkey <path>   - public key file";
const char HELP_STR_PARAM_LS_NV2A_TBL[] =	"-nv2a            - list NV2A table";
const char HELP_STR_PARAM_LS_DATA_TBL[] =	"-datatbl         - list ROM data table";
const char HELP_STR_PARAM_LS_DUMP_KRNL[] =	"-img             - list kernel image header info";
const char HELP_STR_PARAM_LS_KEYS[] =		"-keys            - list rc4 keys";
const char HELP_STR_PARAM_EXTRACT_KEYS[] =	"-keys            - extract rc4 keys";
const char HELP_STR_PARAM_BFM[] =			"-bfm             - build a boot from media BIOS";
const char HELP_STR_PARAM_DECODE_INI[] =	"-ini <path>      - set the decode settings file";
const char HELP_STR_PARAM_IN_FILE[] =		"-in <path>       - input file";
const char HELP_STR_PARAM_OUT_FILE[] =		"-out <path>      - output file";
const char HELP_STR_PARAM_IN_BIOS_FILE[] =	"-in <path>       - BIOS file";
const char HELP_STR_PARAM_OUT_BIOS_FILE[] =	"-out <path>      - BIOS output file; defaults to bios.bin";
const char HELP_STR_PARAM_SIM_SIZE[] =		"-simsize <size>  - size of the sim space in bytes. default is 32 bytes";
const char HELP_STR_PARAM_ROMSIZE[] =		"-romsize <size>  - rom size in kb";
const char HELP_STR_PARAM_BINSIZE[] =		"-binsize <size>  - bin size in kb";
const char HELP_STR_PARAM_BASE[] =			"-base <offset>   - base offset in bytes";
const char HELP_STR_PARAM_HACK_INITTBL[] =	"-hackinittbl     - hack init tbl (size = 0)";
const char HELP_STR_PARAM_HACK_SIGNATURE[] ="-hacksignature   - hack boot signature (signature = 0)";
const char HELP_STR_PARAM_WDIR[] =          "-dir             - working directory";
const char HELP_STR_PARAM_UPDATE_BOOT_PARAMS[] =  "-nobootparams    - dont update 2BL boot params";
const char HELP_STR_PARAM_RESTORE_BOOT_PARAMS[] = "-nobootparams    - dont restore 2BL boot params (FBL BIOSes only)";
const char HELP_STR_PARAM_BRANCH[] =		"-branch          - take unbranchable jumps";

#endif // XB_BIOS_TOOL_COMMANDS_H