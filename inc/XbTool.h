// XbTool.h

/* Copyright(C) 2024 tommojphillips
 *
 * This program is free software : you can redistribute it and /or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see < https://www.gnu.org/licenses/>.
*/

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_BIOS_TOOL_H
#define XB_BIOS_TOOL_H

#include <stdint.h>

// user incl
#include "Bios.h"
#include "Mcpx.h"
#include "cli_tbl.h"

enum XB_CLI_COMMAND : CLI_COMMAND {
	CMD_INFO = CLI_COMMAND_START_INDEX,
	CMD_LIST_BIOS,
	CMD_SPLIT_BIOS,
	CMD_COMBINE_BIOS,
	CMD_EXTRACT_BIOS,
	CMD_BUILD_BIOS,
	CMD_SIMULATE_XCODE,
	CMD_DECODE_XCODE,
	CMD_ENCODE_X86,
	CMD_DUMP_PE_IMG,
	CMD_REPLICATE_BIOS,
	CMD_COMPRESS_FILE,
	CMD_DECOMPRESS_FILE,
};
enum XB_CLI_SWITCH : CLI_SWITCH {
	SW_ROMSIZE = CLI_SWITCH_START_INDEX,
	SW_KEY_KRNL_FILE,
	SW_KEY_BLDR_FILE,
	SW_OUT_FILE,
	SW_IN_FILE,
	SW_BLDR_FILE,
	SW_PRELDR_FILE,
	SW_KRNL_FILE,
	SW_INITTBL_FILE,
	SW_KRNL_DATA_FILE,
	SW_ENC_BLDR,
	SW_ENC_KRNL,
	SW_BINSIZE,
	SW_LS_NV2A_TBL,
	SW_LS_DATA_TBL,
	SW_DUMP_KRNL,
	SW_KEYS,
	SW_NO_MAX_SIZE,
	SW_MCPX_FILE,
	SW_PUB_KEY_FILE,
	SW_CERT_KEY_FILE,
	SW_EEPROM_KEY_FILE,
	SW_BLD_BFM,
	SW_DMP,
	SW_SIM_SIZE,
	SW_BANK1_FILE,
	SW_BANK2_FILE,
	SW_BANK3_FILE,
	SW_BANK4_FILE,
	SW_INI_FILE,
	SW_BASE,
	SW_HACK_INITTBL,
	SW_HACK_SIGNATURE,
	SW_UPDATE_BOOT_PARAMS,
	SW_HELP_ENCRYPTION,
	SW_BRANCH,
	SW_HELP_ALL,
	SW_WORKING_DIRECTORY,
	SW_OFFSET,
	SW_XCODES
};

typedef struct {
	uint32_t romsize;
	uint32_t binsize;
	uint32_t simSize;
	uint32_t base;
	uint32_t offset;
	uint8_t* keyBldr;
	uint8_t* keyKrnl;
	Mcpx mcpx;
	const char* inFile;
	const char* outFile;
	const char* bankFiles[4];
	const char* inittblFile;
	const char* preldrFile;
	const char* bldrFile;
	const char* krnlFile;
	const char* krnlDataFile;
	const char* mcpxFile;
	const char* bldrKeyFile;
	const char* krnlKeyFile;
	const char* pubKeyFile;
	const char* certKeyFile;
	const char* eepromKeyFile;
	const char* settingsFile;
	const char* workingDirectoryPath;
	const char* xcodesFile;
} XbToolParameters;

/* Command functions */

int info();
int help();
int helpAll();
int helpEncryption();
int listBios();
int extractBios();
int buildBios();
int splitBios();
int combineBios();
int replicateBios();
int simulateXcodes();
int decodeXcodes();
int encodeX86();
int dumpCoffPeImg();
int compressFile();
int decompressFile();

void init_parameters(XbToolParameters* params);
void free_parameters(XbToolParameters* params);
int inject_xcodes(uint8_t* data, uint32_t size, uint8_t* xcodes, uint32_t xcodesSize);
uint8_t* load_init_tbl_file(uint32_t* size, uint32_t* base);

/* BIOS print functions */
void printBldrInfo(Bios* bios);
void printPreldrInfo(Bios* bios);
void printInitTblInfo(Bios* bios);
void printNv2aInfo(Bios* bios);
void printDataTblInfo(Bios* bios);
void printKeyInfo(Bios* bios);

int main(int argc, char** argv);

#endif // !XB_BIOS_TOOL_H
