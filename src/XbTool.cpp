// XbTool.cpp : Implements command functions for the Xbox Bios Tool.

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

#include "cstring"

#include "XbTool.h"
#include "XcodeInterp.h"

#include "type_defs.h"
#include "file.h"
#include "util.h"
#include "rc4.h"
#include "nt_headers.h"

void XbTool::deconstruct()
{
	params.deconstruct();

	bios.deconstruct();
};

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
	if (!SUCCESS(result))
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

	if (!SUCCESS(result))
	{
		error("\nError: Failed to write bios to %s\n", filename);
		goto Cleanup;
	}
	else
	{
		print("\nSuccess! -> OUT: %s %dKb\n", filename, params.binsize / 1024);
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
	if (result != BIOS_LOAD_STATUS_SUCCESS) // bldr needs to be valid to extract
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
	if (!SUCCESS(result))
	{
		error("Failed to extract bldr.\n");
		return 1;
	}

	// compressed krnl
	if (params.krnlFile != NULL)
		filename = params.krnlFile;
	else
		filename = "krnl.bin";

	result = bios.saveKernelToFile(filename);
	if (!SUCCESS(result))
	{
		error("Failed to extract krnl.\n");
		return 1;
	}

	// uncompressed krnl data
	if (params.krnlDataFile != NULL)
		filename = params.krnlDataFile;
	else
		filename = "krnl_data.bin";

	result = bios.saveKernelDataToFile(filename);
	if (!SUCCESS(result))
	{
		error("Failed to extract krnl data.\n");
		return 1;
	}
	
	// init tbl
	if (params.inittblFile != NULL)
		filename = params.inittblFile;
	else
		filename = "inittbl.bin";

	result = bios.saveInitTblToFile(filename);
	if (!SUCCESS(result))
	{
		error("Failed to extract init tbl.\n");
		return 1;
	}

	return result;
}

