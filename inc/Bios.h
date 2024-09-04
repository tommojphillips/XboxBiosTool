// Bios.h

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

#ifndef XB_BIOS_H
#define XB_BIOS_H

// user incl
#include "Mcpx.h"
#include "type_defs.h"
#include "bldr.h"
#include "rsa.h"
#include "sha1.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#else
#include <malloc.h>
#endif

const UINT MAX_BIOS_SIZE = 0x100000;	// 1MB max bios size
const UINT KD_DELAY_FLAG = 0x80000000;	// flag for bfm kernel.

const UINT BLDR_BLOCK_SIZE = 24576;     // bldr block size in bytes
const UINT PRELDR_BLOCK_SIZE = 10752;   // preldr block size in bytes
const UINT PRELDR_ROM_DIGEST_SIZE = 384;// preldr rom digest size in bytes
const UINT PRELDR_BASE = (0xFFFFFFFF - MCPX_BLOCK_SIZE - PRELDR_BLOCK_SIZE + 1); // base address for the preldr

const UINT DEFAULT_ROM_SIZE = 256;      // default rom size in Kb
const UINT BLDR_RELOC = 0x00400000;     // relocation address for the bldr
const UINT BLDR_BASE = 0x00090000; 	    // base address for the bldr

const UINT BOOT_SIGNATURE = 2018801994U; // J y T x

enum BIOS_STATUS : int {
	BIOS_LOAD_STATUS_SUCCESS = 0,	// success; the bios is loaded.
	BIOS_LOAD_STATUS_INVALID_BLDR,	// success; but the bldr is invalid.

	// errors after here are fatal. and process should stop working with bios		
	
	BIOS_LOAD_STATUS_FAILED,		// error occurred while loading the bios.
	BIOS_LOAD_STATUS_FAILED_INVALID_ROMSIZE,
	BIOS_LOAD_STATUS_FAILED_INVALID_BINSIZE,
};

enum PRELDR_STATUS : int {
	PRELDR_STATUS_OK = 0,		// preldr found and is valid and was used to load and decrypt the 2bl.
	PRELDR_STATUS_INVALID_BLDR, // preldr found but the bldr is invalid.
	PRELDR_STATUS_NOT_FOUND,	// preldr not found. old bios (mcpx v1.0) or not a valid bios.

	// errors after here are fatal. and process should stop working with preldr
	
	PRELDR_STATUS_ERROR,		// error occurred while processing the preldr.
};

// Preldr structure
typedef struct Preldr {
	UCHAR* data;
	PRELDR_PARAMS* params;
	PRELDR_ENTRY* entry;
	PUBLIC_KEY* pubkey;
	UINT* bldrEntryOffset;
	UCHAR* romDigest;
	UCHAR* nonce;
	UCHAR bldrKey[SHA1_DIGEST_LEN];
	
	PRELDR_STATUS status;
	USHORT jmpInstr;
	UINT jmpOffset;

	Preldr() {
		reset();
	};
	~Preldr() {
		reset();
	};

	void reset() {
		data = NULL;
		params = NULL;
		entry = NULL;
		pubkey = NULL;
		bldrEntryOffset = NULL;
		romDigest = NULL;
		nonce = NULL;
		
		memset(bldrKey, 0, 20);
		
		status = PRELDR_STATUS_ERROR;
		jmpInstr = 0;
		jmpOffset = 0;
	};
} Preldr;

// 2BL structure
typedef struct Bldr {
	UCHAR* data;
	BLDR_ENTRY* entry;
	BLDR_KEYS* keys;
	BOOT_PARAMS* bootParams;
	BOOT_LDR_PARAM* ldrParams;

	UINT entryPointOffset;
	UINT keysOffset;

	bool encryptionState;
	bool signatureValid;
	bool bootParamsValid;

	Bldr() {
		reset();
	};
	~Bldr() {
		reset();
	};

	void reset() {
		data = NULL;
		entry = NULL;
		keys = NULL;
		bootParams = NULL;
		ldrParams = NULL;
		entryPointOffset = 0;
		keysOffset = 0;
		encryptionState = false;
	};
} Bldr;

// Bios Parameters
typedef struct BiosParams {	
	Mcpx* mcpx;	// the instance of mcpx rom class, instance itself cannot be NULL. but the data can be.
	UINT romsize;	// the size of the rom in kb.
	UCHAR* keyBldr;	// the rc4 key for the bldr. (16 bytes) NULL if not required.
	UCHAR* keyKrnl;	// the rc4 key for the kernel. (16 bytes) NULL if not required.
	bool encBldr;	// if true, the bldr is not encrypted. (will not be decrypted)
	bool encKrnl;	// if true, the kernel is not encrypted. (will not be decrypted)

	BiosParams(Mcpx* mcpx, UINT romsize, UCHAR* keyBldr, UCHAR* keyKrnl, bool encBldr, bool encKrnl) {
		this->mcpx = mcpx;
		this->romsize = romsize;
		this->keyBldr = keyBldr;
		this->keyKrnl = keyKrnl;
		this->encBldr = encBldr;
		this->encKrnl = encKrnl;
	};
	BiosParams() {
		reset();
	};
	~BiosParams() {
		reset();
	};

	void reset() {
		romsize = 0;
		keyBldr = NULL;
		keyKrnl = NULL;
		mcpx = NULL;
	};
} BiosParams;

