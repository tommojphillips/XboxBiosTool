// main.cpp: This file contains the 'main' function. Program execution begins and ends there.

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

// Command table. name, switch, type, required switches, optional switches, help string
const CMD_TBL cmd_tbl[] = {
	{ "Help",				"?",			CMD_HELP,				(SW_NONE),					 (SW_NONE),								HELP_STR_HELP },
	{ "List bios",			"ls",			CMD_LIST,				(SW_BIOS_FILE),				 (SW_ROMSIZE | SW_LS_OPT),				HELP_STR_LIST },
	
	{ "Split bios",			"split",		CMD_SPLIT,				(SW_BIOS_FILE | SW_ROMSIZE), (SW_NONE),								HELP_STR_SPLIT },
	{ "Combine banks",		"combine",		CMD_COMBINE,			(SW_NONE),					 (SW_OUT_FILE | SW_BANKS_FILE),			HELP_STR_COMBINE },

	{ "Extract bios",		"extr",			CMD_EXTR,				(SW_BIOS_FILE),				 (SW_BLD_BIOS),							HELP_STR_EXTR_ALL },
	{ "Build bios",			"bld",			CMD_BLD_BIOS,			(SW_BLD_BIOS),				 (SW_OUT_FILE),							HELP_STR_BUILD },

	{ "Simulate xcodes",	"xcode-sim",	CMD_XCODE_SIM,			(SW_NONE),					 (SW_INITTBL_FILE | SW_BIOS_FILE),		HELP_STR_BUILD_INITTBL },
	{ "Decode xcodes",		"xcode-decode",	CMD_XCODE_DECODE,		(SW_NONE),					(SW_INITTBL_FILE | SW_BIOS_FILE),		NULL },

	{ "Decompress krnl",	"decomp-krnl",	CMD_KRNL_DECOMPRESS,	(SW_BIOS_FILE),	(SW_OUT_FILE | SW_PATCH_PUB_KEY | SW_PUB_KEY_FILE),	NULL }
};

PARAM_TBL PARAM_BLDR_KEY = { "key-bldr",	SW_KEY_BLDR_FILE,	&params.keyBldrFile,	PARAM_TBL::STR,		NULL };
PARAM_TBL PARAM_BLDR_ENC = { "enc-bldr",	SW_ENC_BLDR,		&params.encBldr,		PARAM_TBL::BOOL,	NULL };

PARAM_TBL PARAM_KRNL_KEY = { "key-krnl",	SW_KEY_KRNL_FILE,	&params.keyKrnlFile,	PARAM_TBL::STR,		NULL };
PARAM_TBL PARAM_KRNL_ENC = { "enc-krnl",	SW_ENC_KRNL,		&params.encKrnl,		PARAM_TBL::BOOL,	NULL };

