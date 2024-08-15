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

void setParamValue(const PARAM_TBL* param, char* arg, int* flags);

int parseCli(int argc, char* argv[], const CMD_TBL*& cmd, int* flags, const CMD_TBL* cmds, const int cmd_size, const PARAM_TBL* params, const int param_size)
{
	int i;
	int j;
	int k;
	int startIndex;
	bool swNeedValue;

	char arg[CLI_MAX_SWITCH_LEN] = {};

	const int cmd_count = cmd_size / sizeof(CMD_TBL);
	const int param_count = param_size / sizeof(PARAM_TBL);
	
	if (CLI_MAX_SWITCHES - 1 < param_count)
	{
		return CLI_ERROR_INVALID_SW;
	}

	if (argc == 1)
	{
		return CLI_ERROR_NO_CMD;
	}

	if (strlen(argv[1]) < 2)
	{
		printf("Error: Invalid command. %s\n", argv[1]);
		return CLI_ERROR_INVALID_CMD;
	}

	if (argv[1][0] != '-' && argv[1][0] != '/')
		strncpy_s(arg, argv[1], CLI_MAX_SWITCH_LEN-1);
	else
		strncpy_s(arg, argv[1] + 1, CLI_MAX_SWITCH_LEN-1);
	startIndex = 1;
	for (i = 0; i < cmd_count; i++)
	{
		if (strcmp(arg, cmds[i].sw) != 0)
			continue;

		cmd = &cmds[i];
		startIndex = 2;
		break;
	}
	if (cmd == NULL || cmd->type == 0)
	{
		printf("Error: Unknown command. %s\n", arg);
		return CLI_ERROR_UNKNOWN_CMD;
	}

	for (i = startIndex; i < argc; i++)
	{
		if (argv[i][0] != '-' && argv[i][0] != '/')
		{
			for (j = 0; j < sizeof(cmd->inferredSwitches) / sizeof(CLI_SWITCH); j++)
			{
				if (cmd->inferredSwitches[j] == 0)
					break;
				if (isFlagSet(cmd->inferredSwitches[j], flags))
					continue;

				for (k = 0; k < param_count; k++)
				{
					if (params[k].swType != cmd->inferredSwitches[j])
						continue;

					setParamValue(&params[k], argv[i], flags);
					break;
				}
				if (k < param_count)
					break;
			}
			if (j >= sizeof(cmd->inferredSwitches) / sizeof(CLI_SWITCH))
			{
				printf("Error: Invalid switch: %s\n", arg);
				return CLI_ERROR_INVALID_SW;
			}
			continue;
		}

		if (strlen(argv[i]) > CLI_MAX_SWITCH_LEN-1)
		{
			printf("Error: Invalid switch: %s\n", arg);
			return CLI_ERROR_INVALID_SW;
		}

		arg[0] = '\0';
		strcat_s(arg, argv[i] + 1);

		for (j = 0; j < param_count; j++)
		{
			if (strcmp(arg, params[j].sw) != 0)
				continue;

			swNeedValue = (params[j].cmdType != PARAM_TBL::BOOL) && (params[j].cmdType != PARAM_TBL::FLAG);
			if (swNeedValue && i + 1 >= argc)
			{
				printf("Error: '-%s' is missing an argument\n", arg);
				return CLI_ERROR_MISSING_ARG;
			}

			if (swNeedValue && (argv[i + 1][0] == '-' || argv[i + 1][0] == '/'))
			{
				printf("Error: '-%s' has an invaild argument\n", arg);
				return CLI_ERROR_INVALID_ARG;
			}

			setParamValue(&params[j], argv[i + 1], flags);

			if (swNeedValue)
				i++;

			break;
		}

		if (j >= param_count)
		{
			printf("Error: Unknown switch: %s\n", arg);
			return CLI_ERROR_UNKNOWN_SW;
		}
	}

	const int req_count = sizeof(cmd->requiredSwitches) / sizeof(cmd->requiredSwitches[0]);
	for (i = 0; i < param_count; i++)
	{
		for (j = 0; j < req_count; j++)
		{
			if (cmd->requiredSwitches[j] == 0)
				break;

			if (params[i].swType != cmd->requiredSwitches[j])
				continue;

			if (isFlagClear(params[i].swType, flags))
			{
				strcpy_s(arg, params[i].sw);

				printf("Error: Missing switch: -%s\n", params[i].sw);
				return CLI_ERROR_MISSING_SW;
			}
		}
	}

	return 0;
}

void setParamValue(const PARAM_TBL* param, char* arg, int* flags)
{
	setFlag(param->swType, flags);

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
void setFlag(const CLI_SWITCH sw, int* flags)
{
	flags[sw / CLI_SIZE] |= (1 << (sw % CLI_SIZE));
}
void clearFlag(const CLI_SWITCH sw, int* flags)
{
	flags[sw / CLI_SIZE] &= ~(1 << (sw % CLI_SIZE));
}
bool isFlagSet(const CLI_SWITCH sw, const int* flags)
{
	return (flags[sw / CLI_SIZE] & (1 << (sw % CLI_SIZE))) != 0;
}
bool isFlagClear(const CLI_SWITCH sw, const int* flags)
{
	return (flags[sw / CLI_SIZE] & (1 << (sw % CLI_SIZE))) == 0;
}

bool isFlagSetAny(const int sw, const int* flags)
{
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
	if (isFlagSet((CLI_SWITCH)(j-1), flags))
		return true;
	else
		k &= ~(1 << (j-1));

	if (k != 0)
		goto rep;

	return false;
}
