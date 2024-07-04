// Bios.cpp: defines functions for handling bios data

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

extern XbTool xbtool;

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
		bldrKeys = (BLDR_KEYS*)(bldr + keysOffset);
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
	_totalSpaceAvailable = xbtool.params.romsize - BLDR_BLOCK_SIZE - MCPX_BLOCK_SIZE;

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

	if (xbtool.params.mcpx.data == NULL)
	{
		error("Error: MCPX v1.1 rom is required to decrypt the preldr ( -mcpx )\n");
		return PRELDR_STATUS_ERROR;
	}

	if (xbtool.params.mcpx.version != MCPX_ROM::MCPX_V1_1)
	{
		// user has provided the incorrect mcpx rom. let 'em know.
		error("Error: decrypting the bldr. MCPX v1.1 was expected but v1.0 was provided.\n");
		return PRELDR_STATUS_ERROR;
	}

	PRELDR_ENTRY* preldrEntry = (PRELDR_ENTRY*)(preldr + (preldrParams->jmpInstr >> 8) + 0x5 - sizeof(PRELDR_ENTRY)); 

	UCHAR* sbKey = (UCHAR*)(xbtool.params.mcpx.data + MCPX_BLOCK_SIZE - 0x64);
	
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

	UINT entryOffset = *((UINT*)(bldr + BLDR_BLOCK_SIZE - PRELDR_BLOCK_SIZE) - 2);	// should equal 01bc

	print("fixing up bldr ptrs\n");

	// fix up the ptr to the boot params, with a preldr it's -16 bytes where it is normally.
	bootParams = (BOOT_PARAMS*)((UCHAR*)bootParams - 16);
	
	// fixup bldr entry point, this is zeroed during the build process to prevent a resolved rc4 key attack. preldr uses a rel jmp instead.
	ldrParams->bldrEntryPoint = BLDR_BASE + entryOffset;
	
	print("preldr is valid\n\n");

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
	_isBldrEncrypted = xbtool.params.encBldr;
	_isKernelEncrypted = xbtool.params.encKrnl;

	getOffsets();

	// before decrypting the bldr, check if there is a preldr at the end of the bldr block. (preldr is mostly unencrypted)
	// checkForPreldr() will decrypt the bldr if a preldr is found and patch the boot params ptr so it points to the correct location.
	if (checkForPreldr() == PRELDR_STATUS_NOT_FOUND)
	{
		// only decrypt the bldr if there is no preldr. (decryption is different if a preldr is present)

		if (_isBldrEncrypted)
		{
			if (xbtool.params.keyBldr != NULL) // bldr key is provided; use it.
			{
				symmetricEncDecBldr(xbtool.params.keyBldr, KEY_SIZE);
			}
			else if (xbtool.params.mcpxFile != NULL)
			{
				if (xbtool.params.mcpx.version != MCPX_ROM::MCPX_V1_0)
				{
					// user has provided the incorrect mcpx rom. let 'em know.
					error("Error: decrypting the bldr. MCPX v1.0 was expected but v1.1 was provided.\n");
					return BIOS_LOAD_STATUS_FAILED;
				}

				// decrypt the bldr with the mcpx v1.0 key
				symmetricEncDecBldr(xbtool.params.mcpx.data+0x1A5, KEY_SIZE);
			}
		}
	}

	// check if the boot params are valid. All offsets are based on the boot params.
	if (!SUCCESS(validateBldr()))
	{
		return BIOS_LOAD_STATUS_INVALID_BLDR;
	}

	// do stuff that depends on the boot params being decrypted and valid.

	getOffsets2(bootParams->krnlSize, bootParams->krnlDataSize);

	decryptKernel();

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

	return writeFile(path, _bios, _size);
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
	
	ULONG krnlSize = bootParams->krnlSize;
	ULONG krnlDataSize = bootParams->krnlDataSize;

	bool isKrnlSizeValid = krnlSize >= 0 && krnlSize <= xbtool.params.romsize;
	bool isKrnlDataSizeValid = krnlDataSize >= 0 && krnlDataSize <= xbtool.params.romsize;

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
		printData(bldrKeys->eepromKey - 16, KEY_SIZE);

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
	print("Version:\t\t%d ( %04X )\n", initTbl->kernel_ver, initTbl->kernel_ver);

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

	ULONG initTblSize = bootParams->inittblSize;
	bool isInitTblSizeValid = initTblSize >= 0 && initTblSize <= xbtool.params.romsize;

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

	const ULONG krnlSize = bootParams->krnlSize;
	const ULONG krnlDataSize = bootParams->krnlDataSize;
	const ULONG tblSize = bootParams->inittblSize;
	const ULONG availableSpace = _totalSpaceAvailable;	
	const ULONG romSize = xbtool.params.romsize;

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
	else
	{
		print("boot params are valid.\n");
	}

	return 0;
}

