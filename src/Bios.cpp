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
 * along with this program.If not, see < https://www.gnu.org/licenses/>.
*/

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

// std incl
#include <cstdio>
#include <cstdlib>
#include <cstring>

// user incl
#include "Bios.h"
#include "Mcpx.h"
#include "type_defs.h"
#include "bldr.h"
#include "util.h"
#include "file.h"
#include "lzx.h"
#include "rc4.h"
#include "rsa.h"
#include "sha1.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

const char ERROR_MSG_ROMSIZE_INVALID[] = "Error: romsize is less than the total size of the bios.\n";

void Bios::getOffsets2(const UINT krnlSize, const UINT krnlDataSize)
{
	// get the pointers to the bldr entry and keys. Bldr needs to be unencrypted for this to work.
		
	bldr.entryPointOffset = bldr.ldrParams->bldrEntryPoint - BLDR_BASE;
	bldr.entry = (BLDR_ENTRY*)(bldr.data + bldr.entryPointOffset - sizeof(BLDR_ENTRY));
	if (IN_BOUNDS(bldr.entry, bldr.data, BLDR_BLOCK_SIZE) == false)
	{
		bldr.entry = NULL;
	}

	if (bldr.entry != NULL)
	{
		bldr.keysOffset = bldr.entry->keysPtr - BLDR_RELOC;
		
		// the bfm key is just before the keys ptr so we will include it in our struct as well.
		bldr.keys = (BLDR_KEYS*)(bldr.data + bldr.keysOffset - KEY_SIZE); 
		if (IN_BOUNDS(bldr.keys, bldr.data, BLDR_BLOCK_SIZE) == false)
		{
			bldr.keys = NULL;
		}
	}

	// set the kernel pointers.
	krnlData = (bldr.data - krnlDataSize);
	krnl = (krnlData - krnlSize);

	// calculate the available space in the bios.
	totalSpaceAvailable = params.romsize - BLDR_BLOCK_SIZE - MCPX_BLOCK_SIZE;
	availableSpace = totalSpaceAvailable - krnlSize - krnlDataSize;
}
void Bios::getOffsets()
{
	bldr.data = (data + size - BLDR_BLOCK_SIZE - MCPX_BLOCK_SIZE);
	bldr.ldrParams = (BOOT_LDR_PARAM*)(bldr.data);
	bldr.bootParams = (BOOT_PARAMS*)(bldr.data + BLDR_BLOCK_SIZE - sizeof(BOOT_PARAMS));

	initTbl = (INIT_TBL*)(data);
	dataTbl = (ROM_DATA_TBL*)(data + initTbl->data_tbl_offset);
}

