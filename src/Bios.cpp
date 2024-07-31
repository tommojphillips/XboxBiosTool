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
#include "cstring"

// user incl
#include "Bios.h"
#include "Mcpx.h"
#include "XbTool.h"
#include "XcodeInterp.h"

#include "file.h"
#include "rc4.h"
#include "util.h"
#include "xbmem.h"

#include "lzx.h"
#include "sha1.h"

extern Parameters& params;

const char ERROR_MSG_ROMSIZE_INVALID[] = "Error: romsize is less than the total size of the bios.\n";
const char ERROR_MSG_WRONG_MCPX_ROM[] = "Error: decrypting the bldr. MCPX v%s was expected but v%s was provided.\n";

void Bios::reset()
{
	// reset pointers, offsets

	// sizes
	size = 0;
	totalSpaceAvailable = 0;
	availableSpace = 0;
	decompressedKrnlSize = 0;

	bldrEncryptionState = false;
	kernelEncryptionState = false;
	bldrBootParamsValid = false;
	bldrSignatureValid = false;

	// pointers
	bldr = NULL;
	bootParams = NULL;
	ldrParams = NULL;
	initTbl = NULL;
	dataTbl = NULL;
	krnl = NULL;
	krnlData = NULL;
	bldrEntry = NULL;
	bldrKeys = NULL;
	preldr = NULL;
	preldrParams = NULL;
	preldrEntry = NULL;
}
void Bios::construct()
{
	// allocated memory
	data = NULL;
	decompressedKrnl = NULL;
	preldrKey = NULL;

	reset();
}

void Bios::deconstruct()
{
	if (data != NULL)
	{
		xb_free(data);
		data = NULL;
	}
	if (decompressedKrnl != NULL)
	{
		xb_free(decompressedKrnl);
		decompressedKrnl = NULL;
	}
	if (preldrKey != NULL)
	{
		xb_free(preldrKey);
		preldrKey = NULL;
	}

	reset();
}

void Bios::getOffsets2(const UINT krnlSize, const UINT krnlDataSize)
{
	// get the pointers to the bldr entry and keys. Bldr needs to be unencrypted for this to work.
		
	UINT entryPointOffset = ldrParams->bldrEntryPoint - BLDR_BASE;
	bldrEntry = (BLDR_ENTRY*)(bldr + entryPointOffset - sizeof(BLDR_ENTRY));
	if ((UCHAR*)bldrEntry < bldr || (UCHAR*)bldrEntry + sizeof(BLDR_ENTRY) > bldr + BLDR_BLOCK_SIZE)
	{
		bldrEntry = NULL;
	}
	if (bldrEntry != NULL)
	{
		UINT keysOffset = bldrEntry->keysPtr - BLDR_RELOC;
		bldrKeys = (BLDR_KEYS*)(bldr + keysOffset - KEY_SIZE); // bfm key is 16 bytes before the keys ptr.
		if ((UCHAR*)bldrKeys < bldr || (UCHAR*)bldrKeys + sizeof(BLDR_KEYS) > bldr + BLDR_BLOCK_SIZE)
		{
			bldrKeys = NULL;
		}
	}

	// set the kernel pointers.
	krnlData = (bldr - krnlDataSize);
	krnl = (krnlData - krnlSize);

	// calculate the available space in the bios.
	totalSpaceAvailable = params.romsize - BLDR_BLOCK_SIZE - MCPX_BLOCK_SIZE;
	availableSpace = totalSpaceAvailable - krnlSize - krnlDataSize;
}
void Bios::getOffsets()
{		
	initTbl = (INIT_TBL*)(data);
	dataTbl = (ROM_DATA_TBL*)(data + initTbl->data_tbl_offset);
	
	bldr = (data + size - BLDR_BLOCK_SIZE - MCPX_BLOCK_SIZE);
	ldrParams = (BOOT_LDR_PARAM*)(bldr);
	bootParams = (BOOT_PARAMS*)(bldr + BLDR_BLOCK_SIZE - sizeof(BOOT_PARAMS));
}

