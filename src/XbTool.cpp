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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <malloc.h>

// user incl
#include "XbTool.h"
#include "Bios.h"
#include "Mcpx.h"
#include "XcodeInterp.h"
#include "XcodeDecoder.h"
#include "file.h"
#include "util.h"
#include "nt_headers.h"
#include "rc4.h"
#include "rsa.h"
#include "sha1.h"
#include "lzx.h"
#include "help_strings.h"
#include "version.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#endif

static XbToolParameters params;
static const CMD_TBL* cmd;

static const CMD_TBL cmd_tbl[] = {
	{ "?", CMD_HELP, {SW_NONE}, {SW_NONE} },
	{ "info", CMD_INFO, {SW_NONE}, {SW_NONE} },
	{ "ls", CMD_LIST_BIOS, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "extr", CMD_EXTRACT_BIOS, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "bld", CMD_BUILD_BIOS, {SW_BLDR_FILE, SW_KRNL_FILE, SW_KRNL_DATA_FILE, SW_INITTBL_FILE}, {SW_NONE} },
	{ "split", CMD_SPLIT_BIOS, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "combine", CMD_COMBINE_BIOS, {SW_NONE}, {SW_BANK1_FILE, SW_BANK2_FILE, SW_BANK3_FILE, SW_BANK4_FILE} },
	{ "xcode-sim", CMD_SIMULATE_XCODE, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "xcode-decode", CMD_DECODE_XCODE, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "x86-encode", CMD_ENCODE_X86, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "dump-img", CMD_DUMP_PE_IMG, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "replicate", CMD_REPLICATE_BIOS, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "compress", CMD_COMPRESS_FILE, {SW_IN_FILE, SW_OUT_FILE}, {SW_IN_FILE} },
	{ "decompress", CMD_DECOMPRESS_FILE, {SW_IN_FILE, SW_OUT_FILE}, {SW_IN_FILE} },
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
	{ "img", NULL, SW_DUMP_KRNL, PARAM_TBL::FLAG },

	{ "pubkey", &params.pubKeyFile, SW_PUB_KEY_FILE, PARAM_TBL::STR },
	{ "certkey", &params.certKeyFile, SW_CERT_KEY_FILE, PARAM_TBL::STR },
	{ "eepromkey", &params.eepromKeyFile, SW_EEPROM_KEY_FILE, PARAM_TBL::STR },

	{ "bfm", NULL, SW_BLD_BFM, PARAM_TBL::FLAG },
	{ "d", NULL, SW_DMP, PARAM_TBL::FLAG },
	{ "simsize", &params.simSize, SW_SIM_SIZE, PARAM_TBL::INT },
	{ "nomaxsize", NULL, SW_NO_MAX_SIZE, PARAM_TBL::FLAG },

	{ "bank1", &params.bankFiles[0], SW_BANK1_FILE, PARAM_TBL::STR },
	{ "bank2", &params.bankFiles[1], SW_BANK2_FILE, PARAM_TBL::STR },
	{ "bank3", &params.bankFiles[2], SW_BANK3_FILE, PARAM_TBL::STR },
	{ "bank4", &params.bankFiles[3], SW_BANK4_FILE, PARAM_TBL::STR },

	{ "ini", &params.settingsFile, SW_INI_FILE, PARAM_TBL::STR },
	{ "?", NULL, SW_HELP, PARAM_TBL::FLAG },
	{ "keys", NULL, SW_KEYS, PARAM_TBL::FLAG },
	{ "base", &params.base, SW_BASE, PARAM_TBL::INT },
	{ "hackinittbl", NULL, SW_HACK_INITTBL, PARAM_TBL::FLAG },
	{ "hacksignature", NULL, SW_HACK_SIGNATURE, PARAM_TBL::FLAG },
	{ "nobootparams", NULL, SW_UPDATE_BOOT_PARAMS, PARAM_TBL::FLAG },
	{ "help-all", NULL, SW_HELP_ALL, PARAM_TBL::FLAG },
	{ "help-enc", NULL, SW_HELP_ENCRYPTION, PARAM_TBL::FLAG },
	{ "branch", NULL, SW_BRANCH, PARAM_TBL::FLAG },
	{ "dir", &params.workingDirectoryPath, SW_WORKING_DIRECTORY, PARAM_TBL::STR },
	{ "xcodes", &params.xcodesFile, SW_XCODES, PARAM_TBL::STR },
	{ "offset", &params.offset, SW_OFFSET, PARAM_TBL::INT },
};

uint8_t* load_init_tbl_file(uint32_t* size, uint32_t* base);

// Command Functions

