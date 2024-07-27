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

// Command table. [name], [switch], [param type], [required switches], [optional switches], [help string]
const CMD_TBL cmd_tbl[] = {
	{ "Help", "?", CMD_HELP, SW_NONE, SW_NONE, HELP_STR_HELP },
	{ "List bios", "ls", CMD_LIST, SW_IN_FILE, (SW_ROMSIZE | SW_LS_DATA_TBL | SW_LS_NV2A_TBL | SW_LS_DUMP_KRNL), HELP_STR_LIST },
	{ "Extract bios", "extr", CMD_EXTR, SW_IN_FILE, SW_BLD_BIOS, HELP_STR_EXTR_ALL },
	{ "Build bios",	"bld", CMD_BLD_BIOS, SW_BLD_BIOS, (SW_OUT_FILE | SW_ROMSIZE | SW_BINSIZE | SW_BLD_BFM | SW_PATCH_KEYS | SW_EEPROM_KEY_FILE | SW_CERT_KEY_FILE), HELP_STR_BUILD },
	{ "Decompress kernel", "decomp-krnl", CMD_KRNL_DECOMPRESS, SW_IN_FILE, (SW_OUT_FILE | SW_PATCH_KEYS | SW_PUB_KEY_FILE), HELP_STR_DECOMP_KRNL },
	{ "Split bios", "split", CMD_SPLIT, SW_IN_FILE, SW_ROMSIZE, HELP_STR_SPLIT },
	{ "Combine banks", "combine", CMD_COMBINE, SW_NONE, (SW_OUT_FILE | SW_BANK_FILES), HELP_STR_COMBINE },
	{ "Simulate xcodes", "xcode-sim", CMD_XCODE_SIM, SW_IN_FILE, (SW_OUT_FILE | SW_SIM_SIZE), HELP_STR_XCODE_SIM },
	{ "Decode xcodes", "xcode-decode",	CMD_XCODE_DECODE, SW_IN_FILE, SW_NONE, HELP_STR_XCODE_DECODE },
	{ "Encode X86", "x86-encode", CMD_ENCODE_X86, SW_IN_FILE, SW_OUT_FILE, HELP_STR_X86_ENCODE },
	{ "NT image dump", "dump-img", CMD_DUMP_NT_IMG, SW_IN_FILE, SW_NONE, HELP_STR_DUMP_NT_IMG },
};

PARAM_TBL PARAM_BLDR_KEY = { "key-bldr", SW_KEY_BLDR_FILE, &params.bldrKeyFile, PARAM_TBL::STR, NULL };
PARAM_TBL PARAM_KRNL_KEY = { "key-krnl", SW_KEY_KRNL_FILE, &params.krnlKeyFile, PARAM_TBL::STR, NULL };

PARAM_TBL PARAM_BLDR_ENC = { "enc-bldr", SW_ENC_BLDR, NULL, PARAM_TBL::FLAG, NULL };
PARAM_TBL PARAM_KRNL_ENC = { "enc-krnl", SW_ENC_KRNL, NULL, PARAM_TBL::FLAG, NULL };

