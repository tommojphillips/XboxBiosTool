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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

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

int validate_required_space(const uint32_t requiredSpace, uint32_t* size);

int Bios::load(uint8_t* buff, const uint32_t binsize, const BiosParams* biosParams) {
	int result;

	init(buff, binsize, biosParams);

	// verify the presence of FBL and decrypt the 2BL.
	preldrValidateAndDecryptBldr();
	if (preldr.status == PRELDR_STATUS_ERROR) {
		return BIOS_LOAD_STATUS_FAILED;
	}

	// if FBL didnt decrypt 2BL, decrypt 2BL.
	if (preldr.status != PRELDR_STATUS_BLDR_DECRYPTED && bldr.encryptionState) {
		// get sb key
		uint8_t* sbkey = NULL;
		if (params.keyBldr != NULL) {
			sbkey = params.keyBldr;
		}
		else if (params.mcpx->sbkey != NULL) {
			sbkey = params.mcpx->sbkey;
		}

		if (sbkey != NULL) {
			/*if we found FBL, dont mangle FBL section of 2BL.*/
			if (preldr.status == PRELDR_STATUS_FOUND) {
				printf("decrypting 2BL (Preserving FBL)\n");
				preldrSymmetricEncDecBldr(sbkey, XB_KEY_SIZE); 
			}
			else {
				symmetricEncDecBldr(sbkey, XB_KEY_SIZE); /*dont care*/
			}
		}
	}
	
	result = validateBldrBootParams();
	if (result != 0)
		return result;

	getOffsets2();

	// decrypt the kernel
	if (kernelEncryptionState) {
		symmetricEncDecKernel();
	}

	return BIOS_LOAD_STATUS_SUCCESS;
}
int Bios::build(BiosBuildParams* buildParams, uint32_t binsize, BiosParams* biosParams) {

	const uint32_t requiredSpace = BLDR_BLOCK_SIZE + MCPX_BLOCK_SIZE + buildParams->krnlSize + buildParams->krnlDataSize + buildParams->inittblSize;
	int result = 0;

	if (buildParams->bfm) {
		binsize = MAX_BIOS_SIZE;
	}

	if (validate_required_space(requiredSpace, &biosParams->romsize) != 0) {
		return BIOS_LOAD_STATUS_FAILED;
	}

	if (binsize < biosParams->romsize) {
		binsize = biosParams->romsize;
	}

	init(NULL, binsize, biosParams);

	// override encryption flags; reverse for building.
	bldr.encryptionState = params.encBldr;
	kernelEncryptionState = (!params.encKrnl && params.keyKrnl == NULL) || (params.encKrnl && params.keyKrnl != NULL);

	if (buildParams->bldrSize > BLDR_BLOCK_SIZE) {
		printf("Error: 2BL is too big\n");
		return BIOS_LOAD_STATUS_FAILED;
	}

	memcpy(bldr.data, buildParams->bldr, buildParams->bldrSize);

	if (!buildParams->nobootparams) {
		// set boot params.
		printf("updating boot params\n");
		bldr.bootParams->krnlSize = buildParams->krnlSize;
		bldr.bootParams->krnlDataSize = buildParams->krnlDataSize;

		if (buildParams->hacksignature)
			bldr.bootParams->signature = 0xFFFFFFFF;
		else
			bldr.bootParams->signature = BOOT_SIGNATURE;

		if (buildParams->hackinittbl)
			bldr.bootParams->inittblSize = 0;
		else
			bldr.bootParams->inittblSize = buildParams->inittblSize;
	}

	result = validateBldrBootParams();
	if (result != 0)
		return result;

	getOffsets2();

	if (buildParams->eepromKey != NULL) {
		printf("updating eeprom key\n");
		memcpy(bldr.keys->eepromKey, buildParams->eepromKey, XB_KEY_SIZE);
	}

	if (buildParams->certKey != NULL) {
		printf("updating cert key\n");
		memcpy(bldr.keys->certKey, buildParams->certKey, XB_KEY_SIZE);
	}

	// to-do add option to compress a kernel image.

	// copy in the compressed kernel image.
	memcpy(krnl, buildParams->krnl, buildParams->krnlSize);

	// copy in the kernel data section. this is data is used by the kernel, stock dashboard's, launch data path for launch xbes etc.
	memcpy(krnlData, buildParams->krnlData, buildParams->krnlDataSize);

	if (data + buildParams->inittblSize >= krnl) {
		printf("Error: init table is too big\n");
		return BIOS_LOAD_STATUS_FAILED;
	}
	memcpy(data, buildParams->inittbl, buildParams->inittblSize);

	// build a bios that boots from media. ( BFM )
	if (buildParams->bfm) {
		printf("adding kernel delay flag\n");
		uint32_t* bootFlags = (uint32_t*)(&initTbl->init_tbl_identifier);
		*bootFlags |= KD_DELAY_FLAG;
	}

	// encrypt the kernel
	if (!kernelEncryptionState)	{
		symmetricEncDecKernel();

		// if the kernel was encrypted with a key file, update the key in the 2BL.
		if (kernelEncryptionState) {
			if (bldr.keys != NULL && params.keyKrnl != NULL) {
				printf("updating kernel key\n");
				memcpy(bldr.keys->krnlKey, params.keyKrnl, XB_KEY_SIZE);
			}
		}
	}
	/*else {
		if (bldr.keys != NULL && params.keyKrnl != NULL) {
			printf("zeroing kernel key\n");
			memset(bldr.keys->krnlKey, 0, XB_KEY_SIZE);
		}
	}*/

	//printState();

	// encrypt 2bl.
	if (!bldr.encryptionState) {

		// get sb key
		uint8_t* sbkey = NULL;
		if (params.keyBldr != NULL) {
			sbkey = params.keyBldr;
		}
		else if (params.mcpx->sbkey != NULL) {
			sbkey = params.mcpx->sbkey;
		}
		else if (buildParams->bfm && bldr.bfmKey != NULL) {
			sbkey = bldr.bfmKey;
		}

		if (sbkey != NULL) {
			symmetricEncDecBldr(sbkey, XB_KEY_SIZE);
		}
	}

	if (buildParams->preldr != NULL && buildParams->preldrSize > 0) {
		if (buildParams->preldrSize > PRELDR_SIZE) {
			printf("Error: preldr is too big\n");
			return BIOS_LOAD_STATUS_FAILED;
		}

		memcpy(preldr.data, buildParams->preldr, buildParams->preldrSize);
	}

	if (size > params.romsize) {
		if (bios_replicate_data(params.romsize, binsize, data, size) != 0) {
			printf("Error: failed to replicate the bios\n");
			return BIOS_LOAD_STATUS_FAILED;
		}
	}

	return BIOS_LOAD_STATUS_SUCCESS;
}

