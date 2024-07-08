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

const char HELP_STR_HELP[] = "Display help";

const char HELP_STR_LIST[] = "Display bios info: boot params, sizes, signatures, keys, nv2a tbl, etc.";

const char HELP_STR_EXTR_ALL[] = "Extract the 2bl, compressed kernel+data, init table from a bios.";
const char HELP_STR_BUILD[] = "Build a bios from a 2bl, compressed kernel+data, init table";

const char HELP_STR_SPLIT[] = "Split a bios into banks. bios.bin 1Mb -> bios.bin 4x 256Kb";

const char HELP_STR_COMBINE[] = "Combine banks into a single bios. bios.bin x4 256Kb -> bios.bin 1Mb\n" \
"* bank arguments are inferred. eg -> -combine bank1.bin bank2.bin is the\n" \
"  same as -combine -bank1 bank1.bin -bank2 bank2.bin\n" \
"* banks are combined in the order they are specified.\n" \
"* WARNING. Since xcodes are read from the top of the FLASH and everything else\n" \
"  is calculated from the bottom of FLASH, you acan run into a configuration that\n" \
"  just isn't compatible. If you dont know what you are doing, i would highly\n" \
"  recommend NOT using this utility.\n" \
"* This is more so designed for custom bootloaders or modchips.";

const char HELP_STR_XCODE_SIM[] = "Simulate mem-write xcodes and parse x86 instructions. (visor sim)\n" \
" -d\t\t - write sim memory to a file for analysis in a disassembler.";

const char HELP_STR_XCODE_DECODE[] = "Decode xcodes from a bios file. currently only supports retail opcodes.\n" \
" -d\t\t - write decoded xcodes to a .txt. Use -out to set output file.";

const char HELP_STR_PARAM_SIM_SIZE[] = " <size> - The size of the simulated memory in bytes. default is 32.";

const char HELP_STR_PARAM_BIOS_FILE[] = " <path>\t - The bios file";
const char HELP_STR_PARAM_OUT_FILE[] = " <path>\t - The output file";
const char HELP_STR_PARAM_ROMSIZE[] = " <size> - The rom size in Kb. valid opts: 256, 512, 1024";
const char HELP_STR_PARAM_BINSIZE[] = " <size> - The bin size in Kb, valid opts: 256, 512, 1024";

const char HELP_STR_MCPX_ROM[] = " <path>\t  - path to the MCPX ROM file. Can be used for decrypting the\n" \
"\t\t   2bl instead of the -key-bldr switch.\n";

const char HELP_STR_RC4_KEY[] = " <path> - path to the 16 byte rc4_key.bin file.\n" \
"\t\t  * If key is provided, {1} is assumed to be encrypted.\n" \
"\t\t  * Required for en / decrypting the {1}.\n";

const char HELP_STR_KEY_KRNL[] = " <path> - path to the 16 byte rc4_key.bin file.\n" \
"\t\t  * If key is provided, the kernel is assumed to be encrypted.\n" \
"\t\t  * This should not be required unless it's a custom bios, as\n" \
"\t\t    keys are located in the 2bl.\n";

const char HELP_STR_RC4_ENC[] = "\t  - If flag is provided, the {1} is assumed to be unencrypted\n"
"\t\t   or will not be encrypted; depending on context.\n";

const char HELP_STR_PARAM_BANK1[] = " <path>\t - Bank 1 file";
const char HELP_STR_PARAM_BANK2[] = " <path>\t - Bank 2 file";
const char HELP_STR_PARAM_BANK3[] = " <path>\t - Bank 3 file";
const char HELP_STR_PARAM_BANK4[] = " <path>\t - Bank 4 file";

const char HELP_STR_PARAM_BLDR[] = " <path>\t - bldr file";
const char HELP_STR_PARAM_KRNL[] = " <path>\t - Kernel file";
const char HELP_STR_PARAM_KRNL_DATA[] = " <path>  Uncompressed kernel data file";
const char HELP_STR_PARAM_INITTBL[] = " <path> - Init table file";

const char HELP_STR_PARAM_EEPROM_KEY[] = " <path> path to the eeprom key file.";
const char HELP_STR_PARAM_CERT_KEY[] = " <path> - path to the cert key file.";
const char HELP_STR_PARAM_PUB_KEY[] = " <path>  - path to the pub key file.";
const char HELP_STR_PARAM_PATCH_KEYS[] = "\t - Use key switches to patch keys.";

const char HELP_STR_PARAM_BLDR_BLOCK[] = "\t- bldr block size";

const char HELP_STR_PARAM_LS_BIOS[] = "\t\t - List 2bl info";
const char HELP_STR_PARAM_LS_NV2A_TBL[] = "\t\t - List nv2a table";
const char HELP_STR_PARAM_LS_DATA_TBL[] = "\t - List rom data table";

#endif // XB_BIOS_TOOL_COMMANDS_H