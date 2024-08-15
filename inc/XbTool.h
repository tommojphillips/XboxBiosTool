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

// user incl
#include "Bios.h"
#include "Mcpx.h"
#include "cli_tbl.h"
#include "type_defs.h"

enum CLI_COMMAND : char {
	CMD_NONE,
	CMD_HELP,
	CMD_EXTR,
	CMD_LIST,
	CMD_SPLIT,
	CMD_COMBINE,
	CMD_BLD_BIOS,
	CMD_XCODE_SIM,
	CMD_KRNL_DECOMPRESS,
	CMD_XCODE_DECODE,
	CMD_ENCODE_X86,
	CMD_DUMP_NT_IMG
};
enum CLI_SWITCH : char {
	SW_NONE = 0,
	SW_HELP,		
	SW_ROMSIZE,
	SW_KEY_KRNL_FILE,
	SW_KEY_BLDR_FILE,
	SW_OUT_FILE,
	SW_IN_FILE,
	SW_BLDR_FILE,
	SW_KRNL_FILE,
	SW_INITTBL_FILE,
	SW_KRNL_DATA_FILE,
	SW_ENC_BLDR,
	SW_ENC_KRNL,
	SW_BINSIZE,
	SW_LS_NV2A_TBL,
	SW_LS_DATA_TBL,
	SW_LS_DUMP_KRNL,
	SW_XCODE_BASE,
	SW_NO_MAX_SIZE,
	SW_MCPX_FILE,
	SW_PUB_KEY_FILE,
	SW_CERT_KEY_FILE,
	SW_EEPROM_KEY_FILE,
	SW_PATCH_KEYS,
	SW_BLD_BFM,
	SW_DMP,
	SW_SIM_SIZE,
	SW_BANK1_FILE,
	SW_BANK2_FILE,
	SW_BANK3_FILE,
	SW_BANK4_FILE,
	SW_INI_FILE,
};
const int CLI_LS_FLAGS = (1 << SW_LS_DATA_TBL) | (1 << SW_LS_NV2A_TBL) | (1 << SW_LS_DUMP_KRNL);

struct Parameters {
	int sw_flags[CLI_ARRAY_SIZE];

	UINT romsize;
	UINT binsize;
	UINT simSize;
	UINT xcodeBase;
	UCHAR* keyBldr;
	UCHAR* keyKrnl;
	const char* inFile;
	const char* outFile;
	const char* bankFiles[4];
	const char* inittblFile;
	const char* bldrFile;
	const char* krnlFile;
	const char* krnlDataFile;
	const char* mcpxFile;
	const char* bldrKeyFile;
	const char* krnlKeyFile;
	MCPX_ROM mcpx;
	const char* pubKeyFile;
	const char* certKeyFile;
	const char* eepromKeyFile;
	const char* settingsFile;
};

class XbTool {
	public:
		XbTool() {
			cmd = NULL;			
			params.romsize = 0;
			params.binsize = 0;
			params.simSize = 0;
			params.xcodeBase = 0;

			params.keyBldr = NULL;
			params.keyKrnl = NULL;
			params.inFile = NULL;
			params.outFile = NULL;
			params.inittblFile = NULL;
			params.bldrFile = NULL;
			params.krnlFile = NULL;
			params.krnlDataFile = NULL;
			params.mcpxFile = NULL;
			params.bldrKeyFile = NULL;
			params.krnlKeyFile = NULL;
			params.pubKeyFile = NULL;
			params.certKeyFile = NULL;
			params.eepromKeyFile = NULL;
			params.settingsFile = NULL;

			for (int i = 0; i < CLI_ARRAY_SIZE; i++)
			{
				params.sw_flags[i] = 0;
			}
		};
		~XbTool() {
			if (params.keyBldr != NULL)
			{
				xb_free(params.keyBldr);
				params.keyBldr = NULL;
			}
			if (params.keyKrnl != NULL)
			{
				xb_free(params.keyKrnl);
				params.keyKrnl = NULL;
			}
		};

		const CMD_TBL* cmd;
		Parameters params;
		Bios bios;
		
		int run();

		int listBios();
		int extractBios();
		int buildBios();
		int splitBios() const;		
		int combineBios() const;
		int simulateXcodes() const;
		int decodeXcodes() const;
		int decompressKrnl();
		
		int readKeys();		
		int readMCPX();
		
		int extractPubKey(UCHAR* data, UINT size) const;
		int encodeX86() const;
		int dumpImg(UCHAR* data, UINT size) const;
private:
		UCHAR* load_init_tbl_file(UINT& size, UINT& xcodeBase) const;
};

int checkSize(const UINT& size);

#endif // !XB_BIOS_TOOL_H