int Bios::checkForPreldr()
{
	// find preldr entry params, rsa1 header.
	
	preldr = bldr + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE;
	preldrParams = (PRELDR_PARAMS*)preldr;
	
	// check for the jmp instruction. this is patched in during the build process.
	
	if ((preldrParams->jmpInstr & 0xFF) != 0xE9)
	{
		preldr = NULL;
		preldrParams = NULL;
		return PRELDR_STATUS_NOT_FOUND; // preldr was not found in this bios.
	}

	print("\npreldr found\n");

	preldrEntry = (PRELDR_ENTRY*)(preldr + (preldrParams->jmpInstr >> 8) + 0x5 - sizeof(PRELDR_ENTRY));
	if ((UCHAR*)preldrEntry < preldr || (UCHAR*)preldrEntry + sizeof(PRELDR_ENTRY) > preldr + PRELDR_BLOCK_SIZE)
	{
		// preldr entry point ptr is out of bounds
		preldrEntry = NULL;
		return PRELDR_STATUS_ERROR;
	}

#if 0
	// decrypt the preldr pubkey
	const UINT PRELDR_BASE = (0xFFFFFFFF - MCPX_BLOCK_SIZE - PRELDR_BLOCK_SIZE + 1);
	PUBLIC_KEY pubkey = { 0 };
	UINT pubKeyOffset = preldrEntry->encryptedPubKeyPtr - PRELDR_BASE;

	print("Decrypting preldr pubkey..\n");

	// create cpy of the encrypted pub key
	xb_cpy(&pubkey, (preldr + pubKeyOffset), sizeof(PUBLIC_KEY));

	// decrypt the pub key with the first 12 bytes of the sb key
	symmetricEncDec((UCHAR*)&pubkey, sizeof(PUBLIC_KEY), sb_key, 12);
	
	if (verifyPubKey(&pubkey) != 0)
	{
		print("Error: Public key header is invalid\n");
		return PRELDR_STATUS_ERROR;
	}
	print("pubkey is valid\n");
#endif

	if (isBldrEncrypted())
	{
		if (params.mcpx.data == NULL)
		{
			print("Warning: A preldr was found but no mcpx rom was provided.\n");
			return PRELDR_STATUS_ERROR;
		}
		if (params.mcpx.version != MCPX_ROM::MCPX_V1_1)
		{
			print(ERROR_MSG_WRONG_MCPX_ROM, "1.1", "1.0");
			return PRELDR_STATUS_ERROR;
		}

		// create the bldr key
		UCHAR* last16Bytes = (bldr + BLDR_BLOCK_SIZE - KEY_SIZE);
		UCHAR encKey[SHA1_DIGEST_LEN] = { 0 };
		SHA1Context sha;

		SHA1Reset(&sha);
		SHA1Input(&sha, params.mcpx.sbkey, KEY_SIZE);
		SHA1Input(&sha, last16Bytes, KEY_SIZE);
		for (int i = 0; i < KEY_SIZE; i++)
			encKey[i] = params.mcpx.sbkey[i] ^ 0x5C;
		SHA1Input(&sha, encKey, KEY_SIZE);
		SHA1Result(&sha, encKey);

		// cpy built key for later use.
		if (preldrKey == NULL)
		{
			preldrKey = (UCHAR*)xb_alloc(SHA1_DIGEST_LEN);
		}
		xb_cpy(preldrKey, encKey, SHA1_DIGEST_LEN);

		// decrypt the bldr
		symmetricEncDecBldr(encKey, SHA1_DIGEST_LEN);
	}

#if 0
	// fix up the boot params ptr, with a preldr it's -16 bytes where it is normally.
	bootParams = (BOOT_PARAMS*)((UCHAR*)bootParams - 16);
#else
	// copy boot params to where it should be.
	BOOT_PARAMS bootParamsCpy = { 0 };
	xb_cpy(&bootParamsCpy, ((UCHAR*)bootParams - 16), sizeof(BOOT_PARAMS));
	xb_cpy(bootParams, &bootParamsCpy, sizeof(BOOT_PARAMS));
#endif

	// fixup bldr entry point, this is zeroed during the build process to prevent a resolved rc4 key attack. preldr uses a rel jmp instead.
	UINT entryOffset = *(UINT*)(bldr + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE - 8); // should equal 01bc for preldrs (mcpx v1.1)
	ldrParams->bldrEntryPoint = BLDR_BASE + entryOffset;
	
	return PRELDR_STATUS_FOUND;
}

