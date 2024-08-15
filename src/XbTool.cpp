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
#include "rsa.h"
#include "nt_headers.h"

const char SUCCESS_OUT[] = "Success! -> OUT: %s %d Kb\n";
const char FAIL_OUT[] = "Error: Failed to write %s\n";


int XbTool::run()
{
	printf("%s\n", cmd->sw);

	if (readKeys() != 0)
		return 1;

	if (readMCPX() != 0)
		return 1;

	int result = 0;

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
	case CMD_ENCODE_X86:
		result = encodeX86();
		break;
	case CMD_DUMP_NT_IMG:
		result = dumpImg(NULL, 0);
		break;
	default:
		result = 1;
		break;
	}
	return result;
}

int XbTool::buildBios()
{
	int result = 0;
	const char* filename = params.outFile;
	UCHAR* inittbl = NULL;
	UCHAR* bldr = NULL;
	UCHAR* krnl = NULL;
	UCHAR* krnlData = NULL;
	UINT inittblSize = 0;
	UINT bldrSize = 0;
	UINT krnlSize = 0;
	UINT krnlDataSize = 0;
	
	// read in the init tbl file
	printf("inittbl file:\t%s\n", params.inittblFile);
	inittbl = readFile(params.inittblFile, &inittblSize);
	if (inittbl == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// read in the bldr file
	printf("bldr file:\t%s\n", params.bldrFile);
	bldr = readFile(params.bldrFile, &bldrSize);
	if (bldr == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// read in the compressed krnl image
	printf("krnl file:\t%s\n", params.krnlFile);
	krnl = readFile(params.krnlFile, &krnlSize);
	if (krnl == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// read in the uncompressed kernel data
	printf("krnl data file:\t%s\n", params.krnlDataFile);
	krnlData = readFile(params.krnlDataFile, &krnlDataSize);
	if (krnlData == NULL)
	{
		result = 1;
		goto Cleanup;
	}
	// required files were read successfully
	
	// build the bios in memory
	result = bios.build(bldr, bldrSize, inittbl, inittblSize, krnl, krnlSize, krnlData, krnlDataSize);
	if (result != 0)
	{
		printf("Error: Failed to build bios\n");
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
		printf(SUCCESS_OUT, filename, bios.getBiosSize() / 1024);
	}
	else
	{
		printf(FAIL_OUT, "bios");
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

	printf("bios file: %s\n", params.inFile);
	result = bios.loadFromFile(params.inFile);
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
		printf(FAIL_OUT, "bldr");
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
		printf(FAIL_OUT, "krnl");
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
		printf(FAIL_OUT, "krnl data");
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
		printf(FAIL_OUT, "init tbl");
		return 1;
	}

	return result;
}

int XbTool::splitBios() const
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
	int i;
	int j;

	printf("bios file: %s\n", params.inFile);

	//romsize sanity check
	if (params.romsize < DEFAULT_ROM_SIZE * 1024)
		return 1;
		
	data = readFile(params.inFile, &size);
	if (data == NULL)
		return 1;

	result = checkSize(size);
	if (result != 0)
	{
		printf("Error: Invalid bios file size: %d\n", size);
		goto Cleanup;
	}
	
	// check if bios size is less than or equal to rom size
	if (size <= params.romsize)
	{
		printf("Cannot split bios. file size is less than or equal to romsize\n");
		result = 1;
		goto Cleanup;
	}

	printf("Splitting bios into %d banks (%dKB each)\n", (size / params.romsize), (params.romsize / 1024));

	fnLen = strlen(params.inFile);
	biosFn = (char*)xb_alloc(fnLen + 1);
	if (biosFn == NULL)
	{
		result = 1;
		goto Cleanup;
	}
	for (j = fnLen - 1; j > 0; j--)
	{
		if (params.inFile[j] == '\\')
		{
			j += 1;
			break;
		}
	}
	strcpy(biosFn, params.inFile+j);
	fnLen -= j;

	// remove the extention
	for (i = fnLen - 1; i >= 0; i--)
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
		printf("Error: Invalid bios file name. No file extension found\n");
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
			printf("Error: loopCount exceeded 5\n");
			result = 1;
			break;
		}
		loopCount++;

		// set bank filename = [name][suffix][number].[ext] = bios_bank1.bin
		bankFn[0] = '\0';
		sprintf(bankFn, "%s%s%d%s", biosFn, suffix, bank + 1, ext);

		// write bank to file
		printf("Writing bank %d - %s %dKB\n", bank + 1, bankFn, params.romsize / 1024);

		result = writeFile(bankFn, (data + (params.romsize * bank)), params.romsize);
		if (result != 0)
		{
			printf(FAIL_OUT, bankFn);
			goto Cleanup;
		}

		dataLeft -= params.romsize;
		bank++;
	}

	printf("Bios split into %d banks\n", bank);

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

int XbTool::combineBios() const
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
		
		banks[i] = readFile(params.bankFiles[i], &bankSizes[i]);
		if (banks[i] == NULL)
		{
			result = 1;
			goto Cleanup;
		}

		if (checkSize(bankSizes[i]) != 0)
		{
			printf("Error: %s has invalid file size: %d\n", params.bankFiles[i], bankSizes[i]);
			result = 1;
			goto Cleanup;
		}
		
		totalSize += bankSizes[i];
	}

	if (numBanks < 2)
	{
		printf("Error: Not enough banks to combine. Expected atleast 2 banks to be provided\n");
		result = 1;
		goto Cleanup;
	}

	if (checkSize(totalSize) != 0)
	{
		printf("Error: Invalid total bios size: %d\n", totalSize);
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
			printf("Copying %s %dKB into offset 0x%x (bank %d)\n", params.bankFiles[i], bankSizes[i] / 1024, offset, i + 1);
			memcpy(data + offset, banks[i], bankSizes[i]);
			offset += bankSizes[i];
		}
	}

	result = writeFile(filename, data, totalSize);
	if (result == 0)
	{
		printf(SUCCESS_OUT, filename, totalSize / 1024);
	}
	else
	{
		printf(FAIL_OUT, "bios");
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
	printf("bios file: %s\n", params.inFile);
	int result = bios.loadFromFile(params.inFile);
	if (result > Bios::BIOS_LOAD_STATUS_INVALID_BLDR) // bldr doesn't need to be valid to list it.
	{
		return 1;
	}

	if (!isFlagSetAny(CLI_LS_FLAGS, params.sw_flags))
	{
		printf("\nbios size:\t\t%ld KB\n", bios.getBiosSize() / 1024UL);
		printf("rom size:\t\t%ld KB\n", params.romsize / 1024UL);
		printf("mcpx block size:\t%ld bytes\n", MCPX_BLOCK_SIZE);
		printf("bldr block size:\t%ld bytes\n", BLDR_BLOCK_SIZE);
		printf("total space available:\t%ld bytes\n", bios.getTotalSpaceAvailable());

		bios.printBldrInfo();
		bios.printKernelInfo();
		bios.printInitTblInfo();
		bios.printKeys();	
	}

	if (isFlagSet(SW_LS_NV2A_TBL, params.sw_flags))
	{
		bios.printNv2aInfo();
	}

	if (isFlagSet(SW_LS_DATA_TBL, params.sw_flags))
	{
		bios.printDataTbl();
	}

	if (isFlagSet(SW_LS_DUMP_KRNL, params.sw_flags))
	{
		result = bios.decompressKrnl();
		if (result != 0)
		{
			printf("Error: Failed to decompress the kernel image\n");
			return result;
		}

		UCHAR* decompressedKrnl = bios.getDecompressedKrnl();

		dumpImg(decompressedKrnl, bios.getDecompressedKrnlSize());
		print_krnl_data_section_header((IMAGE_DOS_HEADER*)decompressedKrnl);
	}

	return 0;
}

