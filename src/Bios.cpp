// Bios.cpp: Implements functions for loading an Original Xbox BIOS file, decrypting it, and extracting the bootloader, kernel, and other data.

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
 * along with this program.If not, see https://www.gnu.org/licenses/
*/

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

// std incl
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if !__APPLE__
#include <malloc.h>
#endif

// user incl
#include "Bios.h"
#include "Mcpx.h"
#include "bldr.h"
#include "util.h"
#include "lzx.h"
#include "rc4.h"
#include "rsa.h"
#include "sha1.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#endif

static int validate_required_space(const uint32_t requiredSpace, uint32_t* size);

int Bios::load(uint8_t* buff, const uint32_t binsize, const BIOS_LOAD_PARAMS* bios_params) {
	// load bios

	bios_status = init(buff, binsize, bios_params);
	if (bios_status != 0) {
		return bios_status;
	}

	// verify the presence of FBL and decrypt the 2BL.
	preldrValidateAndDecryptBldr();
	if (preldr.status == PRELDR_STATUS_ERROR) {
		bios_status = BIOS_LOAD_STATUS_FAILED;
		return bios_status;
	}

	// if FBL didnt decrypt 2BL, decrypt 2BL.
	if (preldr.status != PRELDR_STATUS_BLDR_DECRYPTED && bldr.encryption_state) {
		// get sb key
		uint8_t* sbkey = NULL;
		if (params.bldr_key != NULL) {
			sbkey = params.bldr_key;
		}
		else if (params.mcpx->sbkey != NULL) {
			sbkey = params.mcpx->sbkey;
		}

		if (sbkey != NULL) {
			/*if we found FBL, dont mangle FBL section of 2BL.*/
			if (preldr.status == PRELDR_STATUS_FOUND) {
				preldrSymmetricEncDecBldr(sbkey, XB_KEY_SIZE);
			}
			else {
				symmetricEncDecBldr(sbkey, XB_KEY_SIZE);
			}
		}
	}

	bios_status = validateBldrBootParams();
	if (bios_status != 0) {
		return bios_status;
	}

	getOffsets2();

	// decrypt the kernel
	if (kernel.encryption_state) {
		symmetricEncDecKernel();
	}

	bios_status = BIOS_LOAD_STATUS_SUCCESS;
	return bios_status;
}
int Bios::build(BIOS_BUILD_PARAMS* build_params, uint32_t binsize, BIOS_LOAD_PARAMS* bios_params) {
	// build a bios from the build parameters

	const uint32_t requiredSpace = BLDR_BLOCK_SIZE + MCPX_BLOCK_SIZE + build_params->kernel_size + build_params->kernel_data_size + build_params->init_tbl_size;

	if (build_params->bfm) {
		binsize = MAX_BIOS_SIZE;
	}

	if (validate_required_space(requiredSpace, &bios_params->romsize) != 0) {
		bios_status = BIOS_LOAD_STATUS_FAILED;
		return bios_status;
	}
        if (params.romsize != bios_params->romsize) {
          printf("Adjusted rom size:\t%u kb\n", bios_params->romsize / 1024);
        }

	if (binsize < bios_params->romsize) {
		binsize = bios_params->romsize;
	}

	bios_status = init(NULL, binsize, bios_params);
	if (bios_status != 0) {
		return bios_status;
	}

	// override encryption flags; reverse for building.
	bldr.encryption_state = params.enc_bldr;
	kernel.encryption_state = (!params.enc_kernel && params.kernel_key == NULL) || (params.enc_kernel && params.kernel_key != NULL);

	if (build_params->bldrSize > BLDR_BLOCK_SIZE) {
		printf("Error: 2BL is too big\n");
		bios_status = BIOS_LOAD_STATUS_FAILED;
		return bios_status;
	}

	memcpy(bldr.data, build_params->bldr, build_params->bldrSize);

	if (!build_params->nobootparams) {
		printf("Updating boot params\n");
		bldr.boot_params->compressed_kernel_size = build_params->kernel_size;
		bldr.boot_params->uncompressed_kernel_data_size = build_params->kernel_data_size;

		if (build_params->hacksignature)
			bldr.boot_params->signature = 0xFFFFFFFF;
		else
			bldr.boot_params->signature = BOOT_SIGNATURE;

		if (build_params->hackinittbl)
			bldr.boot_params->init_tbl_size = 0;
		else
			bldr.boot_params->init_tbl_size = build_params->init_tbl_size;
	}

	bios_status = validateBldrBootParams();
	if (bios_status != 0) {
		return bios_status;
	}

	getOffsets2();

	if (build_params->eeprom_key != NULL) {
		printf("Updating eeprom key\n");
		memcpy(bldr.keys->eeprom_key, build_params->eeprom_key, XB_KEY_SIZE);
	}

	if (build_params->cert_key != NULL) {
		printf("Updating cert key\n");
		memcpy(bldr.keys->cert_key, build_params->cert_key, XB_KEY_SIZE);
	}

	// to-do add option to compress a kernel image.

	// copy in the compressed kernel image.
	memcpy(kernel.compressed_kernel_ptr, build_params->compressed_kernel, build_params->kernel_size);

	// copy in the kernel data section. this is data is used by the kernel, stock dashboard's, launch data path for launch xbes etc.
	memcpy(kernel.uncompressed_data_ptr, build_params->kernel_data, build_params->kernel_data_size);

	if (data + build_params->init_tbl_size >= kernel.compressed_kernel_ptr) {
		printf("Error: Init table is too big\n");
		bios_status = BIOS_LOAD_STATUS_FAILED;
		return bios_status;
	}
	memcpy(data, build_params->init_tbl, build_params->init_tbl_size);

	// build a bios that boots from media. ( BFM )
	if (build_params->bfm) {
		printf("Adding kernel delay flag (bfm)\n");
		uint32_t* bootFlags = (uint32_t*)(&init_tbl->init_tbl_identifier);
		*bootFlags |= KD_DELAY_FLAG;
	}

	// encrypt the kernel
	if (!kernel.encryption_state) {
		symmetricEncDecKernel();

		// if the kernel was encrypted with a key file, update the key in the 2BL.
		if (kernel.encryption_state) {
			if (bldr.keys != NULL && params.kernel_key != NULL) {
				printf("Updating kernel key\n");
				memcpy(bldr.keys->kernel_key, params.kernel_key, XB_KEY_SIZE);
			}
		}
	}
	else {
		if (build_params->zero_kernel_key) {
			if (bldr.keys != NULL && params.kernel_key != NULL) {
				printf("Zeroing kernel key\n");
				memset(bldr.keys->kernel_key, 0, XB_KEY_SIZE);
			}
		}
	}

	// encrypt 2bl.
	if (!bldr.encryption_state) {

		// get sb key
		uint8_t* sbkey = NULL;
		if (params.bldr_key != NULL) {
			sbkey = params.bldr_key;
		}
		else if (params.mcpx->sbkey != NULL) {
			sbkey = params.mcpx->sbkey;
		}
		else if (build_params->bfm && bldr.bfm_key != NULL) {
			sbkey = bldr.bfm_key;
		}

		if (sbkey != NULL) {
			symmetricEncDecBldr(sbkey, XB_KEY_SIZE);
		}
	}

	if (build_params->preldr != NULL && build_params->preldr_size > 0) {
		if (build_params->preldr_size > PRELDR_SIZE) {
			printf("Error: Preldr is too big\n");
			bios_status = BIOS_LOAD_STATUS_FAILED;
			return bios_status;
		}

		memcpy(preldr.data, build_params->preldr, build_params->preldr_size);
	}

	if (size > params.romsize) {
		if (bios_replicate_data(params.romsize, binsize, data, size) != 0) {
			printf("Error: Failed to replicate the bios\n");
			bios_status = BIOS_LOAD_STATUS_FAILED;
			return bios_status;
		}
	}

	bios_status = BIOS_LOAD_STATUS_SUCCESS;
	return bios_status;
}