void Bios::checkForPreldr()
{	
	preldr.data = (bldr.data + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE);
	preldr.params = (PRELDR_PARAMS*)(preldr.data);
	preldr.jmpInstr = (preldr.params->jmpInstr & 0xFF);
	preldr.jmpOffset = (preldr.params->jmpInstr >> 8);
	preldr.romDigest = (preldr.data + PRELDR_BLOCK_SIZE - PRELDR_ROM_DIGEST_SIZE);
	preldr.nonce = (preldr.data + PRELDR_BLOCK_SIZE - KEY_SIZE);
	preldr.bldrEntryOffset = (UINT*)(bldr.data + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE - 8);
	preldr.status = PRELDR_STATUS_OK;
	const UCHAR* sbkey = NULL;

	// check for the jmp instruction. this is patched in during the build process.
	if (preldr.jmpInstr != 0xE9)
	{
		preldr.status = PRELDR_STATUS_NOT_FOUND;
		return;
	}

	printf("\npreldr found\n");

	preldr.entry = (PRELDR_ENTRY*)(preldr.data + preldr.jmpOffset + 0x5 - sizeof(PRELDR_ENTRY));
	if (IN_BOUNDS(preldr.entry, preldr.data, PRELDR_BLOCK_SIZE) == false)
	{
		printf("Error: preldr entry is out of bounds\n");
		preldr.entry = NULL;
		preldr.status = PRELDR_STATUS_ERROR;
		return;
	}

	preldr.pubkey = (PUBLIC_KEY*)(preldr.data + preldr.entry->pubKeyPtr - PRELDR_BASE);
	if (IN_BOUNDS(preldr.pubkey, preldr.data, PRELDR_BLOCK_SIZE) == false)
	{
		printf("Error: preldr public key is out of bounds\n");
		preldr.pubkey = NULL;
		preldr.status = PRELDR_STATUS_ERROR;
		return;
	}

	// continue only if the bldr is encrypted.
	if (!bldr.encryptionState)
	{
		preldr.status = PRELDR_STATUS_INVALID_BLDR;
		return;
	}

	// get sbkey
	if (params.keyBldr != NULL)
	{
		sbkey = params.keyBldr;
	}
	else if (params.mcpx->data != NULL)
	{
		sbkey = params.mcpx->sbkey;
	}

	// check sbkey; cannot build the preldr key without it. sbkey must point to a valid 16 byte key.
	if (sbkey == NULL)
	{
		preldr.status = PRELDR_STATUS_ERROR;
		return;
	}

	// verify the preldr public key; this needs to be moved somewhere else.. its not really needed here.
#if 0 
	printf("decrypting preldr public key..\n");
	symmetricEncDec((UCHAR*)preldr.pubkey, sizeof(PUBLIC_KEY), params.mcpx.sbkey, 12);
	PUBLIC_KEY* pubkey = rsaVerifyPublicKey((UCHAR*)preldr.pubkey);
	if (pubkey == NULL)
	{
		printf("Error: preldr public key is invalid\n");
		preldr.status = PRELDR_STATUS_ERROR;
		return;
	}
	printf("preldr public key is valid\n");
#endif

	// create the preldr key
	SHA1Context sha;
	SHA1Reset(&sha);
	SHA1Input(&sha, params.mcpx->sbkey, KEY_SIZE);
	SHA1Input(&sha, preldr.nonce, KEY_SIZE);
	for (int i = 0; i < KEY_SIZE; i++)
		preldr.bldrKey[i] = params.mcpx->sbkey[i] ^ 0x5C;
	SHA1Input(&sha, preldr.bldrKey, KEY_SIZE);
	SHA1Result(&sha, preldr.bldrKey);

	// decrypt the bldr
	symmetricEncDecBldr(preldr.bldrKey, SHA1_DIGEST_LEN);
		
	if (bldr.data + *preldr.bldrEntryOffset < bldr.data || bldr.data + *preldr.bldrEntryOffset > bldr.data + BLDR_BLOCK_SIZE)
	{
		printf("Error: bldr entry point offset is out of bounds\n");
		preldr.status = PRELDR_STATUS_ERROR;
		return;
	}

	// fixup bldr entry point, this is zeroed during the build process. preldr uses a rel jmp instead.	
	bldr.ldrParams->bldrEntryPoint = BLDR_BASE + *preldr.bldrEntryOffset;
	
	// fixup the bldr boot params.
#if 0 
	// via ptr.
	bldr.bootParams = (BOOT_PARAMS*)((UCHAR*)bldr.bootParams - 16);
#else 
	// via copy.
	BOOT_PARAMS bootParamsCpy = { 0 };
	memcpy(&bootParamsCpy, ((UCHAR*)bldr.bootParams - 16), sizeof(BOOT_PARAMS));
	memcpy(bldr.bootParams, &bootParamsCpy, sizeof(BOOT_PARAMS));
#endif
}

int Bios::load(UCHAR* buff, const UINT buffSize, const BiosParams biosParams)
{
	int result = 0;

	memcpy(&params, &biosParams, sizeof(BiosParams));

	data = buff;
	size = buffSize;
	buffersize = buffSize;

	// check size
	if (checkBiosSize(size) != 0)
	{
		printf("Error: Invalid bios size, expecting 256Kb, 512Kb or 1Mb bios file.\n");
		return BIOS_LOAD_STATUS_FAILED;
	}

	// set encryption states based on bios parameters.
	bldr.encryptionState = !params.encBldr;
	kernelEncryptionState = !params.encKrnl;

	getOffsets();
	
	checkForPreldr();

	// decrypt the 2bl if it is encrypted and a preldr was not found. (mcpx v1.0)
	if (bldr.encryptionState && preldr.status == PRELDR_STATUS_NOT_FOUND)
	{
		if (params.keyBldr != NULL)
		{
			symmetricEncDecBldr(params.keyBldr, KEY_SIZE);
		}
		else if (params.mcpx->data != NULL)
		{
			symmetricEncDecBldr(params.mcpx->sbkey, KEY_SIZE);
		}
	}

	// check if the boot params are valid. All offsets are based on the boot params.
	result = validateBldr();
	if (result != 0)
	{
		return result;
	}

	// do stuff that depends on the boot params being decrypted and valid.

	getOffsets2(bldr.bootParams->krnlSize, bldr.bootParams->krnlDataSize);

	if (isKernelEncrypted())
	{
		symmetricEncDecKernel();
	}

	return BIOS_LOAD_STATUS_SUCCESS;
}

