// cli_tbl.cpp: Implements functions for parsing command line arguments based on a command and parameter table.

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
#include <cstdio>
#include <cstdlib>
#include <cstring>

// user incl
#include "cli_tbl.h"

static int cli_flags[CLI_SWITCH_SIZE] = { 0 };

void setParamValue(const PARAM_TBL* param, char* arg);

int getCmd(const CMD_TBL* cmds, const int cmd_size, const char* arg, const CMD_TBL** cmd)
{
	for (int i = 0; i < (int)(cmd_size / sizeof(CMD_TBL)); i++)
	{
		if (strcmp(arg, cmds[i].sw) == 0)
		{
			*cmd = &cmds[i];
			return 0;
		}
	}
	return 1;
}

int parseCli(int argc, char* argv[], const CMD_TBL*& cmd, const CMD_TBL* cmds, const int cmd_size, const PARAM_TBL* params, const int param_size)
{
	int i;
	int j;
	int k;
	int startIndex;
	bool swNeedValue;
	char arg[CLI_SWITCH_MAX_LEN] = {};
		
	if (CLI_SWITCH_MAX_COUNT - 1 < param_size / sizeof(PARAM_TBL)) 
		return CLI_ERROR_INVALID_SW;
	if (argc == 1) 
		return CLI_ERROR_NO_CMD;
	if (strlen(argv[1]) < 2) 
		return CLI_ERROR_INVALID_CMD;

	if (argv[1][0] != '-' && argv[1][0] != '/')
		strncpy_s(arg, argv[1], CLI_SWITCH_MAX_LEN-1);
	else
		strncpy_s(arg, argv[1] + 1, CLI_SWITCH_MAX_LEN-1);
	
	if(getCmd(cmds, cmd_size, arg, &cmd) == 0)
		startIndex = 2;
	else
		startIndex = 1;

	if (cmd == NULL || cmd->type == 0)
	{
		printf("Error: Unknown command. %s\n", arg);
		return CLI_ERROR_UNKNOWN_CMD;
	}
		
	for (i = startIndex; i < argc; i++)
	{

		// check for inferred switches
		if (argv[i][0] != '-' && argv[i][0] != '/')
		{
			for (j = 0; j < sizeof(cmd->inferredSwitches) / sizeof(CLI_SWITCH); j++)
			{
				if (isFlagClear(cmd->inferredSwitches[j]))
				{
					for (k = 0; k < (int)(param_size / sizeof(PARAM_TBL)); k++)
					{
						if (params[k].swType == cmd->inferredSwitches[j])
						{
							setFlag(params[k].swType);
							setParamValue(&params[k], argv[i]);
							goto NextArg;
						}
					}
				}
			}

			// convert to '<command> -?' for help; user did the reverse '-? <command>'
			if (cmd->type == CMD_HELP)
			{
				getCmd(cmds, cmd_size, argv[i], &cmd);
				setFlag(SW_HELP);
				continue;
			}

			printf("Error: No corresponding switch defined for argument, '%s'.\n\nUsage: -<switch> <value>\n", argv[i]);
			return CLI_ERROR_INVALID_ARG;

		NextArg:
			continue;
		}
		
		/*if (strlen(argv[i]) > CLI_SWITCH_MAX_LEN - 1)
		{
			printf("Error: Invalid switch: %s\n", arg);
			return CLI_ERROR_INVALID_SW;
		}*/

		arg[0] = '\0';
		strcat_s(arg, argv[i] + 1);

		// check for explicit switches
		for (j = 0; j < (int)(param_size / sizeof(PARAM_TBL)); j++)
		{
			if (strcmp(arg, params[j].sw) != 0)
				continue;

			setFlag(params[j].swType);

			if (isFlagClear(SW_HELP))
			{
				swNeedValue = (params[j].cmdType != PARAM_TBL::BOOL) && (params[j].cmdType != PARAM_TBL::FLAG);
				if (swNeedValue && i + 1 >= argc)
				{
					printf("Error: '-%s' switch is missing an argument\n\nUsage: -%s <value>\n", arg, arg);
					return CLI_ERROR_MISSING_ARG;
				}

				if (swNeedValue && (argv[i + 1][0] == '-' || argv[i + 1][0] == '/'))
				{
					printf("Error: '-%s' switch has an invaild argument, '%s'\n\nUsage: -%s <value>\n", arg, argv[i + 1], arg);
					return CLI_ERROR_INVALID_ARG;
				}

				setParamValue(&params[j], argv[i + 1]);

				if (swNeedValue)
					i++;
			}
			break;
		}

		// convert to '<command> -?' for help; user did the reverse '-? <command>'
		if (cmd->type == CMD_HELP)
		{
			getCmd(cmds, cmd_size, arg, &cmd);
			setFlag(SW_HELP);
			i++;
			continue;
		}

		if (isFlagClear(SW_HELP) && j >= (int)(param_size / sizeof(PARAM_TBL)))
		{
			printf("Error: Unknown switch, '-%s'\n", arg);
			return CLI_ERROR_UNKNOWN_SW;
		}
	}

	// skip required switches check for help switch '<command> -?'
	if (isFlagSet(SW_HELP))
		return 0;

	// check for required switches
	
	bool missing = false;
	for (i = 0; i < (int)(param_size / sizeof(PARAM_TBL)); i++)
	{
		for (j = 0; j < sizeof(cmd->requiredSwitches) / sizeof(cmd->requiredSwitches[0]); j++)
		{
			if (cmd->requiredSwitches[j] == 0)
				break;

			if (params[i].swType != cmd->requiredSwitches[j])
				continue;

			if (isFlagClear(params[i].swType))
			{
				strcpy_s(arg, params[i].sw);

				printf("Error: Missing switch, '-%s'\n", params[i].sw);
				missing = true;
			}
		}
	}

	if (missing)
		return CLI_ERROR_MISSING_SW;
	
	return 0;
}