int Bios::init(uint8_t* buff, const uint32_t binsize, const BIOS_LOAD_PARAMS* bios_params) {
	// init bios from buffer

	if (bios_params != NULL) {
		memcpy(&params, bios_params, sizeof(BIOS_LOAD_PARAMS));
	}

	if (buff == NULL) {
		data = (uint8_t*)malloc(binsize);
		if (data == NULL)
			return BIOS_LOAD_STATUS_FAILED;
		memset(data, 0, binsize);
	}
	else {
		data = buff;
	}

	size = binsize;

	if (params.romsize > size) {
		params.romsize = size;
	}

	bldr.encryption_state = !params.enc_bldr;
	kernel.encryption_state = !params.enc_kernel;

	getOffsets();

	return 0;
}

void Bios::getOffsets() {
	// calculate the initial pointers to BIOS components.

	init_tbl = (INIT_TBL*)(data);

	bldr.data = (data + size - BLDR_BLOCK_SIZE - MCPX_BLOCK_SIZE);
	bldr.ldr_params = (BOOT_LDR_PARAM*)(bldr.data);
	bldr.boot_params = (BOOT_PARAMS*)(bldr.data + BLDR_BLOCK_SIZE - sizeof(BOOT_PARAMS));

	preldr.data = (bldr.data + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE);

	rom_digest = (bldr.data + BLDR_BLOCK_SIZE - ROM_DIGEST_SIZE - PRELDR_PARAMS_SIZE);
}
void Bios::getOffsets2() {
	// calculate the pointers to the 2bl entry and keys. 2BL needs to be unencrypted for this to work.

	uint32_t entry_offset = (bldr.ldr_params->bldr_entry_point & 0x0000FFFF/*- BLDR_BASE*/);
	bldr.entry = (BLDR_ENTRY*)(bldr.data + entry_offset - sizeof(BLDR_ENTRY));
	if (IN_BOUNDS(bldr.entry, bldr.data, BLDR_BLOCK_SIZE) == false)	{
		bldr.entry = NULL;
	}
	else {
		uint32_t keys_offset = (bldr.entry->keys_ptr & 0x0000FFFF/*- BLDR_RELOC*/);
		bldr.keys = (BLDR_KEYS*)(bldr.data + keys_offset);
		if (IN_BOUNDS(bldr.keys, bldr.data, BLDR_BLOCK_SIZE) == false) {
			bldr.keys = NULL;
		}

		bldr.bfm_key = (bldr.data + keys_offset - XB_KEY_SIZE);
		if (IN_BOUNDS_BLOCK(bldr.bfm_key, XB_KEY_SIZE, bldr.data, BLDR_BLOCK_SIZE) == false) {
			bldr.bfm_key = NULL;
		}
	}

	// set the kernel pointers.
	kernel.uncompressed_data_ptr = (bldr.data - bldr.boot_params->uncompressed_kernel_data_size);
	kernel.compressed_kernel_ptr = (kernel.uncompressed_data_ptr - bldr.boot_params->compressed_kernel_size);

	// calculate the available space in the bios.
	available_space = params.romsize - MCPX_BLOCK_SIZE - BLDR_BLOCK_SIZE - bldr.boot_params->uncompressed_kernel_data_size - bldr.boot_params->compressed_kernel_size;
}

