// main.cpp: This file contains the 'main' function. Implements the command line interface for the Xbox BIOS Tool.

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
// Project start date: 25.05.2024

// std incl
#include <stdlib.h>
#include <cstring>

// user incl
#include "XbTool.h"
#include "main.h"
#include "bldr.h"
#include "type_defs.h"
#include "util.h"
#include "cli_tbl.h"
#include "help_strings.h"
#include "version.h"

XbTool xbtool;							// global instance of xbtool
Parameters&  params = xbtool.params;	// global instance of parameters.

const CMD_TBL cmd_tbl[] = {
	{ "?", CMD_HELP, {SW_NONE}, {SW_NONE} },	
	{ "ls", CMD_LIST, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "extr", CMD_EXTR, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "bld", CMD_BLD_BIOS, {SW_BLDR_FILE, SW_KRNL_FILE, SW_KRNL_DATA_FILE, SW_INITTBL_FILE}, {SW_IN_FILE} },
	{ "decomp-krnl", CMD_KRNL_DECOMPRESS, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "split", CMD_SPLIT, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "combine", CMD_COMBINE, {SW_NONE}, {SW_BANK1_FILE, SW_BANK2_FILE, SW_BANK3_FILE, SW_BANK4_FILE} },
	{ "xcode-sim", CMD_XCODE_SIM, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "xcode-decode", CMD_XCODE_DECODE, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "x86-encode", CMD_ENCODE_X86, {SW_IN_FILE}, {SW_IN_FILE} },
	{ "dump-img", CMD_DUMP_NT_IMG, {SW_IN_FILE}, {SW_IN_FILE} },
};

PARAM_TBL param_tbl[] = {
	{ "in", &params.inFile, SW_IN_FILE, PARAM_TBL::STR },
	{ "out", &params.outFile, SW_OUT_FILE, PARAM_TBL::STR },
	{ "romsize", &params.romsize, SW_ROMSIZE, PARAM_TBL::INT },
	{ "binsize", &params.binsize, SW_BINSIZE, PARAM_TBL::INT },

	{ "enc-bldr", NULL, SW_ENC_BLDR, PARAM_TBL::FLAG },
	{ "enc-krnl", NULL, SW_ENC_KRNL, PARAM_TBL::FLAG },
	{ "key-krnl", &params.krnlKeyFile, SW_KEY_KRNL_FILE, PARAM_TBL::STR },
	{ "key-bldr", &params.bldrKeyFile, SW_KEY_BLDR_FILE, PARAM_TBL::STR},
	{ "mcpx", &params.mcpxFile, SW_MCPX_FILE, PARAM_TBL::STR },

	{ "bldr", &params.bldrFile, SW_BLDR_FILE, PARAM_TBL::STR },
	{ "krnl", &params.krnlFile, SW_KRNL_FILE, PARAM_TBL::STR },
	{ "krnldata", &params.krnlDataFile, SW_KRNL_DATA_FILE, PARAM_TBL::STR },
	{ "inittbl", &params.inittblFile, SW_INITTBL_FILE, PARAM_TBL::STR },

	{ "nv2a", NULL, SW_LS_NV2A_TBL, PARAM_TBL::FLAG },
	{ "datatbl", NULL, SW_LS_DATA_TBL, PARAM_TBL::FLAG },
	{ "dump-krnl", NULL, SW_LS_DUMP_KRNL, PARAM_TBL::FLAG },
	
	{ "patchkeys", NULL, SW_PATCH_KEYS, PARAM_TBL::FLAG },
	{ "pubkey", &params.pubKeyFile, SW_PUB_KEY_FILE, PARAM_TBL::STR },
	{ "certkey", &params.certKeyFile, SW_CERT_KEY_FILE, PARAM_TBL::STR },
	{ "eepromkey", &params.eepromKeyFile, SW_EEPROM_KEY_FILE, PARAM_TBL::STR },
	
	{ "bfm", NULL, SW_BLD_BFM, PARAM_TBL::FLAG },
	{ "d", NULL, SW_DMP, PARAM_TBL::FLAG },
	{ "simsize", &params.simSize, SW_SIM_SIZE, PARAM_TBL::INT },
	{ "nomaxsize", NULL, SW_NO_MAX_SIZE, PARAM_TBL::FLAG },
	{ "xcodebase", &params.xcodeBase, SW_XCODE_BASE, PARAM_TBL::INT },
	{ "bank1", &params.bankFiles[0], SW_BANK1_FILE, PARAM_TBL::STR },
	{ "bank2", &params.bankFiles[1], SW_BANK2_FILE, PARAM_TBL::STR },
	{ "bank3", &params.bankFiles[2], SW_BANK3_FILE, PARAM_TBL::STR },
	{ "bank4", &params.bankFiles[3], SW_BANK4_FILE, PARAM_TBL::STR },
};

