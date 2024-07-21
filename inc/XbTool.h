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

#include "Bios.h"
#include "Mcpx.h"

#include "cli_tbl.h"
#include "type_defs.h"

#include "version.h"

struct Parameters
{
	int sw_flag; // contains all flags that have been set on the command line.
	int ls_flag;

	UINT romsize;
	UINT binsize;

	UINT simSize;

	UCHAR* keyBldr;
	UCHAR* keyKrnl;

	const char* biosFile;
	const char* outFile;
	const char* bankFiles[4];

	const char* inittblFile;
	const char* bldrFile;
	const char* krnlFile;
	const char* krnlDataFile;

	const char* mcpxFile;

	const char* keyBldrFile;
	const char* keyKrnlFile;

	MCPX_ROM mcpx;

	const char* pubKeyFile;
	const char* certKeyFile;
	const char* eepromKeyFile;

	Parameters() : sw_flag(0), ls_flag(0), romsize(0), binsize(0), simSize(0),
		biosFile(NULL), outFile(NULL), inittblFile(NULL), bldrFile(NULL), krnlFile(NULL), krnlDataFile(NULL),
		keyBldrFile(NULL), keyKrnlFile(NULL),
		keyBldr(NULL), keyKrnl(NULL),
		mcpxFile(NULL), mcpx(), 
		pubKeyFile(NULL), certKeyFile(NULL), eepromKeyFile(NULL)
	{
		for (int i = 0; i < 4; i++)
		{
			bankFiles[i] = NULL;
		}
	}

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
	}
};

class XbTool
{
	public:
		const CMD_TBL* cmd;
		char exe_filename[MAX_FILENAME];		
		Bios bios;

		Parameters params;
		
		XbTool() : exe_filename(), cmd(NULL), bios(), params() { };

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

private:
	// load the inittbl file.
	UCHAR* load_init_tbl_file(UINT& size) const;
};

int verifyPubKey(UCHAR* data, PUBLIC_KEY*& pubkey);
int printPubKey(PUBLIC_KEY* pubkey);

#endif // !XB_BIOS_TOOL_H
