// Bios.h 

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_BIOS_H
#define XB_BIOS_H

#include "type_defs.h"
#include "xbmem.h"

#include "bldr.h"

#define BIOS_LOAD_STATUS_SUCCESS		0	// The bios was loaded successfully and the bldr is valid.
#define BIOS_LOAD_STATUS_INVALID_BLDR	1	// The bios was loaded successfully but the bldr is invalid.
#define BIOS_LOAD_STATUS_FAILED			2	// The bios failed to load.

#define PRELDR_STATUS_FOUND		2		// bios contains a preldr
#define PRELDR_STATUS_NOT_FOUND	0		// bios does not contain a preldr
#define PRELDR_STATUS_ERROR		1		// preldr error; cannot continue.

class Bios
{
public:
	Bios()
	{
		// allocated memory
		_bios = NULL;
		decompressedKrnl = NULL;

		// pointers, offsets
		reset();
	};
	~Bios()
	{
		deconstruct();
	};
	void deconstruct();

	int create(UCHAR* in_bl, UINT in_blSize, UCHAR* in_tbl, UINT in_tblSize, UCHAR* in_k, UINT in_kSize, UCHAR* in_kData, UINT in_kDataSize);

	int loadFromFile(const char* path);	
	int loadFromData(const UCHAR* data, const UINT size);

	int saveBiosToFile(const char* path);

	int saveBldrBlockToFile(const char* filename);
	int saveBldrToFile(const char* filename, const UINT size);
	int saveKernelToFile(const char* filename);
	int saveKernelDataToFile(const char* path); 
	int saveKernelImgToFile(const char* path);
	int saveInitTblToFile(const char* filename);

	void symmetricEncDecBldr(const UCHAR* key, const UINT keyLen);

	void printBldrInfo();
	void printKernelInfo();
	void printInitTblInfo();
	void printNv2aInfo();

	void printXcodes();
	void printDataTbl();

	int decompressKrnl();

	int checkForPreldr();

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

	UINT _size;					// the size of the bios.	
	UINT _decompressedKrnlSize;	// The size of the decompressed kernel
	ULONG _totalSpaceAvailable; // the total space available in the bios. this is the size of the bios minus the bldr BLOCK
	ULONG _availableSpace;		// the available space in the bios. 

	bool _isBldrEncrypted;		// true if the bldr is encrypted.
	bool _isKernelEncrypted;	// true if the kernel is encrypted.

	bool _isBldrSignatureValid;	// true if the bldr signature is valid.
	bool _isBootParamsValid;	// true if the bldr is valid.
		
	int load(UCHAR* data, UINT size);	// load the bios data

	void getOffsets();
	void getOffsets2(const UINT krnlSize, const UINT krnlDataSize);

	int validateBldr();

	int decryptKernel();

	void reset();
};

#endif // !XB_BIOS_H
