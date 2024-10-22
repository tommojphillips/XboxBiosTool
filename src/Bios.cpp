// Bios.cpp: defines functions for handling bios data
 
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
#include "cstring"

// user incl
#include "Bios.h"
#include "XbTool.h"
#include "XcodeInterp.h"

#include "file.h"
#include "rc4.h"
#include "util.h"
#include "xbmem.h"

#include "lzx.h"
#include "sha1.h"

extern Parameters& params;

void Bios::reset()
{
	// reset pointers, offsets

	// sizes
	_size = 0;
	_totalSpaceAvailable = 0;
	_availableSpace = 0;
	_decompressedKrnlSize = 0;

	// flags
	_isBldrEncrypted = false;
	_isKernelEncrypted = false;
	_isBootParamsValid = false;
	_isBldrSignatureValid = false;

	// pointers in to the bios data
	bldr = NULL;
	bootParams = NULL;
	ldrParams = NULL;

	initTbl = NULL;
	dataTbl = NULL;

	krnl = NULL;
	krnlData = NULL;

	bldrEntry = NULL;
	bldrKeys = NULL;
}
void Bios::deconstruct()
{
	// free the allocated memory
	if (_bios != NULL)
	{
		xb_free(_bios);
		_bios = NULL;
	}

	if (decompressedKrnl != NULL)
	{
		xb_free(decompressedKrnl);
		decompressedKrnl = NULL;
	}

	// reset the pointers, offsets
	reset();
};

void Bios::getOffsets2(const UINT krnlSize, const UINT krnlDataSize)
{
	// get the pointers to the bldr entry and keys. Bldr needs to be unencrypted for this to work.
		
	UINT entryPointOffset = ldrParams->bldrEntryPoint - BLDR_BASE;
	bldrEntry = (BLDR_ENTRY*)(bldr + entryPointOffset - sizeof(BLDR_ENTRY));
	if ((UCHAR*)bldrEntry < bldr || (UCHAR*)bldrEntry + sizeof(BLDR_ENTRY) > bldr + BLDR_BLOCK_SIZE)
	{
		// bldr entry point ptr is out of bounds
		bldrEntry = NULL;
	}

	if (bldrEntry != NULL)
	{
		UINT keysOffset = bldrEntry->keysPtr - BLDR_RELOC;
		bldrKeys = (BLDR_KEYS*)(bldr + keysOffset - KEY_SIZE); // bfm key is 16 bytes before the keys ptr.
		if ((UCHAR*)bldrKeys < bldr || (UCHAR*)bldrKeys + sizeof(BLDR_KEYS) > bldr + BLDR_BLOCK_SIZE)
		{
			// bldr keys ptr is out of bounds
			bldrKeys = NULL;
		}
	}

	// set the kernel pointers.
	krnlData = (bldr - krnlDataSize);
	krnl = (krnlData - krnlSize);

	// calculate the total space available in the rom.
	_totalSpaceAvailable = params.romsize - BLDR_BLOCK_SIZE - MCPX_BLOCK_SIZE;

	// calculate the available space in the bios.
	_availableSpace = _totalSpaceAvailable - krnlSize - krnlDataSize;
}
void Bios::getOffsets()
{		
	initTbl = (INIT_TBL*)(_bios);												// init tbl is the first 128 bytes of the bios
	dataTbl = (ROM_DATA_TBL*)(_bios + initTbl->data_tbl_offset);				// rom data tbl			
	
	bldr = (_bios + _size - BLDR_BLOCK_SIZE - MCPX_BLOCK_SIZE);					// bldr is at the end of the bios		
	ldrParams = (BOOT_LDR_PARAM*)(bldr);										// 2bl boot loader params are at the start of the bldr block	
	bootParams = (BOOT_PARAMS*)(bldr + BLDR_BLOCK_SIZE - sizeof(BOOT_PARAMS));	// boot params are at the end of the bldr block
}

