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
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#else
#include <unistd.h>
#include "posix_shims.h"
#endif
#if !__APPLE__
#include <malloc.h>
#else
#include <cstdlib>
#endif

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
	{ "in", &params.in_file, SW_IN_FILE, PARAM_TBL::STR },
	{ "out", &params.out_file, SW_OUT_FILE, PARAM_TBL::STR },
	{ "romsize", &params.romsize, SW_ROMSIZE, PARAM_TBL::INT },
	{ "binsize", &params.binsize, SW_BINSIZE, PARAM_TBL::INT },

	{ "enc-bldr", nullptr, SW_ENC_BLDR, PARAM_TBL::FLAG },
	{ "enc-krnl", nullptr, SW_ENC_KRNL, PARAM_TBL::FLAG },
	{ "key-krnl", &params.kernel_key_file, SW_KEY_KRNL_FILE, PARAM_TBL::STR },
	{ "key-bldr", &params.bldr_key_file, SW_KEY_BLDR_FILE, PARAM_TBL::STR},
	{ "mcpx", &params.mcpx_file, SW_MCPX_FILE, PARAM_TBL::STR },

	{ "bldr", &params.bldr_file, SW_BLDR_FILE, PARAM_TBL::STR },
	{ "preldr", &params.preldr_file, SW_PRELDR_FILE, PARAM_TBL::STR },
	{ "krnl", &params.kernel_file, SW_KRNL_FILE, PARAM_TBL::STR },
	{ "krnldata", &params.kernel_data_file, SW_KRNL_DATA_FILE, PARAM_TBL::STR },
	{ "inittbl", &params.init_tbl_file, SW_INITTBL_FILE, PARAM_TBL::STR },

	{ "nv2a", nullptr, SW_LS_NV2A_TBL, PARAM_TBL::FLAG },
	{ "datatbl", nullptr, SW_LS_DATA_TBL, PARAM_TBL::FLAG },
	{ "img", nullptr, SW_DUMP_KRNL, PARAM_TBL::FLAG },

	{ "pubkey", &params.public_key_file, SW_PUB_KEY_FILE, PARAM_TBL::STR },
	{ "certkey", &params.cert_key_file, SW_CERT_KEY_FILE, PARAM_TBL::STR },
	{ "eepromkey", &params.eeprom_key_file, SW_EEPROM_KEY_FILE, PARAM_TBL::STR },

	{ "bfm", nullptr, SW_BLD_BFM, PARAM_TBL::FLAG },
	{ "d", nullptr, SW_DMP, PARAM_TBL::FLAG },
	{ "simsize", &params.simSize, SW_SIM_SIZE, PARAM_TBL::INT },
	{ "nomaxsize", nullptr, SW_NO_MAX_SIZE, PARAM_TBL::FLAG },

	{ "bank1", &params.bank_files[0], SW_BANK1_FILE, PARAM_TBL::STR },
	{ "bank2", &params.bank_files[1], SW_BANK2_FILE, PARAM_TBL::STR },
	{ "bank3", &params.bank_files[2], SW_BANK3_FILE, PARAM_TBL::STR },
	{ "bank4", &params.bank_files[3], SW_BANK4_FILE, PARAM_TBL::STR },

	{ "ini", &params.settings_file, SW_INI_FILE, PARAM_TBL::STR },
	{ "?", nullptr, SW_HELP, PARAM_TBL::FLAG },
	{ "keys", nullptr, SW_KEYS, PARAM_TBL::FLAG },
	{ "base", &params.base, SW_BASE, PARAM_TBL::INT },
	{ "hackinittbl", nullptr, SW_HACK_INITTBL, PARAM_TBL::FLAG },
	{ "hacksignature", nullptr, SW_HACK_SIGNATURE, PARAM_TBL::FLAG },
	{ "nobootparams", nullptr, SW_UPDATE_BOOT_PARAMS, PARAM_TBL::FLAG },
	{ "help-all", nullptr, SW_HELP_ALL, PARAM_TBL::FLAG },
	{ "help-enc", nullptr, SW_HELP_ENCRYPTION, PARAM_TBL::FLAG },
	{ "branch", nullptr, SW_BRANCH, PARAM_TBL::FLAG },
	{ "dir", &params.working_directory_path, SW_WORKING_DIRECTORY, PARAM_TBL::STR },
	{ "xcodes", &params.xcodes_file, SW_XCODES, PARAM_TBL::STR },
	{ "offset", &params.offset, SW_OFFSET, PARAM_TBL::INT },
};

uint8_t* load_init_tbl_file(uint32_t* size, uint32_t* base);
void lzx_print_error(int error);

// Command Functions