class Bios {
public:
	Bios() {
		reset();
	};
	~Bios() {
		unload();
	};

	// unload the bios from memory. this will free all allocated memory and reset for reuse.
	inline void unload() {
		if (data != NULL)
		{
			free(data);
		}
		if (decompressedKrnl != NULL)
		{
			free(decompressedKrnl);
		}		

		reset();

		preldr.reset();
		bldr.reset();
	};
	
	// load the bios from a file.
	int loadFromFile(const char* filename, BiosParams biosParams);

	// load the bios from a buffer.
	int loadFromData(const UCHAR* data, const UINT size, BiosParams biosParams);

	// build the bios from a preldr, 2bl, inittbl, krnl and krnl section data.
	int build(UCHAR* in_preldr, UINT in_preldrSize,
		UCHAR* in_2bl, UINT in_2blSize,
		UCHAR* in_inittbl, UINT in_inittblSize,
		UCHAR* in_krnl, UINT in_krnlSize,
		UCHAR* in_krnlData, UINT in_krnlDataSize,
		UINT binsize, bool bfm, BiosParams biosParams);

	int saveBiosToFile(const char* filename);
	int saveBldrBlockToFile(const char* filename);
	int saveBldrToFile(const char* filename, const UINT size);
	int saveKernelToFile(const char* filename);
	int saveKernelDataToFile(const char* filename);
	int saveKernelImgToFile(const char* filename);
	int saveInitTblToFile(const char* filename);

	int decompressKrnl();
	void checkForPreldr();
	int convertToBootFromMedia();
	int replicateBios(UINT binsize);

	UCHAR* getBios() const { return data; };
	UINT getBiosSize() const { return size; };
	BiosParams* getParams() { return &params; };
	
	Bldr* getBldr() { return &bldr; };
	UINT getBldrSize() const { return BLDR_BLOCK_SIZE; };

	Preldr* getPreldr() { return &preldr; };
	UINT getPreldrSize() const { return PRELDR_BLOCK_SIZE; };

	INIT_TBL* getInitTbl() const { return initTbl; };
	ROM_DATA_TBL* getDataTbl() const { return dataTbl; };

	UCHAR* getKrnl() const { return krnl; };
	UCHAR* getKrnlData() const { return krnlData; };
	UCHAR* getDecompressedKrnl() const { return decompressedKrnl; };
	UINT getDecompressedKrnlSize() const { return decompressedKrnlSize; };
	UINT getAvailableSpace() const { return availableSpace; };
	UINT getTotalSpaceAvailable() const { return totalSpaceAvailable; };

	bool isKernelEncrypted() const { return kernelEncryptionState; };
	
private:	
	UCHAR* data;				// allocated bios data
	UINT size;					// the size of the data.
	UINT buffersize;			// the size of the allocated BUF for the bios.

	Bldr bldr;					// bldr struct
	
	INIT_TBL* initTbl;			// pointer to init tbl
	ROM_DATA_TBL* dataTbl;		// pointer to drv slw data tbl
	
	UCHAR* krnl;				// pointer to compressed kernel	
	UCHAR* krnlData;			// pointer to uncompressed kernel section data

	UCHAR* decompressedKrnl;	// allocated decompressed kernel
	UINT decompressedKrnlSize;	// decompressed kernel size
	
	Preldr preldr;				// preldr struct
	
	BiosParams params;			// bios params

	UINT totalSpaceAvailable;	// the total space available in the bios. this is the size of the bios minus the bldr BLOCK
	UINT availableSpace;		// the available space in the bios. 

	bool kernelEncryptionState;
		
	int load(UCHAR* buff, const UINT buffSize, const BiosParams biosParams);
	void getOffsets();
	void getOffsets2(const UINT krnlSize, const UINT krnlDataSize);
	int validateBldr();
	void symmetricEncDecBldr(const UCHAR* key, const UINT keyLen);
	void symmetricEncDecKernel();

	inline void reset() {
		// allocated memory
		data = NULL;
		decompressedKrnl = NULL;

		// sizes
		size = 0;
		totalSpaceAvailable = 0;
		availableSpace = 0;
		decompressedKrnlSize = 0;

		kernelEncryptionState = false;

		initTbl = NULL;
		dataTbl = NULL;
		krnl = NULL;
		krnlData = NULL;
	};
};

int checkBiosSize(const UINT size);

#endif // !XB_BIOS_H
