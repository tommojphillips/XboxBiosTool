// Bios.h: handling An Xbox BIOS.

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

#include <stdint.h>
#include <malloc.h>

// user incl
#include "Mcpx.h"
#include "file.h"
#include "bldr.h"
#include "rsa.h"
#include "sha1.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#endif

#define MIN_BIOS_SIZE 0x40000                                                    // Min bios file/rom size in bytes
#define MAX_BIOS_SIZE 0x100000                                                   // Max bios file/rom size in bytes
#define KD_DELAY_FLAG 0x80000000                                                 // Kernel Delay Flag

#define ROM_DIGEST_SIZE 0x100                                                    // rom digest size in bytes

#define PRELDR_BLOCK_SIZE 0x2A00                                                 // preldr block size in bytes
#define PRELDR_PARAMS_SIZE 0x80                                                  // preldr params size in bytes
#define PRELDR_SIZE (PRELDR_BLOCK_SIZE - ROM_DIGEST_SIZE - PRELDR_PARAMS_SIZE)   // preldr size in bytes
#define PRELDR_NONCE_SIZE 0x10                                                   // preldr nonce size in bytes
#define PRELDR_REAL_BASE (0xFFFFFFFF - MCPX_BLOCK_SIZE - PRELDR_BLOCK_SIZE + 1)  // preldr rom base address		; FFFF.D400
#define PRELDR_REAL_END (PRELDR_REAL_BASE + PRELDR_SIZE)					     // preldr rom end address		; FFFF.FC80

#define BLDR_BLOCK_SIZE 0x6000                                                   // 2BL block size in bytes
#define BLDR_RELOC 0x00400000                                                    // 2BL relocation base address
#define BLDR_BASE 0x00090000                                                     // 2BL boot base address
#define BLDR_REAL_BASE (0xFFFFFFFF - MCPX_BLOCK_SIZE - BLDR_BLOCK_SIZE + 1)      // 2BL rom base address

#define BOOT_SIGNATURE 2018801994U // J y T x

// BIOS load status codes
#define	BIOS_LOAD_STATUS_SUCCESS		0 // success; the bios is loaded.
#define	BIOS_LOAD_STATUS_INVALID_BLDR	1 // success; but the bldr is invalid.
#define	BIOS_LOAD_STATUS_FAILED			2 // ERROR

// Preldr status codes
#define	PRELDR_STATUS_BLDR_DECRYPTED	0 // found and was used to load and decrypt the 2bl.
#define PRELDR_STATUS_FOUND				1 // found but was not used to load the 2bl.
#define PRELDR_STATUS_NOT_FOUND			2 // not found. old bios (mcpx v1.0) or not a valid bios.
#define PRELDR_STATUS_ERROR				3 // ERROR

// xbox public key structure
typedef struct _XB_PUBLIC_KEY {
	RSA_HEADER header;		// rsa header structure
	uint8_t modulus[264];   // pointer to the modulus.
} XB_PUBLIC_KEY;

// Preldr structure
typedef struct Preldr {
	uint8_t* data;
	PRELDR_PARAMS* params;
	PRELDR_PTR_BLOCK* ptrBlock;
	PRELDR_FUNC_BLOCK* funcBlock;
	XB_PUBLIC_KEY* pubkey;
	uint8_t* entryPoint;
	uint8_t bldrKey[SHA1_DIGEST_LEN];
	uint32_t jmpOffset;
	int status;
} Preldr;

inline void initPreldr(Preldr* preldr) {
	memset(preldr, 0, sizeof(Preldr));
	preldr->status = PRELDR_STATUS_ERROR;
}

// 2BL structure
typedef struct Bldr {
	uint8_t* data;
	BLDR_ENTRY* entry;
	uint8_t* bfmKey;
	BLDR_KEYS* keys;
	BOOT_PARAMS* bootParams;
	BOOT_LDR_PARAM* ldrParams;
	uint32_t entryOffset;
	uint32_t keysOffset;
	bool encryptionState;
	bool krnlSizeValid;
	bool krnlDataSizeValid;
	bool inittblSizeValid;
	bool signatureValid;
	bool bootParamsValid;
} Bldr;

inline void initBldr(Bldr* bldr) {
	memset(bldr, 0, sizeof(Bldr));
};

// Bios load parameters
typedef struct BiosParams {	
	uint32_t romsize;
	uint8_t* keyBldr;
	uint8_t* keyKrnl;
	Mcpx* mcpx;
	bool encBldr;
	bool encKrnl;
	bool restoreBootParams;
} BiosParams;

