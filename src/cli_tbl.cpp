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
#include <stdlib.h>
#include <cstring>

// user incl
#include "cli_tbl.h"

void setParamValue(const PARAM_TBL* param, char* arg, int* flags);

int parseCli(int argc, char* argv[], const CMD_TBL*& cmd, int* flags, const CMD_TBL* cmd_tbl, const int cmd_tbl_count, const PARAM_TBL* param_tbl, const int param_tbl_count)
{
	int result = 0;
	int i;
	int j;
	int k;

	int len;
	bool swNeedValue;
	char arg[CLI_MAX_SWITCH_LEN] = { };

	if (CLI_MAX_SWITCHES - 1 < param_tbl_count)
		return CLI_ERROR_INVALID_SW;
	
	if (argc == 1)
		return CLI_ERROR_NO_CMD;

	len = strlen(argv[1]);
	if (len < 2 || len > CLI_MAX_SWITCH_LEN)
		return CLI_ERROR_INVALID_CMD;

	cmd = NULL;
	for (i = 0; i < cmd_tbl_count; i++)
	{		
		strcpy(arg, argv[1] + 1); // skip the '-' or '/' in the switch

		if (strcmp(arg, cmd_tbl[i].sw) != 0)
			continue;

		cmd = &cmd_tbl[i];
		break;
	}
	if (cmd == NULL || cmd->type == 0)
		return CLI_ERROR_INVALID_CMD;

	for (i = 2; i < argc; i++)
	{
		if (argv[i][0] != '-' && argv[i][0] != '/')
		{
			for (j = 0; j < sizeof(cmd->inferredSwitches) / sizeof(CLI_SWITCH); j++)
			{
				if (cmd->inferredSwitches[j] == 0)
					break;
				if (isFlagSet(cmd->inferredSwitches[j], flags))
					continue;

				for (k = 0; k < param_tbl_count; k++)
				{
					if (param_tbl[k].swType != cmd->inferredSwitches[j])
						continue;

					setParamValue(&param_tbl[k], argv[i], flags);
					break;
				}
				if (k < param_tbl_count)
					break;
			}
			if (j >= sizeof(cmd->inferredSwitches) / sizeof(CLI_SWITCH))
				return CLI_ERROR_INVALID_SW;
			continue;
		}

		if (strlen(argv[i]) - 1 > CLI_MAX_SWITCH_LEN)
			return CLI_ERROR_INVALID_SW;

		arg[0] = '\0';
		strcat(arg, argv[i] + 1);

		for (j = 0; j < param_tbl_count; j++)
		{
			if (strcmp(arg, param_tbl[j].sw) != 0)
				continue;

			swNeedValue = (param_tbl[j].cmdType != PARAM_TBL::BOOL) && (param_tbl[j].cmdType != PARAM_TBL::FLAG);
			if (swNeedValue && i + 1 >= argc)
				return CLI_ERROR_MISSING_ARG;
			if (swNeedValue && (argv[i + 1][0] == '-' || argv[i + 1][0] == '/'))
				return CLI_ERROR_INVALID_ARG;

			setParamValue(&param_tbl[j], argv[i + 1], flags);

			if (swNeedValue)
				i++;

			break;
		}

		if (j >= param_tbl_count)
			return CLI_ERROR_INVALID_SW;
	}

	for (i = 0; i < param_tbl_count; i++)
	{
		for (j = 0; j < sizeof(cmd->requiredSwitches) / sizeof(cmd->requiredSwitches[0]); j++)
		{
			if (cmd->requiredSwitches[j] == 0)
				break;

			if (param_tbl[i].swType != cmd->requiredSwitches[j])
				continue;

			if (isFlagClear(param_tbl[i].swType, flags))
				return CLI_ERROR_MISSING_SW;
		}
	}

	return 0;
}

void setParamValue(const PARAM_TBL* param, char* arg, int* flags)
{
	setFlag(param->swType, flags);

	if (param->var == NULL)
		return;

	int len = 0;
	switch (param->cmdType)
	{
	case PARAM_TBL::STR:
		*(char**)param->var = arg;
		break;

	case PARAM_TBL::INT:
		len = strlen(arg);
		if (len > 2 && arg[0] == '0' && (arg[1] == 'x' || arg[1] == 'X')) // hex
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

	if (k <= 0)
		return false;
	else
		goto rep;

	return 0;
}