int Bios::load(UCHAR* buff, const UINT buff_size)
{
	int result = 0;

	if (data != NULL)
		return BIOS_LOAD_STATUS_FAILED_ALREADY_LOADED;

	data = buff;
	size = buff_size;
	buffersize = size;

	// check size
	if (checkSize(size) != 0)
	{
		print("Error: Invalid bios size, expecting 256Kb, 512Kb or 1Mb bios file.\n");
		return BIOS_LOAD_STATUS_FAILED;
	}
	
	// set encryption states based on user input.
	bldrEncryptionState = isFlagClear(SW_ENC_BLDR, params.sw_flags);
	kernelEncryptionState = isFlagClear(SW_ENC_KRNL, params.sw_flags);

	getOffsets();

	if (checkForPreldr() == PRELDR_STATUS_NOT_FOUND)
	{
		// only decrypt the bldr if the user has provided a key file or a mcpx rom and the bldr is encrypted.
		if (isBldrEncrypted())
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
					symmetricEncDecBldr(params.mcpx.sbkey, KEY_SIZE);
					break;
				case MCPX_ROM::MCPX_V1_1:
					print(ERROR_MSG_WRONG_MCPX_ROM, "1.0", "1.1");
					return BIOS_LOAD_STATUS_FAILED;
				}
			}
		}
	}

	// check if the boot params are valid. All offsets are based on the boot params.
	result = validateBldr();
	if (result != 0)
	{
		return result;
	}

	// do stuff that depends on the boot params being decrypted and valid.

	getOffsets2(bootParams->krnlSize, bootParams->krnlDataSize);

	if (isKernelEncrypted())
	{
		symmetricEncDecKernel();
	}

	return BIOS_LOAD_STATUS_SUCCESS;
}

int Bios::loadFromFile(const char* filename)
{
	UINT size = 0;
	UCHAR* data = readFile(filename, &size);
	if (data == NULL)
		return BIOS_LOAD_STATUS_FAILED;

	int result = load(data, size);
	if (result == BIOS_LOAD_STATUS_FAILED_ALREADY_LOADED)
		xb_free(data);
	return result;
}
int Bios::loadFromData(const UCHAR* data, const UINT size)
{
	UCHAR* biosData = (UCHAR*)xb_alloc(size);
	if (biosData == NULL)
		return BIOS_LOAD_STATUS_FAILED;

	xb_cpy(biosData, data, size); 
	
	int result = load(biosData, size);
	if (result == BIOS_LOAD_STATUS_FAILED_ALREADY_LOADED)
		xb_free(biosData);
	return result;
}