int buildBios() {
	int result = 0;
	const char* filename = params.out_file;

	Bios bios;
	BIOS_LOAD_PARAMS bios_params;
	BIOS_BUILD_PARAMS build_params;

	printf("Build BIOS\n\n");

	bios_init_params(&bios_params);
	bios_params.mcpx = &params.mcpx;
	bios_params.bldr_key = params.bldr_key;
	bios_params.kernel_key = params.kernel_key;
	bios_params.romsize = params.romsize;
	bios_params.enc_bldr = isFlagSet(SW_ENC_BLDR);
	bios_params.enc_kernel = isFlagSet(SW_ENC_KRNL);

	bios_init_build_params(&build_params);
	build_params.bfm = isFlagSet(SW_BLD_BFM);
	build_params.hackinittbl = isFlagSet(SW_HACK_INITTBL);
	build_params.hacksignature = isFlagSet(SW_HACK_SIGNATURE);
	build_params.nobootparams = isFlagSet(SW_UPDATE_BOOT_PARAMS);

	if (params.mcpx_file != nullptr)
		printf("mcpx file:\t\t%s\n", params.mcpx_file);

	// init tbl file
	printf("Init tbl file:\t\t%s\n", params.init_tbl_file);
	build_params.init_tbl = readFile(params.init_tbl_file, &build_params.init_tbl_size, 0);
	if (build_params.init_tbl == nullptr) {
		result = 1;
		goto Cleanup;
	}

	// preldr file
	printf("Preldr file:\t\t%s\n", params.preldr_file);
	if (params.preldr_file != nullptr) {
		build_params.preldr = readFile(params.preldr_file, &build_params.preldr_size, 0);
		if (build_params.preldr == nullptr) {
			build_params.preldr_size = 0;
		}
	}

	// 2bl file
	printf("2BL file:\t\t%s\n", params.bldr_file);
	build_params.bldr = readFile(params.bldr_file, &build_params.bldrSize, 0);
	if (build_params.bldr == nullptr) {
		result = 1;
		goto Cleanup;
	}

	// compressed krnl image
	printf("Kernel file:\t\t%s\n", params.kernel_file);
	build_params.compressed_kernel = readFile(params.kernel_file, &build_params.kernel_size, 0);
	if (build_params.compressed_kernel == nullptr) {
		result = 1;
		goto Cleanup;
	}

	// uncompressed kernel data
	printf("Kernel data file:\t%s\n", params.kernel_data_file);
	build_params.kernel_data = readFile(params.kernel_data_file, &build_params.kernel_data_size, 0);
	if (build_params.kernel_data == nullptr) {
		result = 1;
		goto Cleanup;
	}

	// eeprom key
	if (params.eeprom_key_file != nullptr) {
		printf("Eeprom key file:\t\t%s\n", params.eeprom_key_file);
		build_params.eeprom_key = readFile(params.eeprom_key_file, nullptr, XB_KEY_SIZE);
	}

	// cert key
	if (params.cert_key_file != nullptr) {
		printf("Cert key file:\t\t%s\n", params.cert_key_file);
		build_params.cert_key = readFile(params.cert_key_file, nullptr, XB_KEY_SIZE);
	}

	printf("rom size:\t\t%u kb\n\n", params.romsize / 1024);

	result = bios.build(&build_params, params.binsize, &bios_params);

	// xcodes
	if (isFlagSet(SW_XCODES)) {
		uint32_t xcodesSize;
		uint8_t* xcodes = readFile(params.xcodes_file, &xcodesSize, 0);
		if (xcodes == nullptr) {
			result = 1;
		}
		else {
			result = inject_xcodes(bios.data, bios.size, xcodes, xcodesSize);
			free(xcodes);
			xcodes = nullptr;
		}
	}

	if (result != 0) {
		printf("Error: Failed to build bios\n");
	}
	else {
		filename = params.out_file;
		if (filename == nullptr)
			filename = "bios.bin";
		result = writeFileF(filename, "bios", bios.data, bios.size);
	}

Cleanup:

	bios_free_build_params(&build_params);

	return result;
}
int extractBios() {
	// Extract components from the bios file.

	int result = 0;
	const char* filename;
	size_t init_tbl_size = 0;
	Bios bios;
	BIOS_LOAD_PARAMS bios_params;


	bios_init_params(&bios_params);
	bios_params.mcpx = &params.mcpx;
	bios_params.bldr_key = params.bldr_key;
	bios_params.kernel_key = params.kernel_key;
	bios_params.romsize = params.romsize;
	bios_params.enc_bldr = isFlagSet(SW_ENC_BLDR);
	bios_params.enc_kernel = isFlagSet(SW_ENC_KRNL);
	bios_params.restore_boot_params = isFlagClear(SW_UPDATE_BOOT_PARAMS);

	printf("Extract BIOS\n\n");

	uint32_t size = 0;
	uint8_t* buffer = readFile(params.in_file, &size, 0);
	if (buffer == nullptr) {
		return 1;
	}

	if (bios_check_size(size) != 0) {
		printf("Error: BIOS size is invalid\n");
		return 1;
	}

	if (params.mcpx_file != nullptr)
		printf("mcpx file: %s\n", params.mcpx_file);
	printf("bios file: %s\nbios size: %d kb\nrom size:  %d kb\n\n", params.in_file, size / 1024, params.romsize / 1024);

	result = bios.load(buffer, size, &bios_params);
	if (result != BIOS_LOAD_STATUS_SUCCESS) {
		printf("Error: invalid 2BL\n");
		return 1;
	}

	// set working directory
	if (isFlagSet(SW_WORKING_DIRECTORY)) {
		if (_chdir(params.working_directory_path) == -1) {
			if (errno == ENOENT) { // directory not found
				printf("Error: '%s' directory not found.\n", params.working_directory_path);
				return 1;
			}
		}
	}

	// zero rom digest so we have a clean 2bl;
	if (bios.rom_digest != nullptr) {
		memset(bios.rom_digest, 0, ROM_DIGEST_SIZE);
	}

	// preldr
	if (bios.preldr.status < PRELDR_STATUS_NOT_FOUND) {
		filename = params.preldr_file;
		if (filename == nullptr)
			filename = "preldr.bin";
		writeFileF(filename, "preldr", bios.preldr.data, PRELDR_SIZE);

		// zero preldr
		memset(bios.preldr.data, 0, PRELDR_SIZE);
		// zero preldr params.
		memset(bios.preldr.data + PRELDR_SIZE + ROM_DIGEST_SIZE, 0, PRELDR_PARAMS_SIZE - sizeof(BOOT_PARAMS));
	}

	// 2bl
	filename = params.bldr_file;
	if (filename == nullptr)
		filename = "bldr.bin";
	writeFileF(filename, "2BL", bios.bldr.data, BLDR_BLOCK_SIZE);

	// extract init tbl
	init_tbl_size = bios.bldr.boot_params->init_tbl_size;
	if (init_tbl_size > 0) {
		filename = params.init_tbl_file;
		if (filename == nullptr)
			filename = "inittbl.bin";
		writeFileF(filename, "init table", bios.data, init_tbl_size);
	}

	// extract compressed kernel
	filename = params.kernel_file;
	if (filename == nullptr)
		filename = "krnl.bin";
	writeFileF(filename, "compressed kernel", bios.kernel.compressed_kernel_ptr, bios.bldr.boot_params->compressed_kernel_size);

	// extract uncompressed kernel section data
	filename = params.kernel_data_file;
	if (filename == nullptr)
		filename = "krnl_data.bin";
	writeFileF(filename, "kernel data", bios.kernel.uncompressed_data_ptr, bios.bldr.boot_params->uncompressed_kernel_data_size);

	// decompress the kernel now so the public key can be extracted.
	if (bios.decompressKrnl() == 0) {
		// extract decompressed kernel image ( pe/coff executable )
		if (bios.kernel.img != nullptr) {
			writeFileF("krnl.img", "decompressed kernel", bios.kernel.img, bios.kernel.img_size);
		}
	}

	if (isFlagSet(SW_KEYS)) {
		// 2BL rc4 keys

		if (bios.bldr.keys != nullptr) {

			// eeprom rc4 key
			filename = params.eeprom_key_file;
			if (filename == nullptr)
				filename = "eeprom_key.bin";
			writeFileF(filename, "eeprom key", bios.bldr.keys->eeprom_key, XB_KEY_SIZE);

			// cert rc4 key
			filename = params.cert_key_file;
			if (filename == nullptr)
				filename = "cert_key.bin";
			writeFileF(filename, "cert key", bios.bldr.keys->cert_key, XB_KEY_SIZE);

			// kernel rc4 key
			writeFileF("krnl_key.bin", "kernel key", bios.bldr.keys->kernel_key, XB_KEY_SIZE);
		}

		// bfm key
		if (bios.bldr.bfm_key != nullptr) {
			writeFileF("bfm_key.bin", "bfm key", bios.bldr.bfm_key, XB_KEY_SIZE);
		}

		// secret boot key
		if (params.mcpx.sbkey != nullptr) {
			writeFileF("sb_key.bin", "secret boot key", params.mcpx.sbkey, XB_KEY_SIZE);
		}

		// extract decompressed kernel rsa pub key
		if (bios.kernel.img != nullptr) {
			PUBLIC_KEY* pubkey;
			filename = params.public_key_file;
			if (filename == nullptr)
				filename = "pubkey.bin";
			if (rsa_findPublicKey(bios.kernel.img, bios.kernel.img_size, &pubkey, nullptr) == RSA_ERROR_SUCCESS)
				writeFileF(filename, "public key", pubkey, RSA_PUBKEY_SIZE(&pubkey->header));
		}

		// preldr; extract preldr components.
		if (bios.preldr.status < PRELDR_STATUS_NOT_FOUND) {
			writeFileF("preldr_key.bin", "preldr key", bios.preldr.bldr_key, SHA1_DIGEST_LEN);
		}
	}

	return 0;
}
int splitBios() {
	int result = 0;
	uint8_t* data = nullptr;
	uint32_t size = 0;
	uint32_t fnLen = 0;
	uint32_t bankFnLen = 0;
	uint32_t dataLeft = 0;
	uint32_t loopCount = 0;
	int bank = 0;
	const char suffix[] = "_bank";
	char* biosFn = nullptr;
	char* ext = nullptr;
	char* bankFn = nullptr;
	int i;
	int j;

	printf("Split BIOS\n\n");

	//romsize sanity check
	if (params.romsize < MIN_BIOS_SIZE)
		return 1;

	data = readFile(params.in_file, &size, 0);
	if (data == nullptr)
		return 1;

	result = bios_check_size(size);
	if (result != 0) {
		printf("Error: Invalid bios file size: %d\n", size);
		goto Cleanup;
	}

	printf("bios file: %s\nbios size: %dkb\nrom size:  %dkb\n\n", params.in_file, size / 1024, params.romsize / 1024);

	// check if bios size is less than or equal to rom size
	if (size <= params.romsize) {
		printf("Nothing to split.\n");
		result = 0;
		goto Cleanup;
	}

	fnLen = strlen(params.in_file);
	biosFn = (char*)malloc(fnLen + 1);
	if (biosFn == nullptr) {
		result = 1;
		goto Cleanup;
	}
	for (j = fnLen - 1; j > 0; j--) {
		if (params.in_file[j] == '\\') {
			j += 1;
			break;
		}
	}
	strcpy(biosFn, params.in_file+j);
	fnLen -= j;

	// remove the extention
	for (i = fnLen - 1; i >= 0; i--) {
		if (biosFn[i] == '.') {
			ext = (char*)malloc(fnLen - i + 1); // +1 for null terminator
			if (ext == nullptr) {
				result = 1;
				goto Cleanup;
			}
			strcpy(ext, biosFn + i);

			biosFn[i] = '\0';
			break;
		}
	}

	if (ext == nullptr) {
		printf("Error: Invalid bios file name. no file extension.\n");
		result = 1;
		goto Cleanup;
	}

	bankFnLen = strlen(biosFn) + strlen(suffix) + strlen(ext) + 3U; // +3 for the bank number
	bankFn = (char*)malloc(bankFnLen);
	if (bankFn == nullptr) {
		result = 1;
		goto Cleanup;
	}

	// split the bios based on rom size

	dataLeft = size;
	loopCount = 0;
	while (dataLeft > 0) {
		// safe guard
		if (loopCount > 4) {
			printf("Error: LoopCount exceeded 5\n");
			result = 1;
			break;
		}
		loopCount++;

		// set bank filename = [name][suffix][number].[ext] = bios_bank1.bin
		bankFn[0] = '\0';
		snprintf(bankFn, bankFnLen, "%s%s%d%s", biosFn, suffix, bank + 1, ext);

		// write bank to file
		printf("Writing bank %d to %s\n", bank + 1, bankFn);
		result = writeFile(bankFn, (data + (params.romsize * bank)), params.romsize);
		if (result != 0) {
			goto Cleanup;
		}

		dataLeft -= params.romsize;
		bank++;
	}

	printf("BIOS split into %d banks\n", bank);

Cleanup:
	if (data != nullptr) {
		free(data);
	}
	if (biosFn != nullptr) {
		free(biosFn);
	}
	if (bankFn != nullptr) {
		free(bankFn);
	}
	if (ext != nullptr) {
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

	uint8_t* data = nullptr;

	uint8_t* banks[MAX_BANKS] = { nullptr };
	uint32_t bankSizes[MAX_BANKS] = { 0 };

	printf("Combine BIOS\n\n");

	const char* filename = params.out_file;
	if (filename == nullptr) {
		filename = "bios.bin";
	}

	for (i = 0; i < MAX_BANKS; i++) {

		if (params.bank_files[i] == nullptr)
			continue;

		numBanks++;

		banks[i] = readFile(params.bank_files[i], &bankSizes[i], 0);
		if (banks[i] == nullptr) {
			result = 1;
			goto Cleanup;
		}

		if (bios_check_size(bankSizes[i]) != 0) {
			printf("Error: %s has invalid file size: %d\n", params.bank_files[i], bankSizes[i]);
			result = 1;
			goto Cleanup;
		}

		totalSize += bankSizes[i];
	}

	if (numBanks < 2) {
		printf("Error: Not enough banks to combine. Expected atleast 2 banks\n");
		result = 1;
		goto Cleanup;
	}

	if (bios_check_size(totalSize) != 0) {
		printf("Error: Invalid total bios size: %d\n", totalSize);
		result = 1;
		goto Cleanup;
	}

	data = (uint8_t*)malloc(totalSize);
	if (data == nullptr) {
		result = 1;
		goto Cleanup;
	}

	for (i = 0; i < MAX_BANKS; i++) {
		if (banks[i] != nullptr) {
			printf("Copying %s %d kb into offset 0x%x (bank %d)\n", params.bank_files[i], bankSizes[i] / 1024, offset, i + 1);
			memcpy(data + offset, banks[i], bankSizes[i]);
			offset += bankSizes[i];
		}
	}

	result = writeFileF(filename, "bios", data, totalSize);

Cleanup:

	if (data != nullptr) {
		free(data);
	}

	for (i = 0; i < MAX_BANKS; i++) {
		if (banks[i] != nullptr) {
			free(banks[i]);
			banks[i] = nullptr;
		}
	}

	return result;
}
int listBios() {
	int result = 0;
	int biosStatus = 0;
	Bios bios;
	BIOS_LOAD_PARAMS bios_params;

	bios_init_params(&bios_params);
	bios_params.mcpx = &params.mcpx;
	bios_params.bldr_key = params.bldr_key;
	bios_params.kernel_key = params.kernel_key;
	bios_params.romsize = params.romsize;
	bios_params.enc_bldr = isFlagSet(SW_ENC_BLDR);
	bios_params.enc_kernel = isFlagSet(SW_ENC_KRNL);
	bios_params.restore_boot_params = isFlagClear(SW_UPDATE_BOOT_PARAMS);

	printf("List BIOS\n\n");

	uint32_t size = 0;
	uint8_t* buffer = readFile(params.in_file, &size, 0);
	if (buffer == nullptr) {
		return 1;
	}

	if (bios_check_size(size) != 0) {
		printf("Error: BIOS size is invalid\n");
		return 1;
	}

	if (params.mcpx_file != nullptr) printf("mcpx file: %s\n", params.mcpx_file);
	printf("bios file: %s\nbios size: %d kb\nrom size:  %d kb\n\n", params.in_file, size / 1024, params.romsize / 1024);

	biosStatus = bios.load(buffer, size, &bios_params);
	if (biosStatus > BIOS_LOAD_STATUS_INVALID_BLDR) {
		printf("Error: Failed to load BIOS\n");
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
		if (bios.kernel.img != nullptr) {
			printf("Image size: %d bytes\n", bios.kernel.img_size);
			dump_nt_headers(bios.kernel.img, bios.kernel.img_size, false);
			print_krnl_data_section_header((IMAGE_DOS_HEADER*)bios.kernel.img);
		}
		else {
			printf("Error: Failed to decompress kernel image\n");
		}
	}
	else {
		// default ls command

		printPreldrInfo(&bios);
		printBldrInfo(&bios);
		printInitTblInfo(&bios);

		printf("Available space:\t");
		int valid = (bios.available_space >= 0 && bios.available_space <= (int)bios.params.romsize);
		uprintc(valid, "%d", bios.available_space);
		printf(" bytes\n");
	}

	return result;
}
int replicateBios() {
	uint32_t size;
	uint8_t* bios = nullptr;
	uint8_t* bank = nullptr;
	int result = 0;
	uint32_t binsize;

	const char* filename = params.out_file;
	if (filename == nullptr) {
		filename = "bios.bin";
	}

	printf("Replicate BIOS\n\n");

	bank = readFile(params.in_file, &size, 0);
	if (bank == nullptr) {
		result = 1;
		goto Cleanup;
	}

	if (bios_check_size(size) != 0) {
		printf("Error: Invalid bank size: %d\n", size);
		result = 1;
		goto Cleanup;
	}

	// did user type romsize instead? (romsize param isnt used in command so free to use either).
	if (isFlagClear(SW_BINSIZE)) {
		binsize = params.romsize;
	}
	else {
		binsize = params.binsize;
	}

	printf("bios file: %s\nbios size: %dkb\nbin size:  %dkb\n\n", params.in_file, size / 1024, binsize / 1024);

	if (size >= binsize) {
		printf("Nothing to replicate.\n");
		result = 0;
		goto Cleanup;
	}

	bios = (uint8_t*)malloc(binsize);
	if (bios == nullptr) {
		result = 1;
		goto Cleanup;
	}

	memcpy(bios, bank, size);
	if (bios_replicate_data(size, binsize, bios, binsize) != 0) {
		printf("Error: Failed to replicate BIOS\n");
		result = 1;
		goto Cleanup;
	}

	result = writeFileF(filename, "bios", bios, binsize);

Cleanup:

	if (bank != nullptr) {
		free(bank);
		bank = nullptr;
	}

	if (bios != nullptr) {
		free(bios);
		bios = nullptr;
	}

	return result;
}
int decodeXcodes() {
	XcodeDecoder decoder;
	DECODE_CONTEXT* context;
	uint8_t* init_tbl = nullptr;
	uint32_t size = 0;
	uint32_t base = 0;
	int result = 0;

	printf("Decode Xcodes\n\n");

	init_tbl = load_init_tbl_file(&size, &base);
	if (init_tbl == nullptr) {
		return 1;
	}

	if (params.settings_file != nullptr) {
		if (!fileExists(params.settings_file)) {
			printf("Error: Settings file not found.\n");
			result = 1;
			goto Cleanup;
		}
	}

	/* init decoder */
	result = decoder.load(init_tbl, size, base, params.settings_file);
	if (result != 0) {
		printf("Error: Failed to init xcode decoder\n");
		result = 1;
		goto Cleanup;
	}
	context = decoder.context;
	context->branch = isFlagSet(SW_BRANCH);

	printf("init tbl file: %s\nxcode count: %d\nxcode size: %d bytes\nxcode base: 0x%x\n\n",
		params.in_file, context->xcodeCount, context->xcodeSize, context->xcodeBase);

	// setup the file stream, if -d flag is set
	if (isFlagSet(SW_DMP)) {
		const char* filename = params.out_file;
		if (filename == nullptr) {
			filename = "xcodes.txt";
		}

		// del file if it exists
		deleteFile(filename);

		printf("Writing xcodes to %s\n", filename);
		FILE* stream = fopen(filename, "w");
		if (stream == nullptr) {
			printf("Error: Failed to open file %s\n", filename);
			result = 1;
			goto Cleanup;
		}
		context->stream = stream;
	}
	else {
		context->stream = stdout;
	}

	result = decoder.decodeXcodes();

	if (isFlagSet(SW_DMP)) {
		fclose(context->stream);
		printf("Done\n");
	}

Cleanup:

	if (init_tbl != nullptr) {
		free(init_tbl);
		init_tbl = nullptr;
	}

	return result;
}
int simulateXcodes() {
	XcodeInterp interp;
	XCODE* xcode = nullptr;
	uint32_t size = 0;
	uint32_t base = 0;
	int result = 0;
	bool hasMemChanges_total = false;
	const char* opcode_str = nullptr;
	uint8_t* mem_sim = nullptr;
	uint8_t* init_tbl = nullptr;
	uint32_t code_size = 0;
	uint32_t mem_size = 0;
	uint32_t offset = 0;

	printf("Simulate Xcodes\n\n");

	init_tbl = load_init_tbl_file(&size, &base);
	if (init_tbl == nullptr)
		return 1;

	result = interp.load(init_tbl + base, size - base);
	if (result != 0) {
		printf("Error: Failed to init xcode interpreter\n");
		result = 1;
		goto Cleanup;
	}

	if (isFlagSet(SW_OFFSET)) {
		if (offset > size - base) {
			printf("Error: Argument: '-offset' is out of bounds.\n");
			result = 1;
			goto Cleanup;
		}
		offset = params.offset;
	}

	mem_size = params.simSize;

	printf("init tbl file: %s\nxcode base: 0x%x\nxcode offset: 0x%x\nmem space: %d bytes\n\n", params.in_file, base, offset, mem_size);

	mem_sim = (uint8_t*)malloc(mem_size);
	if (mem_sim == nullptr) {
		result = 1;
		goto Cleanup;
	}
	memset(mem_sim, 0, mem_size);

	// simulate memory output
	hasMemChanges_total = false;
	printf("Xcodes:\n");
	while (interp.interpretNext(xcode) == 0) {
		// only care about xcodes that write to RAM
		if (xcode->opcode != XC_MEM_WRITE)
			continue;

		// sanity check of addr.
		if (xcode->addr < offset || xcode->addr - offset >= mem_size) {
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
		printf("\t%04lx: %s 0x%02x, 0x%08X\n", (unsigned long)(base + interp.offset - sizeof(XCODE)), opcode_str, xcode->addr, xcode->data);
	}

	if (!hasMemChanges_total) {
		printf("0 memory changes in range 0x0 - 0x%x\n", mem_size);
		goto Cleanup;
	}

	// if -d flag is set, dump the memory to a file, otherwise print the memory dump
	if (isFlagSet(SW_DMP)) {
		const char* filename = params.out_file;
		if (filename == nullptr)
			filename = "mem_sim.bin";

		printf("\n");
		result = writeFileF(filename, "x86 code", mem_sim, code_size);
		if (result != 0) {
			goto Cleanup;
		}
	}
	else {
		// print memory dump
		uint32_t j;
		printf("\nMem dump: ( %d bytes )\n", code_size);
		for (uint32_t i = 0; i < code_size; i += j) {
			if (i + 8 > code_size)
				j = code_size - i;
			else
				j = 8;

			printf("\t%04x: ", i);
			uprinth(mem_sim + i, j);
		}
	}

Cleanup:

	if (init_tbl != nullptr) {
		free(init_tbl);
		init_tbl = nullptr;
	}

	if (mem_sim != nullptr) {
		free(mem_sim);
		mem_sim = nullptr;
	}

	return result;
}
int encodeX86() {
	// encode x86 instructions to xcodes

	uint8_t* data = nullptr;
	uint8_t* buffer = nullptr;
	uint32_t dataSize = 0;
	uint32_t xcodeSize = 0;
	int result = 0;

	const char* filename = nullptr;

	printf("Encode X86\n\n");

	data = readFile(params.in_file, &dataSize, 0);
	if (data == nullptr) {
		return 1;
	}

	printf("x86 file:\t%s\nxcode base:\t0x%x\n\n", params.in_file, params.base);

	result = encodeX86AsMemWrites(data, dataSize, params.base, buffer, &xcodeSize);
	if (result != 0) {
		printf("Error: Failed to encode x86 instructions\n");
		goto Cleanup;
	}

	printf("xcodes: %ld\n", xcodeSize / sizeof(XCODE));

	// write the xcodes to file
	filename = params.out_file;
	if (filename == nullptr) {
		filename = "xcodes.bin";
	}

	result = writeFileF(filename, "xcodes", buffer, xcodeSize);

Cleanup:

	if (data != nullptr) {
		free(data);
	}

	if (buffer != nullptr) {
		free(buffer);
	}

	return 0;
}
int compressFile() {
	// lzx compress file

	uint8_t* data = nullptr;
	uint8_t* buff = nullptr;
	uint32_t dataSize = 0;
	uint32_t compressedSize = 0;
	int result = 0;
	float savings = 0;

	printf("Compress File\n\n");

	data = readFile(params.in_file, &dataSize, 0);
	if (data == nullptr) {
		return 1;
	}

	printf("file: %s\n\n", params.in_file);

	printf("Compressing file\n");
	result = lzx_compress(data, dataSize, &buff, &compressedSize);
	if (result != 0) {
		printf("Error: Compression failed, ");
		lzx_print_error(result);
		goto Cleanup;
	}

	savings = (1 - ((float)compressedSize / (float)dataSize)) * 100;
	printf("Compressed %u -> %u bytes (%.3f%% compression)\n", dataSize, compressedSize, savings);

	result = writeFileF(params.out_file, "compressed file", buff, compressedSize);

Cleanup:

	if (data != nullptr) {
		free(data);
		data = nullptr;
	}

	if (buff != nullptr) {
		free(buff);
		buff = nullptr;
	}

	return result;

}
int decompressFile() {
	// lzx decompress file

	uint8_t* data = nullptr;
	uint32_t dataSize = 0;
	float savings = 0;

	uint8_t* buff = nullptr;
	uint32_t decompressedSize = 0;

	int result = 0;

	printf("Decompress File\n\n");

	data = readFile(params.in_file, &dataSize, 0);
	if (data == nullptr) {
		return 1;
	}

	printf("file: %s\n\n", params.in_file);

	printf("Decompressing file\n");
	result = lzx_decompress(data, dataSize, &buff, nullptr, &decompressedSize);
	if (result != 0) {
		printf("Error: Decompression failed, ");
		lzx_print_error(result);
		goto Cleanup;
	}

	savings = (1 - ((float)dataSize / (float)decompressedSize)) * 100;
	printf("Decompressed %u -> %u bytes (%.3f%% compression)\n", dataSize, decompressedSize, savings);

	result = writeFileF(params.out_file, "decompressed file", buff, decompressedSize);

Cleanup:

	if (data != nullptr) {
		free(data);
		data = nullptr;
	}

	if (buff != nullptr) {
		free(buff);
		buff = nullptr;
	}

	return result;
}
int dumpCoffPeImg() {
	int result = 0;
	uint8_t* data = nullptr;
	uint32_t size = 0;

	printf("List NT Header\n\n");

	data = readFile(params.in_file, &size, 0);
	if (data == nullptr)
		return 1;

	printf("file: %s\nimage size: %d bytes\n\n", params.in_file, size);

	result = dump_nt_headers(data, size, false);

	free(data);
	data = nullptr;

	return result;
}
int info() {

	printf(XB_BIOS_TOOL_NAME_STR);
#ifdef __SANITIZE_ADDRESS__
	printf(" /sanitize");
#endif
#ifdef MEM_TRACKING
	printf(" /memtrack");
#endif
	printf("\nAuthor: tommojphillips\n" \
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
							case LOADINI_SETTING_TYPE_STR:
								setting_type = "str";
								break;
							case LOADINI_SETTING_TYPE_BOOL:
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

	printf("Help\n\nSee command help, use xbios -? <command>\n");
	printf("See encryption help, use xbios -? -help-enc\n");

	printf("\nCommands:\n");
	for (int i = CMD_INFO + 1; i < sizeof(cmd_tbl) / sizeof(CMD_TBL); ++i) {
		printf(" %s\n", cmd_tbl[i].sw);
	}

	printf("\n%s\n", HELP_USAGE_STR);

	return 0;
}
int helpEncryption() {
	printf("Help\n\n2BL encryption / decryption:\n" \
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
	if (_params->bldr_key != nullptr) {
		free(_params->bldr_key);
		_params->bldr_key = nullptr;
	}
	if (_params->kernel_key != nullptr) {
		free(_params->kernel_key);
		_params->kernel_key = nullptr;
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

	INIT_TBL* init_tbl = (INIT_TBL*)data;
	XCODE* xcode = nullptr;

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
	if (init_tbl->data_tbl_offset != 0) {
		if (init_tbl->data_tbl_offset < 0x80 + interp.offset + xcodesSize) {
			//printf("no space for xcodes. only %d bytes available. xcode size: %d bytes\n", (0x80 + interp.offset + xcodesSize - init_tbl->data_tbl_offset), xcodesSize);

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
		printf("XCODE: replacing quit xcode at 0x%lx with jump to free space at 0x%x\n", (unsigned long)((uint8_t*)xcode - data), 0x80 + interp.offset + offset);

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

	if (params.bldr_key_file != nullptr) {
		printf("bldr key file: %s\n", params.bldr_key_file);
		params.bldr_key = readFile(params.bldr_key_file, nullptr, XB_KEY_SIZE);
		if (params.bldr_key == nullptr)
			return 1;
		printf("bldr key: ");
		uprinth(params.bldr_key, XB_KEY_SIZE);
	}

	if (params.kernel_key_file != nullptr) {
		printf("krnl key file: %s\n", params.kernel_key_file);
		params.kernel_key = readFile(params.kernel_key_file, nullptr, XB_KEY_SIZE);
		if (params.kernel_key == nullptr)
			return 1;
		printf("krnl key: ");
		uprinth(params.kernel_key, XB_KEY_SIZE);
	}

	if (params.bldr_key != nullptr || params.kernel_key != nullptr)
		printf("\n");

	return 0;
}
int read_mcpx() {
	// read and verify mcpx rom file.

	uint8_t* mcpxData = nullptr;
	int result = 0;

	if (params.mcpx_file == nullptr)
		return 0;

	mcpxData = readFile(params.mcpx_file, nullptr, MCPX_BLOCK_SIZE);
	if (mcpxData == nullptr)
		return 1;

	result = mcpx_load(&params.mcpx, mcpxData);

	if (params.mcpx.rev == MCPX_REV_UNK) {
		printf("\nError: hash did not match known mcpx roms\n" \
			"See github page for md5 hashes.\n" \
			"1.) Use a MCPX dump\n" \
			"2.) Use a M.O.U.S.E v0.8.0, v0.9.0 rom\n" \
			"3.) Use -bldr-key <path>\n");
	}

	return result;
}

uint8_t* load_init_tbl_file(uint32_t* size, uint32_t* base) {
	uint8_t* init_tbl = nullptr;
	int result = 0;

	init_tbl = readFile(params.in_file, size, 0);
	if (init_tbl == nullptr)
		return nullptr;

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
		printf("Error: Invalid file size: %d. Expected less than 0x%x bytes\n", *size, MAX_BIOS_SIZE);
		result = 1;
		goto Cleanup;
	}

Cleanup:

	if (result != 0) {
		if (init_tbl != nullptr) {
			free(init_tbl);
			init_tbl = nullptr;
		}
		return nullptr;
	}

	return init_tbl;
}

void lzx_print_error(int error) {
	switch (error) {
		case LZX_ERROR_OUT_OF_MEMORY:
			printf("out of memory\n");
			break;
		case LZX_ERROR_INVALID_DATA:
			printf("bad parameters\n");
			break;
		case LZX_ERROR_BUFFER_OVERFLOW:
			printf("buffer overflow\n");
			break;
		case LZX_ERROR_FAILED:
			printf("fatal error\n");
			break;
		case LZX_ERROR_SUCCESS:
			printf("success\n");
			break;
		default:
			printf("unknown error");
			break;
	}
}

/* BIOS print functions */

void printBldrInfo(Bios* bios) {
	BIOS_LOAD_PARAMS bios_params = bios->params;

	printf("2BL:\n");

	if (bios->bios_status == BIOS_LOAD_STATUS_SUCCESS) {
		printf("Entry point:\t\t0x%08x\n", bios->bldr.ldr_params->bldr_entry_point);
		if (bios->bldr.entry != nullptr && bios->bldr.entry->bfm_entry_point != 0) {
			printf("BFM Entry point:\t0x%08x\n", bios->bldr.entry->bfm_entry_point);
		}
	}

	printf("Signature:\t\t");
	uprinth((uint8_t*)&bios->bldr.boot_params->signature, 4);

	uint32_t kernel_size = bios->bldr.boot_params->compressed_kernel_size;
	uint32_t kernel_data_size = bios->bldr.boot_params->uncompressed_kernel_data_size;
	uint32_t init_tbl_size = bios->bldr.boot_params->init_tbl_size;
	uint32_t romsize = bios_params.romsize;

	printf("Init table size:\t");
	uprintc((init_tbl_size >= 0 && init_tbl_size <= romsize), "%u", init_tbl_size);
	printf(" bytes\n");

	printf("Compressed kernel size: ");
	uprintc((kernel_size >= 0 && kernel_size <= romsize), "%u", kernel_size);
	printf(" bytes\n");

	printf("Kernel data size:\t");
	uprintc((kernel_data_size >= 0 && kernel_data_size <= romsize), "%u", kernel_data_size);
	printf(" bytes\n");
	printf("\n");
}
void printPreldrInfo(Bios* bios) {
	BIOS_LOAD_PARAMS bios_params = bios->params;

	if (bios->preldr.status > PRELDR_STATUS_FOUND)
		return;

	int jmp_offset = (int)bios->preldr.params->jmp_offset + 5;
	int jmp_address = PRELDR_REAL_BASE + jmp_offset;

	printf("FBL:\n");
	printf("Entry point:\t\t0x%08x", jmp_address);
	if (jmp_address == PRELDR_TEA_ATTACK_ENTRY_POINT) {
		printf(" ( TEA Attack )\n");
	}
	else {
		if (jmp_offset < 0) {
			printf(" ( x0x%x )\n", jmp_offset);
		}
		else {
			printf(" ( +0x%x )\n", jmp_offset);
		}
	}

	if (jmp_address < PRELDR_REAL_BASE || jmp_address > PRELDR_REAL_BASE + PRELDR_BLOCK_SIZE) {
		// escaped mcpx!
	}

	printf("\n");
}
void printInitTblInfo(Bios* bios) {
	INIT_TBL* init_tbl = bios->init_tbl;
	uint16_t kernel_ver = init_tbl->kernel_ver;

	printf("Init Table:\n");

	if ((kernel_ver & 0x8000) != 0) {
		kernel_ver = kernel_ver & 0x7FFF; // clear the 0x8000 bit
		printf("Kernel delay flag set\n");
	}

	if (kernel_ver != 0) {
		printf("Kernel version:\t\t%d\n", kernel_ver);
	}

	printf("Identifier:\t\t%02x ", (uint8_t)init_tbl->init_tbl_identifier);
	switch (init_tbl->init_tbl_identifier) {
		case 0x30: // xbox devkit beta (mcpx x2)
			printf("dvt3");
			break;
		case 0x46: //xbox devkit (mcpx x2)
			printf("dvt4");
			break;
		case 0x60: // xbox v1.0. (mcpx x3 v1.0) (k:3944 - k:4627)
			printf("dvt6");
			break;
		case 0x70: // xbox v1.1. (mcpx x3 v1.1) (k:4817)
			printf("qt");
			break;
		case 0x80: // xbox v1.2, v1.3, v1.4. (mcpx x3 v1.1) (k:5101 - k:5713)
			printf("xblade");
			break;
		case 0x90: // xbox v1.6a, v1.6b. (mcpx x3 v1.1) (k:5838)
			printf("tuscany");
			break;
	}

	printf("\nRevision:\t\trev %d.%02d\n", init_tbl->revision >> 8, init_tbl->revision & 0xFF);
	printf("\n");
}

void printNv2aInfo(Bios* bios) {
	INIT_TBL* init_tbl = bios->init_tbl;
	int i;
	uint32_t item;

	const uint32_t UINT_ALIGN = sizeof(uint32_t);
	const uint32_t ARRAY_BYTES = sizeof(init_tbl->vals);
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
		item = ((uint32_t*)init_tbl)[i];
		printf("%04x:\t\t0x%08x\n", i * UINT_ALIGN, item);
	}

	// print the array
	char str[14 * 4 + 1] = { 0 };
	bool hasValue = false;
	bool cHasValue = false;
	const uint32_t* valArray = init_tbl->vals;

	for (i = 0; i < ARRAY_SIZE; ++i) {
		cHasValue = false;
		if (valArray[i] != 0) {
			cHasValue = true;
			hasValue = true;
		}

		if (cHasValue && hasValue)
			snprintf(str + i * 3, sizeof(str) - (i * 3), "%02lX ", (unsigned long)valArray[i]);
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
		item = ((uint32_t*)init_tbl)[i];
		printf("%04x:\t\t0x%08x %s\n", i * UINT_ALIGN, item, init_tbl_comments[i - ARRAY_START - ARRAY_SIZE]);
	}
	printf("\n");
}
void printDataTblInfo(Bios* bios) {

	if (bios->init_tbl->data_tbl_offset == 0  || bios->size < bios->init_tbl->data_tbl_offset + sizeof(ROM_DATA_TBL)) {
		printf("Error: Rom Data Table not found.\n");
		return;
	}

	ROM_DATA_TBL* data_tbl = (ROM_DATA_TBL*)(bios->data + bios->init_tbl->data_tbl_offset);

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
		data_tbl->cal.max_m_clk,
		data_tbl->cal.slowest.countA, data_tbl->cal.slowest.countB,
		data_tbl->cal.slow.countA, data_tbl->cal.slow.countB,
		data_tbl->cal.standard.countA, data_tbl->cal.standard.countB,
		data_tbl->cal.fast.countA, data_tbl->cal.fast.countB,
		data_tbl->cal.fastest.countA, data_tbl->cal.fastest.countB);

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
			data_tbl->samsung[i].adr_drv_fall, data_tbl->samsung[i].adr_drv_rise, data_tbl->micron[i].adr_drv_fall, data_tbl->micron[i].adr_drv_rise,
			data_tbl->samsung[i].adr_slw_fall, data_tbl->samsung[i].adr_slw_rise, data_tbl->micron[i].adr_slw_fall, data_tbl->micron[i].adr_slw_rise,
			data_tbl->samsung[i].clk_drv_fall, data_tbl->samsung[i].clk_drv_rise, data_tbl->micron[i].clk_drv_fall, data_tbl->micron[i].clk_drv_rise,
			data_tbl->samsung[i].clk_slw_fall, data_tbl->samsung[i].clk_slw_rise, data_tbl->micron[i].clk_slw_fall, data_tbl->micron[i].clk_slw_rise,
			data_tbl->samsung[i].dat_drv_fall, data_tbl->samsung[i].dat_drv_rise, data_tbl->micron[i].dat_drv_fall, data_tbl->micron[i].dat_drv_rise,
			data_tbl->samsung[i].dat_slw_fall, data_tbl->samsung[i].dat_slw_rise, data_tbl->micron[i].dat_slw_fall, data_tbl->micron[i].dat_slw_rise,
			data_tbl->samsung[i].dqs_drv_fall, data_tbl->samsung[i].dqs_drv_rise, data_tbl->micron[i].dqs_drv_fall, data_tbl->micron[i].dqs_drv_rise,
			data_tbl->samsung[i].dqs_slw_fall, data_tbl->samsung[i].dqs_slw_rise, data_tbl->micron[i].dqs_slw_fall, data_tbl->micron[i].dqs_slw_rise,
			data_tbl->samsung[i].dat_inb_delay, data_tbl->micron[i].dat_inb_delay, data_tbl->samsung[i].clk_ic_delay, data_tbl->micron[i].clk_ic_delay,
			data_tbl->samsung[i].dqs_inb_delay, data_tbl->micron[i].dqs_inb_delay);
	}
	printf("\n");
}
void printKeyInfo(Bios* bios) {

	MCPX* mcpx = bios->params.mcpx;
	PUBLIC_KEY* pubkey;

	if (mcpx->sbkey != nullptr) {
		printf("SB key (+%ld):\t", mcpx->sbkey - mcpx->data);
		uprinth(mcpx->sbkey, XB_KEY_SIZE);
	}

	if (bios->bldr.bfm_key != nullptr) {
		printf("BFM key:\t");
		uprinth(bios->bldr.bfm_key, XB_KEY_SIZE);
	}

	if (bios->bldr.keys != nullptr) {
		printf("EEPROM key:\t");
		uprinth(bios->bldr.keys->eeprom_key, XB_KEY_SIZE);
		printf("Cert key:\t");
		uprinth(bios->bldr.keys->cert_key, XB_KEY_SIZE);
		printf("Kernel key:\t");
		uprinth(bios->bldr.keys->kernel_key, XB_KEY_SIZE);
	}

	if (bios->preldr.status == PRELDR_STATUS_BLDR_DECRYPTED) {
		printf("\nPreldr key:\t");
		uprinth(bios->preldr.bldr_key, SHA1_DIGEST_LEN);
	}

	if (bios->kernel.img != nullptr) {
		if (rsa_findPublicKey(bios->kernel.img, bios->kernel.img_size, &pubkey, nullptr) == RSA_ERROR_SUCCESS) {
			printf("\nPublic key:\b\b\b\b");
			uprinthl((uint8_t*)&pubkey->modulus, RSA_MOD_SIZE(&pubkey->header), 16, "\t\t", 0);
		}
	}
}

int validateArgs() {
	// validate command line arguments

	// rom size in kb
	if (isFlagClear(SW_ROMSIZE)) {
		params.romsize = MIN_BIOS_SIZE;
	}
	else {
		params.romsize *= 1024;
		if (bios_check_size(params.romsize) != 0) {
			printf("Error: invalid rom size: %d\n", params.romsize);
			return 1;
		}
	}

	// bin size in kb
	if (isFlagClear(SW_BINSIZE)) {
		params.binsize = MIN_BIOS_SIZE;
	}
	else {
		params.binsize *= 1024;
		if (bios_check_size(params.binsize) != 0) {
			printf("Error: invalid bin size: %d\n", params.binsize);
			return 1;
		}
	}

	// xcode sim size in bytes
	if (isFlagClear(SW_SIM_SIZE)) {
		params.simSize = 32;
	}
	else {
		if (params.simSize < 4 || params.simSize > (128 * 1024 * 1024)) {
			printf("Error: invalid sim size: %d\n", params.simSize);
			return 1;
		}
		if (params.simSize % 4 != 0) {
			printf("Error: sim size must be devisible by 4.\n");
			return 1;
		}
	}

	return 0;
}

int main(int argc, char** argv) {

	printf("Xbox Bios Tools by tommojphillips\n\n");

	int result = 0;
	cmd = nullptr;
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
