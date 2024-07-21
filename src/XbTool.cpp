// XbTool.cpp : Implements command functions for the Xbox Bios Tool. Logic for each command is implemented here.

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
#include "XbTool.h"
#include "Bios.h"
#include "Mcpx.h"
#include "XcodeInterp.h"
#include "X86Interp.h"
#include "type_defs.h"
#include "file.h"
#include "util.h"
#include "rc4.h"
#include "nt_headers.h"

const char SUCCESS_OUT[] = "Success! -> OUT: %s %d Kb\n";
const char FAIL_OUT[] = "Error: Failed to write %s\n";

void XbTool::deconstruct()
{
	params.deconstruct();
	bios.deconstruct();
};

int XbTool::run()
{
	print("%s\n\n", cmd->name);

	// read in keys if provided.
	if (readKeys() != 0)
		return 1;

	// read in mcpx rom if provided
	if (readMCPX() != 0)
		return 1;

	int result = 0;

	// run command
	switch (cmd->type)
	{
	case CMD_LIST:
		result = listBios();
		break;

	case CMD_EXTR:
		result = extractBios();
		break;
	case CMD_SPLIT:
		result = splitBios();
		break;
	case CMD_COMBINE:
		result = combineBios();
		break;
	case CMD_BLD_BIOS:
		result = buildBios();
		break;

	case CMD_XCODE_SIM:
		result = simulateXcodes();
		break;

	case CMD_KRNL_DECOMPRESS:
		result = decompressKrnl();
		break;

	case CMD_XCODE_DECODE:
		result = decodeXcodes();
		break;

	default:
		error("Command not implemented\n");
		result = 1;
		break;
	}
	return result;
}