int Bios::validateBldrBootParams() {
	// validate 2BL boot params; check bounds, sizes

	const uint32_t kernel_size = bldr.boot_params->compressed_kernel_size;
	const uint32_t kernel_data_size = bldr.boot_params->uncompressed_kernel_data_size;
	const uint32_t inittbl_size = bldr.boot_params->init_tbl_size;
	const uint32_t kernel_size_valid = kernel_size >= 0 && kernel_size <= size;
	const uint32_t kernel_data_size_valid = kernel_data_size >= 0 && kernel_data_size <= size;
	const uint32_t inittbl_size_valid = inittbl_size >= 0 && inittbl_size <= size;

	return (kernel_size_valid && kernel_data_size_valid && inittbl_size_valid) ? BIOS_LOAD_STATUS_SUCCESS : BIOS_LOAD_STATUS_INVALID_BLDR;
}

void Bios::preldrCreateKey(uint8_t* sbkey, uint8_t* key) {
	// create the bldr key from the sb key.

	uint8_t* nonce = (preldr.data + PRELDR_BLOCK_SIZE - PRELDR_NONCE_SIZE);
	SHA1Context sha = { 0 };
	SHA1Reset(&sha);
	SHA1Input(&sha, sbkey, XB_KEY_SIZE);
	SHA1Input(&sha, nonce, PRELDR_NONCE_SIZE);
	for (int i = 0; i < XB_KEY_SIZE; ++i)
		key[i] = sbkey[i] ^ 0x5C;
	SHA1Input(&sha, key, XB_KEY_SIZE);
	SHA1Result(&sha, key);
}
void Bios::preldrValidateAndDecryptBldr() {
	// validate the preldr and decrypt the 2bl.

	preldr.status = PRELDR_STATUS_NOT_FOUND;
	preldr.params = (PRELDR_PARAMS*)(preldr.data);
	preldr.jmp_offset = preldr.params->jmp_offset + 5; // +5 bytes (opcode + offset)

	if (preldr.params->jmp_opcode != 0xE9) {
		return;
	}

	preldr.ptr_block = (PRELDR_PTR_BLOCK*)(preldr.data + preldr.jmp_offset - sizeof(PRELDR_PTR_BLOCK));
	if (IN_BOUNDS(preldr.ptr_block, preldr.data, PRELDR_BLOCK_SIZE) == false) {
		preldr.ptr_block = NULL;
		preldr.public_key = NULL;
		preldr.func_block = NULL;
	}
	else {
		// public key pointer
		preldr.public_key = (XB_PUBLIC_KEY*)(preldr.data + preldr.ptr_block->public_key_ptr - PRELDR_REAL_BASE);
		if (IN_BOUNDS(preldr.public_key, preldr.data, PRELDR_BLOCK_SIZE) == false) {
			preldr.public_key = NULL;
		}

		// sha1 function block pointer
		preldr.func_block = (PRELDR_FUNC_BLOCK*)(preldr.data + preldr.ptr_block->func_block_ptr - PRELDR_REAL_BASE);
		if (IN_BOUNDS(preldr.func_block, preldr.data, PRELDR_BLOCK_SIZE) == false) {
			preldr.func_block = NULL;
		}
	}

	preldr.status = PRELDR_STATUS_FOUND;

	// ignore the preldr if a rev 0 equivalent mcpx was provided.
	if (params.mcpx->rev == MCPX_REV_0) {
		return;
	}

	// get sbkey
	uint8_t* sbkey = NULL;
	if (params.bldr_key != NULL) {
		sbkey = params.bldr_key;
	}
	else if (params.mcpx->sbkey != NULL) {
		sbkey = params.mcpx->sbkey;
	}
	else {
		return;
	}

	preldrCreateKey(sbkey, preldr.bldr_key);

	// continue if 2BL is encrypted.
	if (!bldr.encryption_state)
		return;

	preldrSymmetricEncDecBldr(preldr.bldr_key, SHA1_DIGEST_LEN); /*dont mangle the FBL section of 2BL.*/

	// calculate the offset to the 2BL entry point.
	PRELDR_ENTRY* entry = (PRELDR_ENTRY*)(bldr.data + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE - sizeof(PRELDR_ENTRY));
	if (entry->bldr_entry_offset > BLDR_BLOCK_SIZE) {
		return;
	}

	// restore 2BL loader params.
	// the first 16 bytes of 2BL were zeroed during the build process.
	// it makes up the 2BL entry point (4 bytes), and part of the
	// command line argument buffer (12 bytes)
	memset(bldr.data, 0, 16);
	bldr.ldr_params->bldr_entry_point = BLDR_BASE + entry->bldr_entry_offset;

	if (params.restore_boot_params) {
		// restore 2BL boot params; move the boot params 16 bytes right
		// this will write over the preldr nonce.
		uint8_t* bootParamsPtr = (uint8_t*)bldr.boot_params - PRELDR_NONCE_SIZE;
		for (int i = sizeof(BOOT_PARAMS) - 1; i >= 0; --i) {
			((uint8_t*)bldr.boot_params)[i] = bootParamsPtr[i];
		}
		memset(bootParamsPtr, 0, PRELDR_NONCE_SIZE);
	}
	else {
		// temp fixup of 2BL boot params pointer.
		bldr.boot_params = (BOOT_PARAMS*)((uint8_t*)bldr.boot_params - PRELDR_NONCE_SIZE);
	}

	// FBL load success.
	preldr.status = PRELDR_STATUS_BLDR_DECRYPTED;
}