int XbTool::splitBios()
{
	int result = 0;

	print("bios file: %s\n", params.biosFile);

	//romsize sanity check
	if (params.romsize < DEFAULT_ROM_SIZE * 1024)
	{
		error("Error: Invalid rom size: %d\n", params.romsize);
		return 1;
	}

	result = bios.loadFromFile(params.biosFile);
	if (result == BIOS_LOAD_STATUS_FAILED) // bldr doesn't need to be valid to split
	{
		return result;
	}
	// bios file was read successfully

	UINT fnLen = 0;
	UINT bankFnLen = 0;
	UINT dataLeft = 0;
	UINT loopCount = 0;
	int bank = 0;
	const UINT biosSize = bios.getBiosSize();
	const char suffix[] = "_bank";
	char* biosFn = NULL;
	char* ext = NULL;
	char* bankFn = NULL;
	
	// check if bios size is less than or equal to rom size
	if (biosSize <= params.romsize)
	{
		error("Cannot split bios. file size is less than or equal to romsize\n");
		result = 1;
		goto Cleanup;
	}

	print("Splitting bios into %d banks (%dKB each)\n", (biosSize / params.romsize), (params.romsize / 1024));

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

	dataLeft = biosSize;
	loopCount = 0;
	while (dataLeft > 0)
	{
		// safe guard
		if (loopCount > 4)
		{ 
			error("Error: loopCount exceeded 5\n");
			result = 1;
		}
		loopCount++;

		// set bank filename = [name][suffix][number].[ext] = bios_bank1.bin
		bankFn[0] = '\0';
		format(bankFn, "%s%s%d%s", biosFn, suffix, bank + 1, ext);

		// write bank to file
		print("Writing bank %d - %s %dKB\n", bank + 1, bankFn, params.romsize / 1024);

		result = writeFile(bankFn, bios.getBiosData() + (params.romsize * bank), params.romsize);
		if (!SUCCESS(result))
		{
			error("Error: Failed to write %s\n", bankFn);
			goto Cleanup;
		}

		dataLeft -= params.romsize;
		bank++;
	}

	print("Bios split into %d banks\n", bank);

Cleanup:

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
	const UINT MAX_BIOS_SIZE = 0x100000; // MB
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
				print("Reading %s from bank %d\n", params.bankFiles[i], j + 1);

				banks[i] = (UCHAR*)xb_alloc(bankSizes[j]);
				xb_cpy(banks[i], banks[j], bankSizes[i]);
				bankSizes[i] = bankSizes[j];

				break;
			}
		}

		if (!isLoaded)
		{
			print("Reading %s from file\n", params.bankFiles[i]);

			banks[i] = readFile(params.bankFiles[i], &bankSizes[i]);
			if (banks[i] == NULL) // failed to load bank
			{
				goto Cleanup;
			}

			if (!SUCCESS(checkSize(bankSizes[i])))
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

	if (!SUCCESS(checkSize(totalSize)))
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

	print("Writing bios to %s %dKB\n", filename, totalSize / 1024);

	result = writeFile(filename, data, totalSize);

	if (SUCCESS(result))
	{
		print("Success! -> OUT: %s\n", filename);
	}
	else
	{
		error("Error: Failed to write bios to %s\n", filename);
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
	if (result == BIOS_LOAD_STATUS_FAILED) // bldr doesn't need to be valid to list it.
	{
		error("Error: Failed to load bios\n");
		return 1;
	}

	long long flag = params.ls_flag;

	if (flag == 0)
	{
		flag = SW_LS_OPT; // set to all flags.
	}

	print("\nbios size:\t\t%ld KB\n", bios.getBiosSize() / 1024UL);
	print("rom size:\t\t%ld KB\n", params.romsize / 1024UL);
	print("mcpx block size:\t%ld bytes\n", MCPX_BLOCK_SIZE);
	print("bldr block size:\t%ld bytes\n", BLDR_BLOCK_SIZE);
	print("total space available:\t%ld bytes\n", bios.getTotalSpaceAvailable());

	if (flag & SW_LS_BLDR) // print bldr
	{
		bios.printBldrInfo();
	}

	if (flag & SW_LS_KRNL) // print kernel
	{
		bios.printKernelInfo();
	}

	if (flag & SW_LS_INITTBL) // print init tbl
	{
		bios.printInitTblInfo();
	}

	if (flag & SW_LS_BIOS)
	{
		// invalid boot params. probably encrypted or using the wrong bldr rc4 key.
		if (!bios.isBootParamsValid())
		{
			print("\nbldr boot params are invalid. The bldr is probably encrypted");

			if (params.keyBldrFile != NULL)
				print(" with a different key or bldr is not encrypted.\n");
			else
				print(" (key not provided)");

			print(".\nOffsets are calculated from the values in the bldr boot params.\n");
		}
		else
		{
			// if the boot params are valid, and available space is negative, then the incorrect romsize must have been provided.
			if (bios.getAvailableSpace() < 0)
			{
				print("\nError: Incorrect rom size provided. The rom size is less than the total space available.\n" \
					"Offsets are calculated from the values in the bldr boot params.\n");
			}
		}
	}

	if (flag & SW_LS_NV2A_TBL)
	{
		bios.printNv2aInfo();
	}

	if (flag & SW_LS_DATA_TBL)
	{
		bios.printDataTbl();
	}

	return 0;
}

int XbTool::decodeXcodes() const
{
	const UINT BASE_OFFSET = sizeof(INIT_TBL);

	XcodeInterp interp = XcodeInterp();
	UCHAR* inittbl = NULL;
	UINT size = 0;
	int result = 0;

	// load the inittbl file (inittblFile or biosFile)
 
	const char* filename = params.inittblFile;
	if (filename == NULL)
	{
		filename = params.biosFile;
		if (filename == NULL)
		{
			error("Error: No inittbl file provided\n");
			return 1;
		}
	}

	print("inittbl file: %s\n", filename);
	inittbl = readFile(filename, &size);
	if (inittbl == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// validate the inittbl file
	if (size < BASE_OFFSET)
	{
		error("Error: Invalid inittbl size: %d. Expected atleast %d bytes\n", size, BASE_OFFSET);
		result = 1;
		goto Cleanup;
	}

	// note: im only interested in the xcodes. so the inittbl size should be less than 0x100000 (way less)
	// i could force check to 256, 512, 1024, but that prevents the user from using an extracted inittbl file. 
	// (3-4KB or even more if there were a considerble amount of xcodes added)
	if (size > 0x100000)
	{
		error("Error: Invalid inittbl size: %d. Expected less than %d bytes\n", size, 0x100000);
		result = 1;
		goto Cleanup;
	}
	
	print("\nDecode xcodes:\n");

	result = interp.load(inittbl + BASE_OFFSET, size - BASE_OFFSET);
	if (!SUCCESS(result))
	{
		goto Cleanup;
	}
		
	result = interp.decodeXcodes();

Cleanup:

	if (inittbl != NULL)
	{
		xb_free(inittbl);
	}	

	return result;
}

int XbTool::simulateXcodes() const
{
	// load the inittbl file
	
	// 6 byte encoding
	const USHORT MOV_PTR_INSTRS_2_BYTE[] = {	0x1D8B,		0x0D8B,		0x158B };
	const char* MOV_PTR_NAMES_2_BYTE[] = {		"mov ebx",	"mov ecx",	"mov edx" };
	
	// 5 byte encoding
	const UCHAR MOV_INSTRS[] = { 0xB8,		0xB9,		0xBA,		0xBB,		0xBC,		0xBD,		0xBE,		0xBF };
	const char* MOV_NAMES[] = { "mov eax", "mov ecx",	"mov edx",	"mov ebx",	"mov esp",	"mov ebp",	"mov esi",	"mov edi" };
	
	// 5 byte encoding
	const UCHAR MOV_EAX_PTR_INSTR = 0xA1;
	const char* MOV_EAX_PTR_NAME = "mov eax";

	// 2 byte encoding
	const USHORT JMP_INSTRS[] = {	0xE0FF,		0xE1FF,		0xE2FF,		0xE3FF,		0xE4FF,		0xE5FF,		0xE6FF,		0xE7FF };
	const char* JMP_NAMES[] = {		"jmp eax",	"jmp ecx",	"jmp edx",	"jmp ebx",	"jmp esp",	"jmp ebp",	"jmp esi",	"jmp edi" };
	
	// 2 byte encoding
	const USHORT REP_INSTRS[] = {	0xA5F3 };
	const char* REP_NAMES[] = {		"rep movsd" };

	// 1 byte encoding
	const UCHAR FAR_JMP_INSTR = 0xEA;
	const char* FAR_JMP_NAME = "jmp far";
	
	// 1 byte encoding
	const UCHAR NOP_INSTR = 0x90;
	const char* NOP_NAME = "nop";
	// 1 byte encoding
	const UCHAR CLD_INSTR = 0xFC;
	const char* CLD_NAME = "cld";

	const UINT DEFAULT_SIM_SIZE = 32; // 32KB
	
	const UINT SIM_SIZE = (params.simSize > 0) ? params.simSize : DEFAULT_SIM_SIZE;

	const char* filename = NULL;
	const UCHAR opcode_zero[6] = { 0 };

	int result = 0;
	bool hasMemChanges_total = false;
	UINT size = 0;
	UINT offset = 0;
	UCHAR* inittbl = NULL;	// inittbl file data
	UCHAR* xcodes = NULL;	// xcodes start
	UCHAR* mem_sim = NULL;	// simulated memory
	XcodeInterp interp = XcodeInterp();
	XCODE* xcode;
	bool found = false;
	bool unkInstrs = false;
	char str_opcode[16] = { 0 };

	UINT i, j, dumpTo;

	filename = params.inittblFile;
	if (filename == NULL)
	{
		filename = params.biosFile;
		if (filename == NULL)
		{
			error("Error: No inittbl file provided\n");
			return 1;
		}
	}

	print("inittbl file: %s\n", params.biosFile);
	print("mem space: %d bytes\n\n", SIM_SIZE);

	inittbl = readFile(filename, &size);
	if (inittbl == NULL)
	{
		return 1;
	}

	// validate the inittbl file
	if (size < sizeof(INIT_TBL))
	{
		error("Error: Invalid inittbl size: %d. Expected atleast %d bytes\n", size, sizeof(INIT_TBL));
		result = 1;
		goto Cleanup;
	}	
	if (size > 0x100000)
	{
		error("Error: Invalid inittbl size: %d. Expected less than %d bytes\n", size, 0x100000);
		result = 1;
		goto Cleanup;
	}

	xcodes = (inittbl + sizeof(INIT_TBL));

	result = interp.load(xcodes, size - sizeof(INIT_TBL));
	if (!SUCCESS(result))
	{
		error("Error: failed to load xcodes\n");
		goto Cleanup;
	}

	mem_sim = (UCHAR*)xb_alloc(SIM_SIZE);
	if (mem_sim == NULL)
	{
		result = 1;
		goto Cleanup;
	}

	// simulate memory output
	hasMemChanges_total = false;
	print("xcode sim:\nOFS   XCODE\tADDR  DATA\n");
	while (SUCCESS(interp.interpretNext(xcode)))
	{
		// skip this xcode. only care about xcodes that write to RAM
		if (xcode->opcode != XC_MEM_WRITE)
			continue; 

		// sanity check of addr.
		if (xcode->addr < 0 || xcode->addr >= SIM_SIZE)
			continue;

		hasMemChanges_total = true;

		// write the data to simulated memory
		xb_cpy(mem_sim + xcode->addr, (UCHAR*)&xcode->data, 4);
		
		offset = interp.getOffset() + sizeof(INIT_TBL) - sizeof(XCODE);

		// print the xcode
		xb_zero(str_opcode, sizeof(str_opcode));
		interp.getOpcodeStr(str_opcode);

		print("%04x: %s 0x%02x, 0x%08X\n", offset, str_opcode, xcode->addr, xcode->data);
	}

	if (interp.getStatus() != XcodeInterp::EXIT_OP_FOUND)
	{
		result = 1;
		goto Cleanup;
	}

	if (!hasMemChanges_total)
	{
		print("No Memory changes in range 0x0 - 0x%x\n", SIM_SIZE);
		goto Cleanup;
	}
	
	// after successful simulation, look for some x86 32-bit instructions.

	print("\nx86 32-bit instructions:\nOFS   INSTRUCTION\n");

	// sort by opcode len (2, 1); excluding size of operand/s
	for (i = 0; i < SIM_SIZE;)
	{
		// check if the next [max_instr_size] bytes are zero
		if (xb_cmp(mem_sim + i, opcode_zero, sizeof(opcode_zero)) == 0)
			break;

		found = false;
		
		// check for 2 byte mov ptr instructions
		for (j = 0; j < sizeof(MOV_PTR_INSTRS_2_BYTE) / sizeof(USHORT); j++)
		{
			if (xb_cmp(mem_sim + i, MOV_PTR_INSTRS_2_BYTE + j, 2) == 0)
			{
				print("%04x: %s, [0x%X]\n", i, MOV_PTR_NAMES_2_BYTE[j], *((UINT*)(mem_sim + i + 2)));
				i += 6; // 2 byte opcode + 4 byte data
				found = true;
				break;
			}
		}
		if (found)
			continue;

		// check for 2 byte jmp instructions
		for (j = 0; j < sizeof(JMP_INSTRS) / sizeof(USHORT); j++)
		{
			if (xb_cmp(mem_sim + i, JMP_INSTRS + j, 2) == 0)
			{
				print("%04x: %s\n", i, JMP_NAMES[j]);
				i += 2; // 2 byte opcode
				found = true;
				break;
			}
		}
		if (found)
			continue;

		// check for 2 byte rep instructions
		for (j = 0; j < sizeof(REP_INSTRS) / sizeof(USHORT); j++)
		{
			if (xb_cmp(mem_sim + i, REP_INSTRS + j, 2) == 0)
			{
				print("%04x: %s\n", i, REP_NAMES[j]);
				i += 2; // 2 byte opcode
				found = true;
				break;
			}
		}
		if (found)
			continue;

		// check for 1 byte far jmp instructions
		if (mem_sim[i] == FAR_JMP_INSTR)
		{
			print("%04x: %s 0x%04x:0x%08X\n", i, FAR_JMP_NAME, *((USHORT*)(mem_sim + i + 5)), *((UINT*)(mem_sim + i + 1)));
			i += 6; // 1 byte opcode + 4 byte for seg + 2 byte for offset
			continue;
		}

		// check for 1 byte mov ptr instructions
		if (mem_sim[i] == MOV_EAX_PTR_INSTR)
		{
			print("%04x: %s, [0x%X]\n", i, MOV_EAX_PTR_NAME, *((UINT*)(mem_sim + i + 1)));
			i += 5; // 1 byte opcode + 4 byte data
			continue;
		}

		// check for 1 byte mov instructions
		for (j = 0; j < sizeof(MOV_INSTRS); j++)
		{
			if (xb_cmp(mem_sim + i, MOV_INSTRS + j, 1) == 0)
			{
				print("%04x: %s, 0x%X\n", i, MOV_NAMES[j], *((UINT*)(mem_sim + i + 1)));
				i += 5; // 1 byte opcode + 4 byte data
				found = true;
				break;
			}
		}
		if (found)
			continue;

		if (mem_sim[i] == NOP_INSTR)
		{
			print("%04x: %s\n", i, NOP_NAME);
			i++; // 1 byte opcode
			continue;
		}

		if (mem_sim[i] == CLD_INSTR)
		{
			print("%04x: %s\n", i, CLD_NAME);
			i++; // 1 byte opcode
			continue;
		}

		// opcode not found
		error("Error: Unknown instruction at offset %04x, INSTR: %02X\n", i, mem_sim[i]);
		i++;
		unkInstrs = true;
	}
	dumpTo = i; // save the offset to dump to

	if ((params.sw_flag & SW_DMP) != 0)
	{
		filename = params.outFile;
		if (filename == NULL)
		{
			filename = "mem_sim.bin";
		}

		print("\nWriting memory dump to %s (%d bytes)\n", filename, SIM_SIZE);
		result = writeFile(filename, mem_sim, SIM_SIZE);
		if (!SUCCESS(result))
		{
			error("Error: Failed to write memory dump to %s\n", filename);
		}
	}
	else
	{
		// print memory dump
		print("\nmem dump:\n");
		for (i = 0; i < dumpTo; i += 8)
		{
			print("%04x: ", i);
			printData(mem_sim + i, 8);
		}

		if (unkInstrs)
		{
			print("\nUnknown instructions found. Dump to file ( -d ) to decompile with a x86 32-bit decompiler.\n" \
				"Only a handful of opcodes have been implemented. Better off " \
				"dumping to a file and analyzing with a x86 32-bit compatible decompiler.\n");
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

	print("bios file: %s\n", params.biosFile);
	result = bios.loadFromFile(params.biosFile);

	if (result != BIOS_LOAD_STATUS_SUCCESS) // bldr needs to be valid to decompress kernel. 
	{
		return 1;
	}

	// decompress the kernel
	if (!SUCCESS(bios.decompressKrnl()))
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
	if (!SUCCESS(result))
	{
		error("Error: Failed to write kernel image\n");
		goto Cleanup;
	}
	else
	{
		print("Success! -> OUT: %s\n", filename);
	}

Cleanup:
	
	// bios frees it's allocated memory. no need to free decompressedKrnl

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
		printData(params.keyBldr, KEY_SIZE, false);
		print("%s\n", !params.encBldr ? " ( -enc-bldr )" : "");
	}

	if (params.keyKrnlFile != NULL)
	{
		print("krnl key file: %s\n", params.keyKrnlFile);
		params.keyKrnl = readFile(params.keyKrnlFile, NULL, KEY_SIZE);
		if (params.keyKrnl == NULL)
			return 1;
		print("krnl key: ");
		printData(params.keyKrnl, KEY_SIZE, false);
		print("%s\n", !params.encKrnl ? " ( -enc-krnl )" : "");
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

	params.mcpx.data = readFile(params.mcpxFile, NULL, MCPX_BLOCK_SIZE);
	if (params.mcpx.data == NULL)
		return 1;

	// verify we got a valid mcpx rom. readFile() has already checked the size.
	// create some constants to check against.

	const UINT MCPX_CONST_ONE = 280018995;
	const UINT MCPX_CONST_TWO = 3230587022;
	const UINT MCPX_CONST_THREE = 3993153540;
	const UINT MCPX_CONST_FOUR = 2160066559;

	if (xb_cmp(params.mcpx.data, &MCPX_CONST_ONE, 4) != 0 ||
		xb_cmp(params.mcpx.data + 4, &MCPX_CONST_TWO, 4) != 0 ||
		xb_cmp(params.mcpx.data + MCPX_BLOCK_SIZE - 4, &MCPX_CONST_THREE, 4) != 0)
	{
		error("Error: Invalid MCPX ROM\n");
		return 1;
	}

	// valid mcpx rom

	print("mcpx file: %s ", params.mcpxFile);

	// mcpx v1.0 has a hardcoded signature at offset 0x187.
	if (xb_cmp(params.mcpx.data + 0x187, BOOT_PARAMS_SIGNATURE, 4) == 0)
	{
		print("( v1.0 )\n");
		params.mcpx.version = MCPX_ROM::MCPX_V1_0;
		return 0;
	}

	// verify the mcpx rom is v1.1

	if (xb_cmp(params.mcpx.data + 0xe0, &MCPX_CONST_FOUR, 4) != 0)
	{
		error("Error: Invalid MCPX v1.1 ROM\n");
		return 1;
	}

	print("( v1.1 )\n");
	params.mcpx.version = MCPX_ROM::MCPX_V1_1;

	return 0;
}

int XbTool::extractPubKey(UCHAR* data, UINT size)
{
	// after successful decompression, lets see if we can find the public key.
	// public key should start with the RSA1 magic number.
	// if we find it, we can extract the public key and save it to a file.

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

	// check if user is patching the public key, 
	// the user might be using the 'pubkeyfile' switch to tell us where to save it.
	// otherwise, patch the public key with the provided key. (pubkeyfile is providing us with the key)

	// do nothing if a public key file is not provided
	if (params.pubKeyFile == NULL)
	{
		print("No public key file provided\n");
		return 0;
	}

	// if the 'patch-keys' flag is set, then use that key file to patch the pubkey.
	// otherwise, if the 'patch-keys' flag is not set then save the found pubkey to that file (unmodified).
	if ((params.sw_flag & SW_PATCH_KEYS) != 0)
	{
		print("Writing public key to %s (%d bytes)\n", params.pubKeyFile, sizeof(PUBLIC_KEY));
		if (!SUCCESS(writeFile(params.pubKeyFile, pubkey, sizeof(PUBLIC_KEY))))
		{
			error("Error: Failed to write public key to file\n");
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

		xb_cpy(pubkey->modulus, patch_pubkey->modulus, sizeof(pubkey->modulus));

		xb_free(patch_pubkey);

		printPubKey(pubkey);

		print("Public key patched.\n");
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