int XbTool::buildBios()
{
	int result = 0;
	const char* filename = params.outFile;
	UCHAR* data = NULL;
	UCHAR* inittbl = NULL;
	UCHAR* bldr = NULL;
	UCHAR* krnl = NULL;
	UCHAR* krnlData = NULL;
	UINT inittblSize = 0;
	UINT bldrSize = 0;
	UINT krnlSize = 0;
	UINT krnlDataSize = 0;
	BOOT_PARAMS* bootParams = NULL;

	// read in the init tbl file
	print("inittbl file:\t%s\n", params.inittblFile);
	inittbl = readFile(params.inittblFile, &inittblSize);
	if (inittbl == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// read in the bldr file
	print("bldr file:\t%s\n", params.bldrFile);
	bldr = readFile(params.bldrFile, &bldrSize);
	if (bldr == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// read in the compressed krnl image
	print("krnl file:\t%s\n", params.krnlFile);
	krnl = readFile(params.krnlFile, &krnlSize);
	if (krnl == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// read in the uncompressed kernel data
	print ("krnl data file:\t%s\n", params.krnlDataFile);
	krnlData = readFile(params.krnlDataFile, &krnlDataSize);
	if (krnlData == NULL)
	{
		result = 1;
		goto Cleanup;
	}
	// required files were read successfully
	
	// create the bios in memory
	result = bios.create(bldr, bldrSize, inittbl, inittblSize, krnl, krnlSize, krnlData, krnlDataSize);
	if (result != 0)
	{
		error("Error: Failed to create bios\n");
		goto Cleanup;
	}

	// write data to file
	filename = params.outFile;
	if (filename == NULL)
	{
		filename = "bios.bin";
	}

	result = bios.saveBiosToFile(filename);
	if (result == 0)
	{
		print(SUCCESS_OUT, filename, bios.getBiosSize() / 1024);
	}
	else
	{
		error(FAIL_OUT, "bios");
	}

Cleanup:
	if (inittbl != NULL)
	{
		xb_free(inittbl);
		inittbl = NULL;
	}
	if (bldr != NULL)
	{
		xb_free(bldr);
		bldr = NULL;
	}
	if (krnl != NULL)
	{
		xb_free(krnl);
		krnl = NULL;
	}
	if (krnlData != NULL)
	{
		xb_free(krnlData);
		krnlData = NULL;
	}
	
	return result;
}

int XbTool::extractBios()
{
	int result = 0;

	print("bios file: %s\n", params.biosFile);
	result = bios.loadFromFile(params.biosFile);
	if (result != Bios::BIOS_LOAD_STATUS_SUCCESS) // bldr needs to be valid to extract
	{
		return 1;
	}

	const char* filename = NULL;

	// bootloader
	if (params.bldrFile != NULL)
		filename = params.bldrFile;
	else
		filename = "bldr.bin";

	result = bios.saveBldrBlockToFile(filename);
	if (result != 0)
	{
		error(FAIL_OUT, "bldr");
		return 1;
	}

	// compressed krnl
	if (params.krnlFile != NULL)
		filename = params.krnlFile;
	else
		filename = "krnl.bin";

	result = bios.saveKernelToFile(filename);
	if (result != 0)
	{
		error(FAIL_OUT, "krnl");
		return 1;
	}

	// uncompressed krnl data
	if (params.krnlDataFile != NULL)
		filename = params.krnlDataFile;
	else
		filename = "krnl_data.bin";

	result = bios.saveKernelDataToFile(filename);
	if (result != 0)
	{
		error(FAIL_OUT, "krnl data");
		return 1;
	}
	
	// init tbl
	if (params.inittblFile != NULL)
		filename = params.inittblFile;
	else
		filename = "inittbl.bin";

	result = bios.saveInitTblToFile(filename);
	if (result != 0)
	{
		error(FAIL_OUT, "init tbl");
		return 1;
	}

	return result;
}

int XbTool::splitBios()
{
	int result = 0;
	UCHAR* data = NULL;
	UINT size = 0;
	UINT fnLen = 0;
	UINT bankFnLen = 0;
	UINT dataLeft = 0;
	UINT loopCount = 0;
	int bank = 0;
	const char suffix[] = "_bank";
	char* biosFn = NULL;
	char* ext = NULL;
	char* bankFn = NULL;

	print("bios file: %s\n", params.biosFile);

	//romsize sanity check
	if (params.romsize < DEFAULT_ROM_SIZE * 1024)
		return 1;
		
	data = readFile(params.biosFile, &size);
	if (data == NULL)
		return 1;

	result = checkSize(size);
	if (result != 0)
	{
		error("Error: Invalid bios file size: %d\n", size);
		goto Cleanup;
	}
	
	// check if bios size is less than or equal to rom size
	if (size <= params.romsize)
	{
		error("Cannot split bios. file size is less than or equal to romsize\n");
		result = 1;
		goto Cleanup;
	}

	print("Splitting bios into %d banks (%dKB each)\n", (size / params.romsize), (params.romsize / 1024));

	fnLen = strlen(params.biosFile);
	biosFn = (char*)xb_alloc(fnLen + 1); // +1 for null terminator
	if (biosFn == NULL)
	{
		result = 1;
		goto Cleanup;
	}
	strcpy(biosFn, params.biosFile);

	// remove the extention
	for (int i = fnLen - 1; i >= 0; i--)
	{
		if (biosFn[i] == '.')
		{
			ext = (char*)xb_alloc(fnLen - i + 1); // +1 for null terminator
			if (ext == NULL)
			{
				result = 1;
				goto Cleanup;
			}
			strcpy(ext, biosFn + i);

			biosFn[i] = '\0';
			break;
		}
	}

	if (ext == NULL)
	{
		error("Error: Invalid bios file name. No file extension found\n");
		result = 1;
		goto Cleanup;
	}

	bankFnLen = strlen(biosFn) + strlen(suffix) + strlen(ext) + 3U; // +3 for the bank number
	bankFn = (char*)xb_alloc(bankFnLen);
	if (bankFn == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// split the bios based on rom size

	dataLeft = size;
	loopCount = 0;
	while (dataLeft > 0)
	{
		// safe guard
		if (loopCount > 4)
		{ 
			error("Error: loopCount exceeded 5\n");
			result = 1;
			break;
		}
		loopCount++;

		// set bank filename = [name][suffix][number].[ext] = bios_bank1.bin
		bankFn[0] = '\0';
		format(bankFn, "%s%s%d%s", biosFn, suffix, bank + 1, ext);

		// write bank to file
		print("Writing bank %d - %s %dKB\n", bank + 1, bankFn, params.romsize / 1024);

		result = writeFile(bankFn, (data + (params.romsize * bank)), params.romsize);
		if (result != 0)
		{
			error(FAIL_OUT, bankFn);
			goto Cleanup;
		}

		dataLeft -= params.romsize;
		bank++;
	}

	print("Bios split into %d banks\n", bank);

Cleanup:
	if (data != NULL)
	{
		xb_free(data);
	}
	if (biosFn != NULL)
	{
		xb_free(biosFn);
	}
	if (bankFn != NULL)
	{
		xb_free(bankFn);
	}
	if (ext != NULL)
	{
		xb_free(ext);
	}

	return result;
}

int XbTool::combineBios()
{
	const UINT MAX_BANKS = 4;
	UINT totalSize = 0;
	UINT offset = 0;

	int i;
	int result = 0;
	int numBanks = 0;

	UCHAR* data = NULL;

	UCHAR* banks[MAX_BANKS] = { NULL };
	UINT bankSizes[MAX_BANKS] = { 0 };

	const char* filename = params.outFile;
	if (filename == NULL)
	{
		filename = "bios.bin";
	}

	for (i = 0; i < MAX_BANKS; i++)
	{
		if (params.bankFiles[i] == NULL)
			continue;

		numBanks++;
		
		// check if file is already loaded in a previous bank
		bool isLoaded = false;
		for (int j = 0; j < i; j++)
		{
			if (params.bankFiles[j] == NULL)
				continue;

			if (strcmp(params.bankFiles[i], params.bankFiles[j]) == 0)
			{
				isLoaded = true;
				banks[i] = (UCHAR*)xb_alloc(bankSizes[j]);
				xb_cpy(banks[i], banks[j], bankSizes[i]);
				bankSizes[i] = bankSizes[j];
				break;
			}
		}

		if (!isLoaded)
		{
			banks[i] = readFile(params.bankFiles[i], &bankSizes[i]);
			if (banks[i] == NULL) // failed to load bank
			{
				result = 1;
				goto Cleanup;
			}

			if (checkSize(bankSizes[i]) != 0)
			{
				error("Error: %s has invalid file size: %d\n", params.bankFiles[i], bankSizes[i]);
				result = 1;
				goto Cleanup;
			}
		}
		totalSize += bankSizes[i];
	}

	if (numBanks < 2)
	{
		error("Error: Not enough banks to combine. Expected atleast 2 banks to be provided\n");
		result = 1;
		goto Cleanup;
	}

	if (checkSize(totalSize) != 0)
	{
		error("Error: Invalid total bios size: %d\n", totalSize);
		result = 1;
		goto Cleanup;
	}

	data = (UCHAR*)xb_alloc(totalSize);
	if (data == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	for (i = 0; i < MAX_BANKS; i++)
	{
		if (banks[i] != NULL)
		{
			print("Copying %s %dKB into offset 0x%x (bank %d)\n", params.bankFiles[i], bankSizes[i] / 1024, offset, i + 1);
			xb_cpy(data + offset, banks[i], bankSizes[i]);
			offset += bankSizes[i];
		}
	}

	result = writeFile(filename, data, totalSize);
	if (result == 0)
	{
		print(SUCCESS_OUT, filename, totalSize / 1024);
	}
	else
	{
		error(FAIL_OUT, "bios");
	}

Cleanup:

	if (data != NULL)
	{
		xb_free(data);
	}

	for (i = 0; i < MAX_BANKS; i++)
	{
		if (banks[i] != NULL)
		{
			xb_free(banks[i]);
			banks[i] = NULL;
		}
	}

	return result;
}

int XbTool::listBios()
{
	print("bios file: %s\n", params.biosFile);
	int result = bios.loadFromFile(params.biosFile);
	if (result > Bios::BIOS_LOAD_STATUS_INVALID_BLDR) // bldr doesn't need to be valid to list it.
	{
		return 1;
	}

	int flag = params.ls_flag;

	if (flag == 0)
	{
		print("\nbios size:\t\t%ld KB\n", bios.getBiosSize() / 1024UL);
		print("rom size:\t\t%ld KB\n", params.romsize / 1024UL);
		print("mcpx block size:\t%ld bytes\n", MCPX_BLOCK_SIZE);
		print("bldr block size:\t%ld bytes\n", BLDR_BLOCK_SIZE);
		print("total space available:\t%ld bytes\n", bios.getTotalSpaceAvailable());

		bios.printBldrInfo();
		bios.printKernelInfo();
		bios.printInitTblInfo();
		bios.printKeys();		
	}

	if (flag & LS_NV2A_TBL)
	{
		bios.printNv2aInfo();
	}

	if (flag & LS_DATA_TBL)
	{
		bios.printDataTbl();
	}

	return 0;
}

UCHAR* XbTool::load_init_tbl_file(UINT& size) const
{
	UCHAR* inittbl = NULL;
	int result = 0;

	// load the inittbl file ( -inittbl or -bios )

	const char* filename = params.inittblFile;
	if (filename == NULL)
	{
		filename = params.biosFile;
		if (filename == NULL)
		{
			error("Error: No inittbl file provided\n");
			return NULL;
		}
	}

	print("inittbl file: %s\n", filename);
	inittbl = readFile(filename, &size);
	if (inittbl == NULL)
		return NULL;

	// validate the inittbl file
	if (size < XCODE_BASE + sizeof(XCODE))
	{
		error("Error: Invalid inittbl size: %d. Expected atleast %d bytes\n", size, XCODE_BASE + sizeof(XCODE));
		result = 1;
		goto Cleanup;
	}
	if (size > MAX_BIOS_SIZE)
	{
		error("Error: Invalid inittbl size: %d. Expected less than %d bytes\n", size, MAX_BIOS_SIZE);
		result = 1;
		goto Cleanup;
	}

Cleanup:

	if (result != 0)
	{
		if (inittbl != NULL)
		{
			xb_free(inittbl);
			inittbl = NULL;
		}

		return NULL;
	}

	return inittbl;
}

int XbTool::decodeXcodes() const
{
	XcodeInterp interp = XcodeInterp();
	XCODE* xcode;
	DECODE_CONTEXT context = { xcode, stdout };

	UCHAR* inittbl = NULL;
	UINT size = 0;
	int result = 0;
	
	inittbl = load_init_tbl_file(size);
	if (inittbl == NULL)
		return 1;
	
	result = interp.load(inittbl + XCODE_BASE, size - XCODE_BASE);
	if (result != 0)
		goto Cleanup;
		
	// decode phase 1
	while (interp.interpretNext(xcode) == XcodeInterp::DATA_OK)
	{
		// go through the xcodes and fix up jmp offsets.

		if (xcode->opcode != XC_JMP && xcode->opcode != XC_JNE)
			continue;

		// calculate the offset and set the data to the offset.

		bool isLabel = false;
		UINT labelOffset = interp.getOffset() + xcode->data;

		// Fix up the xcode data to the offset for phase 2 decoding
		xcode->data = labelOffset;

		// check if offset is already a label
		for (LABEL* label : context.labels)
		{
			if (label->offset == labelOffset)
			{
				isLabel = true;
				break;
			}
		}

		// label does not exist. create one.
		if (!isLabel)
		{
			char str[16] = { 0 };
			format(str, "lb_%02d", context.labels.size());

			LABEL* label_ptr = (LABEL*)xb_alloc(sizeof(LABEL));
			if (label_ptr == NULL)
			{
				result = 1;
				goto Cleanup;
			}
			label_ptr->load(labelOffset, str);

			context.labels.push_back(label_ptr);
		}
	}

	if (interp.getStatus() == XcodeInterp::DATA_ERROR)
	{
		error("Error: Failed to decode xcodes\n");
		result = 1;
		goto Cleanup;
	}

	// load the decode settings from the ini file
	if (interp.loadini(context.settings) != 0)
	{
		result = 1;
		goto Cleanup;
	}

	// setup the file stream, if -d flag is set
	if ((params.sw_flag & SW_DMP) != 0)
	{
		const char* filename = params.outFile;
		if (filename == NULL)
		{
			filename = "xcodes.txt";
		}

		// del file if it exists		
		deleteFile(filename);

		print("Writing decoded xcodes to %s (text format)\n", filename, interp.getOffset());
		context.stream = fopen(filename, "w");
		if (context.stream == NULL)
		{
			error("Error: Failed to open file %s\n", filename);
			result = 1;
			goto Cleanup;
		}
	}

	print("xcodes: %d ( %ld bytes )\n", interp.getOffset() / sizeof(XCODE), interp.getOffset());

	// decode phase 2

	interp.reset();
	while (interp.interpretNext(xcode) == XcodeInterp::DATA_OK)
	{
		interp.decodeXcode(context);
	}

	if ((params.sw_flag & SW_DMP) != 0)
	{
		fclose(context.stream);
		print("Done\n");
	}

Cleanup:

	if (inittbl != NULL)
	{
		xb_free(inittbl);
	}

	context.deconstruct();
	interp.deconstruct();

	return result;
}

int XbTool::simulateXcodes() const
{
	XcodeInterp interp = XcodeInterp();
	XCODE* xcode = NULL;
	
	UINT size = 0;
	int result = 0;
	bool hasMemChanges_total = false;
	char* opcode_str = NULL;
	UCHAR* mem_sim = NULL;
	UCHAR* inittbl = NULL;

	inittbl = load_init_tbl_file(size);
	if (inittbl == NULL)
		return 1;

	result = interp.load(inittbl + XCODE_BASE, size - XCODE_BASE);
	if (result != 0)
		goto Cleanup;

	print("mem space: %d bytes\n\n", params.simSize);

	mem_sim = (UCHAR*)xb_alloc(params.simSize);
	if (mem_sim == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// simulate memory output
	hasMemChanges_total = false;
	print("xcode sim:\n");
	while (interp.interpretNext(xcode) == 0)
	{
		// only care about xcodes that write to RAM
		if (xcode->opcode != XC_MEM_WRITE)
			continue;

		// sanity check of addr.
		if (xcode->addr < 0 || xcode->addr >= params.simSize)
		{
			continue;
		}
		hasMemChanges_total = true;

		// write the data to simulated memory
		xb_cpy(mem_sim + xcode->addr, (UCHAR*)&xcode->data, 4);

		if (interp.getDefOpcodeStr(xcode->opcode, opcode_str) != 0)
		{
			error("Error: Unknown opcode %X\n", xcode->opcode);
			result = 1;
			goto Cleanup;
		}

		// print the xcode
		print("%04x: %s 0x%02x, 0x%08X\n", (XCODE_BASE + interp.getOffset() - sizeof(XCODE)), opcode_str, xcode->addr, xcode->data);
	}

	if (interp.getStatus() != XcodeInterp::EXIT_OP_FOUND)
	{
		result = 1;
		goto Cleanup;
	}

	if (!hasMemChanges_total)
	{
		print("No Memory changes in range 0x0 - 0x%x\n", params.simSize);
		goto Cleanup;
	}

	// decode the x86 instructions.
	result = decodeX86(mem_sim, params.simSize, stdout);
	if (result != 0)
	{
		error("Error: Failed to decode x86 instructions\n");
		goto Cleanup;
	}

	print("size of code: %d bytes\n", params.simSize);

	// if -d flag is set, dump the memory to a file, otherwise print the memory dump
	if ((params.sw_flag & SW_DMP) != 0)
	{
		const char* filename = params.outFile;
		if (filename == NULL)
			filename = "mem_sim.bin";

		print("\nWriting memory dump to %s (%d bytes)\n", filename, size);
		if (writeFile(filename, mem_sim, size) != 0)
		{
			result = 1;
			goto Cleanup;
		}
	}
	else
	{
		// print memory dump
		print("\nmem dump:\n");
		for (UINT i = 0; i < params.simSize; i += 8)
		{
			print("%04x: ", i);
			printData(mem_sim + i, 8);
		}
	}

Cleanup:

	if (inittbl != NULL)
	{
		xb_free(inittbl);
	}

	if (mem_sim != NULL)
	{
		xb_free(mem_sim);
	}

	interp.deconstruct();

	return result;
}

int XbTool::decompressKrnl()
{
	int result = 0;

	print("bios file: %s\n", params.biosFile);
	result = bios.loadFromFile(params.biosFile);
	if (result != Bios::BIOS_LOAD_STATUS_SUCCESS) // bldr needs to be valid to decompress kernel. 
	{
		return 1;
	}

	// decompress the kernel
	if (bios.decompressKrnl() != 0)
	{
		error("Error: Failed to decompress the kernel image\n");
		return 1;
	}
	
	UCHAR* decompressedKrnl = bios.getDecompressedKrnl();
	UINT decompressedSize = bios.getDecompressedKrnlSize();

	UINT offset = ((IMAGE_DOS_HEADER*)decompressedKrnl)->e_lfanew;

	if (offset < 0 || offset > decompressedSize)
	{
		error("Error: Invalid NT Header offset\n");
		return 1;
	}

	IMAGE_NT_HEADER* header = (IMAGE_NT_HEADER*)(decompressedKrnl + offset);
	if (header == NULL)
	{
		error("Error: NT Header is NULL\n");
		return 1;
	}

	// check header is in bounds of allocated memory
	if ((UCHAR*)header < decompressedKrnl || (UCHAR*)header > decompressedKrnl + decompressedSize)
	{
		error("Error: NT Header out of bounds\n");
		return 1;
	}

	// check if it's a valid nt header
	if (&header->signature == NULL || header->signature != IMAGE_NT_SIGNATURE)
	{
		error("Error: NT Header is invalid\n");
		return 1;
	}

	print("\nNT Header:\n");
	print_nt_header_basic(header);

	result = extractPubKey(decompressedKrnl, decompressedSize);

	// get the filename
	const char* filename = params.outFile;
	if (filename == NULL)
	{
		// try krnl file
		filename = params.krnlFile;
		if (filename == NULL)
		{
			filename = "krnl.img";
		}
	}

	// write out the kernel image to file
	result = writeFile(filename, decompressedKrnl, decompressedSize);
	if (result == 0)
	{
		print(SUCCESS_OUT, filename, decompressedSize / 1024);
	}
	else
	{
		error(FAIL_OUT, "kernel image");
	}

	return result;
}

int XbTool::readKeys()
{
	// read in rc4 keys that have been provided on the command line

	if (params.keyBldrFile != NULL)
	{
		print("bldr key file: %s\n", params.keyBldrFile);
		params.keyBldr = readFile(params.keyBldrFile, NULL, KEY_SIZE);
		if (params.keyBldr == NULL)
			return 1;
		print("bldr key: ");
		printData(params.keyBldr, KEY_SIZE);
	}

	if (params.keyKrnlFile != NULL)
	{
		print("krnl key file: %s\n", params.keyKrnlFile);
		params.keyKrnl = readFile(params.keyKrnlFile, NULL, KEY_SIZE);
		if (params.keyKrnl == NULL)
			return 1;
		print("krnl key: ");
		printData(params.keyKrnl, KEY_SIZE);
	}

	if (params.keyBldr != NULL || params.keyKrnl != NULL)
		print("\n");

	return 0;
}

int XbTool::readMCPX()
{
	// read in the mcpx file that has been provided on the command line

	if (params.mcpxFile == NULL)
		return 0;

	if (params.mcpx.load(params.mcpxFile) != 0)
		return 1;
	return 0;
}

int XbTool::extractPubKey(UCHAR* data, UINT size)
{
	// do nothing if a public key file is not provided
	if (params.pubKeyFile == NULL)
	{
		return 0;
	}

	if (data == NULL || size == 0)
	{
		error("Error: Invalid data\n");
		return 1;
	}

	PUBLIC_KEY* pubkey = NULL;
	UINT i;

	// search for the RSA1 header
	for (i = 0; i < size - sizeof(PUBLIC_KEY); i++)
	{
		if (verifyPubKey(data + i, pubkey) == 0)
		{
			break;
		}
	}

	if (pubkey == NULL)
	{
		print("Public key not found\n");
		return 1;
	}

	print("Public key found at offset 0x%x\n", i);
	printPubKey(pubkey);

	// if the '-patchkeys' flag is set, then use that key file to patch the pubkey.
	// otherwise, if the '-patchkeys' flag is not set then save the found pubkey to that file (unmodified).
	if ((params.sw_flag & SW_PATCH_KEYS) == 0)
	{
		print("Writing public key to %s (%d bytes)\n", params.pubKeyFile, sizeof(PUBLIC_KEY));
		if (writeFile(params.pubKeyFile, pubkey, sizeof(PUBLIC_KEY)) != 0)
		{
			error(FAIL_OUT, "public key");
			return 1;
		}
	}
	else
	{
		// patch the public key
		print("\nPatching public key..\n");

		PUBLIC_KEY* patch_pubkey = (PUBLIC_KEY*)readFile(params.pubKeyFile, NULL, sizeof(PUBLIC_KEY));
		if (patch_pubkey == NULL)
		{
			error("Error: Failed to patch public key\n");
			return 1;
		}

		if (xb_cmp(pubkey->modulus, patch_pubkey->modulus, sizeof(pubkey->modulus)) == 0)
		{
			print("Public key matches patch key\n");
		}
		else
		{
			xb_cpy(pubkey->modulus, patch_pubkey->modulus, sizeof(pubkey->modulus));
			printPubKey(pubkey);
			print("Public key patched.\n");
		}

		xb_free(patch_pubkey);
	}

	return 0;
}

int verifyPubKey(UCHAR* data, PUBLIC_KEY*& pubkey)
{
	// verify the public key header.

	if (data == NULL)
	{
		error("Error: Public key is NULL\n");
		return 1;
	}

	const RSA_HEADER RSA1_HEADER = { { 'R', 'S', 'A', '1' }, 264, 2048, 255, 65537 };

	if (xb_cmp(data, &RSA1_HEADER, sizeof(RSA_HEADER)) != 0)
	{
		return 1;
	}

	// update the public key struct
	pubkey = (PUBLIC_KEY*)data;

	return 0;
}

int printPubKey(PUBLIC_KEY* pubkey)
{
	const int bytesPerLine = 16;
	char line[bytesPerLine * 3 + 1] = { 0 };

	// print the public key to the console	
	for (int i = 0; i < 264; i += bytesPerLine)
	{
		for (UINT j = 0; j < bytesPerLine; j++)
		{
			if (i + j >= 264)
				break;
			format(line + (j * 3), "%02X ", pubkey->modulus[i + j]);
		}
		print("%s\n", line);
	}

	return 0;
}