void Bios::preldrSymmetricEncDecBldr(const uint8_t* key, const uint32_t len) {
	// encrypt / decrypt 2bl ( preserve preldr block )

	if (!IN_BOUNDS_BLOCK(bldr.data, BLDR_BLOCK_SIZE, data, size)) {
		printf("Error: De/Encrypting 2BL. 2BL ptr is out of bounds\n");
		return;
	}

	printf("%s 2BL (preserving FBL)\n", bldr.encryption_state ? "Decrypting" : "Encrypting");

	RC4_CONTEXT context = { 0 };
	rc4_key(&context, key, len);

	// decrypt 2BL up to FBL
	rc4(&context, bldr.data, BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE);

	// seed context; we do this so we dont mangle the FBL decrypting 2BL
	// but still correctly decrypt parts after the preldr block.
	rc4(&context, NULL, PRELDR_BLOCK_SIZE - PRELDR_PARAMS_SIZE);

	// decrypt parts after the preldr block; FBL params, 2BL boot params.
	rc4(&context, bldr.data + BLDR_BLOCK_SIZE - PRELDR_PARAMS_SIZE, PRELDR_PARAMS_SIZE);

	bldr.encryption_state = !bldr.encryption_state;
}
void Bios::symmetricEncDecBldr(const uint8_t* key, const uint32_t len) {
	// encrypt / decrypt 2bl

	if (!IN_BOUNDS_BLOCK(bldr.data, BLDR_BLOCK_SIZE, data, size)) {
		printf("Error: De/Encrypting 2BL. 2BL ptr is out of bounds\n");
		return;
	}

	printf("%s 2BL\n", bldr.encryption_state ? "Decrypting" : "Encrypting");

	RC4_CONTEXT context = { 0 };
	rc4_key(&context, key, len);
	rc4(&context, bldr.data, BLDR_BLOCK_SIZE);

	bldr.encryption_state = !bldr.encryption_state;
}
void Bios::symmetricEncDecKernel() {
	// encrypt / decrypt kernel

	// if key not provided on cli; try get key from bldr block.
	uint8_t* key = params.kernel_key;
	if (key == NULL) {
		if (bldr.keys == NULL)
			return;

		key = bldr.keys->kernel_key;
		if (key == NULL)
			return;

		uint8_t tmp[XB_KEY_SIZE] = { 0 };
		if (memcmp(key, tmp, XB_KEY_SIZE) == 0)
			return; // kernel key is all 0's

		memset(tmp, 0xFF, XB_KEY_SIZE);
		if (memcmp(key, tmp, XB_KEY_SIZE) == 0)
			return; // kernel key is all FF's

		// key is valid; use it.
	}

	if (!IN_BOUNDS_BLOCK(kernel.compressed_kernel_ptr, bldr.boot_params->compressed_kernel_size, data, size)) {
		printf("Error: De/Encrypting kernel. kernel ptr is out of bounds\n");
		return;
	}

	printf("%s kernel\n", kernel.encryption_state ? "Decrypting" : "Encrypting");

	RC4_CONTEXT context = { 0 };
	rc4_key(&context, key, XB_KEY_SIZE);
	rc4(&context, kernel.compressed_kernel_ptr, bldr.boot_params->compressed_kernel_size);

	kernel.encryption_state = !kernel.encryption_state;
}