// Parameter table. switch, type, variable, variable type, help string
PARAM_TBL param_tbl[] = {
	{ "in", SW_IN_FILE, &params.inFile, PARAM_TBL::STR, HELP_STR_PARAM_IN_FILE },
	{ "out", SW_OUT_FILE, &params.outFile, PARAM_TBL::STR, HELP_STR_PARAM_OUT_FILE },
	{ "romsize", SW_ROMSIZE, &params.romsize, PARAM_TBL::INT, HELP_STR_PARAM_ROMSIZE },
	{ "binsize", SW_BINSIZE, &params.binsize, PARAM_TBL::INT, HELP_STR_PARAM_BINSIZE },

	// crypto switches
	PARAM_BLDR_KEY, PARAM_BLDR_ENC, PARAM_KRNL_KEY, PARAM_KRNL_ENC,
	{ "mcpx", SW_MCPX_FILE, &params.mcpxFile, PARAM_TBL::STR, NULL },

	// bios switches
	{ "bldr", SW_BLDR_FILE,	&params.bldrFile, PARAM_TBL::STR, HELP_STR_PARAM_BLDR },
	{ "krnl", SW_KRNL_FILE,	&params.krnlFile, PARAM_TBL::STR, HELP_STR_PARAM_KRNL },
	{ "krnldata", SW_KRNL_DATA_FILE,&params.krnlDataFile, PARAM_TBL::STR, HELP_STR_PARAM_KRNL_DATA },
	{ "inittbl", SW_INITTBL_FILE,&params.inittblFile, PARAM_TBL::STR, HELP_STR_PARAM_INITTBL },

	// Flags for list bios command. ( -ls )
	{ "nv2a",	SW_LS_NV2A_TBL,	&params.ls_flag, PARAM_TBL::FLAG, HELP_STR_PARAM_LS_NV2A_TBL, LS_NV2A_TBL },
	{ "datatbl",SW_LS_DATA_TBL,	&params.ls_flag, PARAM_TBL::FLAG, HELP_STR_PARAM_LS_DATA_TBL, LS_DATA_TBL },
	{ "dump-krnl", SW_LS_DUMP_KRNL, &params.ls_flag, PARAM_TBL::FLAG, HELP_STR_PARAM_LS_DUMP_KRNL, LS_DUMP_KRNL},
	
	{ "patchkeys", SW_PATCH_KEYS, NULL, PARAM_TBL::FLAG, HELP_STR_PARAM_PATCH_KEYS },
	{ "pubkey", SW_PUB_KEY_FILE, &params.pubKeyFile, PARAM_TBL::STR, HELP_STR_PARAM_PUB_KEY },
	{ "certkey", SW_CERT_KEY_FILE, &params.certKeyFile, PARAM_TBL::STR, HELP_STR_PARAM_CERT_KEY },
	{ "eepromkey", SW_EEPROM_KEY_FILE, &params.eepromKeyFile, PARAM_TBL::STR, HELP_STR_PARAM_EEPROM_KEY },	
	
	{ "bfm", SW_BLD_BFM, NULL, PARAM_TBL::FLAG, NULL },
	{ "d", SW_DMP, NULL, PARAM_TBL::FLAG, NULL },
	{ "simsize", SW_SIM_SIZE, &params.simSize, PARAM_TBL::INT, HELP_STR_PARAM_SIM_SIZE },
};

void printHelp()
{
	print("Usage: %s <command> [switches]\n", xbtool.exe_filename);
	print("\nCommands:\n");

	int i,j;
	
	for (i = 0; i < sizeof(cmd_tbl) / sizeof(CMD_TBL); i++)
	{
		print(" /%s\n", cmd_tbl[i].sw);
		if (cmd_tbl[i].help != NULL)
			print("# %s\n", cmd_tbl[i].help);

		if (cmd_tbl[i].params_req != SW_NONE)
		{
			for (j = 0; j < sizeof(param_tbl) / sizeof(PARAM_TBL); j++)
			{
				if ((cmd_tbl[i].params_req & param_tbl[j].swType) == 0)
					continue;

				if (param_tbl[j].help == NULL)
					print(" -%s (req)\n", param_tbl[j].sw);
				else
					print(" -%s%s (req)\n", param_tbl[j].sw, param_tbl[j].help);
			}
		}
		if (cmd_tbl[i].params_opt != SW_NONE)
		{
			for (j = 0; j < sizeof(param_tbl) / sizeof(PARAM_TBL); j++)
			{
				if ((cmd_tbl[i].params_opt & param_tbl[j].swType) == 0)
					continue;

				if (param_tbl[j].help == NULL)
					print(" -%s\n", param_tbl[j].sw);
				else
					print(" -%s%s\n", param_tbl[j].sw, param_tbl[j].help);
			}
		}
		print("\n");
	}
	
	char helpStr[256] = { 0 };

	// print help for key switches
	print("Switches:\n");

	// bldr key
	print_f(helpStr, sizeof(helpStr), HELP_STR_RC4_KEY, "2bl");
	print(" -%s %s\n", PARAM_BLDR_KEY.sw, &helpStr);

	// krnl key
	print_f(helpStr, sizeof(helpStr), HELP_STR_KEY_KRNL);
	print("\n -%s %s\n", PARAM_KRNL_KEY.sw, &helpStr);
	
	// enc switches
	print_f(helpStr, sizeof(helpStr), HELP_STR_RC4_ENC, "2bl");
	print("\n -%s %s\n", PARAM_BLDR_ENC.sw, &helpStr);

	print_f(helpStr, sizeof(helpStr), HELP_STR_RC4_ENC, "kernel");
	print("\n -%s %s\n", PARAM_KRNL_ENC.sw, &helpStr);

	// mcpx rom
	print("\n -mcpx %s\n", &HELP_STR_MCPX_ROM);
}

