// cli_tbl.h

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

// How to use:
// define a command table (CMD_TBL) and parameter table (PARAM_TBL) in your source file.
// define enums, CLI_COMMAND and CLI_SWITCH, for the command and switch types in your source file.
// first element in the enum should represent no command or switch.
// call parseCli() passing in the command line arguments, and tables.
// set the command and parameter values based on the command line arguments.
// update the switch flags based and cmd ptr based on the command line arguments.

#ifndef CLI_TBL_H
#define CLI_TBL_H

#ifndef CLI_SWITCH_MAX_LEN
#define CLI_SWITCH_MAX_LEN 20
#endif

#define CLI_SWITCH_MAX_COUNT 64

#ifndef CLI_SWITCH_MAX_COUNT
#define CLI_SWITCH_MAX_COUNT 32
#endif

#define CLI_SWITCH_BITS (sizeof(int) * 8)
#define CLI_SWITCH_SIZE ((CLI_SWITCH_MAX_COUNT + CLI_SWITCH_BITS - 1) / CLI_SWITCH_BITS)

typedef unsigned char CLI_SWITCH;
typedef unsigned char CLI_COMMAND;

const CLI_SWITCH SW_NONE = (CLI_SWITCH)0;
const CLI_SWITCH SW_HELP = (CLI_SWITCH)1;
const CLI_COMMAND CMD_NONE = (CLI_COMMAND)0;
const CLI_COMMAND CMD_HELP = (CLI_COMMAND)1;

#define CLI_COMMAND_START_INDEX 2
#define CLI_SWITCH_START_INDEX 2

enum CLI_CMD_ERROR : int {
	CLI_SUCCESS = 0,

	CLI_ERROR_NO_CMD,
	CLI_ERROR_INVALID_CMD,
	CLI_ERROR_UNKNOWN_CMD,

	CLI_ERROR_MISSING_SW,
	CLI_ERROR_INVALID_SW,
	CLI_ERROR_UNKNOWN_SW,
	
	CLI_ERROR_MISSING_ARG,
	CLI_ERROR_INVALID_ARG,
};

typedef struct {
	const char* sw;
	CLI_COMMAND type;
	CLI_SWITCH requiredSwitches[7];
	CLI_SWITCH inferredSwitches[4];
} CMD_TBL;
typedef struct {
	enum PARAM_TYPE : char { NONE, STR, INT, BOOL, FLAG };
	const char* sw;
	void* var;
	CLI_SWITCH swType;
	PARAM_TYPE cmdType;
} PARAM_TBL;

// parse the command line arguments
// argc: the number of arguments
// argv: the arguments
// cmd: the selected/current command
// cmd_tbl: the command table
// cmd_tbl_size: the size of the command table
// param_tbl: the parameter table
// param_tbl_size: the size of the parameter table
// returns: CLI_CMD_ERROR.
int parseCli(int argc, char* argv[],
	const CMD_TBL*& cmd,
	const CMD_TBL* cmd_tbl,
	const int cmd_tbl_size,
	const PARAM_TBL* param_tbl,
	const int param_tbl_size);

void setFlag(const CLI_SWITCH sw);
void clearFlag(const CLI_SWITCH sw);
bool isFlagSet(const CLI_SWITCH sw);
bool isFlagClear(const CLI_SWITCH sw);

// check if any of the flags are set in the switch array
// sw: the SET switches. eg (1 << 19) | (1 << 20)
// flags: the flags array
// returns: true if any of the flags are set.
bool isFlagSetAny(const int sw);

#endif // XB_CLI_TBL_H
