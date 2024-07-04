// help_strings.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_HELP_STR_H
#define XB_HELP_STR_H

const char HELP_STR_HELP[] = "Display help";

const char HELP_STR_LIST[] = "Print bios info: boot params, sizes, xcodes, nv2a tbl, etc.";

const char HELP_STR_EXTR_ALL[] = "Extract the bldr, krnl and initbl from the bios";

const char HELP_STR_SPLIT[] = "Split the bios into banks. bios.bin 1Mb -> bios.bin 4x 256Kb";
const char HELP_STR_COMBINE[] = "Combine banks into a bios. bios.bin x2 256Kb -> bios.bin 512Kb";
const char HELP_STR_BUILD[] = "Build a bios.";
const char HELP_STR_BUILD_INITTBL[] = "Simulate mem-write xcodes and parse x86 opcodes. (visor sim)";

const char HELP_STR_PARAM_BIOS_FILE[] = " <path>\t - The bios file";
const char HELP_STR_PARAM_OUT_FILE[] = " <path>\t - The output file";
const char HELP_STR_PARAM_ROMSIZE[] = " <size> - ROM size";

const char HELP_STR_RC4_KEY[] = " <path> - path to the 16 byte rc4 {1} key .bin file.\n" \
								"\t\t   if key is provided, {1} is assumed to be encrypted and will\n" \
								"\t\t   be decrypted on bios load.\n\t\t   required for en/decrypting the {1}.\n";

const char HELP_STR_RC4_ENC[] = "\t - {1} is assumed to be passed in encrypted if key is provided\n" \
								"\t\t   and the switch is NOT set. Otherwise, if the switch and key\n" \
								"\t\t   is set then the {1} is assumed to be passed in unencrypted.\n";

const char HELP_STR_PARAM_BANK1[] = " <path>\t - Bank 1 file";
const char HELP_STR_PARAM_BANK2[] = " <path>\t - Bank 2 file";
const char HELP_STR_PARAM_BANK3[] = " <path>\t - Bank 3 file";
const char HELP_STR_PARAM_BANK4[] = " <path>\t - Bank 4 file";

const char HELP_STR_PARAM_BLDR[] = " <path>\t - bldr file";
const char HELP_STR_PARAM_KRNL[] = " <path>\t - Kernel file";
const char HELP_STR_PARAM_KRNL_DATA[] = " <path>\t - Uncompressed kernel data file";
const char HELP_STR_PARAM_INITTBL[] = " <path> - Init table file";

const char HELP_STR_PARAM_BLDR_BLOCK[] = "\t- bldr block size";

const char HELP_STR_PARAM_LS_XCODES[] = "\t - List xcodes";
const char HELP_STR_PARAM_LS_NV2A_TBL[] = "\t\t - List nv2a table";
const char HELP_STR_PARAM_LS_BIOS[] = "\t\t - List bios info";
const char HELP_STR_PARAM_LS_DATA_TBL[] = "\t - List rom data table";

#endif // XB_BIOS_TOOL_COMMANDS_H