int Bios::checkForPreldr()
{
	// find preldr entry params, rsa1 header.
	
	const UINT PRELDR_BASE = (0xFFFFFFFF - MCPX_BLOCK_SIZE - PRELDR_BLOCK_SIZE + 1);

	UCHAR* preldr = bldr + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE;

	PRELDR_PARAMS* preldrParams = (PRELDR_PARAMS*)preldr;
	
	// check for the jmp instruction. this is patched in during the build process.
	
	if ((preldrParams->jmpInstr & 0xFF) != 0xE9)
	{
		return PRELDR_STATUS_NOT_FOUND; // preldr was not found in this bios.
	}

	print("\npreldr found\n");

	if (params.mcpx.data == NULL)
	{
		error("Error: MCPX v1.1 rom is required to decrypt the preldr ( -mcpx )\n");
		return PRELDR_STATUS_ERROR;
	}

	if (params.mcpx.version != MCPX_ROM::MCPX_V1_1)
	{
		// user has provided the incorrect mcpx rom. let 'em know.
		error("Error: decrypting the bldr. MCPX v1.1 was expected but v1.0 was provided.\n");
		return PRELDR_STATUS_ERROR;
	}

	PRELDR_ENTRY* preldrEntry = (PRELDR_ENTRY*)(preldr + (preldrParams->jmpInstr >> 8) + 0x5 - sizeof(PRELDR_ENTRY)); 

	UCHAR* sbKey = (UCHAR*)(params.mcpx.data + MCPX_BLOCK_SIZE - 0x64);
	
#if 0
	// decrypt the preldr pubkey
	PUBLIC_KEY pubkey = { 0 };
	UINT pubKeyOffset = preldrEntry->encryptedPubKeyPtr - PRELDR_BASE;

	print("Decrypting preldr pubkey..\n");

	// create cpy of the encrypted pub key
	xb_cpy(&pubkey, (preldr + pubKeyOffset), sizeof(PUBLIC_KEY));

	// decrypt the pub key with the first 12 bytes of the sb key
	symmetricEncDec((UCHAR*)&pubkey, sizeof(PUBLIC_KEY), sb_key, 12);

	// verify the pub key
	if (verifyPubKey(&pubkey) != 0)
	{
		error("Error: Public key header is invalid\n");
		return 1;
	}
	// public key is valid
	print("pubkey is valid\n");
#endif

	// create the bldr key
	print("building key..\n");

	UCHAR* last16Bytes = (bldr + BLDR_BLOCK_SIZE - KEY_SIZE);

	UCHAR encKey[SHA1_DIGEST_LEN] = { 0 };
	SHA1Context sha;

	SHA1Reset(&sha);
	SHA1Input(&sha, sbKey, KEY_SIZE);
	SHA1Input(&sha, last16Bytes, KEY_SIZE);
	for (int i = 0; i < KEY_SIZE; i++)
		encKey[i] = sbKey[i] ^ 0x5C;	
	SHA1Input(&sha, encKey, KEY_SIZE);	
	SHA1Result(&sha, encKey);

	print("bldr key: ");
	printData(encKey, SHA1_DIGEST_LEN);

	// decrypt the bldr
	_isBldrEncrypted = true; // force 
	symmetricEncDecBldr(encKey, SHA1_DIGEST_LEN);

	UINT entryOffset = *(UINT*)(bldr + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE - 8);	// should equal 01bc

	// fix up the ptr to the boot params, with a preldr it's -16 bytes where it is normally.
	bootParams = (BOOT_PARAMS*)((UCHAR*)bootParams - 16);
	
	// fixup bldr entry point, this is zeroed during the build process to prevent a resolved rc4 key attack. preldr uses a rel jmp instead.
	ldrParams->bldrEntryPoint = BLDR_BASE + entryOffset;
	
	return PRELDR_STATUS_FOUND;
}