void setParamValue(const PARAM_TBL* param, char* arg) {
	if (param->var == NULL)
		return;

	switch (param->cmdType)
	{
	case PARAM_TBL::STR:
		*(char**)param->var = arg;
		break;

	case PARAM_TBL::INT:
		if (strlen(arg) > 2 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) // hex
			*(int*)param->var = strtol(arg, NULL, 16);
		else // decimal
			*(int*)param->var = atoi(arg);
		break;

	case PARAM_TBL::BOOL:
		*(bool*)param->var = !(*(bool*)param->var);
		break;

	case PARAM_TBL::FLAG:
			*(int*)param->var |= param->swType;
		break;
	}
}
void setFlag(const CLI_SWITCH sw)
{
	cli_flags[sw / CLI_SWITCH_BITS] |= (1 << (sw % CLI_SWITCH_BITS));
}
void clearFlag(const CLI_SWITCH sw)
{
	cli_flags[sw / CLI_SWITCH_BITS] &= ~(1 << (sw % CLI_SWITCH_BITS));
}
bool isFlagSet(const CLI_SWITCH sw)
{
	return (cli_flags[sw / CLI_SWITCH_BITS] & (1 << (sw % CLI_SWITCH_BITS))) != 0;
}
bool isFlagClear(const CLI_SWITCH sw)
{
	return (cli_flags[sw / CLI_SWITCH_BITS] & (1 << (sw % CLI_SWITCH_BITS))) == 0;
}

bool isFlagSetAny(const int sw)
{
	// check if any of the flags are set in the switch array.
	// sw: the SET switches. eg (1 << 19) | (1 << 20)
	// flags: the flags array
	// returns: true if any of the flags are set.

	// start with the highest switch
	// remove current switch from the switch array
	// loop until zero.

	int k = sw;
	int j;
	int i;

rep:
	j = 0;
	i = k;
	while (i > 0)
	{
		i >>= 1;
		j++;
	}
	if (isFlagSet((CLI_SWITCH)(j-1)))
		return true;
	else
		k &= ~(1 << (j-1));

	if (k != 0)
		goto rep;

	return false;
}