int Bios::init(uint8_t* buff, const uint32_t binsize, const BiosParams* biosParams) {
	if (biosParams != NULL) {
		memcpy(&params, biosParams, sizeof(BiosParams));
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

	bldr.encryptionState = !params.encBldr;
	kernelEncryptionState = !params.encKrnl;

	getOffsets();

	return 0;
}

void Bios::getOffsets() {
	// calculate the initial pointers to BIOS components.

	initTbl = (INIT_TBL*)(data);

	bldr.data = (data + size - BLDR_BLOCK_SIZE - MCPX_BLOCK_SIZE);
	bldr.ldrParams = (BOOT_LDR_PARAM*)(bldr.data);
	bldr.bootParams = (BOOT_PARAMS*)(bldr.data + BLDR_BLOCK_SIZE - sizeof(BOOT_PARAMS));

	preldr.data = (bldr.data + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE);

	romDigest = (bldr.data + BLDR_BLOCK_SIZE - ROM_DIGEST_SIZE - PRELDR_PARAMS_SIZE);
}
void Bios::getOffsets2() {
	// calculate the pointers to the 2bl entry and keys. 2BL needs to be unencrypted for this to work.

	bldr.entryOffset = (bldr.ldrParams->bldrEntryPoint & 0x0000FFFF); /* - BLDR_BASE */

	bldr.entry = (BLDR_ENTRY*)(bldr.data + bldr.entryOffset - sizeof(BLDR_ENTRY));
	if (IN_BOUNDS(bldr.entry, bldr.data, BLDR_BLOCK_SIZE) == false)	{
		bldr.entry = NULL;
	}
	else
	{
		bldr.keysOffset = (bldr.entry->keysPtr & 0x0000FFFF); /* - BLDR_RELOC */

		bldr.keys = (BLDR_KEYS*)(bldr.data + bldr.keysOffset);
		if (IN_BOUNDS(bldr.keys, bldr.data, BLDR_BLOCK_SIZE) == false) {
			bldr.keys = NULL;
		}

		bldr.bfmKey = (bldr.data + bldr.keysOffset - XB_KEY_SIZE);
		if (IN_BOUNDS_BLOCK(bldr.bfmKey, XB_KEY_SIZE, bldr.data, BLDR_BLOCK_SIZE) == false) {
			bldr.bfmKey = NULL;
		}
	}

	// set the kernel pointers.
	krnlData = (bldr.data - bldr.bootParams->krnlDataSize);
	krnl = (krnlData - bldr.bootParams->krnlSize);

	// calculate the available space in the bios.
	availableSpace = params.romsize - BLDR_BLOCK_SIZE - MCPX_BLOCK_SIZE - bldr.bootParams->krnlSize - bldr.bootParams->krnlDataSize;
}

int Bios::validateBldrBootParams() {

	const uint32_t romsize = size;
	const uint32_t krnlSize = bldr.bootParams->krnlSize;
	const uint32_t krnlDataSize = bldr.bootParams->krnlDataSize;
	const uint32_t tblSize = bldr.bootParams->inittblSize;
	
	bldr.krnlSizeValid = krnlSize >= 0 && krnlSize <= romsize;
	bldr.krnlDataSizeValid = krnlDataSize >= 0 && krnlDataSize <= romsize;
	bldr.inittblSizeValid = tblSize >= 0 && tblSize <= romsize;	
	bldr.signatureValid = (BOOT_SIGNATURE == bldr.bootParams->signature);
	bldr.bootParamsValid = bldr.krnlSizeValid && bldr.krnlDataSizeValid && bldr.inittblSizeValid;

	if (!bldr.bootParamsValid) {
		return BIOS_LOAD_STATUS_INVALID_BLDR;
	}

	return 0;
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
	preldr.status = PRELDR_STATUS_NOT_FOUND;
	preldr.params = (PRELDR_PARAMS*)(preldr.data);
	preldr.jmpOffset = preldr.params->jmpOffset + 5; // +5 bytes (opcode + offset)
	preldr.entryPoint = (preldr.data + preldr.jmpOffset);
	
	if (preldr.params->jmpOpcode != 0xE9) {
		return;
	}

	preldr.ptrBlock = (PRELDR_PTR_BLOCK*)(preldr.entryPoint - sizeof(PRELDR_PTR_BLOCK));
	if (IN_BOUNDS(preldr.ptrBlock, preldr.data, PRELDR_BLOCK_SIZE) == false) {
		preldr.ptrBlock = NULL;
		preldr.pubkey = NULL;
		preldr.funcBlock = NULL;
	}
	else {
		// public key pointer
		preldr.pubkey = (XB_PUBLIC_KEY*)(preldr.data + preldr.ptrBlock->pubKeyPtr - PRELDR_REAL_BASE);
		if (IN_BOUNDS(preldr.pubkey, preldr.data, PRELDR_BLOCK_SIZE) == false) {
			preldr.pubkey = NULL;
		}

		// sha1 function block pointer
		preldr.funcBlock = (PRELDR_FUNC_BLOCK*)(preldr.data + preldr.ptrBlock->funcBlockPtr - PRELDR_REAL_BASE);
		if (IN_BOUNDS(preldr.funcBlock, preldr.data, PRELDR_BLOCK_SIZE) == false) {
			preldr.funcBlock = NULL;
		}
	}

	preldr.status = PRELDR_STATUS_FOUND;

	// ignore the preldr if a rev 0 equivalent mcpx was provided.
	if (params.mcpx->version == MCPX_VERSION_MCPX_V1_0 ||
		params.mcpx->version == MCPX_VERSION_MOUSE_V1_0) {
		return;
	}

	// get sbkey
	uint8_t* sbkey = NULL;
	if (params.keyBldr != NULL) {
		sbkey = params.keyBldr;
	}
	else if (params.mcpx->sbkey != NULL) {
		sbkey = params.mcpx->sbkey;
	}
	else {
		return;
	}

	preldrCreateKey(sbkey, preldr.bldrKey);

	// continue if 2BL is encrypted.
	if (!bldr.encryptionState)
		return;

	printf("decrypting 2BL.. (preserving FBL)\n");
	preldrSymmetricEncDecBldr(preldr.bldrKey, SHA1_DIGEST_LEN); /*dont mangle the FBL section of 2BL.*/

	// calculate the offset to the 2BL entry point.
	PRELDR_ENTRY* entry = (PRELDR_ENTRY*)(bldr.data + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE - sizeof(PRELDR_ENTRY));
	if (entry->bldrEntryOffset > BLDR_BLOCK_SIZE) {
		printf("revert preldr decryption.. 2BL entry offset is out of bounds.\n");
		preldrSymmetricEncDecBldr(preldr.bldrKey, SHA1_DIGEST_LEN);/*revert*/
		return;
	}

	// restore 2BL loader params.
	// the first 16 bytes of 2BL were zeroed during the build process.
	// it makes up the 2BL entry point (4 bytes), and part of the 
	// command line argument buffer (12 bytes)
	memset(bldr.data, 0, 16);
	bldr.ldrParams->bldrEntryPoint = BLDR_BASE + entry->bldrEntryOffset;

	if (params.restoreBootParams) {
		// restore 2BL boot params; move the boot params 16 bytes right
		// this will write over the preldr nonce.
		uint8_t* bootParamsPtr = (uint8_t*)bldr.bootParams - PRELDR_NONCE_SIZE;
		for (int i = sizeof(BOOT_PARAMS) - 1; i >= 0; --i) {
			((uint8_t*)bldr.bootParams)[i] = bootParamsPtr[i];
		}
		memset(bootParamsPtr, 0, PRELDR_NONCE_SIZE);
	}
	else {
		// temp fixup of 2BL loader params pointer.
		bldr.bootParams = (BOOT_PARAMS*)((uint8_t*)bldr.bootParams - PRELDR_NONCE_SIZE);
	}

	// FBL load success.
	preldr.status = PRELDR_STATUS_BLDR_DECRYPTED;
}

void Bios::preldrSymmetricEncDecBldr(const uint8_t* key, const uint32_t len) {
	if (bldr.data == NULL)
		return;

	if (!IN_BOUNDS_BLOCK(bldr.data, BLDR_BLOCK_SIZE, data, size)) {
		printf("Error: de/encrypting 2BL. 2BL ptr is out of bounds\n");
		return;
	}

	RC4_CONTEXT context = { 0 };
	rc4_key(&context, key, len);

	// decrypt 2BL up to FBL
	rc4(&context, bldr.data, BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE);

	// seed context; we do this so we dont mangle the FBL decrypting 2BL
	// but still correctly decrypt parts after the preldr block.
	rc4(&context, NULL, PRELDR_BLOCK_SIZE - PRELDR_PARAMS_SIZE);

	// decrypt parts after the preldr block; FBL params, 2BL boot params.
	rc4(&context, bldr.data + BLDR_BLOCK_SIZE - PRELDR_PARAMS_SIZE, PRELDR_PARAMS_SIZE);
}
void Bios::symmetricEncDecBldr(const uint8_t* key, const uint32_t len) {
	
	if (bldr.data == NULL)
		return;

	if (!IN_BOUNDS_BLOCK(bldr.data, BLDR_BLOCK_SIZE, data, size)) {
		printf("Error: de/encrypting 2BL. 2BL ptr is out of bounds\n");
		return;
	}

	printf("%s 2BL\n", bldr.encryptionState ? "decrypting" : "encrypting");
	
	RC4_CONTEXT context = { 0 };
	rc4_key(&context, key, len);
	rc4(&context, bldr.data, BLDR_BLOCK_SIZE);

	bldr.encryptionState = !bldr.encryptionState;
}
void Bios::symmetricEncDecKernel() {

	if (krnl == NULL || bldr.bootParams == NULL)
		return;

	// if key not provided on cli; try get key from bldr block.
	uint8_t* key = params.keyKrnl;
	if (key == NULL) {
		if (bldr.keys == NULL)
			return;
		
		key = bldr.keys->krnlKey;
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

	if (!IN_BOUNDS_BLOCK(krnl, bldr.bootParams->krnlSize, data, size)) {
		printf("Error: de/encrypting kernel. kernel ptr is out of bounds\n");
		return;
	}

	printf("%s kernel\n", kernelEncryptionState ? "decrypting" : "encrypting");
		
	RC4_CONTEXT context = { 0 };
	rc4_key(&context, key, XB_KEY_SIZE);
	rc4(&context, krnl, bldr.bootParams->krnlSize);

	kernelEncryptionState = !kernelEncryptionState;
}

int Bios::decompressKrnl() {

	if (krnl == NULL || bldr.bootParamsValid == false) {
		return 1;
	}

#if 0
	// use decompression function.
	#define LZX_BUFFER_SIZE (1 * 1024 * 1024 / 2) // 512 kb ( 26 blocks )
	decompressedKrnl = (uint8_t*)malloc(bufferSize);
	if (decompressedKrnl == NULL)
		return 1;
	if (lzxDecompress(krnl, bldr.bootParams->krnlSize, &decompressedKrnl, &bufferSize, &decompressedKrnlSize) != 0)
		return 1;
#else
	// use image decompression function.
	decompressedKrnl = lzxDecompressImage(krnl, bldr.bootParams->krnlSize, &decompressedKrnlSize);
	if (decompressedKrnl == NULL)
		return 1;
#endif
		
	return 0;
}
int Bios::preldrDecryptPublicKey() {

	if (preldr.pubkey == NULL)
		return 1;

	// get sbkey
	uint8_t* sbkey = NULL;
	if (params.keyBldr != NULL) {
		sbkey = params.keyBldr;
	}
	else if (params.mcpx->sbkey != NULL) {
		sbkey = params.mcpx->sbkey;
	}
	else {
		return 1;
	}
		
	RC4_CONTEXT context = { 0 };
	rc4_key(&context, sbkey, 12);
	rc4(&context, (uint8_t*)preldr.pubkey, sizeof(XB_PUBLIC_KEY));

	return 0;
}

void Bios::resetValues() {
	bios_init_params(&params);
	bios_init_preldr(&preldr);
	bios_init_bldr(&bldr);

	data = NULL;
	size = 0;

	initTbl = NULL;
	krnl = NULL;
	krnlData = NULL;
	romDigest = NULL;
	availableSpace = -1;

	decompressedKrnl = NULL;
	decompressedKrnlSize = 0;
	kernelEncryptionState = false;
}
void Bios::unload() {
	if (data != NULL) {
		free(data);
	}

	if (decompressedKrnl != NULL) {
		free(decompressedKrnl);
	}

	resetValues();
}

void bios_print_state(Bios* bios) {
	printf("\n2BL entry:\t0x%08X\nsignature:\t", bios->bldr.ldrParams->bldrEntryPoint);
	uprinth((uint8_t*)&bios->bldr.bootParams->signature, 4);
	printf("krnl data size:\t%u bytes\nkrnl size:\t%u bytes\n" \
		"2bl size:\t%u bytes\ninit tbl size:\t%u bytes\n" \
		"avail space:\t%u bytes\n\n",
		bios->bldr.bootParams->krnlDataSize, bios->bldr.bootParams->krnlSize,
		BLDR_BLOCK_SIZE, bios->bldr.bootParams->inittblSize, bios->availableSpace);
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
void bios_init_preldr(Preldr* preldr) {
	preldr->data = NULL;
	preldr->params = NULL;
	preldr->ptrBlock = NULL;
	preldr->funcBlock = NULL;
	preldr->pubkey = NULL;
	preldr->entryPoint = NULL;
	for (int i = 0; i < SHA1_DIGEST_LEN; ++i) {
		preldr->bldrKey[i] = 0;
	}
	preldr->jmpOffset = 0;
	preldr->status = PRELDR_STATUS_ERROR;
}
void bios_init_bldr(Bldr* bldr) {
	bldr->data = NULL;
	bldr->entry = NULL;
	bldr->bfmKey = NULL;
	bldr->keys = NULL;
	bldr->bootParams = NULL;
	bldr->ldrParams = NULL;
	bldr->entryOffset = 0;
	bldr->keysOffset = 0;
	bldr->encryptionState = false;
	bldr->krnlSizeValid = false;
	bldr->krnlDataSizeValid = false;
	bldr->inittblSizeValid = false;
	bldr->signatureValid = false;
	bldr->bootParamsValid = false;
}
void bios_init_params(BiosParams* params) {
	params->romsize = 0;
	params->keyBldr = NULL;
	params->keyKrnl = NULL;
	params->mcpx = NULL;
	params->encBldr = false;
	params->encKrnl = false;
	params->restoreBootParams = true;
}
void bios_init_build_params(BiosBuildParams* params) {
	params->inittbl = NULL;
	params->preldr = NULL;
	params->bldr = NULL;
	params->krnl = NULL;
	params->krnlData = NULL;
	params->eepromKey = NULL;
	params->certKey = NULL;
	params->preldrSize = 0;
	params->bldrSize = 0;
	params->inittblSize = 0;
	params->krnlSize = 0;
	params->krnlDataSize = 0;
	params->bfm = false;
	params->hackinittbl = false;
	params->hacksignature = false;
	params->nobootparams = false;
}
void bios_free_build_params(BiosBuildParams* params) {
	if (params->preldr != NULL) {
		free(params->preldr);
		params->preldr = NULL;
	}
	if (params->bldr != NULL) {
		free(params->bldr);
		params->bldr = NULL;
	}
	if (params->inittbl != NULL) {
		free(params->inittbl);
		params->inittbl = NULL;
	}
	if (params->krnl != NULL) {
		free(params->krnl);
		params->krnl = NULL;
	}
	if (params->krnlData != NULL) {
		free(params->krnlData);
		params->krnlData = NULL;
	}
	if (params->eepromKey != NULL) {
		free(params->eepromKey);
		params->eepromKey = NULL;
	}
	if (params->certKey != NULL) {
		free(params->certKey);
		params->certKey = NULL;
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