int Bios::load(UCHAR* data, const UINT size)
{
	if (_bios != NULL)
	{
		error("Error: Data already loaded\n");
		return BIOS_LOAD_STATUS_FAILED;
	}

	_bios = data;
	_size = size;
	
	// set encryption states based on user input.
	_isBldrEncrypted = params.encBldr;
	_isKernelEncrypted = params.encKrnl;

	getOffsets();

	// before decrypting the bldr, check if there is a preldr at the end of the bldr block. (preldr is mostly unencrypted)
	// checkForPreldr() will decrypt the bldr if a preldr is found, patch the boot params ptr so it points to the correct location.
	if (checkForPreldr() == PRELDR_STATUS_NOT_FOUND)
	{
		// only decrypt the bldr if the user has provided a key file or a mcpx rom and the bldr is encrypted.
		if (_isBldrEncrypted)
		{
			if (params.keyBldr != NULL) // bldr key is provided; use it.
			{
				symmetricEncDecBldr(params.keyBldr, KEY_SIZE);
			}
			else if (params.mcpxFile != NULL)
			{
				switch (params.mcpx.version)
				{
				case MCPX_ROM::MCPX_V1_0:
					symmetricEncDecBldr(params.mcpx.data+0x1A5, KEY_SIZE);
					break;
				case MCPX_ROM::MCPX_V1_1:
					error("Error: decrypting the bldr. MCPX v1.0 was expected but v1.1 was provided.\n");
					return BIOS_LOAD_STATUS_FAILED;
				}
			}
		}
	}

	// check if the boot params are valid. All offsets are based on the boot params.
	if (!SUCCESS(validateBldr()))
	{
		return BIOS_LOAD_STATUS_INVALID_BLDR;
	}

	// do stuff that depends on the boot params being decrypted and valid.

	// check size is big enough for the bios.
	if (_size < bootParams->inittblSize + BLDR_BLOCK_SIZE + MCPX_BLOCK_SIZE + bootParams->krnlSize + bootParams->krnlDataSize)
	{
		error("Error: romsize is less than the size of the bios\n");
		return BIOS_LOAD_STATUS_FAILED;
	}

	getOffsets2(bootParams->krnlSize, bootParams->krnlDataSize);

	// decrypt the kernel if it is encrypted.
	if (_isKernelEncrypted) 
	{
		symmetricEncDecKernel();
	}

	return BIOS_LOAD_STATUS_SUCCESS;
}

int Bios::loadFromFile(const char* path)
{
	UINT size = 0;
	UCHAR* biosData = readFile(path, &size);
	if (biosData == NULL)
	{
		error("Error: bios data is NULL\n");
		return BIOS_LOAD_STATUS_FAILED;
	}

	return load(biosData, size);
}
int Bios::loadFromData(const UCHAR* data, const UINT size)
{
	if (data == NULL)
	{
		error("Error: bios data is NULL\n");
		return BIOS_LOAD_STATUS_FAILED;
	}

	UCHAR* biosData = (UCHAR*)xb_alloc(size);
	if (biosData == NULL)
	{
		return BIOS_LOAD_STATUS_FAILED;
	}
	xb_cpy(biosData, data, size);

	return load(biosData, size);
}