inline void initBiosParams(BiosParams* params) {
	memset(params, 0, sizeof(BiosParams));
	params->restoreBootParams = true;
};

// Bios build parmeters 
typedef struct BiosBuildParams {
	uint8_t* inittbl;
	uint8_t* preldr;
	uint8_t* bldr;
	uint8_t* krnl;
	uint8_t* krnlData;
	uint8_t* eepromKey;
	uint8_t* certKey;
	uint32_t preldrSize;
	uint32_t bldrSize; 
	uint32_t inittblSize;
	uint32_t krnlSize;
	uint32_t krnlDataSize;
	bool bfm;
	bool hackinittbl;
	bool hacksignature;
	bool nobootparams;
} BiosBuildParams;

inline void initBiosBuildParams(BiosBuildParams* params) {
	memset(params, 0, sizeof(BiosBuildParams));
};
inline void freeBiosBuildParams(BiosBuildParams* params) {
	if (params->preldr != NULL)	{
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
};

// Bios
class Bios {
public:
	BiosParams params;
	uint8_t* data;
	uint32_t size;
	Bldr bldr;
	Preldr preldr;
	INIT_TBL* initTbl;
	uint8_t* krnl;
	uint8_t* krnlData;
	uint8_t* romDigest;
	int availableSpace;
	uint8_t* decompressedKrnl;
	uint32_t decompressedKrnlSize;
	bool kernelEncryptionState;

	Bios() {
		resetValues();
	};
	~Bios() {
		unload();
	};

	// reset bios; reset values.
	inline void resetValues() {
		initBiosParams(&params);
		initPreldr(&preldr);
		initBldr(&bldr);

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
	};

	// unload the bios. reset values and free memory.
	inline void unload() {
		if (data != NULL) {
			free(data);
		}

		if (decompressedKrnl != NULL) {
			free(decompressedKrnl);
		}

		resetValues();
	};
	
	// load bios from memory.
	int load(uint8_t* buff, const uint32_t binsize, const BiosParams* biosParams);

	// load bios from file.
	int loadFromFile(const char* filename, const BiosParams* biosParams);
		
	// build bios. 
	int build(BiosBuildParams* buildParams, uint32_t binsize, BiosParams* biosParams);

	// initialize bios and calculate offsets and pointers.
	int init(uint8_t* buff, const uint32_t binsize, const BiosParams* biosParams);
		
	// calculate initial offsets for the bios. based on params.
	// sets up bldr and preldr structs, initTbl and dataTbl pointers.
	// Should be invoked when the bios is loaded.
	void getOffsets();
	
	// calculate offsets and pointers. base on bios data.
	// sets up 2bl, krnl and krnlData pointers.
	// Should be invoked when the 2bl is valid. (not encrypted).
	void getOffsets2();
	
	// validate the 2BL boot param sizes and romsize.
	int validateBldrBootParams();

	// preldr create the bldr key
	void preldrCreateKey(uint8_t* sbkey, uint8_t* key);

	// preldr symmetric encryption and decryption for the 2BL.
	void preldrSymmetricEncDecBldr(const uint8_t* key, const uint32_t len);

	// validate the preldr and decrypt the 2bl.
	// sets up the preldr struct and decrypts the 2bl.
	void preldrValidateAndDecryptBldr();

	// symmetric encryption and decryption for the 2BL.
	void symmetricEncDecBldr(const uint8_t* key, const uint32_t len);

	// symmetric encryption and decryption for the kernel.
	void symmetricEncDecKernel();

	// decompress the kernel image from the bios.
	// stores results in decompressedKrnl pointer and decompressedKrnlSize.
	// returns 0 if successful,
	int decompressKrnl();

	// replicate the bios with a new size.
	// buffSize: the new size of the bios.
	// returns 0 if successful,
	int replicateBios(uint32_t buffSize);

	// preldr decrypt preldr public key.
	int preldrDecryptPublicKey();

	// print current state of the bios.
	int printState();
	
	inline int saveKernelToFile(const char* filename) { return writeFileF(filename, krnl, bldr.bootParams->krnlSize); };
	inline int saveKernelDataToFile(const char* filename) { return writeFileF(filename, krnlData, bldr.bootParams->krnlDataSize); };
};

int checkBiosSize(const uint32_t size);
int validateRequiredSpace(const uint32_t requiredSpace, uint32_t* size);
int replicateData(uint32_t from, uint32_t to, uint8_t* buffer, uint32_t buffersize);

#endif // !XB_BIOS_H