int Bios::loadFromFile(const char* filename, BiosParams biosParams)
{
	UINT biosSize = 0;
	UCHAR* bios = readFile(filename, &biosSize);
	if (bios == NULL)
		return BIOS_LOAD_STATUS_FAILED;
		
	return load(bios, biosSize, biosParams);
}
int Bios::loadFromData(const UCHAR* bios, const UINT biosSize, BiosParams biosParams)
{
	UCHAR* biosData = (UCHAR*)malloc(biosSize);
	if (biosData == NULL)
		return BIOS_LOAD_STATUS_FAILED;

	memcpy(biosData, bios, biosSize);
	
	return load(biosData, biosSize, biosParams);
}

int Bios::saveBiosToFile(const char* filename)
{
	return writeFileF(filename, data, size, "bios");
}
int Bios::saveBldrBlockToFile(const char* filename)
{
	return saveBldrToFile(filename, BLDR_BLOCK_SIZE);
}
int Bios::saveBldrToFile(const char* filename, const UINT bldrSize)
{
	return writeFileF(filename, bldr.data, bldrSize, "bldr");
}
int Bios::saveKernelToFile(const char* filename)
{
	return writeFileF(filename, krnl, bldr.bootParams->krnlSize, "compressed kernel");
}
int Bios::saveKernelDataToFile(const char* filename)
{
	return writeFileF(filename, krnlData, bldr.bootParams->krnlDataSize, "kernel section data");
}
int Bios::saveKernelImgToFile(const char* filename)
{
	UINT krnlImgSize = bldr.bootParams->krnlSize + bldr.bootParams->krnlDataSize;
	return writeFileF(filename, krnl, krnlImgSize, "kernel image");
}
int Bios::saveInitTblToFile(const char* filename)
{
	return writeFileF(filename, data, bldr.bootParams->inittblSize, "init table");
}

void Bios::symmetricEncDecBldr(const UCHAR* key, const UINT keyLen)
{
	if (bldr.data == NULL)
		return;

	printf("%s 2BL..\n", bldr.encryptionState ? "decrypting" : "encrypting");

	// verify the bldr ptr is within the bounds or this will cause an access violation.
	if (bldr.data < data || (bldr.data + BLDR_BLOCK_SIZE) > (data + size))
	{
		printf("Error: 2BL ptr is out of bounds\n");
		return;
	}

	symmetricEncDec(bldr.data, BLDR_BLOCK_SIZE, key, keyLen);

	bldr.encryptionState = !bldr.encryptionState;
}
void Bios::symmetricEncDecKernel()
{
	if (krnl == NULL || bldr.bootParams == NULL)
		return;

	UCHAR* key = params.keyKrnl;
	if (key == NULL)
	{
		// key not provided on cli; try get key from bldr block.

		if (bldr.keys == NULL)
			return;

		key = bldr.keys->krnlKey;
		if (key == NULL)
			return;

		UCHAR tmp[KEY_SIZE] = { 0 };
		if (memcmp(key, tmp, KEY_SIZE) == 0)
			return; // kernel key is all 0's

		memset(tmp, 0xFF, KEY_SIZE);
		if (memcmp(key, tmp, KEY_SIZE) == 0)
			return; // kernel key is all FF's

		// key is valid; use it.
	}

	// en/decrypt the kernel.
	printf("%s kernel.. (using key %s)\n", (kernelEncryptionState ? "decrypting" : "encrypting"), (params.keyKrnl != NULL ? "from CLI" : "found in 2BL"));

	// verify the krnl is within the bounds or this will cause an access violation.
	if (krnl < data || (krnl + bldr.bootParams->krnlSize) > (data + size))
	{
		printf("Error: krnl ptr is out of bounds\n");
		return;
	}

	symmetricEncDec(krnl, bldr.bootParams->krnlSize, key, KEY_SIZE);

	kernelEncryptionState = !kernelEncryptionState;
}