int Bios::saveBiosToFile(const char* filename)
{
	return writeFile(filename, data, size);
}
int Bios::saveBldrBlockToFile(const char* filename)
{
	return saveBldrToFile(filename, BLDR_BLOCK_SIZE);
}
int Bios::saveBldrToFile(const char* filename, const UINT size)
{
	print("Writing bldr to %s (%d bytes)\n", filename, size);
	return writeFile(filename, bldr, size);
}
int Bios::saveKernelToFile(const char* filename)
{
	print("Writing krnl to %s (%d bytes)\n", filename, bootParams->krnlSize);
	return writeFile(filename, krnl, bootParams->krnlSize);
}
int Bios::saveKernelDataToFile(const char* filename)
{
	print("Writing krnl data to %s (%d bytes)\n", filename, bootParams->krnlDataSize);
	return writeFile(filename, krnlData, bootParams->krnlDataSize);
}
int Bios::saveKernelImgToFile(const char* filename)
{
	// extract the compressed kernel + uncompressed krnl data
	UINT size = bootParams->krnlSize + bootParams->krnlDataSize;
	print("Writing krnl img (compressed krnl+data) to %s (%d bytes)\n", filename, size);
	return writeFile(filename, krnl, size); // krnl data is at the end of the kernel
}
int Bios::saveInitTblToFile(const char* filename)
{
	// extract the init table
	print("Writing init tbl to %s (%d bytes)\n", filename, bootParams->inittblSize);
	return writeFile(filename, data, bootParams->inittblSize);
}

void Bios::symmetricEncDecBldr(const UCHAR* key, const UINT keyLen)
{
	if (bldr == NULL)
		return;

	print("%s Bldr..\n", bldrEncryptionState ? "Decrypting" : "Encrypting");

	// verify the bldr ptr is within the bounds or this will cause an access violation.
	if (bldr < data || (bldr + BLDR_BLOCK_SIZE) > (data + size))
	{
		print("Error: bldr ptr is out of bounds\n");
		return;
	}

	symmetricEncDec(bldr, BLDR_BLOCK_SIZE, key, keyLen);

	bldrEncryptionState = !bldrEncryptionState;
}
void Bios::symmetricEncDecKernel()
{
	if (krnl == NULL || bootParams == NULL)
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
		if (xb_cmp(key, tmp, KEY_SIZE) == 0)
			return; // kernel key is all 0's

		xb_set(tmp, 0xFF, KEY_SIZE);
		if (xb_cmp(key, tmp, KEY_SIZE) == 0)
			return; // kernel key is all FF's

		// key is valid; use it.
		print("krnl key from 2bl: ");
		printData(key, KEY_SIZE);
	}

	// en/decrypt the kernel.

	print("%s Kernel..\n", kernelEncryptionState ? "Decrypting" : "Encrypting");

	// verify the krnl is within the bounds or this will cause an access violation.
	if (krnl < data || (krnl + bootParams->krnlSize) > (data + size))
	{
		print("Error: krnl ptr is out of bounds\n");
		return;
	}

	symmetricEncDec(krnl, bootParams->krnlSize, key, KEY_SIZE);

	kernelEncryptionState = !kernelEncryptionState;
}