int Bios::saveBiosToFile(const char* path)
{
	if (_bios == NULL)
	{
		error("Error: No data loaded\n");
		return 1;
	}

	if (params.binsize < _size)
	{
		error("Error: Desired bin size is less than the total size of the bios\n");
		return 1;
	}

	// replicate up to 1Mb

	UCHAR* biosData = (UCHAR*)xb_alloc(params.binsize);

	xb_cpy(biosData, _bios, _size); // copy the bios data

	// replicate to 512 KB
	if (_size < 0x80000 && params.binsize >= 0x80000)
	{
		xb_cpy(biosData + 0x40000, biosData, _size);
	}

	// replicate to 1 MB
	if (_size < 0x100000 && params.binsize >= 0x100000)
	{
		xb_cpy(biosData + 0x80000, biosData, 0x80000);
	}

	int result = writeFile(path, biosData, params.binsize);

	xb_free(biosData);

	return result;
}
int Bios::saveBldrBlockToFile(const char* path)
{
	// extract the bldr block
	// bldr block consists of:
	// the ldr params at the start.
	// the bldr after the ldr params.
	// and boot params at the very end of the block.
	// their is more than likely some 0 space between the end of the bldr and the boot params.

	return saveBldrToFile(path, BLDR_BLOCK_SIZE);
}
int Bios::saveBldrToFile(const char* path, const UINT size)
{
	// extract the bldr

	print("Writing bldr to %s (%d bytes)\n", path, size);
	return writeFile(path, bldr, size);
}
int Bios::saveKernelToFile(const char* path)
{
	// extract the compressed krnl

	print("Writing krnl to %s (%d bytes)\n", path, bootParams->krnlSize);
	return writeFile(path, krnl, bootParams->krnlSize);
}
int Bios::saveKernelDataToFile(const char* path)
{
	// extract the uncompressed krnl data

	print("Writing krnl data to %s (%d bytes)\n", path, bootParams->krnlDataSize);
	return writeFile(path, krnlData, bootParams->krnlDataSize);
}
int Bios::saveKernelImgToFile(const char* path)
{
	// extract the compressed kernel + uncompressed krnl data

	print("Writing krnl img (compressed krnl+data) to %s (%d bytes)\n", path, bootParams->krnlSize+bootParams->krnlDataSize);
	return writeFile(path, krnl, bootParams->krnlSize+bootParams->krnlDataSize); // krnl data is at the end of the kernel
}
int Bios::saveInitTblToFile(const char* path)
{
	// extract the init table

	print("Writing init tbl to %s (%d bytes)\n", path, bootParams->inittblSize);
	return writeFile(path, _bios, bootParams->inittblSize);
}

void Bios::symmetricEncDecBldr(const UCHAR* key, const UINT keyLen)
{
	if (bldr == NULL)
		return;

	print("%s Bldr..\n", _isBldrEncrypted ? "Decrypting" : "Encrypting");

	// verify the bldr ptr is within the bounds or this will cause an access violation.
	if (bldr < _bios || (bldr + BLDR_BLOCK_SIZE) > (_bios + _size))
	{
		error("Error: bldr ptr is out of bounds\n");
		return;
	}

	symmetricEncDec(bldr, BLDR_BLOCK_SIZE, key, keyLen);

	_isBldrEncrypted = !_isBldrEncrypted;
}
void Bios::symmetricEncDecKernel()
{
	if (krnl == NULL)
		return;

	UCHAR* key = params.keyKrnl;
	if (key == NULL)
	{
		// key not provided on cli; try get key from bldr block.

		if (bldrKeys == NULL)
			return;

		key = bldrKeys->krnlKey;
		if (key == NULL)
			return;

		UCHAR tmp[KEY_SIZE] = { 0 };
		if (memcmp(key, tmp, KEY_SIZE) == 0)
			return; // kernel key is all 0's

		xb_set(tmp, 0xFF, KEY_SIZE);
		if (memcmp(key, tmp, KEY_SIZE) == 0)
			return; // kernel key is all FF's

	}

	// en/decrypt the kernel.

	print("krnl key: ");
	printData(key, KEY_SIZE);

	print("%s Kernel..\n", _isKernelEncrypted ? "Decrypting" : "Encrypting");

	// verify the krnl is within the bounds or this will cause an access violation.
	if (krnl < _bios || (krnl + bootParams->krnlSize) >(_bios + _size))
	{
		error("Error: krnl ptr is out of bounds\n");
		return;
	}

	symmetricEncDec(krnl, bootParams->krnlSize, key, KEY_SIZE);

	_isKernelEncrypted = !_isKernelEncrypted;
}