int Bios::validateBldr()
{
	const UINT krnlSize = bldr.bootParams->krnlSize;
	const UINT krnlDataSize = bldr.bootParams->krnlDataSize;
	const UINT tblSize = bldr.bootParams->inittblSize;

	const bool isKrnlSizeValid = krnlSize >= 0 && krnlSize <= size;
	const bool isDataSizeValid = krnlDataSize >= 0 && krnlDataSize <= size;
	const bool istblSizeValid = tblSize >= 0 && tblSize <= size;
	
	bldr.signatureValid = (BOOT_SIGNATURE == bldr.bootParams->signature);
	bldr.bootParamsValid = isKrnlSizeValid && isDataSizeValid && istblSizeValid;

	if (!bldr.bootParamsValid)
	{
		return BIOS_LOAD_STATUS_INVALID_BLDR;
	}

	// verify the rom size is big enough for the bios.
	if (params.romsize < MCPX_BLOCK_SIZE + BLDR_BLOCK_SIZE + krnlSize + krnlDataSize + tblSize)
	{
		printf(ERROR_MSG_ROMSIZE_INVALID);
		return BIOS_LOAD_STATUS_FAILED_INVALID_ROMSIZE;
	}

	return 0;
}

int Bios::decompressKrnl()
{
	if (decompressedKrnl != NULL)
		return 1;

	printf("decompressing kernel... (%d bytes)\n", bldr.bootParams->krnlSize);

#if 0
	// set initial value, decompress() will reallocate if needed. 
	UINT bufferSize = 1 * 1024 * 1024 / 2; // 512 kb ( 26 blocks )
	
	decompressedKrnl = (UCHAR*)xb_alloc(bufferSize); // allocate initial buffer for the decompressed kernel. buff will be reallocated if needed.
	if (decompressedKrnl == NULL)
	{
		return ERROR_OUT_OF_MEMORY;
	}
		
	if (decompress(krnl, bldr.bootParams->krnlSize, decompressedKrnl, bufferSize, &decompressedKrnlSize) != 0)
	{
		return 1;
	}
#else
	decompressedKrnl = decompressImg(krnl, bldr.bootParams->krnlSize, &decompressedKrnlSize);
	if (decompressedKrnl == NULL)
	{
		return 1;
	}
#endif
			
	return 0;
}