// Parameter table. switch, type, variable, variable type, help string
PARAM_TBL param_tbl[] = {
	{ "bios",		SW_BIOS_FILE,	&params.biosFile,		PARAM_TBL::STR,		HELP_STR_PARAM_BIOS_FILE },
	{ "out",		SW_OUT_FILE,	&params.outFile,		PARAM_TBL::STR,		HELP_STR_PARAM_OUT_FILE },
	{ "romsize",	SW_ROMSIZE,		&params.romsize,		PARAM_TBL::INT,		HELP_STR_PARAM_ROMSIZE },

	// crypto switches
	PARAM_BLDR_KEY, PARAM_BLDR_ENC, PARAM_KRNL_KEY, PARAM_KRNL_ENC,

	// bank switches (used for split, combine commands)
	{ "bank1",		SW_BANK1_FILE,	&params.bankFiles[0],	PARAM_TBL::STR,	HELP_STR_PARAM_BANK1 },
	{ "bank2",		SW_BANK2_FILE,	&params.bankFiles[1],	PARAM_TBL::STR,	HELP_STR_PARAM_BANK2 },
	{ "bank3",		SW_BANK3_FILE,	&params.bankFiles[2],	PARAM_TBL::STR,	HELP_STR_PARAM_BANK3 },
	{ "bank4",		SW_BANK4_FILE,	&params.bankFiles[3],	PARAM_TBL::STR,	HELP_STR_PARAM_BANK4 },

	// bios switches
	{ "bldr",		SW_BLDR_FILE,	&params.bldrFile,		PARAM_TBL::STR,	HELP_STR_PARAM_BLDR },
	{ "krnl",		SW_KRNL_FILE,	&params.krnlFile,		PARAM_TBL::STR,	HELP_STR_PARAM_KRNL },
	{ "krnldata",	SW_KRNL_DATA_FILE,&params.krnlDataFile,	PARAM_TBL::STR,	HELP_STR_PARAM_KRNL_DATA },
	{ "inittbl",	SW_INITTBL_FILE,&params.inittblFile,	PARAM_TBL::STR,	HELP_STR_PARAM_INITTBL },
		
	// Flags for list bios command. ( -ls )
	{ "nv2a",		SW_LS_NV2A_TBL,	&params.ls_flag,	PARAM_TBL::FLAG,	HELP_STR_PARAM_LS_NV2A_TBL },
	{ "2bl",		SW_LS_BLDR,		&params.ls_flag,	PARAM_TBL::FLAG,	HELP_STR_PARAM_LS_BIOS },
	{ "datatbl",	SW_LS_DATA_TBL,	&params.ls_flag,	PARAM_TBL::FLAG,	HELP_STR_PARAM_LS_DATA_TBL },

	// MCPX switches (used for en/decrypting preldr, 2bl and compresed kernel img)
	{ "mcpx",		SW_MCPX,		&params.mcpxFile,	PARAM_TBL::STR,		NULL },

	// public key switches (used for decompresed krnl extraction)
	{ "pubkey",		SW_PUB_KEY_FILE,  &params.pubKeyFile,	PARAM_TBL::STR,	NULL },
	{ "patchpubkey",SW_PATCH_PUB_KEY, &params.patchPubKey,	PARAM_TBL::BOOL,NULL }
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

	// print help for key switches
	print("Switches:\n");

	// bldr key
	print(" -%s", PARAM_BLDR_KEY.sw);
	print_f(HELP_STR_RC4_KEY, "Bldr");
	print("\n -%s", PARAM_BLDR_ENC.sw);
	print_f(HELP_STR_RC4_ENC, "Bldr");

	// krnl key
	print("\n -%s", PARAM_KRNL_KEY.sw);
	print_f(HELP_STR_RC4_KEY, "Krnl");
	print("\n -%s", PARAM_KRNL_ENC.sw);
	print_f(HELP_STR_RC4_ENC, "Krnl");
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
			error("Error: Invalid filename: %s\n", path);
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
	// first arg is the executable name
	if (!SUCCESS(getFilename(argv[0])))
	{
		return 1;
	}

	if (argc == 1)
	{
		error("Error: No command specified\n");
		return 1;
	}

	// parse switches
	char arg[MAX_SWITCH_LEN] = { 0 };
	bool swMatch;
	bool swNeedValue;
	int i, j;

	for (i = 2; i < argc; i++) // skip the first and second arg, which are the exe name and the command.
	{
		if (argv[i][0] != '-' && argv[i][0] != '/') // not a switch
		{
			// not a switch. check if bios file has been provided
			if (params.biosFile == NULL)
			{
				params.biosFile = argv[i];
				params.sw_flag |= SW_BIOS_FILE;
			}
			else
			{
				// bios file already provided. unknown arg
				error("Error: Unknown argument %s\n", argv[i]);
				return 1;
			}

			continue;
		}

		if (strlen(argv[i]) - 1 > MAX_SWITCH_LEN)
		{
			error("Error: Switch %s is too long\n", argv[i]);
			return 1;
		}

		arg[0] = '\0';
		strcat(arg, argv[i] + 1); // skip the '-' or '/' in the switch

		swMatch = false;
		for (j = 0; j < sizeof(param_tbl) / sizeof(PARAM_TBL); j++)
		{
			//if (strcmp(arg, param_tbl[j].sw) != 0)
			if (!SUCCESS(strcmp(arg, param_tbl[j].sw)))
				continue;

			// found a match in cmd table

			swNeedValue = (param_tbl[j].cmdType != PARAM_TBL::BOOL) && (param_tbl[j].cmdType != PARAM_TBL::FLAG);

			if (swNeedValue && i + 1 >= argc) // make sure there is a value after the switch
			{
				error("Error: No value specified for %s\n", param_tbl[j].sw);
				return 1;
			}

			swMatch = true;

			switch (param_tbl[j].cmdType)
			{
			case PARAM_TBL::STR:
				*(char**)param_tbl[j].var = (char*)argv[i + 1];
				#ifdef DEBUG
				print("%s:\t%s\n", param_tbl[j].sw, *(char**)param_tbl[j].var);
				#endif
				break;

			case PARAM_TBL::INT:
				*(int*)param_tbl[j].var = atoi(argv[i + 1]);
				#ifdef DEBUG
				print("%s:\t%d\n", param_tbl[j].sw, *(int*)param_tbl[j].var);
				#endif
				break;

			case PARAM_TBL::BOOL:
				*(bool*)param_tbl[j].var = !(*(bool*)param_tbl[j].var);
				#ifdef DEBUG
				print("%s:\t%s\n", param_tbl[j].sw, (*(bool*)param_tbl[j].var) ? "true" : "false");
				#endif
				break;

			case PARAM_TBL::FLAG:
				*(int*)param_tbl[j].var |= param_tbl[j].swType;
				#ifdef DEBUG
				print("%s:\t%04x\n", param_tbl[j].sw, *(int*)param_tbl[j].var);
				#endif
				break;

			default:
				error("Error: Type not implemented.\n");
				return 1;
			}

			// set the switch flag
			params.sw_flag |= param_tbl[j].swType;

			if (swNeedValue)
				i++; // value consumed; move to next arg

			break;
		}

		if (!swMatch)
		{
			error("Error: Unknown switch %s\n", argv[i]);
			return 1;
		}
	}

	bool isSwitchMissing = false;

	// parse cmd type
	if (strlen(argv[1]) < 2) // cmd len must be at least 3. 1 for '-' or '/', 1 char for the cmd, 1 char for null terminator
	{
		error("Error: Invalid command length: %s\n", argv[1]);
		return 1;
	}

	for (i = 0; i < sizeof(cmd_tbl) / sizeof(CMD_TBL); i++)
	{
		xb_zero(arg, MAX_SWITCH_LEN);

		if (strlen(argv[1]) > 1)
			strcat(arg, argv[1] + 1); // remove the '-' or '/' in the switch

		if (!SUCCESS(strcmp(arg, cmd_tbl[i].sw)))
			continue;

		xbtool.cmd = &cmd_tbl[i];

		// check if required switches are defined

		for (j = 0; j < sizeof(param_tbl) / sizeof(PARAM_TBL); j++)
		{
			if ((cmd_tbl[i].params_req & param_tbl[j].swType) == 0) // switch not required for this command
				continue;

			if ((params.sw_flag & param_tbl[j].swType) == 0) // switch not provided
			{
				error("Error: Required switch '-%s' was not supplied.\n", param_tbl[j].sw);
				isSwitchMissing = true;
			}
		}

		if (isSwitchMissing)
		{
			return 1;
		}

		break;
	}

	if (xbtool.cmd == NULL || xbtool.cmd->type == CMD_NONE)
	{
		error("Error: Unknown command: %s\n", argv[1]);
		return 1;
	}

	return 0;
}