int Bios::decompressKrnl() {
	// decompress kernel

	if (kernel.compressed_kernel_ptr == NULL || bios_status != BIOS_LOAD_STATUS_SUCCESS) {
		return 1;
	}

	// use decompression function.
	uint32_t buffer_size = (1 * 1024 * 1024 / 2); // 512 kb ( 26 blocks )
	kernel.img = (uint8_t*)malloc(buffer_size);
	if (kernel.img == NULL)
		return 1;
	if (lzx_decompress(kernel.compressed_kernel_ptr, bldr.boot_params->compressed_kernel_size, &kernel.img, &buffer_size, &kernel.img_size) != 0)
		return 1;
	return 0;
}
int Bios::preldrDecryptPublicKey() {
	// decrypt the preldr public key

	if (preldr.public_key == NULL)
		return 1;

	// get sbkey
	uint8_t* sbkey = NULL;
	if (params.bldr_key != NULL) {
		sbkey = params.bldr_key;
	}
	else if (params.mcpx->sbkey != NULL) {
		sbkey = params.mcpx->sbkey;
	}
	else {
		return 1;
	}

	RC4_CONTEXT context = { 0 };
	rc4_key(&context, sbkey, 12);
	rc4(&context, (uint8_t*)preldr.public_key, sizeof(XB_PUBLIC_KEY));

	return 0;
}

void Bios::resetValues() {
	// reset bios class values.

	bios_init_params(&params);
	bios_init_preldr(&preldr);
	bios_init_bldr(&bldr);
	bios_init_kernel(&kernel);

	data = NULL;
	size = 0;

	init_tbl = NULL;
	rom_digest = NULL;
	available_space = -1;

	bios_status = BIOS_LOAD_STATUS_SUCCESS;
}
void Bios::unload() {
	// unload bios

	if (data != NULL) {
		free(data);
		data = NULL;
	}

	if (kernel.img != NULL) {
		free(kernel.img);
		kernel.img = NULL;
	}

	resetValues();
}