void Bios::printBldrInfo()
{
	print("\nBldr:\n");

	if (bldrBootParamsValid)
	{
		print("entry point:\t\t0x%08X\n", ldrParams->bldrEntryPoint);

		if (bldrEntry != NULL)
		{
			print("bfm entry point:\t0x%08X\n", bldrEntry->bfmEntryPoint);
		}
	}

	print("signature:\t\t");
	printData((UCHAR*)&bootParams->signature, 4);
	
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

	bool isValid = availableSpace >= 0 && availableSpace <= size;

	print("Available space:\t");
	print((CON_COL)isValid, "%ld", availableSpace);
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
		case 0x80: // retail > v1.1. i've only seen this ID in bios built for mcpx v1.1
			print("Retail");
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
		print(" ( not hashed )");
	}
	print("\n");
}
void Bios::printNv2aInfo()
{	
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

	print("\nNV2A Init Tbl:\n");

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

	print("\n\t\t Samsung\t Micron\n\t\tFALL RISE\tFALL RISE");
	for (int i = 0; i < 5; i++)
	{
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
void Bios::printKeys()
{
	// print the keys
	print("\nKeys:\n");
	if (params.mcpxFile != NULL && params.mcpx.version != MCPX_ROM::MCPX_UNK)
	{
		print("sb key (+%d):\t", params.mcpx.sbkey - params.mcpx.data);
		printData(params.mcpx.sbkey, KEY_SIZE);

		if (params.mcpx.version == MCPX_ROM::MCPX_V1_1 && preldrKey != NULL)
		{
			// print built key
			print("built bldr key: ");
			printData(preldrKey, SHA1_DIGEST_LEN);
		}
	}
	if (bldrKeys != NULL)
	{
		print("bfm key:\t");
		printData(bldrKeys->bfmKey, KEY_SIZE);
		print("eeprom key:\t");
		printData(bldrKeys->eepromKey, KEY_SIZE);
		print("cert key:\t");
		printData(bldrKeys->certKey, KEY_SIZE);
		print("kernel key:\t");
		printData(bldrKeys->krnlKey, KEY_SIZE);
	}
	else
	{
		print("Not found\n");
	}
}

int Bios::validateBldr()
{
	const UINT krnlSize = bootParams->krnlSize;
	const UINT krnlDataSize = bootParams->krnlDataSize;
	const UINT tblSize = bootParams->inittblSize;
	const UINT availableSpace = totalSpaceAvailable;

	const bool isKrnlSizeValid = krnlSize >= 0 && krnlSize <= size;
	const bool isDataSizeValid = krnlDataSize >= 0 && krnlDataSize <= size;
	const bool istblSizeValid = tblSize >= 0 && tblSize <= size;
	const bool isAvailableSpaceValid = availableSpace >= 0 && availableSpace <= size;

	bldrSignatureValid = (BOOT_PARAMS_SIGNATURE == bootParams->signature);
	bldrBootParamsValid = isKrnlSizeValid && isDataSizeValid && istblSizeValid;

	if (!bldrBootParamsValid)
	{
		print("boot params are invalid/encrypted\n");
		return BIOS_LOAD_STATUS_INVALID_BLDR;
	}

	// verify the rom size is big enough for the bios.
	if (params.romsize < MCPX_BLOCK_SIZE + BLDR_BLOCK_SIZE + krnlSize + krnlDataSize + tblSize)
	{
		print(ERROR_MSG_ROMSIZE_INVALID);
		return BIOS_LOAD_STATUS_FAILED_INVALID_ROMSIZE;
	}

	return 0;
}

int Bios::decompressKrnl()
{
	if (decompressedKrnl != NULL)
		return 1;

	UINT bufferSize = 1 * 1024 * 1024; // 1MB ( 52 blocks ); initial value, decompress() will reallocate if needed. 
	
	decompressedKrnl = (UCHAR*)xb_alloc(bufferSize); // allocate initial buffer for the decompressed kernel. buff will be reallocated if needed.
	if (decompressedKrnl == NULL)
	{
		return ERROR_OUT_OF_MEMORY;
	}

	print("Decompressing kernel (%d bytes)\n", bootParams->krnlSize);
	
	if (decompress(krnl, bootParams->krnlSize, decompressedKrnl, bufferSize, decompressedKrnlSize) != 0)
	{
		return 1;
	}
			
	return 0;
}

int Bios::build(UCHAR* in_bl, UINT in_blSize, UCHAR* in_tbl, UINT in_tblSize, UCHAR* in_k, UINT in_kSize, UCHAR* in_kData, UINT in_kDataSize)
{
	if (data != NULL)
		return BIOS_LOAD_STATUS_FAILED;

	// verify the rom size is big enough for the bios.
	if (params.romsize < BLDR_BLOCK_SIZE + MCPX_BLOCK_SIZE + in_kSize + in_kDataSize + in_tblSize)
	{
		print(ERROR_MSG_ROMSIZE_INVALID);
		return BIOS_LOAD_STATUS_FAILED_INVALID_ROMSIZE;
	}
	if (params.binsize < params.romsize)
	{
		print("Error: binsize is less than the romsize\n");
		return BIOS_LOAD_STATUS_FAILED_INVALID_BINSIZE;
	}

	data = (UCHAR*)xb_alloc(params.binsize);
	if (data == NULL)
		return BIOS_LOAD_STATUS_FAILED;

	size = params.romsize;
	buffersize = params.binsize;

	// set encryption states.
	bldrEncryptionState = isFlagSet(SW_ENC_BLDR, params.sw_flags);
	// is flag set = not encrypted ; key provided = encrypted ; flag set + key provided = not encrypted
	kernelEncryptionState = (isFlagClear(SW_ENC_KRNL, params.sw_flags) && params.keyKrnl == NULL) || (isFlagSet(SW_ENC_KRNL, params.sw_flags) && params.keyKrnl != NULL);

	getOffsets();

	// copy in init tbl
	xb_cpy(data, in_tbl, in_tblSize);

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
	printData((UCHAR*)&bootParams->signature, 4);
	print("Krnl data size:\t%ld bytes\n" \
		"Krnl size:\t%ld bytes\n" \
		"2BL size:\t%ld bytes\n" \
		"Init tbl size:\t%ld bytes\n" \
		"Avail space:\t%ld bytes\n" \
		"rom size:\t%ld Kb\n" \
		"bin size:\t%ld Kb\n",
		bootParams->krnlDataSize, bootParams->krnlSize,
		BLDR_BLOCK_SIZE, bootParams->inittblSize, availableSpace,
		params.romsize / 1024, params.binsize / 1024);

	// encrypt the kernel
	if (!isKernelEncrypted())
	{
		symmetricEncDecKernel();
		if (isKernelEncrypted())
		{
			// TO-DO: replace kernel rc4 key in the 2bl with the key used to encrypt it.
		}
	}

	// build a bios that boots from media. ( BFM )
	if (isFlagSet(SW_BLD_BFM, params.sw_flags))
	{
		convertToBootFromMedia();
	}

	// only encrypt the bldr if the user has provided a key file or a mcpx rom and the bldr is not already encrypted.
	if (!isBldrEncrypted())
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
					symmetricEncDecBldr(params.mcpx.sbkey, KEY_SIZE);
					break;
			}
		}
	}

	if (params.binsize > params.romsize)
	{
		if (replicateBios() != 0)
		{
			print("Error: failed to replicate the bios\n");
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

	print("Adding KD Delay flag ( BFM )\n");

	UINT* bootFlags = (UINT*)(&initTbl->init_tbl_identifier);

	*bootFlags |= KD_DELAY_FLAG;

	if (params.keyBldr == NULL)
	{
		if (bldrKeys == NULL)
			return 1;

		symmetricEncDecBldr(bldrKeys->bfmKey, KEY_SIZE);
	}

	// force the bfm bios to be 1Mb in size.
	params.binsize = 1 * 1024 * 1024; // 1Mb

	return 0;
}

int Bios::replicateBios()
{
	// replicate the bios file base on binsize

	UINT offset = 0;
	UINT new_size = 0;
	UINT binsize = params.binsize;
	UINT romsize = params.romsize;

	if (binsize < romsize || buffersize < binsize)
		return 1;

	offset = romsize;
	while (offset < binsize)
	{
		new_size = offset * 2;

		if (offset < new_size && binsize >= new_size)
		{
			print("Replicating to 0x%08X-0x%08X (%d bytes)\n", offset, new_size, offset);
			xb_cpy((data + offset), data, offset);
		}

		offset = new_size;
	}

#if 0
	print("Verifying replicated bios..\n");
	
	// verify we got to the end of the bios size
	if (new_size != params.binsize)
	{
		print("Error: Replicated bios is not the correct size\n");
		return 1;
	}

	// verify replications
	offset = params.romsize;
	while (offset < new_size)
	{
		if (xb_cmp(_bios, (_bios + offset), offset) != 0)
		{
			print("Error: Replicated bios does not match the original\n");
			return 1;
		}
		offset *= 2;
	}
#endif

	// update the size of the bios
	size = new_size;

	return 0;
}