void Bios::printBldrInfo()
{
	print("\nBldr:\n");

	if (_isBootParamsValid)
	{
		print("entry point:\t\t0x%08X\n", ldrParams->bldrEntryPoint);

		if (bldrEntry != NULL)
		{
			print("bfm entry point:\t0x%08X\n", bldrEntry->bfmEntryPoint);
		}
	}

	print("signature:\t\t");
	printData(bootParams->signature, 4);
	
	UINT krnlSize = bootParams->krnlSize;
	UINT krnlDataSize = bootParams->krnlDataSize;

	bool isKrnlSizeValid = krnlSize >= 0 && krnlSize <= params.romsize;
	bool isKrnlDataSizeValid = krnlDataSize >= 0 && krnlDataSize <= params.romsize;

	print("kernel data size:\t");
	print((CON_COL)isKrnlDataSizeValid, "%ld", krnlDataSize);
	print(" bytes\n");

	print("kernel size:\t\t");
	print((CON_COL)isKrnlSizeValid, "%ld", krnlSize);
	print(" bytes\n");

	char tmp[16] = { 0 };

	if (memcmp(bootParams->digest, tmp, 16) != 0)
	{
		print("rom digest:\t\t");
		printData(bootParams->digest, 16, false);
		print("...\n");
	}

	// print the keys

	if (bldrKeys != NULL)
	{
		print("\nKeys:\n");

		print("bfm key:\t");
		printData(bldrKeys->bfmKey, KEY_SIZE);

		print("eeprom key:\t");
		printData(bldrKeys->eepromKey, KEY_SIZE);
		print("cert key:\t");
		printData(bldrKeys->certKey, KEY_SIZE);

		if (xb_cmp(bldrKeys->krnlKey, tmp, KEY_SIZE) != 0)
		{
			print("kernel key:\t");
			printData(bldrKeys->krnlKey, KEY_SIZE);
		}
	}
	else
	{
		print("Keys:\t\t\tNot found\n");
	}
}
void Bios::printKernelInfo()
{
	print("\nKrnl:\n");

	USHORT kernel_ver = initTbl->kernel_ver;
	if ((kernel_ver & 0x8000) != 0) // does the kernel version have the 0x8000 bit set?
	{
		kernel_ver = kernel_ver & 0x7FFF; // clear the 0x8000 bit
		print("KD Delay flag set\t0x%04X\n", 0x8000);
	}

	print("Version:\t\t%d ( %04X )\n", kernel_ver, kernel_ver);

	bool isValid = _availableSpace >= 0 && _availableSpace <= _size;

	print("Available space:\t");
	print((CON_COL)isValid, "%ld", _availableSpace);
	print(" bytes\n");
}
void Bios::printInitTblInfo()
{
	print("\nInit tbl:\n");
	
	print("Identifier:\t\t");
	switch (initTbl->init_tbl_identifier)
	{
		case 0x30: // dvt3
			print("DVT3");
			break;
		case 0x46: // dvt4.
			print("DVT4");
			break;
		case 0x60: // dvt6.
			print("DVT6");
			break;
		case 0x70: // retail v1.0 - v1.1. i've only seen this ID in bios built for mcpx v1.0
			print("Retail <= xb v1.1");
			break;
		case 0x80: // retail > v1.1. i've only seen this ID in bios built for mcpx v1.1
			print("Retail > xb v1.1");
			break;
		default:
			print("UKN");
			break;
	}
	print(" ( %04x )\nRevision:\t\trev %d.%02d\n", initTbl->init_tbl_identifier, initTbl->revision >> 8, initTbl->revision & 0xFF);

	UINT initTblSize = bootParams->inittblSize;
	bool isInitTblSizeValid = initTblSize >= 0 && initTblSize <= params.romsize;

	print("Size:\t\t\t");
	print((CON_COL)isInitTblSizeValid, "%ld", initTblSize);
	print(" bytes");

	if (bootParams->inittblSize == 0)
	{
		print(" ( /HACKINITTBL )");
	}
	print("\n");
}
void Bios::printNv2aInfo()
{
	print("\nNV2A Init Tbl:\n");
	
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

	// print the first part of the table
	for (i = 0; i < ARRAY_START; i++)
	{
		item = ((UINT*)initTbl)[i];
		print("%04x:\t\t0x%08x\n", i * UINT_ALIGN, item);
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
			format(str + i * 3, "%02X ", *(UINT*)valArray[i]);
	}
	if (hasValue)
	{
		if (str[0] == '\0')
		{
			i = 0;
			while (str[i] == '\0' && i < ARRAY_SIZE * 3) // sanity check
			{
				xb_cpy(str + i, "00 ", 3);
				i += 3;
			}
		}
	}
	else
	{
		strcat(str, "0x00..0x00");
	}
	print("%04x-%04x:\t%s\n", ARRAY_START * UINT_ALIGN, (ARRAY_START * UINT_ALIGN) + (ARRAY_SIZE * UINT_ALIGN) - UINT_ALIGN, str);

	// print the second part of the table
	for (i = ARRAY_START + ARRAY_SIZE; i < sizeof(INIT_TBL) / UINT_ALIGN; i++)
	{
		item = ((UINT*)initTbl)[i];
		print("%04x:\t\t0x%08x %s\n", i * UINT_ALIGN, item, init_tbl_comments[i - ARRAY_START - ARRAY_SIZE]);
	}
}
void Bios::printDataTbl()
{
	print("\nDrv/Slw data tbl:\n");

	print("max mem clk: %d\n" \
		"\nCalibrat: COUNT A\tCOUNT B\n" \
		"slow_ext: 0x%04X\t0x%04X\n" \
		"slow_avg: 0x%04X\t0x%04X\n" \
		"typi:     0x%04X\t0x%04X\n" \
		"fast_avg: 0x%04X\t0x%04X\n" \
		"fast_ext: 0x%04X\t0x%04X\n",
		dataTbl->cal.max_m_clk,
		dataTbl->cal.slow_count_ext, dataTbl->cal.slow_countB_ext,
		dataTbl->cal.slow_count_avg, dataTbl->cal.slow_countB_avg,
		dataTbl->cal.typi_count, dataTbl->cal.typi_countB,
		dataTbl->cal.fast_count_avg, dataTbl->cal.fast_countB_avg,
		dataTbl->cal.fast_count_ext, dataTbl->cal.fast_countB_ext);

	const char* str[5] = {
		"Exteme Fast",
		"Fast",
		"Typical",
		"Slow",
		"Exteme Slow"
	};

	print("\n\t\t Samsung\t Micron\n" \
		"\t\tFALL RISE\tFALL RISE");
	for (int i = 0; i < 5; i++)
	{
		print("\n%s:\n", str[i]);
		print("\n%s:\naddr drv:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"addr slw:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"clk drv:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"clk slw:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"dat drv:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"dat slw:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"dsq drv:\t0x%02X 0x%02X\t0x%02X 0x%02X\n" \
			"dsq slw:\t0x%02X 0x%02X\t0x%02X 0x%02X\n\n" \
			"data delay:\t0x%02X\t\t0x%02X\n" \
			"clk delay:\t0x%02X\t\t0x%02X\n" \
			"dsq delay:\t0x%02X\t\t0x%02X\n", str[i],
			dataTbl->samsung[i].adr_drv_fall, dataTbl->samsung[i].adr_drv_rise, dataTbl->micron[i].adr_drv_fall, dataTbl->micron[i].adr_drv_rise,
			dataTbl->samsung[i].adr_slw_fall, dataTbl->samsung[i].adr_slw_rise, dataTbl->micron[i].adr_slw_fall, dataTbl->micron[i].adr_slw_rise,
			dataTbl->samsung[i].clk_drv_fall, dataTbl->samsung[i].clk_drv_rise, dataTbl->micron[i].clk_drv_fall, dataTbl->micron[i].clk_drv_rise,
			dataTbl->samsung[i].clk_slw_fall, dataTbl->samsung[i].clk_slw_rise, dataTbl->micron[i].clk_slw_fall, dataTbl->micron[i].clk_slw_rise,
			dataTbl->samsung[i].dat_drv_fall, dataTbl->samsung[i].dat_drv_rise, dataTbl->micron[i].dat_drv_fall, dataTbl->micron[i].dat_drv_rise,
			dataTbl->samsung[i].dat_slw_fall, dataTbl->samsung[i].dat_slw_rise, dataTbl->micron[i].dat_slw_fall, dataTbl->micron[i].dat_slw_rise,
			dataTbl->samsung[i].dqs_drv_fall, dataTbl->samsung[i].dqs_drv_rise, dataTbl->micron[i].dqs_drv_fall, dataTbl->micron[i].dqs_drv_rise,
			dataTbl->samsung[i].dqs_slw_fall, dataTbl->samsung[i].dqs_slw_rise, dataTbl->micron[i].dqs_slw_fall, dataTbl->micron[i].dqs_slw_rise,
			dataTbl->samsung[i].dat_inb_delay, dataTbl->micron[i].dat_inb_delay,
			dataTbl->samsung[i].clk_ic_delay, dataTbl->micron[i].clk_ic_delay,
			dataTbl->samsung[i].dqs_inb_delay, dataTbl->micron[i].dqs_inb_delay);
	}
}