void printHelp()
{
	print("Usage: xbios.exe <command> [switches]\n");
	print("\nCommands:\n");

	print(" /ls\n# %s\n %s (req)\n %s %s\n %s\n %s\n %s\n\n",
		HELP_STR_LIST, HELP_STR_PARAM_BIOS_FILE, HELP_STR_PARAM_ROMSIZE, HELP_STR_VALID_ROM_SIZES, HELP_STR_PARAM_LS_DATA_TBL,
		HELP_STR_PARAM_LS_NV2A_TBL, HELP_STR_PARAM_LS_DUMP_KRNL);

	print(" /extr\n# %s\n %s (req)\n %s %s\n\n", HELP_STR_EXTR_ALL, HELP_STR_PARAM_BIOS_FILE, HELP_STR_PARAM_ROMSIZE, HELP_STR_VALID_ROM_SIZES);

	print(" /bld\n# %s\n %s (req)\n %s (req)\n %s (req)\n %s (req)\n %s\n %s %s\n %s %s\n %s\n %s\n %s\n %s\n\n",
		HELP_STR_BUILD, HELP_STR_PARAM_BLDR, HELP_STR_PARAM_KRNL, HELP_STR_PARAM_KRNL_DATA, HELP_STR_PARAM_INITTBL, HELP_STR_PARAM_OUT_FILE, 
		HELP_STR_PARAM_ROMSIZE, HELP_STR_VALID_ROM_SIZES, HELP_STR_PARAM_BINSIZE, HELP_STR_VALID_ROM_SIZES, HELP_STR_PARAM_PATCH_KEYS,
		HELP_STR_PARAM_CERT_KEY, HELP_STR_PARAM_EEPROM_KEY, HELP_STR_PARAM_BFM);

	print(" /decomp-krnl\n# %s\n %s (req)\n %s\n %s\n %s\n\n",
		HELP_STR_DECOMP_KRNL, HELP_STR_PARAM_BIOS_FILE, HELP_STR_PARAM_OUT_FILE, HELP_STR_PARAM_PATCH_KEYS, HELP_STR_PARAM_PUB_KEY);

	print(" /split\n# %s\n %s (req)\n %s %s\n\n", HELP_STR_SPLIT, HELP_STR_PARAM_BIOS_FILE, HELP_STR_PARAM_ROMSIZE, HELP_STR_VALID_ROM_SIZES);

	print(" /combine\n# %s\n -bank[1-4] <path> - bank file (req)\n %s\n\n", HELP_STR_COMBINE, HELP_STR_PARAM_OUT_FILE);

	print(" /xcode-sim\n# %s\n %s (req)\n %s\n %s\n %s\n\n", HELP_STR_XCODE_SIM, HELP_STR_PARAM_IN_FILE, HELP_STR_PARAM_SIM_SIZE,
		HELP_STR_PARAM_XCODE_BASE, HELP_STR_PARAM_NO_MAX_SIZE);

	print(" /xcode-decode\n# %s\n %s (req)\n %s\n %s\n\n", HELP_STR_XCODE_DECODE, HELP_STR_PARAM_IN_FILE,
		HELP_STR_PARAM_XCODE_BASE, HELP_STR_PARAM_NO_MAX_SIZE);

	print(" /x86-encode\n# %s\n %s (req)\n %s\n\n", HELP_STR_X86_ENCODE, HELP_STR_PARAM_IN_FILE, HELP_STR_PARAM_OUT_FILE);

	print(" /dump-img\n# %s\n %s\n\n", HELP_STR_DUMP_NT_IMG, HELP_STR_PARAM_IN_FILE);
	
	char helpStr[256] = { 0 };

	// print help for key switches
	print("Switches:\n");

	// bldr key
	print_f(helpStr, sizeof(helpStr), HELP_STR_RC4_KEY, "2bl");
	print(" -key-bldr %s\n", &helpStr);

	// krnl key
	print_f(helpStr, sizeof(helpStr), HELP_STR_KEY_KRNL);
	print("\n -key-krnl %s\n", &helpStr);
	
	// mcpx rom
	print("\n %s\n", &HELP_STR_MCPX_ROM);

	// enc switches
	print_f(helpStr, sizeof(helpStr), HELP_STR_RC4_ENC, "2bl");
	print("\n -enc-bldr %s\n", &helpStr);

	print_f(helpStr, sizeof(helpStr), HELP_STR_RC4_ENC, "kernel");
	print("\n -enc-krnl %s\n", &helpStr);

}

