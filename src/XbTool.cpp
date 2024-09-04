// XbTool.cpp : Implements command functions for the Xbox Bios Tool. Logic for each command is implemented here.

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

// std incl
#include <cstdio>
#include <cstdlib>
#include <cstring>

// user incl
#include "XbTool.h"
#include "Bios.h"
#include "Mcpx.h"
#include "XcodeInterp.h"
#include "XcodeDecoder.h"
#include "X86Interp.h"

#include "type_defs.h"
#include "file.h"
#include "util.h"
#include "nt_headers.h"
#include "rc4.h"
#include "rsa.h"
#include "sha1.h"
#include "help_strings.h"
#include "version.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

enum XB_CLI_COMMAND : CLI_COMMAND {
	CMD_INFO = CLI_COMMAND_START_INDEX,
	CMD_LIST_BIOS,
	CMD_SPLIT_BIOS,
	CMD_COMBINE_BIOS,
	CMD_EXTRACT_BIOS,
	CMD_BUILD_BIOS,
	CMD_DECOMPRESS_KERNEL,
	CMD_SIMULATE_XCODE,
	CMD_DECODE_XCODE,
	CMD_ENCODE_X86,
	CMD_DUMP_PE_IMG,
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
	SW_LS_DUMP_KRNL,
	SW_KEYS,
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
const int CLI_LS_FLAGS = (1 << SW_LS_DATA_TBL) | (1 << SW_LS_NV2A_TBL) | (1 << SW_LS_DUMP_KRNL) | (1 << SW_KEYS);

typedef struct Parameters {
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
	const char* preldrFile;
	const char* bldrFile;
	const char* krnlFile;
	const char* krnlDataFile;
	const char* mcpxFile;
	const char* bldrKeyFile;
	const char* krnlKeyFile;
	Mcpx mcpx;
	const char* pubKeyFile;
	const char* certKeyFile;
	const char* eepromKeyFile;
	const char* settingsFile;

	Parameters() {
		romsize = 0;
		binsize = 0;
		simSize = 0;
		xcodeBase = 0;
		preldrFile = NULL;
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
		settingsFile = NULL;

		for (int i = 0; i < 4; i++)
		{
			bankFiles[i] = NULL;
		}
	};
	~Parameters() {
		if (keyBldr != NULL)
		{
			free(keyBldr);
			keyBldr = NULL;
		}
		if (keyKrnl != NULL)
		{
			free(keyKrnl);
			keyKrnl = NULL;
		}
	};
} Parameters;

static Parameters params = {};
static const CMD_TBL* cmd = NULL;

static const CMD_TBL cmd_tbl[] = {
	{ "?", CMD_HELP, {SW_NONE}, {SW_NONE} },
	{ "info", CMD_INFO, {SW_NONE}, {SW_NONE} },
	{ "ls", CMD_LIST_BIOS, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "extr", CMD_EXTRACT_BIOS, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "bld", CMD_BUILD_BIOS, {SW_BLDR_FILE, SW_KRNL_FILE, SW_KRNL_DATA_FILE, SW_INITTBL_FILE}, {SW_NONE} },
	{ "decomp-krnl", CMD_DECOMPRESS_KERNEL, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "split", CMD_SPLIT_BIOS, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "combine", CMD_COMBINE_BIOS, {SW_NONE}, {SW_BANK1_FILE, SW_BANK2_FILE, SW_BANK3_FILE, SW_BANK4_FILE} },
	{ "xcode-sim", CMD_SIMULATE_XCODE, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "xcode-decode", CMD_DECODE_XCODE, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "x86-encode", CMD_ENCODE_X86, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "dump-img", CMD_DUMP_PE_IMG, {SW_IN_FILE}, {SW_IN_FILE} },
};
static const PARAM_TBL param_tbl[] = {
	{ "in", &params.inFile, SW_IN_FILE, PARAM_TBL::STR },
	{ "out", &params.outFile, SW_OUT_FILE, PARAM_TBL::STR },
	{ "romsize", &params.romsize, SW_ROMSIZE, PARAM_TBL::INT },
	{ "binsize", &params.binsize, SW_BINSIZE, PARAM_TBL::INT },

	{ "enc-bldr", NULL, SW_ENC_BLDR, PARAM_TBL::FLAG },
	{ "enc-krnl", NULL, SW_ENC_KRNL, PARAM_TBL::FLAG },
	{ "key-krnl", &params.krnlKeyFile, SW_KEY_KRNL_FILE, PARAM_TBL::STR },
	{ "key-bldr", &params.bldrKeyFile, SW_KEY_BLDR_FILE, PARAM_TBL::STR},
	{ "mcpx", &params.mcpxFile, SW_MCPX_FILE, PARAM_TBL::STR },

	{ "bldr", &params.bldrFile, SW_BLDR_FILE, PARAM_TBL::STR },
	{ "preldr", &params.preldrFile, SW_PRELDR_FILE, PARAM_TBL::STR },
	{ "krnl", &params.krnlFile, SW_KRNL_FILE, PARAM_TBL::STR },
	{ "krnldata", &params.krnlDataFile, SW_KRNL_DATA_FILE, PARAM_TBL::STR },
	{ "inittbl", &params.inittblFile, SW_INITTBL_FILE, PARAM_TBL::STR },

	{ "nv2a", NULL, SW_LS_NV2A_TBL, PARAM_TBL::FLAG },
	{ "datatbl", NULL, SW_LS_DATA_TBL, PARAM_TBL::FLAG },
	{ "dump-krnl", NULL, SW_LS_DUMP_KRNL, PARAM_TBL::FLAG },

	{ "patchkeys", NULL, SW_PATCH_KEYS, PARAM_TBL::FLAG },
	{ "pubkey", &params.pubKeyFile, SW_PUB_KEY_FILE, PARAM_TBL::STR },
	{ "certkey", &params.certKeyFile, SW_CERT_KEY_FILE, PARAM_TBL::STR },
	{ "eepromkey", &params.eepromKeyFile, SW_EEPROM_KEY_FILE, PARAM_TBL::STR },

	{ "bfm", NULL, SW_BLD_BFM, PARAM_TBL::FLAG },
	{ "d", NULL, SW_DMP, PARAM_TBL::FLAG },
	{ "simsize", &params.simSize, SW_SIM_SIZE, PARAM_TBL::INT },
	{ "nomaxsize", NULL, SW_NO_MAX_SIZE, PARAM_TBL::FLAG },
	{ "xcodebase", &params.xcodeBase, SW_XCODE_BASE, PARAM_TBL::INT },
	{ "bank1", &params.bankFiles[0], SW_BANK1_FILE, PARAM_TBL::STR },
	{ "bank2", &params.bankFiles[1], SW_BANK2_FILE, PARAM_TBL::STR },
	{ "bank3", &params.bankFiles[2], SW_BANK3_FILE, PARAM_TBL::STR },
	{ "bank4", &params.bankFiles[3], SW_BANK4_FILE, PARAM_TBL::STR },
	{ "ini", &params.settingsFile, SW_INI_FILE, PARAM_TBL::STR },

	{ "?", NULL, SW_HELP, PARAM_TBL::FLAG },
	{ "keys", NULL, SW_KEYS, PARAM_TBL::FLAG },
};

UCHAR* load_init_tbl_file(UINT& size, UINT& xcodeBase);
int validateArgs();
void printBldrInfo(Bios* bios);
void printPreldrInfo(Bios* bios);
void printInitTblInfo(Bios* bios);
void printNv2aInfo(Bios* bios);
void printDataTbl(Bios* bios);
void printKeys(Bios* bios);
void printHelp();

int main(int argc, char* argv[]);

int buildBios()
{
	int result = 0;
	const char* filename = params.outFile;
	UCHAR* inittbl = NULL;
	UCHAR* preldr = NULL;
	UCHAR* bldr = NULL;
	UCHAR* krnl = NULL;
	UCHAR* krnlData = NULL;
	UINT inittblSize = 0;
	UINT preldrSize = 0;
	UINT bldrSize = 0;
	UINT krnlSize = 0;
	UINT krnlDataSize = 0;

	Bios bios;
	BiosParams biosParams = {
		&params.mcpx,
		params.romsize,
		params.keyBldr,
		params.keyKrnl,
		isFlagSet(SW_ENC_BLDR),
		isFlagSet(SW_ENC_KRNL)
	};

	bool buildBfm = isFlagSet(SW_BLD_BFM);
	
	// read in the init tbl file
	printf("inittbl file:\t%s\n", params.inittblFile);
	inittbl = readFile(params.inittblFile, &inittblSize);
	if (inittbl == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// read in the preldr file
	printf("preldr file:\t%s\n", params.preldrFile);
	preldr = readFile(params.preldrFile, &preldrSize);
	if (preldr == NULL)
	{
		preldrSize = 0;
	}

	// read in the bldr file
	printf("bldr file:\t%s\n", params.bldrFile);
	bldr = readFile(params.bldrFile, &bldrSize);
	if (bldr == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// read in the compressed krnl image
	printf("krnl file:\t%s\n", params.krnlFile);
	krnl = readFile(params.krnlFile, &krnlSize);
	if (krnl == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// read in the uncompressed kernel data
	printf("krnl data file:\t%s\n", params.krnlDataFile);
	krnlData = readFile(params.krnlDataFile, &krnlDataSize);
	if (krnlData == NULL)
	{
		result = 1;
		goto Cleanup;
	}
	// required files were read successfully
	
	// build the bios in memory	
	result = bios.build(preldr, preldrSize,
		bldr, bldrSize, 
		inittbl, inittblSize, 
		krnl, krnlSize, 
		krnlData, krnlDataSize, 
		params.binsize, buildBfm, biosParams);

	if (result != 0)
	{
		printf("Error: Failed to build bios\n");
		goto Cleanup;
	}

	// write data to file
	filename = params.outFile;
	if (filename == NULL)
	{
		filename = "bios.bin";
	}

	result = bios.saveBiosToFile(filename);
	
Cleanup:
	if (preldr != NULL)
	{
		free(preldr);
		preldr = NULL;
	}
	if (inittbl != NULL)
	{
		free(inittbl);
		inittbl = NULL;
	}
	if (bldr != NULL)
	{
		free(bldr);
		bldr = NULL;
	}
	if (krnl != NULL)
	{
		free(krnl);
		krnl = NULL;
	}
	if (krnlData != NULL)
	{
		free(krnlData);
		krnlData = NULL;
	}
	
	return result;
}
int extractBios()
{
	int result = 0;

	printf("bios file: %s\n", params.inFile);
	Bios bios;
	BiosParams biosParams = { 
		&params.mcpx,
		params.romsize,
		params.keyBldr,
		params.keyKrnl, 
		isFlagSet(SW_ENC_BLDR),
		isFlagSet(SW_ENC_KRNL)
	};
	result = bios.loadFromFile(params.inFile, biosParams);
	if (result != BIOS_LOAD_STATUS_SUCCESS) // bldr needs to be valid to extract
	{
		return 1;
	}

	const char* filename = NULL;

	// bootloader
	printf("\n");
	if (params.bldrFile != NULL)
		filename = params.bldrFile;
	else
		filename = "bldr.bin";
	result = bios.saveBldrBlockToFile(filename);
	if (result != 0)
		return 1;
	
	// compressed kernel
	printf("\n");
	if (params.krnlFile != NULL)
		filename = params.krnlFile;
	else
		filename = "krnl.bin";
	result = bios.saveKernelToFile(filename);
	if (result != 0)
		return 1;
	
	// uncompressed kernel data
	printf("\n");
	if (params.krnlDataFile != NULL)
		filename = params.krnlDataFile;
	else
		filename = "krnl_data.bin";
	result = bios.saveKernelDataToFile(filename);
	if (result != 0)
		return 1;
	
	// init tbl
	printf("\n");
	if (params.inittblFile != NULL)
		filename = params.inittblFile;
	else
		filename = "inittbl.bin";
	result = bios.saveInitTblToFile(filename);
	if (result != 0)
		return 1;

	if (isFlagSet(SW_KEYS))
	{
		Bldr* bldr = bios.getBldr();
		if (bldr->keys != NULL)
		{
			printf("\n");
			result = writeFileF("bfm_key.bin", bldr->keys->bfmKey, KEY_SIZE, "bfm key");
			if (result != 0)
				return 1;

			printf("\n");
			result = writeFileF("krnl_key.bin", bldr->keys->krnlKey, KEY_SIZE, "kernel key");
			if (result != 0)
				return 1;

			printf("\n");
			filename = params.eepromKeyFile;
			if (filename == NULL)
			{
				filename = "eeprom_key.bin";
			}
			result = writeFileF(filename, bldr->keys->eepromKey, KEY_SIZE, "eeprom key");
			if (result != 0)
				return 1;

			printf("\n");
			filename = params.certKeyFile;
			if (filename == NULL)
			{
				filename = "cert_key.bin";
			}
			result = writeFileF(filename, bldr->keys->certKey, KEY_SIZE, "cert key");
			if (result != 0)
				return 1;
		}

		if (params.mcpx.sbkey != NULL)
		{
			printf("\n");
			result = writeFileF("sb_key.bin", params.mcpx.sbkey, KEY_SIZE, "secret boot key");
			if (result != 0)
				return 1;
		}
	}

	Preldr* preldr = bios.getPreldr();
	if (preldr->status == PRELDR_STATUS_OK)
	{
		if (isFlagSet(SW_KEYS))
		{
			printf("\n");
			result = writeFileF("preldr_key.bin", preldr->bldrKey, SHA1_DIGEST_LEN, "preldr key");
			if (result != 0)
				return 1;
		}

		biosParams.encBldr = true; // force no decryption; bldr decryption mangles the preldr
		biosParams.encKrnl = true; // dont care about the kernel

		// reload bios so preldr is not mangled by 2bl decryption
		bios.unload();
		result = bios.loadFromFile(params.inFile, biosParams);
		if (result > BIOS_LOAD_STATUS_INVALID_BLDR)
			return 1;

		preldr = bios.getPreldr();
		UINT preldrSize = bios.getPreldrSize() - PRELDR_ROM_DIGEST_SIZE;

		if (preldr->status != PRELDR_STATUS_INVALID_BLDR)
		{
			printf("Error: Failed to read Preldr\n");
			return 1;
		}

		filename = params.preldrFile;
		if (filename == NULL)
		{
			filename = "preldr.bin";
		}

		result = writeFileF(filename, preldr->data, preldrSize, "preldr");
		if (result != 0)
		{
			return 1;
		}
	}

	return 0;
}
int splitBios()
{
	int result = 0;
	UCHAR* data = NULL;
	UINT size = 0;
	UINT fnLen = 0;
	UINT bankFnLen = 0;
	UINT dataLeft = 0;
	UINT loopCount = 0;
	int bank = 0;
	const char suffix[] = "_bank";
	char* biosFn = NULL;
	char* ext = NULL;
	char* bankFn = NULL;
	int i;
	int j;

	printf("bios file: %s\nrom size:  %dkb\n", params.inFile, params.romsize / 1024);

	//romsize sanity check
	if (params.romsize < DEFAULT_ROM_SIZE * 1024)
		return 1;
		
	data = readFile(params.inFile, &size);
	if (data == NULL)
		return 1;

	result = checkBiosSize(size);
	if (result != 0)
	{
		printf("Error: Invalid bios file size: %d\n", size);
		goto Cleanup;
	}
	
	// check if bios size is less than or equal to rom size
	if (size <= params.romsize)
	{
		printf("Cannot split bios. file size is less than or equal to romsize\n");
		result = 1;
		goto Cleanup;
	}

	fnLen = strlen(params.inFile);
	biosFn = (char*)malloc(fnLen + 1);
	if (biosFn == NULL)
	{
		result = 1;
		goto Cleanup;
	}
	for (j = fnLen - 1; j > 0; j--)
	{
		if (params.inFile[j] == '\\')
		{
			j += 1;
			break;
		}
	}
	strcpy(biosFn, params.inFile+j);
	fnLen -= j;

	// remove the extention
	for (i = fnLen - 1; i >= 0; i--)
	{
		if (biosFn[i] == '.')
		{
			ext = (char*)malloc(fnLen - i + 1); // +1 for null terminator
			if (ext == NULL)
			{
				result = 1;
				goto Cleanup;
			}
			strcpy(ext, biosFn + i);

			biosFn[i] = '\0';
			break;
		}
	}

	if (ext == NULL)
	{
		printf("Error: Invalid bios file name. No file extension found\n");
		result = 1;
		goto Cleanup;
	}

	bankFnLen = strlen(biosFn) + strlen(suffix) + strlen(ext) + 3U; // +3 for the bank number
	bankFn = (char*)malloc(bankFnLen);
	if (bankFn == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// split the bios based on rom size

	dataLeft = size;
	loopCount = 0;
	while (dataLeft > 0)
	{
		// safe guard
		if (loopCount > 4)
		{ 
			printf("Error: loopCount exceeded 5\n");
			result = 1;
			break;
		}
		loopCount++;

		// set bank filename = [name][suffix][number].[ext] = bios_bank1.bin
		bankFn[0] = '\0';
		sprintf(bankFn, "%s%s%d%s", biosFn, suffix, bank + 1, ext);

		// write bank to file
		printf("Writing bank %d to %s\n", bank + 1, bankFn);
		result = writeFile(bankFn, (data + (params.romsize * bank)), params.romsize);
		if (result != 0)
		{
			goto Cleanup;
		}

		dataLeft -= params.romsize;
		bank++;
	}

	printf("Bios split into %d banks\n", bank);

Cleanup:
	if (data != NULL)
	{
		free(data);
	}
	if (biosFn != NULL)
	{
		free(biosFn);
	}
	if (bankFn != NULL)
	{
		free(bankFn);
	}
	if (ext != NULL)
	{
		free(ext);
	}

	return result;
}
int combineBios() 
{
	const UINT MAX_BANKS = 4;
	UINT totalSize = 0;
	UINT offset = 0;

	int i;
	int result = 0;
	int numBanks = 0;

	UCHAR* data = NULL;

	UCHAR* banks[MAX_BANKS] = { NULL };
	UINT bankSizes[MAX_BANKS] = { 0 };

	const char* filename = params.outFile;
	if (filename == NULL)
	{
		filename = "bios.bin";
	}

	for (i = 0; i < MAX_BANKS; i++)
	{
		if (params.bankFiles[i] == NULL)
			continue;

		numBanks++;
		
		banks[i] = readFile(params.bankFiles[i], &bankSizes[i]);
		if (banks[i] == NULL)
		{
			result = 1;
			goto Cleanup;
		}

		if (checkBiosSize(bankSizes[i]) != 0)
		{
			printf("Error: %s has invalid file size: %d\n", params.bankFiles[i], bankSizes[i]);
			result = 1;
			goto Cleanup;
		}
		
		totalSize += bankSizes[i];
	}

	if (numBanks < 2)
	{
		printf("Error: Not enough banks to combine. Expected atleast 2 banks to be provided\n");
		result = 1;
		goto Cleanup;
	}

	if (checkBiosSize(totalSize) != 0)
	{
		printf("Error: Invalid total bios size: %d\n", totalSize);
		result = 1;
		goto Cleanup;
	}

	data = (UCHAR*)malloc(totalSize);
	if (data == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	for (i = 0; i < MAX_BANKS; i++)
	{
		if (banks[i] != NULL)
		{
			printf("Copying %s %dKB into offset 0x%x (bank %d)\n", params.bankFiles[i], bankSizes[i] / 1024, offset, i + 1);
			memcpy(data + offset, banks[i], bankSizes[i]);
			offset += bankSizes[i];
		}
	}

	result = writeFileF(filename, data, totalSize, "bios");
	
Cleanup:

	if (data != NULL)
	{
		free(data);
	}

	for (i = 0; i < MAX_BANKS; i++)
	{
		if (banks[i] != NULL)
		{
			free(banks[i]);
			banks[i] = NULL;
		}
	}

	return result;
}
int listBios()
{
	Bios bios;
	BiosParams biosParams = {
		&params.mcpx,
		params.romsize,
		params.keyBldr,
		params.keyKrnl,
		isFlagSet(SW_ENC_BLDR),
		isFlagSet(SW_ENC_KRNL)
	};
	UINT biosSize = 0;
	UINT availableSpace = 0;

	printf("bios file: %s\n", params.inFile);
	int result = bios.loadFromFile(params.inFile, biosParams);
	if (result > BIOS_LOAD_STATUS_INVALID_BLDR) // bldr doesn't need to be valid to list it.
	{
		return 1;
	}

	if (!isFlagSetAny(CLI_LS_FLAGS))
	{
		biosSize = bios.getBiosSize();
		availableSpace = bios.getAvailableSpace();

		//printf("\nbios size:\t\t%ld KB\n", biosSize / 1024UL);
		//printf("rom size:\t\t%ld KB\n", params.romsize / 1024UL);

		printPreldrInfo(&bios);
		printBldrInfo(&bios);
		printInitTblInfo(&bios);

		printf("\nAvailable space:\t");
		print((CON_COL)(availableSpace >= 0 && availableSpace <= biosSize), "%ld", availableSpace);
		printf(" bytes\n");

		return 0;
	}

	if (isFlagSet(SW_KEYS))
	{
		printKeys(&bios);
	}

	if (isFlagSet(SW_LS_NV2A_TBL))
	{
		printNv2aInfo(&bios);
	}

	if (isFlagSet(SW_LS_DATA_TBL))
	{
		printDataTbl(&bios);
	}

	if (isFlagSet(SW_LS_DUMP_KRNL))
	{
		result = bios.decompressKrnl();
		if (result != 0)
		{
			printf("Error: Failed to decompress the kernel image\n");
			return result;
		}

		UCHAR* decompressedKrnl = bios.getDecompressedKrnl();

		dumpImg(decompressedKrnl, bios.getDecompressedKrnlSize());
		print_krnl_data_section_header((IMAGE_DOS_HEADER*)decompressedKrnl);
	}

	return 0;
}

int decodeXcodes() 
{
	XcodeDecoder decoder;
	DECODE_CONTEXT* context;
	UCHAR* inittbl;

	int result = 0;

	UINT size;
	UINT base;
	inittbl = load_init_tbl_file(size, base);
	if (inittbl == NULL)
		return 1;

	result = decoder.load(inittbl, size, base, params.settingsFile);
	if (result != 0)
	{
		printf("Error: Failed to init xcode decoder\n");
		goto Cleanup;
	}

	context = decoder.context;
	
	printf("xcode count: %d\nxcode size: %d bytes\nxcode base: 0x%x\n", context->xcodeCount, context->xcodeSize, context->xcodeBase);
	
	// setup the file stream, if -d flag is set
	if (isFlagSet(SW_DMP))
	{
		const char* filename = params.outFile;
		if (filename == NULL)
		{
			filename = "xcodes.txt";
		}

		// del file if it exists
		deleteFile(filename);

		printf("Writing xcodes to %s\n", filename);
		FILE* stream = fopen(filename, "w");
		if (stream == NULL)
		{
			printf("Error: Failed to open file %s\n", filename);
			result = 1;
			goto Cleanup;
		}
		context->stream = stream;
	}
	else // print to stdout
	{
		if (context->xcodeCount > 2048)
		{
			printf("Error: Too many xcodes. Use -d flag to dump to file\n");
			result = 1;
			goto Cleanup;
		}
		context->stream = stdout;
	}
	
	result = decoder.decodeXcodes();

	if (isFlagSet(SW_DMP))
	{
		fclose(context->stream);
		printf("Done\n");
	}

Cleanup:

	if (inittbl != NULL)
	{
		free(inittbl);
	}

	return result;
}
int simulateXcodes() 
{
	XcodeInterp interp;
	XCODE* xcode = NULL;
	
	UINT size = 0;
	int result = 0;
	bool hasMemChanges_total = false;
	const char* opcode_str = NULL;
	UCHAR* mem_sim = NULL;
	UCHAR* inittbl = NULL;
	UINT xcodeBase = 0;
	UINT codeSize = 0;

	inittbl = load_init_tbl_file(size, xcodeBase);
	if (inittbl == NULL)
		return 1;

	result = interp.load(inittbl + xcodeBase, size - xcodeBase);
	if (result != 0)
		goto Cleanup;

	printf("xcode base: 0x%x\nmem space: %d bytes\n\n", xcodeBase, params.simSize);

	mem_sim = (UCHAR*)malloc(params.simSize);
	if (mem_sim == NULL)
	{
		result = 1;
		goto Cleanup;
	}
	memset(mem_sim, 0, params.simSize);

	// simulate memory output
	hasMemChanges_total = false;
	printf("xcode sim:\n");
	while (interp.interpretNext(xcode) == 0)
	{
		// only care about xcodes that write to RAM
		if (xcode->opcode != XC_MEM_WRITE)
			continue;

		// sanity check of addr.
		if (xcode->addr < 0 || xcode->addr >= params.simSize)
		{
			continue;
		}
		hasMemChanges_total = true;

		// write the data to simulated memory
		memcpy(mem_sim + xcode->addr, (UCHAR*)&xcode->data, 4);

		if (getOpcodeStr(getOpcodeMap(), xcode->opcode, opcode_str) != 0)
		{			
			result = 1;
			goto Cleanup;
		}

		// print the xcode
		printf("%04x: %s 0x%02x, 0x%08X\n", (xcodeBase + interp.getOffset() - sizeof(XCODE)), opcode_str, xcode->addr, xcode->data);
	}
	
	if (!hasMemChanges_total)
	{
		printf("No Memory changes in range 0x0 - 0x%x\n", params.simSize);
		goto Cleanup;
	}

	// decode the x86 instructions.
	result = decodeX86(mem_sim, params.simSize, stdout, &codeSize);
	if (result != 0)
	{
		printf("Error: Failed to decode x86 instructions\n");
		goto Cleanup;
	}

	// if -d flag is set, dump the memory to a file, otherwise print the memory dump
	if (isFlagSet(SW_DMP))
	{
		const char* filename = params.outFile;
		if (filename == NULL)
			filename = "mem_sim.bin";

		printf("\n");
		result = writeFileF(filename, mem_sim, codeSize, "memory dump");
		if (result != 0)
		{
			goto Cleanup;
		}
	}
	else
	{
		// print memory dump
		printf("\nmem dump: ( %d bytes )\n", codeSize);
		UINT j;
		for (UINT i = 0; i < codeSize; i += j)
		{
			if (i + 8 > codeSize)
				j = codeSize - i;
			else
				j = 8;

			printf("%04x: ", i);
			printData(mem_sim + i, j);
		}
	}

Cleanup:

	if (inittbl != NULL)
	{
		free(inittbl);
	}

	if (mem_sim != NULL)
	{
		free(mem_sim);
	}

	return result;
}
int encodeX86()
{
	// encode x86 instructions to xcodes

	UCHAR* data = NULL;
	UCHAR* buffer = NULL;
	UINT dataSize = 0;
	UINT xcodeSize = 0;
	int result = 0;

	const char* filename = NULL;

	printf("x86 file: %s\n", params.inFile);

	data = readFile(params.inFile, &dataSize);
	if (data == NULL)
	{
		return 1;
	}

	result = encodeX86AsMemWrites(data, dataSize, buffer, &xcodeSize);
	if (result != 0)
	{
		printf("Error: Failed to encode x86 instructions\n");
		goto Cleanup;
	}

	printf("xcodes: %d\n", xcodeSize / sizeof(XCODE));

	// write the xcodes to file
	filename = params.outFile;
	if (filename == NULL)
	{
		filename = "xcodes.bin";
	}

	result = writeFileF(filename, buffer, xcodeSize, "xcodes");

Cleanup:

	if (data != NULL)
	{
		free(data);
	}

	if (buffer != NULL)
	{
		free(buffer);
	}

	return 0;
}

int decompressKrnl()
{
	int result = 0;
	Bios bios;
	BiosParams biosParams = {
		&params.mcpx,
		params.romsize,
		params.keyBldr,
		params.keyKrnl,
		isFlagSet(SW_ENC_BLDR),
		isFlagSet(SW_ENC_KRNL)
	};

	printf("bios file: %s\n", params.inFile);
	result = bios.loadFromFile(params.inFile, biosParams);
	if (result != BIOS_LOAD_STATUS_SUCCESS) // bldr needs to be valid to decompress kernel. 
	{
		return 1;
	}

	// decompress the kernel
	if (bios.decompressKrnl() != 0)
	{
		printf("Error: Failed to decompress the kernel image\n");
		return 1;
	}
	
	UCHAR* decompressedKrnl = bios.getDecompressedKrnl();
	UINT decompressedSize = bios.getDecompressedKrnlSize();

	result = dump_nt_headers(decompressedKrnl, decompressedSize, true);
	if (result != 0)
	{
		return 1;
	}
	
	result = extractPubKey(decompressedKrnl, decompressedSize);

	// get the filename
	const char* filename = params.outFile;
	if (filename == NULL)
	{
		// try krnl file
		filename = params.krnlFile;
		if (filename == NULL)
		{
			filename = "krnl.img";
		}
	}

	// write out the kernel image to file
	result = writeFileF(filename, decompressedKrnl, decompressedSize, "kernel image");
	
	return result;
}

int readKeys()
{
	if (params.bldrKeyFile != NULL)
	{
		printf("bldr key file: %s\n", params.bldrKeyFile);
		params.keyBldr = readFile(params.bldrKeyFile, NULL, KEY_SIZE);
		if (params.keyBldr == NULL)
			return 1;
		printf("bldr key: ");
		printData(params.keyBldr, KEY_SIZE);
	}

	if (params.krnlKeyFile != NULL)
	{
		printf("krnl key file: %s\n", params.krnlKeyFile);
		params.keyKrnl = readFile(params.krnlKeyFile, NULL, KEY_SIZE);
		if (params.keyKrnl == NULL)
			return 1;
		printf("krnl key: ");
		printData(params.keyKrnl, KEY_SIZE);
	}

	if (params.keyBldr != NULL || params.keyKrnl != NULL)
		printf("\n");

	return 0;
}
int readMCPX()
{
	UCHAR* mcpxData = NULL;
	int result = 0;

	if (params.mcpxFile == NULL)
		return 0;

	mcpxData = readFile(params.mcpxFile, NULL, MCPX_BLOCK_SIZE);
	if (mcpxData == NULL)
		return 1;

	result = params.mcpx.load(mcpxData);

	printf("mcpx file: %s ", params.mcpxFile);
	switch (params.mcpx.version)
	{
		case Mcpx::MCPX_V1_0:
			printf("( mcpx v1.0 )\n");
			break;
		case Mcpx::MCPX_V1_1:
			printf("( mcpx v1.1 )\n");
			break;
		case Mcpx::MOUSE_V1_0:
			printf("( mouse rev 0 )\n");
			break;
		case Mcpx::MOUSE_V1_1:
			printf("( mouse rev 1 )\n");
			break;
		default:
			printf("\nError: mcpx is invalid\n");
			break;
	}

	return result;
}

int extractPubKey(UCHAR* data, UINT size) 
{
	if (params.pubKeyFile == NULL)
	{
		return 0;
	}

	if (data == NULL || size == 0 || size < sizeof(PUBLIC_KEY))
	{
		printf("Error: Invalid data\n");
		return 1;
	}

	PUBLIC_KEY* pubkey = NULL;	
	for (UINT i = 0; i < size - sizeof(PUBLIC_KEY); i++)
	{
		pubkey = rsaVerifyPublicKey(data + i);
		if (pubkey != NULL)
		{
			break;
		}
	}

	if (pubkey == NULL)
	{
		printf("Public key not found\n");
		return 1;
	}

	printf("Public key found at offset 0x%x\n", (UINT)((UCHAR*)pubkey - data));
	rsaPrintPublicKey(pubkey);
	
	if (isFlagClear(SW_PATCH_KEYS))
	{
		if (writeFileF(params.pubKeyFile, pubkey, sizeof(PUBLIC_KEY), "public key") != 0)
		{
			return 1;
		}
	}
	else
	{
		printf("\nPatching public key..\n");

		PUBLIC_KEY* patch_pubkey = (PUBLIC_KEY*)readFile(params.pubKeyFile, NULL, sizeof(PUBLIC_KEY));
		if (patch_pubkey == NULL)
		{
			printf("Error: Failed to patch public key\n");
			return 1;
		}

		if (memcmp(pubkey->modulus, patch_pubkey->modulus, sizeof(pubkey->modulus)) == 0)
		{
			printf("Public key matches patch key\n");
		}
		else
		{
			memcpy(pubkey->modulus, patch_pubkey->modulus, sizeof(pubkey->modulus));
			rsaPrintPublicKey(pubkey);
			printf("Public key patched.\n");
		}

		free(patch_pubkey);
		patch_pubkey = NULL;
	}

	return 0;
}
int dumpImg(UCHAR* data, UINT size) 
{
	int result = 0;
	UCHAR* allocData = NULL;

	if (data == NULL) // read in the kernel image if not provided
	{		
		printf("Image file: %s\n", params.inFile);
		allocData = readFile(params.inFile, &size);
		if (allocData == NULL)
		{
			return 1;
		}
		data = allocData;
	}

	// dump the kernel image to the console
	printf("Image size: %d bytes\n", size);
		
	result = dump_nt_headers(data, size, false);
	
	if (allocData != NULL)
	{
		free(allocData);
		allocData = NULL;
		data = NULL;
	}

	return result;
}

UCHAR* load_init_tbl_file(UINT& size, UINT& xcodeBase)
{
	UCHAR* inittbl = NULL;
	int result = 0;

	printf("inittbl file: %s\n", params.inFile);

	inittbl = readFile(params.inFile, &size);
	if (inittbl == NULL)
		return NULL;

	if (isFlagSet(SW_XCODE_BASE))
	{
		xcodeBase = params.xcodeBase;
	}
	else
	{
		if (size < sizeof(INIT_TBL))
		{
			xcodeBase = 0;
		}
		else
		{
			xcodeBase = sizeof(INIT_TBL);
		}
	}

	if (xcodeBase >= size)
	{
		printf("Error: Invalid xcode base: %d. Expected less than %d bytes\n", xcodeBase, size);
		result = 1;
		goto Cleanup;
	}

	if (isFlagClear(SW_NO_MAX_SIZE) && size > MAX_BIOS_SIZE)
	{
		printf("Error: Invalid inittbl size: %d. Expected less than %d bytes\n", size, MAX_BIOS_SIZE);
		result = 1;
		goto Cleanup;
	}

Cleanup:

	if (result != 0)
	{
		if (inittbl != NULL)
		{
			free(inittbl);
			inittbl = NULL;
		}

		return NULL;
	}

	return inittbl;
}
void printBldrInfo(Bios* bios)
{
	Bldr* bldr = bios->getBldr();
	
	printf("\n2BL:\n");

	if (bldr->bootParamsValid)
	{
		printf("entry point:\t\t0x%X\n", bldr->ldrParams->bldrEntryPoint);

		if (bldr->entry != NULL)
		{
			printf("bfm entry point:\t0x%X\n", bldr->entry->bfmEntryPoint);
		}
	}

	printf("signature:\t\t");
	printData((UCHAR*)&bldr->bootParams->signature, 4);

	UINT krnlSize = bldr->bootParams->krnlSize;
	UINT krnlDataSize = bldr->bootParams->krnlDataSize;
	UINT initTblSize = bldr->bootParams->inittblSize;

	printf("inittbl size:\t\t");
	print((CON_COL)(initTblSize >= 0 && initTblSize <= params.romsize), "%ld", initTblSize);
	printf(" bytes");
	if (initTblSize == 0)
		printf(" ( not hashed )");
	printf("\n");

	printf("kernel size:\t\t");
	print((CON_COL)(krnlSize >= 0 && krnlSize <= params.romsize), "%ld", krnlSize);
	printf(" bytes\n");

	printf("kernel data size:\t");
	print((CON_COL)(krnlDataSize >= 0 && krnlDataSize <= params.romsize), "%ld", krnlDataSize);
	printf(" bytes\n");

}
void printPreldrInfo(Bios* bios)
{
	Preldr* preldr = bios->getPreldr();

	if (preldr->status > PRELDR_STATUS_INVALID_BLDR)
		return;

	printf("\nPreldr:\n");
	printf("preldr entry offset:\t0x%X\n", preldr->jmpOffset);

	if (preldr->status == PRELDR_STATUS_OK)
	{
		printf("bldr entry offset:\t0x%X\n", *preldr->bldrEntryOffset);
	}
}
void printInitTblInfo(Bios* bios)
{
	INIT_TBL* initTbl = bios->getInitTbl();
	USHORT kernel_ver = initTbl->kernel_ver;

	printf("\nInit tbl:\n");

	if ((kernel_ver & 0x8000) != 0)
	{
		kernel_ver = kernel_ver & 0x7FFF; // clear the 0x8000 bit
		printf("KD Delay flag set\t0x%04X\n", 0x8000);
	}

	if (kernel_ver != 0)
	{
		printf("Version:\t\t%d ( %04X )\n", kernel_ver, kernel_ver);
	}

	printf("Identifier:\t\t");
	switch (initTbl->init_tbl_identifier)
	{
	case 0x30: // dvt3
		printf("DVT3");
		break;
	case 0x46: // dvt4.
		printf("DVT4");
		break;
	case 0x60: // dvt6.
		printf("DVT6");
		break;
	case 0x70: // retail v1.0 - v1.1. i've only seen this ID in bios built for mcpx v1.0
	case 0x80: // retail > v1.1. i've only seen this ID in bios built for mcpx v1.1
		printf("Retail");
		break;
	default:
		printf("UKN");
		break;
	}
	printf(" ( %04x )\nRevision:\t\trev %d.%02d\n", initTbl->init_tbl_identifier, initTbl->revision >> 8, initTbl->revision & 0xFF);
}
void printNv2aInfo(Bios* bios)
{
	INIT_TBL* initTbl = bios->getInitTbl();

	int i;
	UINT item;

	const UINT UINT_ALIGN = sizeof(UINT);
	const UINT ARRAY_BYTES = sizeof(initTbl->vals);
	const UINT ARRAY_SIZE = ARRAY_BYTES / UINT_ALIGN;
	const UINT ARRAY_START = (ARRAY_BYTES - UINT_ALIGN) / UINT_ALIGN;

	const char* init_tbl_comments[5] = {
		"; init tbl revision","","",
		"; kernel version + init tbl identifier",
		"; rom data tbl offset",
	};

	printf("\nNV2A Init Tbl:\n");

	// print the first part of the table
	for (i = 0; i < ARRAY_START; i++)
	{
		item = ((UINT*)initTbl)[i];
		printf("%04x:\t\t0x%08x\n", i * UINT_ALIGN, item);
	}

	// print the array
	char str[14 * 4 + 1] = { 0 };
	bool hasValue = false;
	bool cHasValue = false;
	const UINT* valArray = initTbl->vals;

	for (i = 0; i < ARRAY_SIZE; i++)
	{
		cHasValue = false;

		if (*(UINT*)&valArray[i] != 0)
		{
			cHasValue = true;
			hasValue = true;
		}

		if (cHasValue && hasValue)
			sprintf(str + i * 3, "%02X ", *(UINT*)valArray[i]);
	}
	if (hasValue)
	{
		if (str[0] == '\0')
		{
			i = 0;
			while (str[i] == '\0' && i < ARRAY_SIZE * 3) // sanity check
			{
				memcpy(str + i, "00 ", 3);
				i += 3;
			}
		}
	}
	else
	{
		strcat(str, "0x00..0x00");
	}
	printf("%04x-%04x:\t%s\n", ARRAY_START * UINT_ALIGN, (ARRAY_START * UINT_ALIGN) + (ARRAY_SIZE * UINT_ALIGN) - UINT_ALIGN, str);

	// print the second part of the table
	for (i = ARRAY_START + ARRAY_SIZE; i < sizeof(INIT_TBL) / UINT_ALIGN; i++)
	{
		item = ((UINT*)initTbl)[i];
		printf("%04x:\t\t0x%08x %s\n", i * UINT_ALIGN, item, init_tbl_comments[i - ARRAY_START - ARRAY_SIZE]);
	}
}
void printDataTbl(Bios* bios)
{
	ROM_DATA_TBL* dataTbl = bios->getDataTbl();

	static const char* padTblName[5] = {
		"Fastest",
		"Fast",
		"Standard",
		"Slow",
		"Slowest"
	};

	printf("\nDrv/Slw data tbl:\n" \
		"\tmax mem clk: %d\n\n" \
		"Calibration parameters:\n\t\t\tCOUNT A\t\tCOUNT B\n" \
		"\tslowest:\t0x%04X\t\t0x%04X\n" \
		"\tslow:\t\t0x%04X\t\t0x%04X\n" \
		"\tstandard:\t0x%04X\t\t0x%04X\n" \
		"\tfast:\t\t0x%04X\t\t0x%04X\n" \
		"\tfastest:\t0x%04X\t\t0x%04X\n",
		dataTbl->cal.max_m_clk,
		dataTbl->cal.slowest.countA, dataTbl->cal.slowest.countB,
		dataTbl->cal.slow.countA, dataTbl->cal.slow.countB,
		dataTbl->cal.standard.countA, dataTbl->cal.standard.countB,
		dataTbl->cal.fast.countA, dataTbl->cal.fast.countB,
		dataTbl->cal.fastest.countA, dataTbl->cal.fastest.countB);

	printf("\nPad parameters:\t\t Samsung\t Micron\n\t\t\tFALL RISE\tFALL RISE");
	for (int i = 0; i < 5; i++)
	{
		printf("\n%s:\n" \
			"\taddr drv:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"\taddr slw:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"\tclk drv:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"\tclk slw:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"\tdat drv:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"\tdat slw:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"\tdsq drv:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"\tdsq slw:\t0x%02X 0x%02X\t0x%02X 0x%02X\n\n" \
			"\tdata delay:\t0x%02X\t\t0x%02X\n" \
			"\tclk delay:\t0x%02X\t\t0x%02X\n" \
			"\tdsq delay:\t0x%02X\t\t0x%02X\n", padTblName[i],
			dataTbl->samsung[i].adr_drv_fall, dataTbl->samsung[i].adr_drv_rise, dataTbl->micron[i].adr_drv_fall, dataTbl->micron[i].adr_drv_rise,
			dataTbl->samsung[i].adr_slw_fall, dataTbl->samsung[i].adr_slw_rise, dataTbl->micron[i].adr_slw_fall, dataTbl->micron[i].adr_slw_rise,
			dataTbl->samsung[i].clk_drv_fall, dataTbl->samsung[i].clk_drv_rise, dataTbl->micron[i].clk_drv_fall, dataTbl->micron[i].clk_drv_rise,
			dataTbl->samsung[i].clk_slw_fall, dataTbl->samsung[i].clk_slw_rise, dataTbl->micron[i].clk_slw_fall, dataTbl->micron[i].clk_slw_rise,
			dataTbl->samsung[i].dat_drv_fall, dataTbl->samsung[i].dat_drv_rise, dataTbl->micron[i].dat_drv_fall, dataTbl->micron[i].dat_drv_rise,
			dataTbl->samsung[i].dat_slw_fall, dataTbl->samsung[i].dat_slw_rise, dataTbl->micron[i].dat_slw_fall, dataTbl->micron[i].dat_slw_rise,
			dataTbl->samsung[i].dqs_drv_fall, dataTbl->samsung[i].dqs_drv_rise, dataTbl->micron[i].dqs_drv_fall, dataTbl->micron[i].dqs_drv_rise,
			dataTbl->samsung[i].dqs_slw_fall, dataTbl->samsung[i].dqs_slw_rise, dataTbl->micron[i].dqs_slw_fall, dataTbl->micron[i].dqs_slw_rise,
			dataTbl->samsung[i].dat_inb_delay, dataTbl->micron[i].dat_inb_delay, dataTbl->samsung[i].clk_ic_delay, dataTbl->micron[i].clk_ic_delay,
			dataTbl->samsung[i].dqs_inb_delay, dataTbl->micron[i].dqs_inb_delay);
	}
}
void printKeys(Bios* bios)
{
	Bldr* bldr = bios->getBldr();
	Preldr* preldr = bios->getPreldr();
	Mcpx* mcpx = bios->getParams()->mcpx;

	if (bldr->keys != NULL)
	{
		printf("\nBldr keys:\nbfm key:\t");
		printData(bldr->keys->bfmKey, KEY_SIZE);
		printf("eeprom key:\t");
		printData(bldr->keys->eepromKey, KEY_SIZE);
		printf("cert key:\t");
		printData(bldr->keys->certKey, KEY_SIZE);
		printf("kernel key:\t");
		printData(bldr->keys->krnlKey, KEY_SIZE);
	}

	if (preldr->status == PRELDR_STATUS_OK)
	{
		printf("\npreldr key:\t");
		printData(preldr->bldrKey, SHA1_DIGEST_LEN);
	}

	if (mcpx->sbkey != NULL)
	{
		printf("\nsb key (+%d):\t", mcpx->sbkey - mcpx->data);
		printData(mcpx->sbkey, KEY_SIZE);
	}
}

void info()
{
	printf("Author: tommojphillips\n" \
		"Github: https://github.com/tommojphillips\n" \
		"Bulit: %s %s\n", \
		__TIME__, __DATE__);

	// license
	printf("\nThis program is free software: you can redistribute it and/or modify\n" \
		"it under the terms of the GNU General Public License as published by\n" \
		"the Free Software Foundation, either version 3 of the License, or\n" \
		"(at your option) any later version.\n");
}
void printHelp()
{
	if (isFlagSet(SW_HELP))
	{
		switch (cmd->type)
		{
		case CMD_HELP:
			printf("# Get Specific help about a command.\n\n");
			printf("Usage: xbios <command> -?\n");
			break;
		case CMD_LIST_BIOS:
			printf("# %s\n\n %s (req) *inferred\n %s\n %s\n %s\n %s\n\n",
				HELP_STR_LIST, HELP_STR_PARAM_BIOS_FILE, HELP_STR_PARAM_LS_DATA_TBL,
				HELP_STR_PARAM_LS_NV2A_TBL, HELP_STR_PARAM_LS_DUMP_KRNL, HELP_STR_PARAM_LS_KEYS);
			printf("Usage: xbios -ls <bios_path> [switches]\n");
			break;
		case CMD_EXTRACT_BIOS:
			printf("# %s\n\n %s (req) *inferred\n %s\n\n",
				HELP_STR_EXTR_ALL, HELP_STR_PARAM_BIOS_FILE, HELP_STR_PARAM_EXTRACT_KEYS);
			printf("Usage: xbios -extr <bios_path> [switches]\n");
			break;
		case CMD_BUILD_BIOS:
			printf("# %s\n\n %s (req)\n %s (req)\n %s (req)\n %s (req)\n %s\n %s\n %s %s\n %s %s\n %s\n %s\n %s\n %s\n\n",
				HELP_STR_BUILD, HELP_STR_PARAM_BLDR, HELP_STR_PARAM_KRNL, HELP_STR_PARAM_KRNL_DATA, HELP_STR_PARAM_INITTBL, HELP_STR_PARAM_PRELDR,
				HELP_STR_PARAM_OUT_FILE, HELP_STR_PARAM_ROMSIZE, HELP_STR_VALID_ROM_SIZES, HELP_STR_PARAM_BINSIZE, HELP_STR_VALID_ROM_SIZES,
				HELP_STR_PARAM_PATCH_KEYS, HELP_STR_PARAM_CERT_KEY, HELP_STR_PARAM_EEPROM_KEY, HELP_STR_PARAM_BFM);
			printf("Usage:\nxbios -bld -bldr <path> -krnl <path> -krnldata <path> -inittbl <path> [switches]\n");
			break;
		case CMD_DECOMPRESS_KERNEL:
			printf("# %s\n\n %s (req) *inferred\n %s\n %s\n %s\n\n",
				HELP_STR_DECOMP_KRNL, HELP_STR_PARAM_BIOS_FILE, HELP_STR_PARAM_OUT_FILE, HELP_STR_PARAM_PATCH_KEYS, HELP_STR_PARAM_PUB_KEY);
			printf("Usage: xbios -decomp-krnl <bios_path> [switches]\n");
			break;
		case CMD_SPLIT_BIOS:
			printf("# %s\n\n %s (req) *inferred\n %s %s\n\n",
				HELP_STR_SPLIT, HELP_STR_PARAM_BIOS_FILE, HELP_STR_PARAM_ROMSIZE, HELP_STR_VALID_ROM_SIZES);
			printf("Usage: xbios -split <bios_path> [-romsize <size>]\n");
			break;
		case CMD_COMBINE_BIOS:
			printf("# %s\n\n -bank[1-4] <path>  bank file (req) *inferred\n %s\n\n",
				HELP_STR_COMBINE, HELP_STR_PARAM_OUT_FILE);
			printf("Usage: xbios -combine <bios_path1> <bios_path2> .. [switches]\n");
			break;
		case CMD_SIMULATE_XCODE:
			printf("# %s\n\n %s (req) *inferred\n %s\n %s\n %s\n -d\t\t  - write sim to a file. Use -out to set output.\n\n",
				HELP_STR_XCODE_SIM, HELP_STR_PARAM_IN_FILE, HELP_STR_PARAM_SIM_SIZE,
				HELP_STR_PARAM_XCODE_BASE, HELP_STR_PARAM_NO_MAX_SIZE);
			printf("Usage: xbios -xcode-sim <path> [switches]\n");
			break;
		case CMD_DECODE_XCODE:
		{
			if (isFlagSet(SW_INI_FILE))
			{
				printf("Usage: xbios -xcode-decode <bios_path> -ini <ini_path> [switches]\n");

				LOADINI_RETURN_MAP map = getDecodeSettingsMap();
				const char* setting_type;
				printf("\nDecode settings:\n");
				for (UINT i = 0; i < map.count; i++)
				{
					switch (map.s[i].type)
					{
					case LOADINI_SETTING_TYPE::STR:
						setting_type = "str";
						break;
					case LOADINI_SETTING_TYPE::BOOL:
						setting_type = "bool";
						break;
					default:
						setting_type = "unknown";
						break;
					}
					printf(" [%s]\t%s\n", setting_type, map.s[i].key);
				}
			}
			else
			{
				printf("# %s\n\n %s (req) *inferred\n %s\n %s\n %s\n -d\t\t  - write xcodes to a file. Use -out to set output.\n\n",
					HELP_STR_XCODE_DECODE, HELP_STR_PARAM_IN_FILE, HELP_STR_PARAM_DECODE_INI, HELP_STR_PARAM_XCODE_BASE, HELP_STR_PARAM_NO_MAX_SIZE);
				printf("Usage: xbios -xcode-decode <bios_path> [switches]\nUse -xcode-decode -? -ini to get a list of decode settings.\n");
			}
			break;
		}
		case CMD_ENCODE_X86:
			printf("# %s\n\n %s (req) *inferred\n %s\n\n",
				HELP_STR_X86_ENCODE, HELP_STR_PARAM_IN_FILE, HELP_STR_PARAM_OUT_FILE);
			printf("Usage: xbios -x86-encode <path> [switches]\n");
			break;
		case CMD_DUMP_PE_IMG:
			printf("# %s\n\n %s (req) *inferred\n\n",
				HELP_STR_DUMP_NT_IMG, HELP_STR_PARAM_IN_FILE);
			printf("Usage: xbios -dump-img <pe_img_path>\n");
			break;
		case CMD_INFO:
			printf("# %s\n\n",
				HELP_STR_INFO);
			printf("Usage: xbios info\n");
			break;
		}
		return;
	}

	printf("Commands:\n");
	for (int i = CMD_HELP; i < sizeof(cmd_tbl) / sizeof(CMD_TBL); i++)
	{
		printf(" %s\n", cmd_tbl[i].sw);
	}
	printf("\n%s\n\nFor more info on a specific command, use xbios <command> -?\n", HELP_USAGE_STR);
}

int validateArgs()
{
	// rom size in kb
	if (isFlagClear(SW_ROMSIZE))
	{
		params.romsize = DEFAULT_ROM_SIZE;
	}
	params.romsize *= 1024;
	if (checkBiosSize(params.romsize) != 0)
	{
		printf("Error: Invalid rom size: %d\n", params.romsize);
		return 1;
	}

	// bin size in kb
	if (isFlagClear(SW_BINSIZE))
	{
		params.binsize = DEFAULT_ROM_SIZE;
	}
	params.binsize *= 1024;
	if (checkBiosSize(params.binsize) != 0)
	{
		printf("Error: Invalid bin size: %d\n", params.binsize);
		return 1;
	}

	// xcode sim size in bytes
	if (isFlagClear(SW_SIM_SIZE))
	{
		params.simSize = 32;
	}
	if (params.simSize < 32 || params.simSize >(128 * 1024 * 1024)) // 32bytes - 128mb
	{
		printf("Error: Invalid sim size: %d\n", params.simSize);
		return 1;
	}
	if (params.simSize % 4 != 0)
	{
		printf("Error: simsize must be devisible by 4.\n");
		return 1;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	int result = 0;

	printf(XB_BIOS_TOOL_NAME_STR);
#ifdef __SANITIZE_ADDRESS__
	printf(" /fsanitize");
#endif
	printf("\n\n");

	result = parseCli(argc, argv, cmd, cmd_tbl, sizeof(cmd_tbl), param_tbl, sizeof(param_tbl));
	if (result != 0)
	{
		switch (result)
		{
		case CLI_ERROR_NO_CMD:
		case CLI_ERROR_INVALID_CMD:
		case CLI_ERROR_UNKNOWN_CMD:
			printf("%s\nUse xbios -? for more info.\n", HELP_USAGE_STR);
			break;
		}

		if (isFlagClear(SW_HELP))
		{
			result = ERROR_FAILED;
			goto Exit;
		}
	}

	result = validateArgs();
	if (result != 0)
	{
		goto Exit;
	}

	if (cmd->type == CMD_HELP || isFlagSet(SW_HELP))
	{
		printHelp();
		goto Exit;
	}

	if (readKeys() != 0)
		return 1;

	if (readMCPX() != 0)
		return 1;

	switch (cmd->type)
	{
		case CMD_LIST_BIOS:
			result = listBios();
			break;
		case CMD_EXTRACT_BIOS:
			result = extractBios();
			break;
		case CMD_SPLIT_BIOS:
			result = splitBios();
			break;
		case CMD_COMBINE_BIOS:
			result = combineBios();
			break;
		case CMD_BUILD_BIOS:
			result = buildBios();
			break;
		case CMD_SIMULATE_XCODE:
			result = simulateXcodes();
			break;
		case CMD_DECOMPRESS_KERNEL:
			result = decompressKrnl();
			break;
		case CMD_DECODE_XCODE:
			result = decodeXcodes();
			break;
		case CMD_ENCODE_X86:
			result = encodeX86();
			break;
		case CMD_DUMP_PE_IMG:
			result = dumpImg(NULL, 0);
			break;
		case CMD_INFO:
			info();

		default:
			result = 1;
			break;
	}

Exit:

	if (result == 0)
	{
		result = getErrorCode();
		if (result != 0)
		{
			printf("Error: %d\n", result);
		}
	}

	params.~Parameters();

#ifdef MEM_TRACKING
	mem_report();
#endif

	return result;
}