int Bios::validateBldr()
{
	if (bootParams == NULL)
		return 1;

	const UINT krnlSize = bootParams->krnlSize;
	const UINT krnlDataSize = bootParams->krnlDataSize;
	const UINT tblSize = bootParams->inittblSize;
	const UINT availableSpace = _totalSpaceAvailable;
	const UINT romSize = params.romsize;

	const bool isKrnlSizeValid = krnlSize >= 0 && krnlSize <= romSize;
	const bool isDataSizeValid = krnlDataSize >= 0 && krnlDataSize <= romSize;
	const bool istblSizeValid = tblSize >= 0 && tblSize <= romSize;
	const bool isAvailableSpaceValid = availableSpace >= 0 && availableSpace <= romSize;

	_isBldrSignatureValid = xb_cmp(BOOT_PARAMS_SIGNATURE, bootParams->signature, 4) == 0;
	_isBootParamsValid = isKrnlSizeValid && isDataSizeValid && istblSizeValid;

	if (!_isBootParamsValid)
	{
		error("Error: boot params are invalid\n\n");
		return 1;
	}

	return 0;
}

int Bios::decompressKrnl()
{
	if (decompressedKrnl != NULL)
	{
		error("Error: buffer is not NULL\n");
		return 1;
	}

	if (krnl == NULL)
	{
		error("Error: compressed kernel ptr is NULL\n");
		return 1;
	}

	UINT bufferSize = 1 * 1024 * 1024; // 1MB ( 52 blocks ); initial value, decompress() will reallocate if needed. 
	
	decompressedKrnl = (UCHAR*)xb_alloc(bufferSize); // allocate initial buffer for the decompressed kernel. buff will be reallocated if needed.
	if (decompressedKrnl == NULL)
	{
		return 1;
	}

	print("Decompressing kernel (%d bytes)\n", bootParams->krnlSize);
	
	int result = decompress(krnl, bootParams->krnlSize, decompressedKrnl, bufferSize, _decompressedKrnlSize);
	if (!SUCCESS(result))
	{
		error("Error: failed to decompress kernel\n");
		return 1;
	}
			
	return 0;
}

