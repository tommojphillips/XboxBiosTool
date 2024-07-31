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
#include "version.h"

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
};
const int CLI_LS_FLAGS = (1 << SW_LS_DATA_TBL) | (1 << SW_LS_NV2A_TBL) | (1 << SW_LS_DUMP_KRNL);

struct Parameters
{
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

	void construct()
	{
		for (int i = 0; i < CLI_ARRAY_SIZE; i++)
		{
			sw_flags[i] = 0;
		}
		romsize = 0;
		binsize = 0;
		simSize = 0;
		xcodeBase = 0;

		keyBldr = NULL;
		keyKrnl = NULL;
		inFile = NULL;
		outFile = NULL;
		inittblFile = NULL;
		bldrFile = NULL;
		krnlFile = NULL;
		krnlDataFile = NULL;
		mcpxFile = NULL;
		bldrKeyFile = NULL;
		krnlKeyFile = NULL;
		pubKeyFile = NULL;
		certKeyFile = NULL;
		eepromKeyFile = NULL;

		mcpx.construct();
	};

	void deconstruct()
	{
		if (keyBldr != NULL)
		{
			xb_free(keyBldr);
			keyBldr = NULL;
		}

		if (keyKrnl != NULL)
		{
			xb_free(keyKrnl);
			keyKrnl = NULL;
		}

		mcpx.deconstruct();
	};
};

class XbTool
{
	public:
		const CMD_TBL* cmd;
		Parameters params;
		Bios bios;
				
		void construct();
		void deconstruct();

		int run();

		// print the bios info. (boot params, sizes, xcodes, nv2a tbl, etc.)
		int listBios();

		// extract the bios. (bldr, krnl, krnl data, inittbl) to their respective files.
		int extractBios();

		// build the bios. (bldr, krnl, inittbl) from the files provided.
		int buildBios();

		// split the bios into separate banks. based on the rom size provided.
		int splitBios();		
		
		// combine the bios files provided into a single bios file. (bank1, bank2, bank3, bank4)
		int combineBios();

		// simulate the xcodes.
		int simulateXcodes() const;
		
		// decode the xcodes. 
		int decodeXcodes() const;

		// decompress the kernel from the bios file provided.
		int decompressKrnl();

		// read in the keys from the key files provided. (key-bldr, key-krnl)
		int readKeys();		
		
		// read in the mcpx file provided. 
		int readMCPX();
		
		// attempt to find the public key in the ptr.
		int extractPubKey(UCHAR* data, UINT size);

		// encode x86 code as xcode mem write instructions.
		int encodeX86();

		// dump nt image info to the console.
		int dumpImg(UCHAR* data, UINT size) const;
private:
	// load the inittbl file.
	UCHAR* load_init_tbl_file(UINT& size, UINT& xcodeBase) const;
};

int verifyPubKey(UCHAR* data, PUBLIC_KEY*& pubkey);
int printPubKey(PUBLIC_KEY* pubkey);

#endif // !XB_BIOS_TOOL_H