int Bios::build(UCHAR* in_preldr, UINT in_preldrSize,
	UCHAR* in_2bl, UINT in_2blSize,
	UCHAR* in_inittbl, UINT in_inittblSize,
	UCHAR* in_krnl, UINT in_krnlSize,
	UCHAR* in_krnlData, UINT in_krnlDataSize,
	UINT binsize, bool bfm, BiosParams biosParams)
{
	if (data != NULL)
	{
		return BIOS_LOAD_STATUS_FAILED;
	}

	memcpy(&params, &biosParams, sizeof(BiosParams));

	// verify the rom size is big enough for the bios.
	if (params.romsize < BLDR_BLOCK_SIZE + MCPX_BLOCK_SIZE + in_krnlSize + in_krnlDataSize + in_inittblSize)
	{
		printf(ERROR_MSG_ROMSIZE_INVALID);
		return BIOS_LOAD_STATUS_FAILED_INVALID_ROMSIZE;
	}

	if (binsize < params.romsize)
	{
		printf("Error: binsize is less than the romsize\n");
		return BIOS_LOAD_STATUS_FAILED_INVALID_BINSIZE;
	}

	data = (UCHAR*)malloc(binsize);
	if (data == NULL)
	{
		return BIOS_LOAD_STATUS_FAILED;
	}

	size = params.romsize;
	buffersize = binsize;

	// set encryption states.
	bldr.encryptionState = params.encBldr;
	// is flag set = not encrypted ; key provided = encrypted ; flag set + key provided = not encrypted
	kernelEncryptionState = (!params.encKrnl && params.keyKrnl == NULL) || (params.encKrnl && params.keyKrnl != NULL);

	getOffsets();

	// init table
	memcpy(data, in_inittbl, in_inittblSize);
	
	// 2bl
	memcpy(bldr.data, in_2bl, in_2blSize);

	getOffsets2(in_krnlSize, in_krnlDataSize);

	// compress kernel image.
	memcpy(krnl, in_krnl, in_krnlSize);
	
	// uncompressed kernel section data
	memcpy(krnlData, in_krnlData, in_krnlDataSize);

	// fixup the boot params.
	bldr.bootParams->krnlSize = in_krnlSize;
	bldr.bootParams->krnlDataSize = in_krnlDataSize;
	bldr.bootParams->inittblSize = in_inittblSize;

	printf("\nBLDR Entry:\t0x%08X\n\nSignature:\t", bldr.ldrParams->bldrEntryPoint);
	printData((UCHAR*)&bldr.bootParams->signature, 4);
	printf("Krnl data size:\t%ld bytes\n" \
		"Krnl size:\t%ld bytes\n" \
		"2BL size:\t%ld bytes\n" \
		"Init tbl size:\t%ld bytes\n" \
		"Avail space:\t%ld bytes\n" \
		"rom size:\t%ld Kb\n" \
		"bin size:\t%ld Kb\n",
		bldr.bootParams->krnlDataSize, bldr.bootParams->krnlSize,
		BLDR_BLOCK_SIZE, bldr.bootParams->inittblSize, availableSpace,
		params.romsize / 1024, binsize / 1024);

	// encrypt the kernel
	if (!isKernelEncrypted())
	{
		symmetricEncDecKernel();
		if (isKernelEncrypted())
		{
			// if the kernel was encrypted with a key file, update the key in the 2BL.
			if (bldr.keys != NULL && params.keyKrnl != NULL)
			{
				memcpy(bldr.keys->krnlKey, params.keyKrnl, KEY_SIZE);
				printf("Updated kernel key in 2BL\n");
			}
		}
	}

	// build a bios that boots from media. ( BFM )
	if (bfm)
	{
		convertToBootFromMedia();

		// force the bfm bios to be 1Mb in size.
		binsize = 1 * 1024 * 1024; // 1Mb
	}

	// encrypt if a key file or mcpx rom is provided and it is not already encrypted.
	if (!bldr.encryptionState)
	{
		if (params.keyBldr != NULL)
		{
			symmetricEncDecBldr(params.keyBldr, KEY_SIZE);
		}
		else if (params.mcpx->sbkey != NULL)
		{
			symmetricEncDecBldr(params.mcpx->sbkey, KEY_SIZE);
		}
	}

	// copy in the preldr if it was provided.
	if (in_preldr != NULL && in_preldrSize > 0)
	{
		preldr.data = (bldr.data + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE);
		if (in_preldrSize < PRELDR_BLOCK_SIZE)
		{
			memcpy(preldr.data, in_preldr, in_preldrSize);
		}
	}

	if (binsize > params.romsize)
	{
		if (replicateBios(binsize) != 0)
		{
			printf("Error: failed to replicate the bios\n");
			return BIOS_LOAD_STATUS_FAILED;
		}
	}

	return BIOS_LOAD_STATUS_SUCCESS;
}

int Bios::convertToBootFromMedia()
{
	// convert the bios to boot from media.
	// KD_DELAY_FLAG is or'd with with the value at 120 (0x78) (4 bytes) in the init tbl.

	// BFM bios needs to be encrypted with the dev/bfm 2BL key.
	// BFM bios needs to be 1Mb in size. - and will faild to load otherwise.

	printf("Adding KD Delay flag ( BFM )\n");

	UINT* bootFlags = (UINT*)(&initTbl->init_tbl_identifier);

	*bootFlags |= KD_DELAY_FLAG;

	if (params.keyBldr == NULL)
	{
		if (bldr.keys == NULL)
			return 1;

		symmetricEncDecBldr(bldr.keys->bfmKey, KEY_SIZE);
	}

	return 0;
}

int Bios::replicateBios(UINT binsize)
{
	// replicate the bios file base on binsize

	UINT offset = 0;
	UINT new_size = 0;
	UINT romsize = params.romsize;

	if (binsize < romsize || buffersize < binsize)
		return 1;

	offset = romsize;
	while (offset < binsize)
	{
		new_size = offset * 2;

		if (offset < new_size && binsize >= new_size)
		{
			printf("Replicating to 0x%08X-0x%08X (%d bytes)\n", offset, new_size, offset);
			memcpy((data + offset), data, offset);
		}

		offset = new_size;
	}

	size = new_size;

	return 0;
}

int checkBiosSize(const UINT size)
{
	switch (size)
	{
	case 0x40000U:
	case 0x80000U:
	case 0x100000U:
		return 0;
	}
	return 1;
}