int Bios::create(UCHAR* in_bl, UINT in_blSize, UCHAR* in_tbl, UINT in_tblSize, UCHAR* in_k, UINT in_kSize, UCHAR* in_kData, UINT in_kDataSize)
{
	if (_bios != NULL)
	{
		error("Error: Data already loaded\n");
		return BIOS_LOAD_STATUS_FAILED;
	}

	// check romsize is big enough for the bios.
	if (params.romsize < in_tblSize + BLDR_BLOCK_SIZE + MCPX_BLOCK_SIZE + in_kSize + in_kDataSize)
	{
		error("Error: romsize is less than the total size of the bios\n");
		return BIOS_LOAD_STATUS_FAILED;
	}

	_bios = (UCHAR*)xb_alloc(params.romsize);
	if (_bios == NULL)
	{
		error("Error: Failed to allocate memory\n");
		return BIOS_LOAD_STATUS_FAILED;
	}
	_size = params.romsize;

	// set encryption states based on user input.
	// when building a bios. the bldr file, and kernel file is expected to be passed in UNENCRYPTED.
	// This is because a bios can't be extracted unless the 2bl is decrypted.
	_isBldrEncrypted = !params.encBldr;
	_isKernelEncrypted = !params.encKrnl;

	getOffsets();

	// copy in init tbl
	xb_cpy(_bios, in_tbl, in_tblSize);

	// copy in bldr
	xb_cpy(bldr, in_bl, in_blSize);

	getOffsets2(in_kSize, in_kDataSize);

	// copy in the compressed kernel image
	xb_cpy(krnl, in_k, in_kSize);

	// copy in the uncompressed kernel data
	xb_cpy(krnlData, in_kData, in_kDataSize);

	// fixup the boot params.
	bootParams->krnlSize = in_kSize;
	bootParams->krnlDataSize = in_kDataSize;
	bootParams->inittblSize = in_tblSize;

	print("\nBLDR Entry:\t0x%08X\n\nSignature:\t", ldrParams->bldrEntryPoint);
	printData(bootParams->signature, 4);
	print("Krnl data size:\t%ld bytes\n" \
		"Krnl size:\t%ld bytes\n" \
		"2BL size:\t%ld bytes\n" \
		"Init tbl size:\t%ld bytes\n" \
		"Avail space:\t%ld bytes\n",
		bootParams->krnlDataSize, bootParams->krnlSize, BLDR_BLOCK_SIZE, bootParams->inittblSize, _availableSpace);

	// encrypt the kernel
	if (!_isKernelEncrypted)
	{
		symmetricEncDecKernel();
		if (_isKernelEncrypted)
		{
			// kernel was encrypted successfully. update the kernel key in the bldr.
			
			if (bldrKeys != NULL && params.keyKrnl != NULL)
			{
				print("Updating kernel key in bldr..\n");
				xb_cpy(bldrKeys->krnlKey, params.keyKrnl, KEY_SIZE);
			}
		}
	}

	if ((params.sw_flag & SW_BLD_BFM) != 0) // build a bios that boots from media.
	{
		convertToBootFromMedia();
	}

	// only encrypt the bldr if the user has provided a key file or a mcpx rom and the bldr is not already encrypted.
	if (!_isBldrEncrypted)
	{
		if (params.keyBldr != NULL)
		{
			symmetricEncDecBldr(params.keyBldr, KEY_SIZE);
		}
		else if (params.mcpxFile != NULL)
		{
			switch (params.mcpx.version)
			{
			case MCPX_ROM::MCPX_V1_0:
				symmetricEncDecBldr(params.mcpx.data+0x1A5, KEY_SIZE);
				break;
			case MCPX_ROM::MCPX_V1_1:
				break;
			}
		}
	}

	return BIOS_LOAD_STATUS_SUCCESS;
}