void bios_print_state(Bios* bios) {
	printf("\n2BL entry:\t0x%08X\nsignature:\t", bios->bldr.ldr_params->bldr_entry_point);
	uprinth((uint8_t*)&bios->bldr.boot_params->signature, 4);
	printf("krnl data size:\t%u bytes\nkrnl size:\t%u bytes\n" \
		"2bl size:\t%u bytes\ninit tbl size:\t%u bytes\n" \
		"avail space:\t%u bytes\n\n",
		bios->bldr.boot_params->uncompressed_kernel_data_size, bios->bldr.boot_params->compressed_kernel_size,
		BLDR_BLOCK_SIZE, bios->bldr.boot_params->init_tbl_size, bios->available_space);
}
int bios_check_size(const uint32_t size) {
	switch (size) {
		case 0x40000U:
		case 0x80000U:
		case 0x100000U:
			return 0;
	}
	return 1;
}
int bios_replicate_data(uint32_t from, uint32_t to, uint8_t* buffer, uint32_t buffersize) {
	// replicate buffer based on to

	uint32_t offset = 0;
	uint32_t new_size = 0;

	if (from >= to || buffersize < to)
		return 1;

	offset = from;
	while (offset < to) {
		new_size = offset * 2;
		if (offset < new_size && to >= new_size) {
			memcpy(buffer + offset, buffer, offset);
		}

		offset = new_size;
	}

	return 0;
}

void bios_init_preldr(PRELDR* preldr) {
	preldr->data = NULL;
	preldr->params = NULL;
	preldr->ptr_block = NULL;
	preldr->func_block = NULL;
	preldr->public_key = NULL;
	for (int i = 0; i < SHA1_DIGEST_LEN; ++i) {
		preldr->bldr_key[i] = 0;
	}
	preldr->jmp_offset = 0;
	preldr->status = PRELDR_STATUS_ERROR;
}
void bios_init_bldr(BLDR* bldr) {
	bldr->data = NULL;
	bldr->entry = NULL;
	bldr->bfm_key = NULL;
	bldr->keys = NULL;
	bldr->boot_params = NULL;
	bldr->ldr_params = NULL;
	bldr->encryption_state = false;
}
void bios_init_kernel(KERNEL* kernel) {
	kernel->compressed_kernel_ptr = NULL;
	kernel->uncompressed_data_ptr = NULL;
	kernel->img = NULL;
	kernel->img_size = 0;
	kernel->encryption_state = false;
}
void bios_init_params(BIOS_LOAD_PARAMS* params) {
	params->romsize = 0;
	params->bldr_key = NULL;
	params->kernel_key = NULL;
	params->mcpx = NULL;
	params->enc_bldr = false;
	params->enc_kernel = false;
	params->restore_boot_params = true;
}
void bios_init_build_params(BIOS_BUILD_PARAMS* params) {
	params->init_tbl = NULL;
	params->preldr = NULL;
	params->bldr = NULL;
	params->compressed_kernel = NULL;
	params->kernel_data = NULL;
	params->eeprom_key = NULL;
	params->cert_key = NULL;
	params->preldr_size = 0;
	params->bldrSize = 0;
	params->init_tbl_size = 0;
	params->kernel_size = 0;
	params->kernel_data_size = 0;
	params->bfm = false;
	params->hackinittbl = false;
	params->hacksignature = false;
	params->nobootparams = false;
}
void bios_free_build_params(BIOS_BUILD_PARAMS* params) {
	if (params->preldr != NULL) {
		free(params->preldr);
		params->preldr = NULL;
	}
	if (params->bldr != NULL) {
		free(params->bldr);
		params->bldr = NULL;
	}
	if (params->init_tbl != NULL) {
		free(params->init_tbl);
		params->init_tbl = NULL;
	}
	if (params->compressed_kernel != NULL) {
		free(params->compressed_kernel);
		params->compressed_kernel = NULL;
	}
	if (params->kernel_data != NULL) {
		free(params->kernel_data);
		params->kernel_data = NULL;
	}
	if (params->eeprom_key != NULL) {
		free(params->eeprom_key);
		params->eeprom_key = NULL;
	}
	if (params->cert_key != NULL) {
		free(params->cert_key);
		params->cert_key = NULL;
	}
}

static int validate_required_space(const uint32_t requiredSpace, uint32_t* size) {
	// verify the rom size is big enough for the bios image.
	if (*size < requiredSpace) {
		while (*size < requiredSpace && *size < MAX_BIOS_SIZE) {
			*size *= 2;
		}

		if (bios_check_size(*size) != 0) {
			printf("Error: romsize is less than the total size of the bios.\n");
			return 1;
		}
	}

	return 0;
}