int buildBios() {
	int result = 0;
	const char* filename = params.outFile;
	
	Bios bios;
	BiosParams biosParams;
	BiosBuildParams buildParams;

	bios_init_params(&biosParams);
	biosParams.mcpx = &params.mcpx;
	biosParams.keyBldr = params.keyBldr;
	biosParams.keyKrnl = params.keyKrnl;
	biosParams.romsize = params.romsize;
	biosParams.encBldr = isFlagSet(SW_ENC_BLDR);
	biosParams.encKrnl = isFlagSet(SW_ENC_KRNL);

	bios_init_build_params(&buildParams);
	buildParams.bfm = isFlagSet(SW_BLD_BFM);
	buildParams.hackinittbl = isFlagSet(SW_HACK_INITTBL);
	buildParams.hacksignature = isFlagSet(SW_HACK_SIGNATURE);
	buildParams.nobootparams = isFlagSet(SW_UPDATE_BOOT_PARAMS);
	
	// init tbl file	
	buildParams.inittbl = readFile(params.inittblFile, &buildParams.inittblSize, 0);
	if (buildParams.inittbl == NULL) {
		result = 1;
		goto Cleanup;
	}

	// preldr file
	if (params.preldrFile != NULL) {
		buildParams.preldr = readFile(params.preldrFile, &buildParams.preldrSize, 0);
		if (buildParams.preldr == NULL) {
			buildParams.preldrSize = 0;
		}
	}

	// 2bl file
	buildParams.bldr = readFile(params.bldrFile, &buildParams.bldrSize, 0);
	if (buildParams.bldr == NULL) {
		result = 1;
		goto Cleanup;
	}

	// compressed krnl image
	buildParams.krnl = readFile(params.krnlFile, &buildParams.krnlSize, 0);
	if (buildParams.krnl == NULL) {
		result = 1;
		goto Cleanup;
	}

	// uncompressed kernel data
	buildParams.krnlData = readFile(params.krnlDataFile, &buildParams.krnlDataSize, 0);
	if (buildParams.krnlData == NULL) {
		result = 1;
		goto Cleanup;
	}

	// eeprom key
	if (params.eepromKeyFile != NULL) {
		buildParams.eepromKey = readFile(params.eepromKeyFile, NULL, XB_KEY_SIZE);		
	}

	// cert key
	if (params.certKeyFile != NULL) {
		buildParams.certKey = readFile(params.certKeyFile, NULL, XB_KEY_SIZE);		
	}

	result = bios.build(&buildParams, params.binsize, &biosParams);

	// xcodes
	if (isFlagSet(SW_XCODES)) {
		uint32_t xcodesSize;
		uint8_t* xcodes = readFile(params.xcodesFile, &xcodesSize, 0);
		if (xcodes == NULL) {
			result = 1;
		}
		else {
			result = inject_xcodes(bios.data, bios.size, xcodes, xcodesSize);
			free(xcodes);
			xcodes = NULL;
		}
	}

	if (result != 0) {
		printf("failed to build bios\n");
	}
	else {
		filename = params.outFile;
		if (filename == NULL)
			filename = "bios.bin";
		result = writeFileF(filename, bios.data, bios.size);
	}

Cleanup:
		
	bios_free_build_params(&buildParams);
	
	return result;
}
int extractBios() {
	// Extract components from the bios file.

	int result = 0;
	const char* filename;	
	size_t inittblSize = 0;
	Bios bios;
	BiosParams biosParams;

	bios_init_params(&biosParams);
	biosParams.mcpx = &params.mcpx;
	biosParams.keyBldr = params.keyBldr;
	biosParams.keyKrnl = params.keyKrnl;
	biosParams.romsize = params.romsize;
	biosParams.encBldr = isFlagSet(SW_ENC_BLDR);
	biosParams.encKrnl = isFlagSet(SW_ENC_KRNL);
	biosParams.restoreBootParams = isFlagClear(SW_UPDATE_BOOT_PARAMS);
	
	uint32_t size = 0;
	uint8_t* buffer = readFile(params.inFile, &size, 0);
	if (buffer == NULL) {
		return 1;
	}

	if (bios_check_size(size) != 0) {
		printf("Error: BIOS size is invalid\n");
		return 1;
	}

	result = bios.load(buffer, size, &biosParams);
	if (result != BIOS_LOAD_STATUS_SUCCESS) {
		printf("Error: invalid 2BL\n");		
		return 1;
	}
	
	// set working directory
	if (isFlagSet(SW_WORKING_DIRECTORY)) {
		if (_chdir(params.workingDirectoryPath) == -1) {
			if (errno == ENOENT) { // directory not found
				printf("Error: '%s' directory not found.\n", params.workingDirectoryPath);				
				return 1;
			}
		}
	}

	// zero rom digest so we have a clean 2bl;
	if (bios.romDigest != NULL) {
		memset(bios.romDigest, 0, ROM_DIGEST_SIZE);
	}

	// preldr
	if (bios.preldr.status < PRELDR_STATUS_NOT_FOUND) {
		filename = params.preldrFile;
		if (filename == NULL)
			filename = "preldr.bin";
		writeFileF(filename, bios.preldr.data, PRELDR_SIZE);

		// zero preldr
		memset(bios.preldr.data, 0, PRELDR_SIZE);
		// zero preldr params.
		memset(bios.preldr.data + PRELDR_SIZE + ROM_DIGEST_SIZE, 0, PRELDR_PARAMS_SIZE - sizeof(BOOT_PARAMS));
	}

	// 2bl
	filename = params.bldrFile;
	if (filename == NULL)
		filename = "bldr.bin";
	writeFileF(filename, bios.bldr.data, BLDR_BLOCK_SIZE);
	
	// extract init tbl
	inittblSize = bios.bldr.bootParams->inittblSize;
	if (inittblSize > 0) {
		filename = params.inittblFile;
		if (filename == NULL)
			filename = "inittbl.bin";
		writeFileF(filename, bios.data, inittblSize);
	}

	// extract compressed kernel
	filename = params.krnlFile;
	if (filename == NULL)
		filename = "krnl.bin";
	writeFileF(filename, bios.krnl, bios.bldr.bootParams->krnlSize);
	
	// extract uncompressed kernel section data
	filename = params.krnlDataFile;
	if (filename == NULL)
		filename = "krnl_data.bin";
	writeFileF(filename, bios.krnlData, bios.bldr.bootParams->krnlDataSize);
	
	// decompress the kernel now so the public key can be extracted.
	if (bios.decompressKrnl() == 0) {

		// extract decompressed kernel image ( pe/coff executable )
		if (bios.decompressedKrnl != NULL) {
			writeFileF("krnl.img", bios.decompressedKrnl, bios.decompressedKrnlSize);
		}
	}

	if (isFlagSet(SW_KEYS)) {
		// 2BL rc4 keys

		if (bios.bldr.keys != NULL) {
			
			// eeprom rc4 key
			filename = params.eepromKeyFile;
			if (filename == NULL)
				filename = "eeprom_key.bin";
			writeFileF(filename, bios.bldr.keys->eepromKey, XB_KEY_SIZE);

			// cert rc4 key
			filename = params.certKeyFile;
			if (filename == NULL)
				filename = "cert_key.bin";
			writeFileF(filename, bios.bldr.keys->certKey, XB_KEY_SIZE);
			
			// kernel rc4 key
			writeFileF("krnl_key.bin", bios.bldr.keys->krnlKey, XB_KEY_SIZE);
		}

		// bfm key
		if (bios.bldr.bfmKey != NULL) {
			writeFileF("bfm_key.bin", bios.bldr.bfmKey, XB_KEY_SIZE);
		}

		// secret boot key
		if (params.mcpx.sbkey != NULL) {
			writeFileF("sb_key.bin", params.mcpx.sbkey, XB_KEY_SIZE);
		}

		// extract decompressed kernel rsa pub key
		if (bios.decompressedKrnl != NULL) {
			PUBLIC_KEY* pubkey;
			filename = params.pubKeyFile;
			if (filename == NULL)
				filename = "pubkey.bin";
			if (rsa_findPublicKey(bios.decompressedKrnl, bios.decompressedKrnlSize, &pubkey, NULL) == RSA_ERROR_SUCCESS)
				writeFileF(filename, pubkey, RSA_PUBKEY_SIZE(&pubkey->header));
		}

		// preldr; extract preldr components.
		if (bios.preldr.status < PRELDR_STATUS_NOT_FOUND) {
			writeFileF("preldr_key.bin", bios.preldr.bldrKey, SHA1_DIGEST_LEN);
		}
	}

	return 0;
}
int splitBios() {
	int result = 0;
	uint8_t* data = NULL;
	uint32_t size = 0;
	uint32_t fnLen = 0;
	uint32_t bankFnLen = 0;
	uint32_t dataLeft = 0;
	uint32_t loopCount = 0;
	int bank = 0;
	const char suffix[] = "_bank";
	char* biosFn = NULL;
	char* ext = NULL;
	char* bankFn = NULL;
	int i;
	int j;

	//romsize sanity check
	if (params.romsize < MIN_BIOS_SIZE)
		return 1;
		
	data = readFile(params.inFile, &size, 0);
	if (data == NULL)
		return 1;

	printf("bios file: %s\nbios size: %dkb\nrom size:  %dkb\n", params.inFile, size / 1024, params.romsize / 1024);

	result = bios_check_size(size);
	if (result != 0) {
		printf("Error: invalid bios file size: %d\n", size);
		goto Cleanup;
	}
	
	// check if bios size is less than or equal to rom size
	if (size <= params.romsize) {
		printf("Nothing to split.\n");
		result = 0;
		goto Cleanup;
	}

	fnLen = strlen(params.inFile);
	biosFn = (char*)malloc(fnLen + 1);
	if (biosFn == NULL) {
		result = 1;
		goto Cleanup;
	}
	for (j = fnLen - 1; j > 0; j--) {
		if (params.inFile[j] == '\\') {
			j += 1;
			break;
		}
	}
	strcpy(biosFn, params.inFile+j);
	fnLen -= j;

	// remove the extention
	for (i = fnLen - 1; i >= 0; i--) {
		if (biosFn[i] == '.') {
			ext = (char*)malloc(fnLen - i + 1); // +1 for null terminator
			if (ext == NULL) {
				result = 1;
				goto Cleanup;
			}
			strcpy(ext, biosFn + i);

			biosFn[i] = '\0';
			break;
		}
	}

	if (ext == NULL) {
		printf("Error: invalid bios file name. no file extension.\n");
		result = 1;
		goto Cleanup;
	}

	bankFnLen = strlen(biosFn) + strlen(suffix) + strlen(ext) + 3U; // +3 for the bank number
	bankFn = (char*)malloc(bankFnLen);
	if (bankFn == NULL) {
		result = 1;
		goto Cleanup;
	}

	// split the bios based on rom size

	dataLeft = size;
	loopCount = 0;
	while (dataLeft > 0) {
		// safe guard
		if (loopCount > 4) { 
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
		if (result != 0) {
			goto Cleanup;
		}

		dataLeft -= params.romsize;
		bank++;
	}

	printf("bios split into %d banks\n", bank);

Cleanup:
	if (data != NULL) {
		free(data);
	}
	if (biosFn != NULL) {
		free(biosFn);
	}
	if (bankFn != NULL) {
		free(bankFn);
	}
	if (ext != NULL) {
		free(ext);
	}

	return result;
}
int combineBios() {
	const uint32_t MAX_BANKS = MAX_BIOS_SIZE / MIN_BIOS_SIZE;
	uint32_t totalSize = 0;
	uint32_t offset = 0;
	int i;
	int result = 0;
	int numBanks = 0;

	uint8_t* data = NULL;

	uint8_t* banks[MAX_BANKS] = { NULL };
	uint32_t bankSizes[MAX_BANKS] = { 0 };

	const char* filename = params.outFile;
	if (filename == NULL) {
		filename = "bios.bin";
	}

	for (i = 0; i < MAX_BANKS; i++) {

		if (params.bankFiles[i] == NULL)
			continue;

		numBanks++;
		
		banks[i] = readFile(params.bankFiles[i], &bankSizes[i], 0);
		if (banks[i] == NULL) {
			result = 1;
			goto Cleanup;
		}

		if (bios_check_size(bankSizes[i]) != 0) {
			printf("Error: %s has invalid file size: %d\n", params.bankFiles[i], bankSizes[i]);
			result = 1;
			goto Cleanup;
		}
		
		totalSize += bankSizes[i];
	}

	if (numBanks < 2) {
		printf("Error: not enough banks to combine. Expected atleast 2 banks\n");
		result = 1;
		goto Cleanup;
	}

	if (bios_check_size(totalSize) != 0) {
		printf("Error: invalid total bios size: %d\n", totalSize);
		result = 1;
		goto Cleanup;
	}

	data = (uint8_t*)malloc(totalSize);
	if (data == NULL) {
		result = 1;
		goto Cleanup;
	}

	for (i = 0; i < MAX_BANKS; i++) {
		if (banks[i] != NULL) {
			printf("copying %s %d kb into offset 0x%x (bank %d)\n", params.bankFiles[i], bankSizes[i] / 1024, offset, i + 1);
			memcpy(data + offset, banks[i], bankSizes[i]);
			offset += bankSizes[i];
		}
	}

	result = writeFileF(filename, data, totalSize);
	
Cleanup:

	if (data != NULL) {
		free(data);
	}

	for (i = 0; i < MAX_BANKS; i++) {
		if (banks[i] != NULL) {
			free(banks[i]);
			banks[i] = NULL;
		}
	}

	return result;
}
int listBios() {
	int result = 0;
	int biosStatus = 0;
	Bios bios;
	BiosParams biosParams;

	bios_init_params(&biosParams);
	biosParams.mcpx = &params.mcpx;
	biosParams.keyBldr = params.keyBldr;
	biosParams.keyKrnl = params.keyKrnl;
	biosParams.romsize = params.romsize;
	biosParams.encBldr = isFlagSet(SW_ENC_BLDR);
	biosParams.encKrnl = isFlagSet(SW_ENC_KRNL);
	biosParams.restoreBootParams = isFlagClear(SW_UPDATE_BOOT_PARAMS);

	uint32_t size = 0;
	uint8_t* buffer = readFile(params.inFile, &size, 0);
	if (buffer == NULL) {
		return 1;
	}

	if (bios_check_size(size) != 0) {
		printf("Error: BIOS size is invalid\n");
		return 1;
	}

	biosStatus = bios.load(buffer, size, &biosParams);	
	if (biosStatus > BIOS_LOAD_STATUS_INVALID_BLDR) {
		printf("Error: failed to load BIOS\n");
		return 1;
	}
		
	if (isFlagSet(SW_LS_NV2A_TBL)) {
		// nv2a
		printNv2aInfo(&bios);
	}
	else if (isFlagSet(SW_LS_DATA_TBL)) {
		// rom data table
		printDataTblInfo(&bios);
	}
	else if (isFlagSet(SW_KEYS)) {
		// keys
		if (biosStatus != BIOS_LOAD_STATUS_SUCCESS) {
			printf("Error: 2BL is invalid.\n");
			return 1;
		}

		// decompress the kernel so the public key can be dumped.
		bios.decompressKrnl();

		printf("\nKeys:\n");
		printKeyInfo(&bios);
	}
	else if (isFlagSet(SW_DUMP_KRNL)) {
		// kernel pe/coff header

		if (biosStatus != BIOS_LOAD_STATUS_SUCCESS) {
			printf("Error: 2BL is invalid.\n");
			return 1;
		}

		// decompress the kernel so the NT header can be dumped.
		bios.decompressKrnl();

		printf("Kernel:\n");
		if (bios.decompressedKrnl != NULL) {
			printf("image size: %d bytes\n", bios.decompressedKrnlSize);
			dump_nt_headers(bios.decompressedKrnl, bios.decompressedKrnlSize, false);
			print_krnl_data_section_header((IMAGE_DOS_HEADER*)bios.decompressedKrnl);
		}
		else {
			printf("Error: failed to decompress kernel image\n");
		}
	}
	else {
		// default ls command
		int valid = (bios.availableSpace >= 0 && bios.availableSpace <= (int)bios.params.romsize);

		printPreldrInfo(&bios);
		printBldrInfo(&bios);
		printInitTblInfo(&bios);

		printf("binsize:\t\t%d kb\n", bios.size / 1024);
		printf("romsize:\t\t%d kb\n", bios.params.romsize / 1024);
		printf("available space:\t");
		uprintc(valid, "%d", bios.availableSpace);
		printf(" bytes\n");
	}
	
	return result;
}
int replicateBios() {
	printf("cmd needs work.\n");
	return 1;
}
int decodeXcodes() {
	XcodeDecoder decoder;
	DECODE_CONTEXT* context;
	uint8_t* inittbl = NULL;
	uint32_t size = 0;
	uint32_t base = 0;
	int result = 0;

	if (params.settingsFile != NULL) {
		if (!fileExists(params.settingsFile)) {
			printf("Error: ini file not found.\n");
			goto Cleanup;
		}
	}

	inittbl = load_init_tbl_file(&size, &base);
	if (inittbl == NULL)
		return 1;

	result = decoder.load(inittbl, size, base, params.settingsFile);
	if (result != 0) {
		printf("failed to init xcode decoder\n");
		goto Cleanup;
	}

	context = decoder.context;
	context->branch = isFlagSet(SW_BRANCH);
	
	printf("xcode count: %d\nxcode size: %d bytes\nxcode base: 0x%x\n", context->xcodeCount, context->xcodeSize, context->xcodeBase);
	
	// setup the file stream, if -d flag is set
	if (isFlagSet(SW_DMP)) {
		const char* filename = params.outFile;
		if (filename == NULL) {
			filename = "xcodes.txt";
		}

		// del file if it exists
		deleteFile(filename);

		printf("Writing xcodes to %s\n", filename);
		FILE* stream = fopen(filename, "w");
		if (stream == NULL) {
			printf("failed to open file %s\n", filename);
			result = 1;
			goto Cleanup;
		}
		context->stream = stream;
	}
	else { // print to stdout
		if (context->xcodeCount > 2048) {
			printf("too many xcodes. Use -d flag to dump to file\n");
			result = 1;
			goto Cleanup;
		}
		context->stream = stdout;
	}
	
	result = decoder.decodeXcodes();

	if (isFlagSet(SW_DMP)) {
		fclose(context->stream);
		printf("done\n");
	}

Cleanup:

	if (inittbl != NULL) {
		free(inittbl);
	}

	return result;
}
int simulateXcodes() {
	XcodeInterp interp;
	XCODE* xcode = NULL;
	uint32_t size = 0;
	uint32_t base = 0;
	int result = 0;
	bool hasMemChanges_total = false;
	const char* opcode_str = NULL;
	uint8_t* mem_sim = NULL;
	uint8_t* inittbl = NULL;
	uint32_t codeSize = 0;
	uint32_t offset = 0;

	inittbl = load_init_tbl_file(&size, &base);
	if (inittbl == NULL)
		return 1;

	result = interp.load(inittbl + base, size - base);
	if (result != 0)
		goto Cleanup;

	if (isFlagSet(SW_OFFSET)) {
		if (offset > size - base) {
			printf("Error: offset out of bounds.\n");
			result = 1;
			goto Cleanup;
		}
		offset = params.offset;
	}

	printf("xcode base: 0x%x\nmem space: %d bytes\n\n", base, params.simSize);

	mem_sim = (uint8_t*)malloc(params.simSize);
	if (mem_sim == NULL) {
		result = 1;
		goto Cleanup;
	}
	memset(mem_sim, 0, params.simSize);

	// simulate memory output
	hasMemChanges_total = false;
	printf("Xcodes:\n");
	while (interp.interpretNext(xcode) == 0) {
		// only care about xcodes that write to RAM
		if (xcode->opcode != XC_MEM_WRITE)
			continue;

		// sanity check of addr.
		if (xcode->addr < offset || xcode->addr - offset >= params.simSize) {
			continue;
		}

		hasMemChanges_total = true;

		// write the data to simulated memory
		memcpy(mem_sim + xcode->addr - offset, (uint8_t*)&xcode->data, 4);

		if (getOpcodeStr(xcode_opcode_map, xcode->opcode, opcode_str) != 0) {
			result = 1;
			goto Cleanup;
		}

		// print the xcode
		printf("\t%04x: %s 0x%02x, 0x%08X\n", (base + interp.offset - sizeof(XCODE)), opcode_str, xcode->addr, xcode->data);
	}
	
	if (!hasMemChanges_total) {
		printf("0 memory changes in range 0x0 - 0x%x\n", params.simSize);
		goto Cleanup;
	}

	// if -d flag is set, dump the memory to a file, otherwise print the memory dump
	if (isFlagSet(SW_DMP)) {
		const char* filename = params.outFile;
		if (filename == NULL)
			filename = "mem_sim.bin";

		printf("\n");
		result = writeFileF(filename, mem_sim, codeSize);
		if (result != 0) {
			goto Cleanup;
		}
	}
	else {
		// print memory dump
		uint32_t j;
		printf("\nMem dump: ( %d bytes )\n", codeSize);
		for (uint32_t i = 0; i < codeSize; i += j) {
			if (i + 8 > codeSize)
				j = codeSize - i;
			else
				j = 8;

			printf("\t%04x: ", i);
			uprinth(mem_sim + i, j);
		}
	}

Cleanup:

	if (inittbl != NULL) {
		free(inittbl);
		inittbl = NULL;
	}

	if (mem_sim != NULL) {
		free(mem_sim);
		mem_sim = NULL;
	}

	return result;
}
int encodeX86() {
	// encode x86 instructions to xcodes

	uint8_t* data = NULL;
	uint8_t* buffer = NULL;
	uint32_t dataSize = 0;
	uint32_t xcodeSize = 0;
	int result = 0;

	const char* filename = NULL;

	printf("x86 file:\t%s\nxcode base:\t0x%x\n", params.inFile, params.base);

	data = readFile(params.inFile, &dataSize, 0);
	if (data == NULL) {
		return 1;
	}

	result = encodeX86AsMemWrites(data, dataSize, params.base, buffer, &xcodeSize);
	if (result != 0) {
		printf("Error: failed to encode x86 instructions\n");
		goto Cleanup;
	}

	printf("xcodes: %d\n", xcodeSize / sizeof(XCODE));
		
	// write the xcodes to file
	filename = params.outFile;
	if (filename == NULL) {
		filename = "xcodes.bin";
	}

	result = writeFileF(filename, buffer, xcodeSize);

Cleanup:

	if (data != NULL) {
		free(data);
	}

	if (buffer != NULL) {
		free(buffer);
	}

	return 0;
}
int compressFile() {
	// lzx compress file

	uint8_t* data = NULL;
	uint8_t* buff = NULL;
	uint32_t dataSize = 0;
	uint32_t compressedSize = 0;
	int result = 0;
	float savings = 0;

	data = readFile(params.inFile, &dataSize, 0);
	if (data == NULL) {
		return 1;
	}

	result = lzxCompress(data, dataSize, &buff, &compressedSize);
	if (result != 0) {
		printf("Error: compression failed");
		lzxPrintError(result);
		goto Cleanup;
	}

	savings = (1 - ((float)compressedSize / (float)dataSize)) * 100;
	printf("compressed %u -> %u bytes (%.3f%% compression)\n", dataSize, compressedSize, savings);

	result = writeFileF(params.outFile, buff, compressedSize);

Cleanup:

	if (data != NULL) {
		free(data);
		data = NULL;
	}

	if (buff != NULL) {
		free(buff);
		buff = NULL;
	}

	return result;

}
int decompressFile() {
	// lzx decompress file

	uint8_t* data = NULL;
	uint32_t dataSize = 0;
	float savings = 0;

	uint8_t* buff = NULL;
	uint32_t decompressedSize = 0;

	int result = 0;

	data = readFile(params.inFile, &dataSize, 0);
	if (data == NULL) {
		return 1;
	}


	result = lzxDecompress(data, dataSize, &buff, NULL, &decompressedSize);
	if (result != 0) {
		printf("Error: decompression failed");
		lzxPrintError(result);
		goto Cleanup;
	}

	savings = (1 - ((float)dataSize / (float)decompressedSize)) * 100;	
	printf("decompressed %u -> %u bytes (%.3f%% compression)\n", dataSize, decompressedSize, savings);

	result = writeFileF(params.outFile, buff, decompressedSize);

Cleanup:

	if (data != NULL) {
		free(data);
		data = NULL;
	}

	if (buff != NULL) {
		free(buff);
		buff = NULL;
	}

	return result;
}
int dumpCoffPeImg() {
	int result = 0;
	uint8_t* data = NULL;
	uint32_t size = NULL;

	printf("image file: %s\n", params.inFile);

	data = readFile(params.inFile, &size, 0);
	if (data == NULL)
		return 1;

	printf("image size: %d bytes\n", size);

	result = dump_nt_headers(data, size, false);

	free(data);
	data = NULL;

	return result;
}
int info() {
	printf("Author: tommojphillips\n" \
		"Github: https://github.com/tommojphillips/XboxBiosTool/\n" \
		"Bulit:  %s %s\n", \
		__TIME__, __DATE__);

	printf("\nThis program is free software: you can redistribute it and/or modify\n" \
		"it under the terms of the GNU General Public License as published by\n" \
		"the Free Software Foundation.\n");

	return 0;
}
int help() {
	
	if (isFlagSet(SW_HELP_ENCRYPTION)) {
		helpEncryption();
		return 0;
	}

	if (isFlagSet(SW_HELP_ALL)) {
		helpAll();
		return 0;
	}

	if (isFlagSet(SW_HELP)) {
		switch (cmd->type) {
			case CMD_LIST_BIOS:
				printf("# %s\n\n %s (req) *inferred\n %s\n %s\n %s\n %s\n\n",
					HELP_STR_LIST, HELP_STR_PARAM_IN_BIOS_FILE, HELP_STR_PARAM_LS_DATA_TBL,
					HELP_STR_PARAM_LS_NV2A_TBL, HELP_STR_PARAM_LS_DUMP_KRNL, HELP_STR_PARAM_LS_KEYS);
				printf("Usage: xbios -ls <bios_path> [switches]\n");
				return 0;

			case CMD_EXTRACT_BIOS:
				printf("# %s\n\n %s (req) *inferred\n %s\n %s\n %s\n\n",
					HELP_STR_EXTR_ALL, HELP_STR_PARAM_IN_BIOS_FILE, HELP_STR_PARAM_EXTRACT_KEYS, HELP_STR_PARAM_RESTORE_BOOT_PARAMS, HELP_STR_PARAM_WDIR);
				printf("Usage: xbios -extr <bios_path> [switches]\n");
				return 0;

			case CMD_BUILD_BIOS:
				printf("# %s\n\n %s (req)\n %s (req)\n %s (req)\n %s (req)\n %s\n %s\n %s %s\n %s %s\n %s\n %s\n %s\n %s\n\n",
					HELP_STR_BUILD, HELP_STR_PARAM_BLDR, HELP_STR_PARAM_KRNL, HELP_STR_PARAM_KRNL_DATA, HELP_STR_PARAM_INITTBL, HELP_STR_PARAM_PRELDR,
					HELP_STR_PARAM_OUT_BIOS_FILE, HELP_STR_PARAM_ROMSIZE, HELP_STR_VALID_ROM_SIZES, HELP_STR_PARAM_BINSIZE, HELP_STR_VALID_ROM_SIZES,
					HELP_STR_PARAM_BFM, HELP_STR_PARAM_HACK_INITTBL, HELP_STR_PARAM_HACK_SIGNATURE, HELP_STR_PARAM_UPDATE_BOOT_PARAMS);
				printf("Usage:\nxbios -bld -bldr <path> -krnl <path> -krnldata <path> -inittbl <path> [switches]\n");
				return 0;

			case CMD_SPLIT_BIOS:
				printf("# %s\n\n %s (req) *inferred\n %s %s\n\n",
					HELP_STR_SPLIT, HELP_STR_PARAM_IN_BIOS_FILE, HELP_STR_PARAM_ROMSIZE, HELP_STR_VALID_ROM_SIZES);
				printf("Usage: xbios -split <bios_path> [-romsize <size>]\n");
				return 0;

			case CMD_COMBINE_BIOS:
				printf("# %s\n\n -bank[1-4] <path>  bank file (req) *inferred\n %s\n\n",
					HELP_STR_COMBINE, HELP_STR_PARAM_OUT_FILE);
				printf("Usage: xbios -combine <bios_path1> <bios_path2> .. [switches]\n");
				return 0;

			case CMD_SIMULATE_XCODE:
				printf("# %s\n\n %s (req) *inferred\n %s\n %s\n -d\t\t  - write sim to a file. Use -out to set output.\n\n",
					HELP_STR_XCODE_SIM, HELP_STR_PARAM_IN_FILE, HELP_STR_PARAM_SIM_SIZE, HELP_STR_PARAM_BASE);
				printf("Usage: xbios -xcode-sim <path> [switches]\n");
				return 0;

			case CMD_DECODE_XCODE: {
				if (isFlagSet(SW_INI_FILE)) {
					LOADINI_RETURN_MAP map = decode_settings_map;
					const char* setting_type;
					printf("format: setting=value or setting='value'\n");
					printf("\nDecode settings:\n");
					for (uint32_t i = 0; i < map.count; i++) {
						switch (map.s[i].type) {
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
					printf("# %s\n\n %s (req) *inferred\n %s\n %s\n %s\n -d\t\t  - write xcodes to a file. Use -out to set output\n\n",
						HELP_STR_XCODE_DECODE, HELP_STR_PARAM_IN_FILE, HELP_STR_PARAM_DECODE_INI, HELP_STR_PARAM_BASE, HELP_STR_PARAM_BRANCH);
					printf("Use -xcode-decode -? -ini to get a list of decode settings\n\nUsage: xbios -xcode-decode <bios_path> [switches]\n");
				}
			} return 0;

			case CMD_ENCODE_X86:
				printf("# %s\n\n %s (req) *inferred\n %s\n\n",
					HELP_STR_X86_ENCODE, HELP_STR_PARAM_IN_FILE, HELP_STR_PARAM_OUT_FILE);
				printf("Usage: xbios -x86-encode <path> [switches]\n");
				return 0;

			case CMD_DUMP_PE_IMG:
				printf("# %s\n\n %s (req) *inferred\n\n",
					HELP_STR_DUMP_NT_IMG, HELP_STR_PARAM_IN_FILE);
				printf("Usage: xbios -dump-img <pe_img_path>\n");
				return 0;

			case CMD_INFO:
				printf("# %s\n\n",
					HELP_STR_INFO);
				printf("Usage: xbios -info\n");
				return 0;

			case CMD_COMPRESS_FILE:
				printf("# %s\n\n %s (req) *inferred\n %s (req)\n\n",
					HELP_STR_COMPRESS_FILE, HELP_STR_PARAM_IN_FILE, HELP_STR_PARAM_OUT_FILE);
				printf("Usage: xbios -compress <path> [switches]\n");
				return 0;

			case CMD_DECOMPRESS_FILE:
				printf("# %s\n\n %s (req) *inferred\n %s (req)\n\n",
					HELP_STR_DECOMPRESS_FILE, HELP_STR_PARAM_IN_FILE, HELP_STR_PARAM_OUT_FILE);
				printf("Usage: xbios -decompress <path> [switches]\n");
				return 0;

			case CMD_REPLICATE_BIOS:
				printf("# %s\n\n %s (req) *inferred\n %s (req) %s\n %s\n\n",
					HELP_STR_REPLICATE, HELP_STR_PARAM_IN_BIOS_FILE, HELP_STR_PARAM_BINSIZE, HELP_STR_VALID_ROM_SIZES, HELP_STR_PARAM_OUT_FILE);
				return 0;
							
			case CMD_HELP:
				break;

			default:
				return 1;
		}
	}

	printf("See command help, use xbios -? <command>\n");
	printf("See encryption help, use xbios -? -help-enc\n");
	
	printf("\nCommands:\n");
	for (int i = CMD_INFO + 1; i < sizeof(cmd_tbl) / sizeof(CMD_TBL); ++i) {
		printf(" %s\n", cmd_tbl[i].sw);
	}
	
	printf("\n%s\n", HELP_USAGE_STR);

	return 0;
}
int helpEncryption() {
	printf("\n2BL encryption / decryption:\n" \
		"Use one of the switches below to specify a key for 2BL encryption / decryption.\n"\
		"Not providing a switch results in 2BL not being encrypted / decrypted\n\n");
	
	// mcpx rom
	printf(" %s\n", HELP_STR_MCPX_ROM);

	// 2BL key
	printf("\n -key-bldr");
	printf(HELP_STR_RC4_KEY, "2BL");

	// kernel 
	printf("\n\nKernel encryption / decryption:\nOnly needed for custom BIOSes as keys are located in the 2BL.\n\n");
	printf(" -key-krnl");
	printf(HELP_STR_RC4_KEY, "kernel");

	printf("\n\n -enc-krnl");
	printf(HELP_STR_RC4_ENC, "kernel");

	printf("\n\n");
	return 0;
}
int helpAll() {
	int result = 0;
	clearFlag(SW_HELP_ALL);
	setFlag(SW_HELP);

	for (int i = 0; i < sizeof(cmd_tbl) / sizeof(CMD_TBL); i++) {
		cmd = &cmd_tbl[i];

		printf("\n");
		result = help();

		if (result != 0) {
			printf("\nhelp for '-%s' not defined\n", cmd->sw);
			break;
		}
	}

	return result;
}

// Helper functions
void init_parameters(XbToolParameters* _params) {
	memset(_params, 0, sizeof(XbToolParameters));
}
void free_parameters(XbToolParameters* _params) {
	if (_params->keyBldr != NULL) {
		free(_params->keyBldr);
		_params->keyBldr = NULL;
	}
	if (_params->keyKrnl != NULL) {
		free(_params->keyKrnl);
		_params->keyKrnl = NULL;
	}
	mcpx_free(&_params->mcpx);
}

int inject_xcodes(uint8_t* data, uint32_t size, uint8_t* xcodes, uint32_t xcodesSize) {
	int result;
	XcodeInterp interp;
	result = interp.load(data + 0x80, size - 0x80);
	if (result != 0) {
		return result;
	}

	INIT_TBL* inittbl = (INIT_TBL*)data;
	XCODE* xcode = NULL;

	while (interp.interpretNext(xcode) == 0) {
		if (xcode->opcode == XC_EXIT)
			break;
	}

	if (xcode->opcode != XC_EXIT) {
		printf("XCODE: exit xcode not found.\n");
		return 1;
	}

	bool jmp = false;
	uint32_t offset = 0;
	uint32_t count = 0;

	// if data_tbl_offset is 0 then no rom data table.
	if (inittbl->data_tbl_offset != 0) {
		if (inittbl->data_tbl_offset < 0x80 + interp.offset + xcodesSize) {
			//printf("no space for xcodes. only %d bytes available. xcode size: %d bytes\n", (0x80 + interp.offset + xcodesSize - inittbl->data_tbl_offset), xcodesSize);

			jmp = true;
			offset = sizeof(ROM_DATA_TBL);
		}
	}

	// sanity check.
	for (; offset < size - 0x80 - interp.offset - xcodesSize; offset++) {
		if (*(data + 0x80 + interp.offset + offset) != 0x0) {
			//printf("no zero space for xcodes.\n");
			count = 0;
			jmp = true;
			continue;
		}
		if (count == xcodesSize) {
			offset -= count;
			break;
		}
		count++;
	}

	xcode = (XCODE*)(data + 0x80 + interp.offset - sizeof(XCODE));

	if (jmp) {
		printf("XCODE: replacing quit xcode at 0x%x with jump to free space at 0x%x\n", (uint8_t*)xcode - data, 0x80 + interp.offset + offset);

		// patch quit xcode to a jmp xcode.
		xcode->opcode = XC_JMP;
		xcode->addr = 0;
		xcode->data = offset;

		// update xcode ptr.
		xcode = (XCODE*)(data + 0x80 + interp.offset + offset);
	}

	printf("XCODE: adding xcodes\n");

	// copy in xcodes.
	memcpy(xcode, xcodes, xcodesSize);

	// copy in quit xcode.
	xcode = (XCODE*)((uint8_t*)xcode + xcodesSize);
	xcode->opcode = XC_EXIT;
	xcode->addr = 0x806;
	xcode->data = 0;

	interp.~XcodeInterp();

	return 0;
}

int read_keys() {
	// read key files from command line.

	if (params.bldrKeyFile != NULL) {
		printf("bldr key file: %s\n", params.bldrKeyFile);
		params.keyBldr = readFile(params.bldrKeyFile, NULL, XB_KEY_SIZE);
		if (params.keyBldr == NULL)
			return 1;
		printf("bldr key: ");
		uprinth(params.keyBldr, XB_KEY_SIZE);
	}

	if (params.krnlKeyFile != NULL) {
		printf("krnl key file: %s\n", params.krnlKeyFile);
		params.keyKrnl = readFile(params.krnlKeyFile, NULL, XB_KEY_SIZE);
		if (params.keyKrnl == NULL)
			return 1;
		printf("krnl key: ");
		uprinth(params.keyKrnl, XB_KEY_SIZE);
	}

	if (params.keyBldr != NULL || params.keyKrnl != NULL) 
		printf("\n");

	return 0;
}
int read_mcpx() {
	// read and verify mcpx rom file.

	uint8_t* mcpxData = NULL;
	int result = 0;

	if (params.mcpxFile == NULL)
		return 0;

	mcpxData = readFile(params.mcpxFile, NULL, MCPX_BLOCK_SIZE);
	if (mcpxData == NULL)
		return 1;

	result = mcpx_load(&params.mcpx, mcpxData);

	switch (params.mcpx.version) {
		case MCPX_VERSION_MCPX_V1_0:
		case MCPX_VERSION_MCPX_V1_1:
		case MCPX_VERSION_MOUSE_V1_0:
		case MCPX_VERSION_MOUSE_V1_1:
			break;

		default:
			printf("\n\nError: hash did not match known mcpx roms\n" \
				"See github page for md5 hashes.\n" \
				"Use MCPX dump or\n" \
				"Use M.O.U.S.E v0.8.0 or\n" \
				"Dump the 16 - byte secret boot key into a file and use that with -bldr-key <path>\n");
			return 1;
	}

	return result;
}

uint8_t* load_init_tbl_file(uint32_t* size, uint32_t* base) {
	uint8_t* inittbl = NULL;
	int result = 0;

	inittbl = readFile(params.inFile, size, 0);
	if (inittbl == NULL)
		return NULL;

	if (isFlagSet(SW_BASE)) {
		*base = params.base;
	}
	else {
		if (*size < sizeof(INIT_TBL)) {
			*base = 0;
		}
		else {
			*base = sizeof(INIT_TBL);
		}
	}

	if (*base >= *size) {
		printf("Error: base: %d is larger than file size: %d bytes\n", *base, *size);
		result = 1;
		goto Cleanup;
	}

	if (isFlagClear(SW_NO_MAX_SIZE) && *size > MAX_BIOS_SIZE) {
		printf("Error: invalid file size: %d. Expected less than 0x%x bytes\n", *size, MAX_BIOS_SIZE);
		result = 1;
		goto Cleanup;
	}

Cleanup:

	if (result != 0) {
		if (inittbl != NULL) {
			free(inittbl);
			inittbl = NULL;
		}
		return NULL;
	}

	return inittbl;
}

/* BIOS print functions */

void printBldrInfo(Bios* bios) {
	BiosParams biosParams = bios->params;
	
	printf("2BL:\n");

	if (bios->bldr.bootParamsValid) {
		printf("entry point:\t\t0x%08x\n", bios->bldr.ldrParams->bldrEntryPoint);
		if (bios->bldr.entry != NULL && bios->bldr.entry->bfmEntryPoint != 0) {
			printf("bfm entry point:\t0x%08x\n", bios->bldr.entry->bfmEntryPoint);
		}
	}

	printf("signature:\t\t");
	uprinth((uint8_t*)&bios->bldr.bootParams->signature, 4);

	uint32_t krnlSize = bios->bldr.bootParams->krnlSize;
	uint32_t krnlDataSize = bios->bldr.bootParams->krnlDataSize;
	uint32_t initTblSize = bios->bldr.bootParams->inittblSize;
	uint32_t romsize = biosParams.romsize;

	printf("inittbl size:\t\t");
	uprintc((initTblSize >= 0 && initTblSize <= romsize), "%u", initTblSize);
	printf(" bytes");
	if (initTblSize == 0)
		printf(" ( not hashed )");
	printf("\n");

	printf("kernel size:\t\t");
	uprintc((krnlSize >= 0 && krnlSize <= romsize), "%u", krnlSize);
	printf(" bytes\n");

	printf("kernel data size:\t");
	uprintc((krnlDataSize >= 0 && krnlDataSize <= romsize), "%u", krnlDataSize);
	printf(" bytes\n");
	printf("\n");
}
void printPreldrInfo(Bios* bios) {
	BiosParams biosParams = bios->params;

	if (bios->preldr.status > PRELDR_STATUS_FOUND)
		return;

	int jmp_offset = (int)bios->preldr.params->jmpOffset + 5;
	int jmp_address = PRELDR_REAL_BASE + jmp_offset;

	printf("FBL:\n");
	printf("entry point:\t\t0x%08x (0x%08x)\n", jmp_offset, jmp_address);

	if (jmp_address < PRELDR_REAL_BASE || jmp_address > PRELDR_REAL_BASE + PRELDR_BLOCK_SIZE) {
		// escaped mcpx!
	}

	printf("\n");
}
void printInitTblInfo(Bios* bios) {
	INIT_TBL* initTbl = bios->initTbl;
	uint16_t kernel_ver = initTbl->kernel_ver;

	printf("Init tbl:\n");

	if ((kernel_ver & 0x8000) != 0) {
		kernel_ver = kernel_ver & 0x7FFF; // clear the 0x8000 bit
		printf("kernel delay flag set\n");
	}

	if (kernel_ver != 0) {
		printf("kernel version:\t\t%d\n", kernel_ver);
	}

	printf("identifier:\t\t%02x ", (uint8_t)initTbl->init_tbl_identifier);
	switch (initTbl->init_tbl_identifier) {
		case 0x30: // xbox devkit beta
			printf("dvt3 (devkit beta)");
			break;
		case 0x46: //xbox devkit
			printf("dvt4 (devkit)");
			break;
		case 0x60: // xbox v1.0. (mcpx v1.0) (k:3944 - k:4627)
			printf("dvt6 (xbox v1.0)");
			break;
		case 0x70: // xbox v1.1. (mcpx v1.1) (k:4817)
			printf("qt (xbox v1.1)");
			break;
		case 0x80: // xbox v1.2, v1.3, v1.4. (mcpx v1.1) (k:5101 - k:5713)
			printf("xblade (xbox v1.2 - v1.4)");
			break;
		case 0x90: // xbox v1.6a, v1.6b. (mcpx v1.1) (k:5838)
			printf("tuscany (xbox v1.6)");
			break;
	}

	printf("\nrevision:\t\trev %d.%02d\n", initTbl->revision >> 8, initTbl->revision & 0xFF);
	printf("\n");
}
void printNv2aInfo(Bios* bios) {
	INIT_TBL* initTbl = bios->initTbl;

	int i;
	uint32_t item;

	const uint32_t UINT_ALIGN = sizeof(uint32_t);
	const uint32_t ARRAY_BYTES = sizeof(initTbl->vals);
	const uint32_t ARRAY_SIZE = ARRAY_BYTES / UINT_ALIGN;
	const uint32_t ARRAY_START = (ARRAY_BYTES - UINT_ALIGN) / UINT_ALIGN;

	const char* init_tbl_comments[5] = {
		"; init tbl revision","","",
		"; kernel version + init tbl identifier",
		"; rom data tbl offset",
	};

	printf("NV2A Init Tbl:\n");

	// print the first part of the table
	for (i = 0; i < ARRAY_START; ++i) {
		item = ((uint32_t*)initTbl)[i];
		printf("%04x:\t\t0x%08x\n", i * UINT_ALIGN, item);
	}

	// print the array
	char str[14 * 4 + 1] = { 0 };
	bool hasValue = false;
	bool cHasValue = false;
	const uint32_t* valArray = initTbl->vals;

	for (i = 0; i < ARRAY_SIZE; ++i) {
		cHasValue = false;
		if (*(uint32_t*)&valArray[i] != 0) {
			cHasValue = true;
			hasValue = true;
		}

		if (cHasValue && hasValue)
			sprintf(str + i * 3, "%02X ", *(uint32_t*)valArray[i]);
	}
	if (hasValue) {
		if (str[0] == '\0') {
			i = 0;
			while (str[i] == '\0' && i < ARRAY_SIZE * 3) { // sanity check
				memcpy(str + i, "00 ", 3);
				i += 3;
			}
		}
	}
	else {
		strcat(str, "0x00..0x00");
	}

	printf("%04x-%04x:\t%s\n", ARRAY_START * UINT_ALIGN, (ARRAY_START * UINT_ALIGN) + (ARRAY_SIZE * UINT_ALIGN) - UINT_ALIGN, str);

	// print the second part of the table
	for (i = ARRAY_START + ARRAY_SIZE; i < sizeof(INIT_TBL) / UINT_ALIGN; ++i) {
		item = ((uint32_t*)initTbl)[i];
		printf("%04x:\t\t0x%08x %s\n", i * UINT_ALIGN, item, init_tbl_comments[i - ARRAY_START - ARRAY_SIZE]);
	}
	printf("\n");
}
void printDataTblInfo(Bios* bios) {
	if (bios->initTbl->data_tbl_offset == 0  || bios->size < bios->initTbl->data_tbl_offset + sizeof(ROM_DATA_TBL)) {
		printf("rom data table not found.\n");
		return;
	}

	ROM_DATA_TBL* dataTbl = (ROM_DATA_TBL*)(bios->data + bios->initTbl->data_tbl_offset);

	static const char* padTblName[5] = {
		"Fastest",
		"Fast",
		"Standard",
		"Slow",
		"Slowest"
	};

	printf("Drv/Slw data tbl:\n" \
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
	for (int i = 0; i < 5; i++) {
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
	printf("\n");
}
void printKeyInfo(Bios* bios) {
	Mcpx* mcpx = bios->params.mcpx;
	PUBLIC_KEY* pubkey;
	
	if (mcpx->sbkey != NULL) {
		printf("sb key (+%d):\t", mcpx->sbkey - mcpx->data);
		uprinth(mcpx->sbkey, XB_KEY_SIZE);
	}

	if (bios->bldr.bfmKey != NULL) {
		printf("bfm key:\t");
		uprinth(bios->bldr.bfmKey, XB_KEY_SIZE);
	}

	if (bios->bldr.keys != NULL) {
		printf("eeprom key:\t");
		uprinth(bios->bldr.keys->eepromKey, XB_KEY_SIZE);
		printf("cert key:\t");
		uprinth(bios->bldr.keys->certKey, XB_KEY_SIZE);
		printf("kernel key:\t");
		uprinth(bios->bldr.keys->krnlKey, XB_KEY_SIZE);
	}

	if (bios->preldr.status == PRELDR_STATUS_BLDR_DECRYPTED) {
		printf("\npreldr key:\t");
		uprinth(bios->preldr.bldrKey, SHA1_DIGEST_LEN);
	}

	if (bios->decompressedKrnl != NULL) {
		if (rsa_findPublicKey(bios->decompressedKrnl, bios->decompressedKrnlSize, &pubkey, NULL) == RSA_ERROR_SUCCESS) {
			printf("\npublic Key:\b\b\b\b");
			uprinthl((uint8_t*)&pubkey->modulus, RSA_MOD_SIZE(&pubkey->header), 16, "\t\t", 0);
		}
	}
}

int validateArgs() {
	// rom size in kb
	if (isFlagClear(SW_ROMSIZE)) {
		params.romsize = MIN_BIOS_SIZE;
	}
	else {
		params.romsize *= 1024;
	}

	if (bios_check_size(params.romsize) != 0) {
		printf("Error: invalid rom size: %d\n", params.romsize);
		return 1;
	}

	// bin size in kb
	if (isFlagClear(SW_BINSIZE)) {
		params.binsize = MIN_BIOS_SIZE;
	}
	else {
		params.binsize *= 1024;
	}
	if (bios_check_size(params.binsize) != 0) {
		printf("Error: invalid bin size: %d\n", params.binsize);
		return 1;
	}

	// xcode sim size in bytes
	if (isFlagClear(SW_SIM_SIZE)) {
		params.simSize = 32;
	}

	if (params.simSize < 32 || params.simSize > (128 * 1024 * 1024)) {
		printf("Error: invalid sim size: %d\n", params.simSize);
		return 1;
	}

	if (params.simSize % 4 != 0) {
		printf("Error: sim size must be devisible by 4.\n");
		return 1;
	}

	return 0;
}

int main(int argc, char** argv) {
	int result = 0;

	printf(XB_BIOS_TOOL_NAME_STR);
#ifdef __SANITIZE_ADDRESS__
	printf(" /sanitize");
#endif
#ifdef MEM_TRACKING
	printf(" /memtrack");
#endif
	printf("\n\n");

	cmd = NULL;
	init_parameters(&params);

	result = parseCli(argc, argv, cmd, cmd_tbl, sizeof(cmd_tbl), param_tbl, sizeof(param_tbl));
	if (result != 0) {
		switch (result) {
			case CLI_ERROR_NO_CMD:
			case CLI_ERROR_INVALID_CMD:
			case CLI_ERROR_UNKNOWN_CMD:
				printf("%s\nUse xbios -? for more info.\n", HELP_USAGE_STR);
				break;
		}

		if (isFlagClear(SW_HELP)) {
			result = ERROR_FAILED;
			goto Exit;
		}
	}

	result = validateArgs();
	if (result != 0) {
		goto Exit;
	}

	if (isFlagSet(SW_HELP)) {
		result = help();
		goto Exit;
	}

	if (read_keys() != 0)
		goto Exit;

	if (read_mcpx() != 0)
		goto Exit;
	
	switch (cmd->type) {
		case CMD_INFO:
			result = info();
			break;

		case CMD_HELP:
			result = help();
			break;

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

		case CMD_REPLICATE_BIOS:
			result = replicateBios();
			break;

		case CMD_SIMULATE_XCODE:
			result = simulateXcodes();
			break;

		case CMD_DECODE_XCODE:
			result = decodeXcodes();
			break;

		case CMD_ENCODE_X86:
			result = encodeX86();
			break;

		case CMD_COMPRESS_FILE:
			result = compressFile();
			break;

		case CMD_DECOMPRESS_FILE:
			result = decompressFile();
			break;

		case CMD_DUMP_PE_IMG:
			result = dumpCoffPeImg();
			break;
		
		default:
			result = ERROR_FAILED;
			break;
	}

Exit:

	free_parameters(&params);

#ifdef MEM_TRACKING
	memtrack_report();
#endif

	return result;
}