UCHAR* XbTool::load_init_tbl_file(UINT& size, UINT& xcodeBase) const
{
	UCHAR* inittbl = NULL;
	int result = 0;

	printf("inittbl file: %s\n", params.inFile);

	inittbl = readFile(params.inFile, &size);
	if (inittbl == NULL)
		return NULL;

	if (isFlagSet(SW_XCODE_BASE, params.sw_flags))
	{
		xcodeBase = params.xcodeBase;
	}
	else
	{
		if (size < sizeof(INIT_TBL))
		{
			xcodeBase = 0;
		}
		else
		{
			xcodeBase = sizeof(INIT_TBL);
		}
	}

	if (xcodeBase >= size)
	{
		printf("Error: Invalid xcode base: %d. Expected less than %d bytes\n", xcodeBase, size);
		result = 1;
		goto Cleanup;
	}

	if (isFlagClear(SW_NO_MAX_SIZE, params.sw_flags) && size > MAX_BIOS_SIZE)
	{
		printf("Error: Invalid inittbl size: %d. Expected less than %d bytes\n", size, MAX_BIOS_SIZE);
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
	DECODE_CONTEXT context = { NULL, stdout };

	UCHAR* inittbl = NULL;
	UINT size = 0;
	UINT xcodeNum = 0;
	int result = 0;

	inittbl = load_init_tbl_file(size, context.base);
	if (inittbl == NULL)
		return 1;

	result = interp.load(inittbl + context.base, size - context.base);
	if (result != 0)
		goto Cleanup;

	printf("xcode base: 0x%x\n", context.base);

	result = interp.initDecoder(params.settingsFile, context);
	if (result != 0)
	{
		printf("Error: Failed to init xcode decoder\n");
		goto Cleanup;
	}

	xcodeNum = interp.getOffset() / sizeof(XCODE);
	printf("xcodes: %d ( %ld bytes )\n", xcodeNum, interp.getOffset());
	
	// setup the file stream, if -d flag is set
	if (isFlagSet(SW_DMP, params.sw_flags))
	{
		const char* filename = params.outFile;
		if (filename == NULL)
		{
			filename = "xcodes.txt";
		}

		// del file if it exists
		deleteFile(filename);

		printf("Writing xcodes to %s\n", filename);
		context.stream = fopen(filename, "w");
		if (context.stream == NULL)
		{
			printf(FAIL_OUT, filename);
			result = 1;
			goto Cleanup;
		}
	}
	else // print to stdout
	{
		if (xcodeNum > 2048)
		{
			printf("Error: Too many xcodes. Use -d flag to dump to file\n");
			result = 1;
			goto Cleanup;
		}
	}

	interp.reset();
	while (interp.interpretNext(context.xcode) == 0)
	{
		result = interp.decodeXcode(context);
		if (result != 0)
		{
			printf("Error decoding xcode:\n\t%04X, OP: %02X, ADDR: %04X, DATA: %04X",
				context.base+interp.getOffset()-sizeof(XCODE), 
				context.xcode->opcode,
				context.xcode->addr,
				context.xcode->data);

			goto Cleanup;
		}
	}

	if (isFlagSet(SW_DMP, params.sw_flags))
	{
		fclose(context.stream);
		printf("Done\n");
	}

Cleanup:

	if (inittbl != NULL)
	{
		xb_free(inittbl);
	}

	// context
	for (LABEL* label : context.labels)
	{
		if (label != NULL)
		{
			xb_free(label);
			label = NULL;
		}
	}
	context.labels.clear();
	
	// settings
	if (context.settings.format_str != NULL)
	{
		xb_free(context.settings.format_str);
		context.settings.format_str = NULL;
	}
	if (context.settings.jmp_str != NULL)
	{
		xb_free(context.settings.jmp_str);
		context.settings.jmp_str = NULL;
	}
	if (context.settings.no_operand_str != NULL)
	{
		xb_free(context.settings.no_operand_str);
		context.settings.no_operand_str = NULL;
	}
	if (context.settings.num_str != NULL)
	{
		xb_free(context.settings.num_str);
		context.settings.num_str = NULL;
	}
	if (context.settings.num_str_format != NULL)
	{
		xb_free(context.settings.num_str_format);
		context.settings.num_str_format = NULL;
	}

	if (context.settings.prefix_str != NULL)
	{
		xb_free(context.settings.prefix_str);
		context.settings.prefix_str = NULL;
	}
	
	for (int i = 0; i < 5; i++)
	{
		if (context.settings.format_map[i].field != NULL)
		{
			xb_free(context.settings.format_map[i].field);
			context.settings.format_map[i].field = NULL;
		}

		if (context.settings.format_map[i].str != NULL)
		{
			xb_free(context.settings.format_map[i].str);
			context.settings.format_map[i].str = NULL;
		}
	}

	for (int i = 0; i < XCODE_OPCODE_COUNT; i++)
	{
		if (context.settings.opcodes[i].str != NULL)
		{
			xb_free((char*)context.settings.opcodes[i].str);
		}
	}

	return result;
}

int XbTool::simulateXcodes() const
{
	XcodeInterp interp = XcodeInterp();
	XCODE* xcode = NULL;
	
	UINT size = 0;
	int result = 0;
	bool hasMemChanges_total = false;
	const char* opcode_str = NULL;
	UCHAR* mem_sim = NULL;
	UCHAR* inittbl = NULL;
	UINT xcodeBase = 0;

	inittbl = load_init_tbl_file(size, xcodeBase);
	if (inittbl == NULL)
		return 1;

	result = interp.load(inittbl + xcodeBase, size - xcodeBase);
	if (result != 0)
		goto Cleanup;

	printf("xcode base: 0x%x\nmem space: %d bytes\n\n", xcodeBase, params.simSize);

	mem_sim = (UCHAR*)xb_alloc(params.simSize);
	if (mem_sim == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// simulate memory output
	hasMemChanges_total = false;
	printf("xcode sim:\n");
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
		memcpy(mem_sim + xcode->addr, (UCHAR*)&xcode->data, 4);

		if (getOpcodeStr(getOpcodeMap(), xcode->opcode, opcode_str) != 0)
		{			
			result = 1;
			goto Cleanup;
		}

		// print the xcode
		printf("%04x: %s 0x%02x, 0x%08X\n", (xcodeBase + interp.getOffset() - sizeof(XCODE)), opcode_str, xcode->addr, xcode->data);
	}
	
	if (!hasMemChanges_total)
	{
		printf("No Memory changes in range 0x0 - 0x%x\n", params.simSize);
		goto Cleanup;
	}

	// decode the x86 instructions.
	result = decodeX86(mem_sim, params.simSize, stdout);
	if (result != 0)
	{
		printf("Error: Failed to decode x86 instructions\n");
		goto Cleanup;
	}

	// if -d flag is set, dump the memory to a file, otherwise print the memory dump
	if (isFlagSet(SW_DMP, params.sw_flags))
	{
		const char* filename = params.outFile;
		if (filename == NULL)
			filename = "mem_sim.bin";

		printf("\nWriting memory dump to %s (%d bytes)\n", filename, params.simSize);
		if (writeFile(filename, mem_sim, params.simSize) != 0)
		{
			result = 1;
			goto Cleanup;
		}
	}
	else
	{
		// print memory dump
		printf("\nmem dump:\n");
		for (UINT i = 0; i < params.simSize; i += 8)
		{
			printf("%04x: ", i);
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

	return result;
}

int XbTool::decompressKrnl()
{
	int result = 0;

	printf("bios file: %s\n", params.inFile);
	result = bios.loadFromFile(params.inFile);
	if (result != Bios::BIOS_LOAD_STATUS_SUCCESS) // bldr needs to be valid to decompress kernel. 
	{
		return 1;
	}

	// decompress the kernel
	if (bios.decompressKrnl() != 0)
	{
		printf("Error: Failed to decompress the kernel image\n");
		return 1;
	}
	
	UCHAR* decompressedKrnl = bios.getDecompressedKrnl();
	UINT decompressedSize = bios.getDecompressedKrnlSize();

	result = dump_nt_headers(decompressedKrnl, decompressedSize, true);
	if (result != 0)
	{
		return 1;
	}

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
		printf(SUCCESS_OUT, filename, decompressedSize / 1024);
	}
	else
	{
		printf(FAIL_OUT, "kernel image");
	}

	return result;
}

int XbTool::readKeys()
{
	// read in rc4 keys that have been provided on the command line

	if (params.bldrKeyFile != NULL)
	{
		printf("bldr key file: %s\n", params.bldrKeyFile);
		params.keyBldr = readFile(params.bldrKeyFile, NULL, KEY_SIZE);
		if (params.keyBldr == NULL)
			return 1;
		printf("bldr key: ");
		printData(params.keyBldr, KEY_SIZE);
	}

	if (params.krnlKeyFile != NULL)
	{
		printf("krnl key file: %s\n", params.krnlKeyFile);
		params.keyKrnl = readFile(params.krnlKeyFile, NULL, KEY_SIZE);
		if (params.keyKrnl == NULL)
			return 1;
		printf("krnl key: ");
		printData(params.keyKrnl, KEY_SIZE);
	}

	if (params.keyBldr != NULL || params.keyKrnl != NULL)
		printf("\n");

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

int XbTool::extractPubKey(UCHAR* data, UINT size) const
{
	if (params.pubKeyFile == NULL)
	{
		return 0;
	}

	if (data == NULL || size == 0 || size < sizeof(PUBLIC_KEY))
	{
		printf("Error: Invalid data\n");
		return 1;
	}

	PUBLIC_KEY* pubkey = NULL;	
	for (UINT i = 0; i < size - sizeof(PUBLIC_KEY); i++)
	{
		pubkey = rsaVerifyPublicKey(data + i);
		if (pubkey != NULL)
		{
			break;
		}
	}

	if (pubkey == NULL)
	{
		printf("Public key not found\n");
		return 1;
	}

	printf("Public key found at offset 0x%x\n", (UINT)((UCHAR*)pubkey - data));
	rsaPrintPublicKey(pubkey);

	// if the '-patchkeys' flag is set, then use that key file to patch the pubkey.
	// otherwise, if the '-patchkeys' flag is not set then save the found pubkey to that file (unmodified).
	
	if (isFlagClear(SW_PATCH_KEYS, params.sw_flags))
	{
		printf("Writing public key to %s (%d bytes)\n", params.pubKeyFile, sizeof(PUBLIC_KEY));
		if (writeFile(params.pubKeyFile, pubkey, sizeof(PUBLIC_KEY)) != 0)
		{
			printf(FAIL_OUT, "public key");
			return 1;
		}
	}
	else
	{
		printf("\nPatching public key..\n");

		PUBLIC_KEY* patch_pubkey = (PUBLIC_KEY*)readFile(params.pubKeyFile, NULL, sizeof(PUBLIC_KEY));
		if (patch_pubkey == NULL)
		{
			printf("Error: Failed to patch public key\n");
			return 1;
		}

		if (memcmp(pubkey->modulus, patch_pubkey->modulus, sizeof(pubkey->modulus)) == 0)
		{
			printf("Public key matches patch key\n");
		}
		else
		{
			memcpy(pubkey->modulus, patch_pubkey->modulus, sizeof(pubkey->modulus));
			rsaPrintPublicKey(pubkey);
			printf("Public key patched.\n");
		}

		xb_free(patch_pubkey);
	}

	return 0;
}

int XbTool::encodeX86() const
{
	// encode x86 instructions to xcodes

	UCHAR* data = NULL;
	UCHAR* buffer = NULL;
	UINT dataSize = 0;
	UINT xcodeSize = 0;
	int result = 0;

	const char* filename = NULL;

	printf("x86 file: %s\n", params.inFile);

	data = readFile(params.inFile, &dataSize);
	if (data == NULL)
	{
		return 1;
	}

	result = encodeX86AsMemWrites(data, dataSize, buffer, &xcodeSize);
	if (result != 0)
	{
		printf("Error: Failed to encode x86 instructions\n");
		goto Cleanup;
	}

	printf("xcodes: %d\n", xcodeSize / sizeof(XCODE));

	// write the xcodes to file
	filename = params.outFile;
	if (filename == NULL)
	{
		filename = "xcodes.bin";
	}

	result = writeFile(filename, buffer, xcodeSize);
	if (result == 0)
	{
		printf(SUCCESS_OUT, filename, xcodeSize / 1024);
	}
	else
	{
		printf(FAIL_OUT, "xcodes");
	}

Cleanup:

	if (data != NULL)
	{
		xb_free(data);
	}

	if (buffer != NULL)
	{
		xb_free(buffer);
	}

	return 0;
}

int XbTool::dumpImg(UCHAR* data, UINT size) const
{
	int result = 0;
	UCHAR* allocData = NULL;

	if (data == NULL) // read in the kernel image if not provided
	{		
		printf("Image file: %s\n", params.inFile);
		allocData = readFile(params.inFile, &size);
		if (allocData == NULL)
		{
			return 1;
		}
		data = allocData;
	}

	// dump the kernel image to the console
	printf("Image size: %d bytes\n", size);
		
	result = dump_nt_headers(data, size, false);

	if (allocData != NULL)
	{
		xb_free(allocData);
		allocData = NULL;
		data = NULL;
	}

	return result;
}

int checkSize(const UINT& size)
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