int Bios::decryptKernel()
{
	if (!_isKernelEncrypted)
	{
		return 1; // user has specified the kernel is not encrypted, (-enc-krnl). don't decrypt the kernel.
	}

	UCHAR* key = xbtool.params.keyKrnl;
	if (key == NULL)
	{
		// key not provided on cli; try get key from bldr block.

		if (bldrKeys == NULL)
		{
			return 1; 
		}

		key = bldrKeys->krnlKey;

		if (key == NULL)
		{
			return 1; // kernel key not found
		}

		UCHAR tmp[KEY_SIZE] = { 0 };
		if (memcmp(key, tmp, KEY_SIZE) == 0)
		{
			return 1; // kernel key is all 0's
		}
	}

	// decrypt the kernel.

	print("krnl key: ");
	printData(key, KEY_SIZE);

	print("%s Kernel..\n", _isKernelEncrypted ? "Decrypting" : "Encrypting");

	// verify the krnl is within the bounds or this will cause an access violation.
	if (krnl < _bios || (krnl + bootParams->krnlSize) > (_bios + _size))
	{
		error("Error: krnl ptr is out of bounds\n");
		return 1;
	}

	symmetricEncDec(krnl, bootParams->krnlSize, key, KEY_SIZE);

	_isKernelEncrypted = !_isKernelEncrypted;

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

	_bios = (UCHAR*)xb_alloc(xbtool.params.romsize);
	if (_bios == NULL)
	{
		error("Error: Failed to allocate memory\n");
		return BIOS_LOAD_STATUS_FAILED;
	}
	_size = xbtool.params.romsize;

	getOffsets();
	getOffsets2(in_kSize, in_kDataSize);

	// copy in init tbl
	xb_cpy(_bios, in_tbl, in_tblSize);

	// copy in bldr
	xb_cpy(bldr, in_bl, in_blSize);

	// copy in the compressed kernel image
	xb_cpy(krnl, in_k, in_kSize);

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

	// encrypt the kernel if keys are provided
	if (xbtool.params.keyKrnl != NULL)
	{
		//symmetricEncDecKernel(xbtool.params.keyKrnl);
		// TODO: update the kernel key in the bldr prior to encrypting it.
	}

	// encrypt the boot loader if key is provided
	if (xbtool.params.keyBldr != NULL) // bldr key is provided; use it.
	{
		symmetricEncDecBldr(xbtool.params.keyBldr, KEY_SIZE);
	}
	else if (xbtool.params.mcpxFile != NULL)
	{
		switch (xbtool.params.mcpx.version)
		{
			case MCPX_ROM::MCPX_V1_0:
				symmetricEncDecBldr(xbtool.params.mcpx.data+0x1A5, KEY_SIZE);
				break;
			case MCPX_ROM::MCPX_V1_1:
				// not implemented yet.
				break;
		}
	}

	return BIOS_LOAD_STATUS_SUCCESS;
}