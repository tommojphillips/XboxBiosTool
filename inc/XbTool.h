// XbTool.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_BIOS_TOOL_H
#define XB_BIOS_TOOL_H

#include "Bios.h"

#include "cli_tbl.h"
#include "type_defs.h"

#include "version.h"

struct MCPX_ROM
{
	enum MCPX_ROM_VERSION { MCPX_UNK, MCPX_V1_0, MCPX_V1_1 };

	MCPX_ROM_VERSION version;
	UCHAR* data;

	MCPX_ROM() : version(MCPX_UNK), data(NULL) { };

	~MCPX_ROM()
	{
		deconstruct();
	}

	void deconstruct()
	{
		if (data != NULL)
		{
			xb_free(data);
			data = NULL;
		}
	}
};

struct Parameters
{
	int sw_flag;			// contains all flags that have been set on the command line.
	int ls_flag;			// contains all set flags for the list command.

	UINT romsize;

	bool encBldr;
	bool encKrnl;

	const char* biosFile;
	const char* outFile;
	const char* bankFiles[4];

	const char* inittblFile;
	const char* bldrFile;
	const char* krnlFile;

	const char* mcpxFile;

	const char* keyBldrFile;
	const char* keyKrnlFile;

	UCHAR* keyBldr;
	UCHAR* keyKrnl;

	MCPX_ROM mcpx;

	bool patchPubKey;
	const char* pubKeyFile;

	Parameters() : sw_flag(0), ls_flag(0), romsize(0), encBldr(true), encKrnl(true),
		biosFile(NULL), outFile(NULL), inittblFile(NULL), bldrFile(NULL), 
		krnlFile(NULL), keyBldrFile(NULL), keyKrnlFile(NULL), 
		keyBldr(NULL), keyKrnl(NULL), mcpxFile(NULL), mcpx(), 
		pubKeyFile(NULL), patchPubKey(false)
	{
		for (int i = 0; i < 4; i++)
		{
			bankFiles[i] = NULL;
		}
	}

	~Parameters()
	{
		deconstruct();
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
		~XbTool() {
			deconstruct();
		};

		void deconstruct();

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
};


int verifyPubKey(UCHAR* data, PUBLIC_KEY*& pubkey);
int printPubKey(PUBLIC_KEY* pubkey);

#endif // !XB_BIOS_TOOL_H