int getFilename(char* path)
{
	// find last backslash
	const int pathLen = strlen(path);
	int i = 0;
	bool slashFound = false;

	for (i = pathLen - 1; i >= 0; i--)
	{
		if (path[i] == '\\' || path[i] == '/')
		{
			slashFound = true;
			break;
		}
	}

	if (slashFound)
	{
		// file name validation
		if (i < 0 || pathLen - i >= MAX_FILENAME)
		{
			print("Error: Invalid filename: %s\n", path);
			return 1;
		}
		strcpy(xbtool.exe_filename, path + i + 1);
	}
	else
	{
		strcpy(xbtool.exe_filename, path);
	}
	return 0;
}

int parseArgs(int argc, char* argv[])
{
	int result = 0;

	// first arg is the executable name
	result = getFilename(argv[0]);
	if (result != 0)
		return result;

	if (argc == 1) // no args
	{
		print("Error: No command specified\n");
		return 1;
	}

	// parse switches
	char arg[MAX_SWITCH_LEN] = { 0 };
	UINT len = 0;

	bool swMatch;
	bool swNeedValue;
	int i, j;

	len = strlen(argv[1]);

	// parse cmd type
	if (len < 2 || len > MAX_SWITCH_LEN)
	{
		print("Error: Invalid command length: %s\n", argv[1]);
		return 1;
	}

	// find the command
	xbtool.cmd = NULL;
	for (i = 0; i < sizeof(cmd_tbl) / sizeof(CMD_TBL); i++)
	{
		xb_zero(arg, sizeof(arg));
		strcpy(arg, argv[1] + 1); // skip the '-' or '/' in the switch

		if (strcmp(arg, cmd_tbl[i].sw) != 0)
			continue;

		xbtool.cmd = &cmd_tbl[i];
		break;
	}

	if (xbtool.cmd == NULL || xbtool.cmd->type == CMD_NONE)
	{
		print("Error: Unknown command: %s\n", argv[1]);
		return 1;
	}

	for (i = 2; i < argc; i++) // skip the first and second arg, which are the exe name and the command.
	{
		if (argv[i][0] != '-' && argv[i][0] != '/') // not a switch
		{
			// check if in file is set in the req or opt params, if so then assume this is the in file.
			if (params.inFile == NULL && ((xbtool.cmd->params_req & SW_IN_FILE) != 0 || (xbtool.cmd->params_opt & SW_IN_FILE) != 0))
			{
				params.inFile = argv[i];
				params.sw_flag |= SW_IN_FILE;
				continue;
			}
			
			// check if bank files are set in the req or opt params, if so then assume this is a bank file.
			if ((xbtool.cmd->params_req & SW_BANK_FILES) != 0 || (xbtool.cmd->params_opt & SW_BANK_FILES) != 0)
			{
				params.sw_flag |= SW_BANK_FILES;

				for (j = 0; j < 4; j++)
				{
					if (params.bankFiles[j] == NULL)
					{
						params.bankFiles[j] = argv[i];						
						break;
					}
				}
				if (j < 4) // bank file found
					continue;
			}

			
			print("Error: Unknown argument %s\n", argv[i]);
			return 1;
		}

		if (strlen(argv[i]) - 1 > MAX_SWITCH_LEN)
		{
			print("Error: Switch %s is too long\n", argv[i]);
			return 1;
		}

		arg[0] = '\0';
		strcat(arg, argv[i] + 1); // skip the '-' or '/' in the switch

		swMatch = false;

		for (j = 0; j < sizeof(param_tbl) / sizeof(PARAM_TBL); j++)
		{
			if (strcmp(arg, param_tbl[j].sw) != 0)
				continue;

			// found a match in cmd table

			swNeedValue = (param_tbl[j].cmdType != PARAM_TBL::BOOL) && (param_tbl[j].cmdType != PARAM_TBL::FLAG);

			if (swNeedValue && i + 1 >= argc) // make sure there is a value after the switch
			{
				print("Error: No value specified for %s\n", param_tbl[j].sw);
				return 1;
			}

			swMatch = true;

			if (param_tbl[j].var != NULL)
			{
				switch (param_tbl[j].cmdType)
				{
				case PARAM_TBL::STR:
					*(char**)param_tbl[j].var = (char*)argv[i + 1];
					break;

				case PARAM_TBL::INT:
					*(int*)param_tbl[j].var = atoi(argv[i + 1]);
					break;

				case PARAM_TBL::BOOL:
					*(bool*)param_tbl[j].var = !(*(bool*)param_tbl[j].var);
					break;

				case PARAM_TBL::FLAG:
					if (param_tbl[j].val != NULL)
						*(int*)param_tbl[j].var |= param_tbl[j].val;
					else
						*(int*)param_tbl[j].var |= param_tbl[j].swType;
					break;

				default:
					return 1;
				}
			}

			// set the switch flag
			params.sw_flag |= param_tbl[j].swType;

			if (swNeedValue)
				i++; // value consumed; move over next arg

			break;
		}

		if (!swMatch)
		{
			print("Error: Unknown switch %s\n", argv[i]);
			return 1;
		}
	}

	bool isSwitchMissing = false;

	// check if required switches are defined
	for (i = 0; i < sizeof(param_tbl) / sizeof(PARAM_TBL); i++)
	{
		if ((xbtool.cmd->params_req & param_tbl[i].swType) == 0) // switch not required for this command
			continue;

		if ((params.sw_flag & param_tbl[i].swType) == 0) // switch not provided
		{
			print("Error: Required switch '-%s' was not supplied.\n", param_tbl[i].sw);
			isSwitchMissing = true;
		}
	}

	if (isSwitchMissing)
		return 1;

	return 0;
}

int validateArgs()
{
	// rom size in kb
	if (params.romsize == 0)
	{
		params.romsize = DEFAULT_ROM_SIZE;
	}
	params.romsize *= 1024; // convert to bytes
	if (checkSize(params.romsize) != 0)
	{
		print("Error: Invalid rom size: %d\n", params.romsize);
		return 1;
	}

	// bin size in kb
	if (params.binsize == 0)
	{
		params.binsize = DEFAULT_ROM_SIZE;
	}
	params.binsize *= 1024; // convert to bytes
	if (checkSize(params.binsize) != 0)
	{
		print("Error: Invalid bin size: %d\n", params.binsize);
		return 1;
	}
	
	// xcode sim size in bytes
	if (params.simSize < 32)
	{
		params.simSize = 32;
	}
	if (params.simSize > (128 * 1024 * 1024)) // 128mb
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

	atexit(cleanup); // register cleanup function; called on exit()

#ifndef _DEBUG
	print(XB_BIOS_TOOL_HEADER_STR);
#endif

	result = parseArgs(argc, argv);
	if (result != 0)
		goto Exit;

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

	// check global proc error code
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