int Bios::convertToBootFromMedia()
{
	// convert the bios to boot from media.
	// KD_DELAY_FLAG is or'd with the init_tbl_identifier.

	// BFM bios should be encrypted with the DEV 2BL key.
	// BFM bios should be 1Mb in size. - a Kernel will not load if the BFM is not 1Mb in size.

	print("Adding KD Delay flag ( BFM )\n");

	const UINT KD_DELAY_FLAG = 0x80000000;

	UINT* bootFlags = (UINT*)(&initTbl->init_tbl_identifier);

	*bootFlags |= KD_DELAY_FLAG;

	// encrypt the bldr with the bfm key.

	if (params.keyBldr == NULL)
	{
		if (bldrKeys == NULL)
		{
			error("Error: bldr keys not found\n");
			return 1;
		}

		print("BFM key:\t");
		printData(bldrKeys->bfmKey, KEY_SIZE);
		symmetricEncDecBldr(bldrKeys->bfmKey, KEY_SIZE);
	}
	else
	{
		if (xb_cmp(params.keyBldr, bldrKeys->bfmKey, KEY_SIZE) != 0)
		{
			error("Error: BFM key does not match the key in the bldr\n");
			return 1;
		}
		print("A BFM bios should be encrypted with the DEV 2BL key.\n");
	}

	// force the bfm bios to be 1Mb in size.
	params.binsize = 1 * 1024 * 1024; // 1Mb

	return 0;
}