int validateArgs()
{
	// rom size
	if (params.romsize == 0)
	{
		params.romsize = DEFAULT_ROM_SIZE;
	}

	params.romsize *= 1024;
	if (!SUCCESS(checkSize(params.romsize)))
	{
		error("Error: Invalid rom size: %d\n", params.romsize);
		return 1;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	int result = 0;
	const CMD_TBL* cmd = NULL;

	// header
	print(XB_BIOS_TOOL_HEADER_STR);
	
	result = parseArgs(argc, argv);
	if (!SUCCESS(result))
		goto Exit;

	result = validateArgs();
	if (!SUCCESS(result))
		goto Exit;

	cmd = xbtool.cmd;

	if (cmd->type == CMD_HELP)
	{
		printHelp();
		goto Exit;
	}

	print("%s\n\n", xbtool.cmd->name);
	
	// read in keys if provided.
	result = xbtool.readKeys(); 
	if (!SUCCESS(result))
		goto Exit;

	// read in mcpx rom if provided
	result = xbtool.readMCPX();
	if (!SUCCESS(result))
		goto Exit;

	// run command

	switch (cmd->type)
	{
		case CMD_LIST:
			result = xbtool.listBios();
			break;

		case CMD_EXTR:
			result = xbtool.extractBios();
			break;
		case CMD_SPLIT:
			result = xbtool.splitBios();
			break;
		case CMD_COMBINE:
			result = xbtool.combineBios();
			break;
		case CMD_BLD_BIOS:
			result = xbtool.buildBios();
			break;

		case CMD_XCODE_SIM:
			result = xbtool.simulateXcodes();
			break;

		case CMD_KRNL_DECOMPRESS:
			result = xbtool.decompressKrnl();
			break;

		case CMD_XCODE_DECODE:
			result = xbtool.decodeXcodes();
			break;

		default:
			error("Command not implemented\n");
			result = 1;
			break;
	}

Exit:

	xbtool.deconstruct();

	xb_leaks();	// check for memory leaks. All memory should be freed by now.

	if (!SUCCESS(result)) // force exit code to be -1 if error
	{
		result = -1;
	}

	return result;
}