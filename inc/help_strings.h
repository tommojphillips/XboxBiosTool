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

const char HELP_STR_EXTR_ALL[] = "Extract the preldr, 2BL, kernel, section data, init table.";
const char HELP_STR_BUILD[] = "Build a BIOS from a preldr, 2BL, kernel, section data, init table.\n" \
"* The romsize dictates the amount of space available for the BIOS image.\n" \
"* The BIOS image is replicated upto the binsize";

const char HELP_STR_SPLIT[] = "Split a BIOS into banks. Eg: bios.bin 1mb -> bios.bin 4x 256kb.\n" \
"* Use -romsize <size> to split the BIOS into 256kb or 512kb banks";

const char HELP_STR_COMBINE[] = "Combine BIOSes into a single BIOS. Eg: bios.bin x4 256kb -> bios.bin 1mb.\n" \
"* Provide atleast 2 BIOSes to combine.";

const char HELP_STR_XCODE_SIM[] = "Simulate mem-write xcodes and parse x86 instructions. (visor sim)";

const char HELP_STR_XCODE_DECODE[] = "Decode xcodes from a BIOS or init tbl.\n" \
"* Supports retail opcodes.\n" \
"* Supports custom decode format.";

const char HELP_STR_DUMP_NT_IMG[] = "Dump COFF/PE image infomation. headers, sections etc.";

const char HELP_STR_INFO[] = "Display licence, version and author information.";

const char HELP_STR_X86_ENCODE[] = "Encode x86 instructions as mem write xcodes. (visor)\n" \
"* input file must be a binary file containing x86 instructions.";

const char HELP_STR_DECOMP_KRNL[] = "Decompress and extract the kernel from a BIOS.";

const char HELP_STR_PARAM_SIM_SIZE[] = "-simsize <size>  - size of the sim space in bytes. default is 32 bytes";

const char HELP_STR_PARAM_BIOS_FILE[] = "-in <path>\t  - BIOS file.";
const char HELP_STR_PARAM_IN_FILE[] = "-in <path>\t  - input file.";
const char HELP_STR_PARAM_OUT_FILE[] = "-out <path>\t  - output file.";

const char HELP_STR_PARAM_ROMSIZE[] = "-romsize <size>  - rom size in kb.";
const char HELP_STR_PARAM_BINSIZE[] = "-binsize <size>  - bin size in kb.";
const char HELP_STR_VALID_ROM_SIZES[] = "valid opts: 256, 512, 1024.";

const char HELP_STR_MCPX_ROM[] = "-mcpx <path>\n# path to the MCPX ROM file. Should be 512 bytes\n" \
"* Use MCPX 1.0 for BIOS versions < 4817 and MCPX 1.1 for BIOS versions >= 4817.\n" \
"* Used for en/decrypting the 2BL.";

const char HELP_STR_RC4_KEY[] = " <path>\n" \
"# Path to the 16 byte rc4_key.bin file.\n" \
"* Used for en/decrypting the {1}.";

const char HELP_STR_KEY_KRNL[] = " <path>\n" \
"# Path to the 16 byte rc4_key.bin file.\n" \
"* This should not be required unless it's a custom BIOS,\n  as keys are located in the 2BL.";

const char HELP_STR_RC4_ENC[] = "\n# Assume the {1} is unencrypted. Decryption will be skipped.";

const char HELP_STR_PARAM_BLDR[] = "-bldr <path>\t  - boot loader file";
const char HELP_STR_PARAM_PRELDR[] = "-preldr <path>\t  - preldr file";
const char HELP_STR_PARAM_KRNL[] = "-krnl <path>\t  - kernel file";
const char HELP_STR_PARAM_KRNL_DATA[] = "-krnldata <path> - kernel data section file";
const char HELP_STR_PARAM_INITTBL[] = "-inittbl <path>  - init table file";

const char HELP_STR_PARAM_EEPROM_KEY[] = "-eepromkey <path> path to the eeprom key file.";
const char HELP_STR_PARAM_CERT_KEY[] = "-certkey <path>  - path to the cert key file.";
const char HELP_STR_PARAM_PUB_KEY[] = "-pubkey <path>   - path to the public key file.";
const char HELP_STR_PARAM_PATCH_KEYS[] = "-patchkeys\t  - use key switches to patch keys.";

const char HELP_STR_PARAM_LS_NV2A_TBL[] = "-nv2a\t\t  - dump NV2A table";
const char HELP_STR_PARAM_LS_DATA_TBL[] = "-datatbl\t  - dump ROM data table";
const char HELP_STR_PARAM_LS_DUMP_KRNL[] = "-dump-krnl\t  - dump kernel image header info";
const char HELP_STR_PARAM_LS_KEYS[] = "-keys\t\t  - dump rc4 keys";

const char HELP_STR_PARAM_BFM[] = "-bfm\t\t  - build a boot from media BIOS.";

const char HELP_STR_PARAM_XCODE_BASE[] = "-xcodebase <addr>  base address of the xcodes. default is 0x80.";
const char HELP_STR_PARAM_NO_MAX_SIZE[] = "-nomaxsize\t  - do not limit the size of the xcode file.";
const char HELP_STR_PARAM_DECODE_INI[] = "-ini <path>\t  - set the decode settings file.";
const char HELP_STR_PARAM_EXTRACT_KEYS[] = "-keys\t\t  - extract rc4 keys (bfm, krnl, eeprom, cert, sb, preldr)";

#endif // XB_BIOS_TOOL_COMMANDS_H