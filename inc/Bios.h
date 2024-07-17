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

#include "type_defs.h"
#include "xbmem.h"

#include "bldr.h"

const UINT MAX_BIOS_SIZE = 0x100000;	// 1MB max bios size
const UINT KD_DELAY_FLAG = 0x80000000;	// flag for bfm kernel.

const UINT BLDR_BLOCK_SIZE = 24576;         // bldr block size in bytes
const UINT PRELDR_BLOCK_SIZE = 10752;       // preldr block size in bytes
const UINT DEFAULT_ROM_SIZE = 256;          // default rom size in Kb
const UINT BLDR_RELOC = 0x00400000;         // relocation address for the bldr
const UINT BLDR_BASE = 0x00090000; 	    // base address for the bldr

class Bios
{
public:
	Bios()
	{
		// allocated memory
		_bios = NULL;
		decompressedKrnl = NULL;
		preldr_key = NULL;

		// pointers, offsets
		reset();
	};
	~Bios()
	{
		deconstruct();
	};
	
	enum BIOS_LOAD_STATUS : int
	{
		BIOS_LOAD_STATUS_SUCCESS = 0,		// The bios was loaded successfully and the bldr is valid.
		BIOS_LOAD_STATUS_INVALID_BLDR = 1,	// The bios was loaded successfully but the bldr is invalid.
		
		// errors after here are fatal. and process should stop working with bios
		
		BIOS_LOAD_STATUS_FAILED = 2,		// The bios failed to load. catastrophic error.
		BIOS_LOAD_STATUS_FAILED_ALREADY_LOADED = 3, // The bios failed to load. It is already loaded.
		BIOS_LOAD_STATUS_FAILED_INVALID_ROMSIZE = 4, // The bios failed to load. The rom size is invalid.
		BIOS_LOAD_STATUS_FAILED_INVALID_BINSIZE = 5, // The bios failed to load. The bin size is invalid.
	};
	enum PRELDR_STATUS : int
	{
		PRELDR_STATUS_FOUND = 2,		// bios contains a preldr
		PRELDR_STATUS_NOT_FOUND = 0,	// bios does not contain a preldr
		PRELDR_STATUS_ERROR = 1			// preldr error; cannot continue.
	};

	void deconstruct();

	int create(UCHAR* in_bl, UINT in_blSize, UCHAR* in_tbl, UINT in_tblSize, UCHAR* in_k, UINT in_kSize, UCHAR* in_kData, UINT in_kDataSize);

	int loadFromFile(const char* filename);
	int loadFromData(const UCHAR* data, const UINT size);

	int saveBiosToFile(const char* filename);
	int saveBldrBlockToFile(const char* filename);
	int saveBldrToFile(const char* filename, const UINT size);
	int saveKernelToFile(const char* filename);
	int saveKernelDataToFile(const char* filename);
	int saveKernelImgToFile(const char* filename);
	int saveInitTblToFile(const char* filename);

	void printBldrInfo();
	void printKernelInfo();
	void printInitTblInfo();
	void printNv2aInfo();
	void printDataTbl();
	void printKeys();

	int decompressKrnl();

	int checkForPreldr();

	int convertToBootFromMedia();

	int replicateBios();

	UCHAR* getBiosData() const { return _bios; };
	UINT getBiosSize() const { return _size; };

	UCHAR* getDecompressedKrnl() const { return decompressedKrnl; };
	UINT getDecompressedKrnlSize() const { return _decompressedKrnlSize; };

	UINT getAvailableSpace() const { return _availableSpace; };
	UINT getTotalSpaceAvailable() const { return _totalSpaceAvailable; };

	bool isBootParamsValid() const { return _isBootParamsValid; };

	bool isLoaded() const { return _bios != NULL; };
	
	bool isBldrEncrypted() const { return _isBldrEncrypted; };
	bool isKernelEncrypted() const { return _isKernelEncrypted; };
	
private:	
	UCHAR* _bios;				// The allocated bios data	

	UCHAR* decompressedKrnl;	// The allocated decompressed kernel

	UCHAR* bldr;				// pointer to the bldr _bios
	BLDR_ENTRY* bldrEntry;		// pointer to the bldr entry struct in the bldr
	BLDR_KEYS* bldrKeys;		// pointer to the keys struct in the bldr

	BOOT_PARAMS* bootParams;	// pointer to the boot parameters in the bldr
	BOOT_LDR_PARAM* ldrParams;	// pointer to the boot loader parameters in the bldr
	
	INIT_TBL* initTbl;			// pointer to the init tbl in _bios (nv2a, mcpx, xcodes)
	
	ROM_DATA_TBL* dataTbl;		// pointer to the drv slw data tbl in _bios

	UCHAR* krnl;				// pointer to the compressed kernel in _bios	
	UCHAR* krnlData;			// pointer to the uncompressed kernel data in _bios

	UCHAR* preldr;				// pointer to the preldr in _bios
	PRELDR_PARAMS* preldrParams;// pointer to the preldr parameters in _bios
	PRELDR_ENTRY* preldrEntry;	// pointer to the preldr entry in _bios
	UCHAR* preldr_key;			// allocated preldr key

	UINT _size;					// the size of the bios.	
	UINT _buffersize;			// the size of the allocated BUF for the bios.
	UINT _decompressedKrnlSize;	// The size of the decompressed kernel
	UINT _totalSpaceAvailable;	// the total space available in the bios. this is the size of the bios minus the bldr BLOCK
	UINT _availableSpace;		// the available space in the bios. 

	bool _isBldrEncrypted;		// true if the bldr is encrypted.
	bool _isKernelEncrypted;	// true if the kernel is encrypted.

	bool _isBldrSignatureValid;	// true if the bldr signature is valid.
	bool _isBootParamsValid;	// true if the bldr is valid.
		
	int load(UCHAR* data, const UINT size);	// load the bios data

	void getOffsets();
	void getOffsets2(const UINT krnlSize, const UINT krnlDataSize);

	int validateBldr();

	void symmetricEncDecBldr(const UCHAR* key, const UINT keyLen);
	void symmetricEncDecKernel();

	void reset();
};

#endif // !XB_BIOS_H