int validateArgs()
{
	// rom size in kb
	if (isFlagClear(SW_ROMSIZE, params.sw_flags))
	{
		params.romsize = DEFAULT_ROM_SIZE;
	}
	params.romsize *= 1024;
	if (checkSize(params.romsize) != 0)
	{
		print("Error: Invalid rom size: %d\n", params.romsize);
		return 1;
	}

	// bin size in kb
	if (isFlagClear(SW_BINSIZE, params.sw_flags))
	{
		params.binsize = DEFAULT_ROM_SIZE;
	}
	params.binsize *= 1024;
	if (checkSize(params.binsize) != 0)
	{
		print("Error: Invalid bin size: %d\n", params.binsize);
		return 1;
	}
	
	// xcode sim size in bytes
	if (isFlagClear(SW_SIM_SIZE, params.sw_flags))
	{
		params.simSize = 32;
	}
	if (params.simSize < 32 || params.simSize > (128 * 1024 * 1024)) // 32bytes - 128mb
	{
		print("Error: Invalid sim size: %d\n", params.simSize);
		return 1;
	}
	if (params.simSize % 4 != 0)
	{
		print("Error: simsize must be devisible by 4.\n");
		return 1;
	}

	return 0;
}

void cleanup()
{
	xbtool.deconstruct();
	xb_leaks();
}

int main(int argc, char* argv[])
{
	int result = 0;

	atexit(cleanup);

#ifndef _DEBUG
	print(XB_BIOS_TOOL_HEADER_STR);
#endif

	xbtool.construct();

	result = parseCli(argc, argv, xbtool.cmd, params.sw_flags, cmd_tbl, (sizeof(cmd_tbl) / sizeof(CMD_TBL)), param_tbl, (sizeof(param_tbl) / sizeof(PARAM_TBL)));
	if (result != 0)
	{
		switch (result)
		{
			case CLI_ERROR_NO_CMD:
				print("Error: No command provided\n");
				break;
			case CLI_ERROR_INVALID_CMD:
				print("Error: Invalid command\n");
				break;
			case CLI_ERROR_INVALID_SW:
				print("Error: Invalid switch\n");
				break;
			case CLI_ERROR_MISSING_SW:
				print("Error: Missing required switch\n");
				break;
			case CLI_ERROR_MISSING_ARG:
				print("Error: Switch is missing argument\n");
				break;
				case CLI_ERROR_INVALID_ARG:
				print("Error: Invalid argument\n");
				break;
			default:
				print("Error: Unknown error\n");
				break;
		}
		result = ERROR_FAILED;
		goto Exit;
	}
	result = validateArgs();
	if (result != 0)
		goto Exit;

	if (xbtool.cmd->type == CMD_HELP)
	{
		printHelp();
		goto Exit;
	}
	
	result = xbtool.run();

Exit:

	// check global error code
	if (result == 0)
	{
		result = getErrorCode();
		if (result != 0)
		{
			print("Error: %d\n", result);
		}
	}
	return result;
}

