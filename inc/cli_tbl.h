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

#ifndef XB_CLI_TBL_H
#define XB_CLI_TBL_H

// the max length of a switch
const int MAX_SWITCH_LEN = 20;

enum COMMAND {
	CMD_NONE, CMD_HELP, CMD_EXTR, CMD_LIST, CMD_SPLIT, CMD_COMBINE,
	CMD_BLD_BIOS, CMD_XCODE_SIM, CMD_KRNL_DECOMPRESS, CMD_XCODE_DECODE, CMD_ENCODE_X86, CMD_DUMP_NT_IMG
};

enum SWITCH : int {
	SW_NONE =			1 << 0, 
	SW_HELP =			1 << 1,
	
	// ?? =				1 << 2,
	
	SW_ROMSIZE =		1 << 3,

	SW_KEY_KRNL_FILE =	1 << 4,
	SW_KEY_BLDR_FILE =	1 << 5,
	SW_OUT_FILE =		1 << 6,

	SW_BANK_FILES =		1 << 7,

	SW_IN_FILE =		1 << 8,

	// ?? =				1 << 9,
	// ?? =				1 << 10,

	SW_BLDR_FILE =		1 << 11,
	SW_KRNL_FILE =		1 << 12,
	SW_INITTBL_FILE =	1 << 13,
	SW_KRNL_DATA_FILE =	1 << 14,

	SW_ENC_BLDR =		1 << 15,
	SW_ENC_KRNL =		1 << 16,
	
	SW_BINSIZE =		1 << 17,

	SW_LS_NV2A_TBL =	1 << 18,
	SW_LS_DATA_TBL =	1 << 19,
	SW_LS_DUMP_KRNL =	1 << 20,
	
	// ?? =				1 << 20,
	// ?? =				1 << 21,
	// ?? =				1 << 22,
	// ?? =				1 << 23,
	
	SW_MCPX_FILE =		1 << 24,

	SW_PUB_KEY_FILE =	1 << 25,
	SW_CERT_KEY_FILE =	1 << 26,
	SW_EEPROM_KEY_FILE = 1 << 27,

	SW_PATCH_KEYS =		1 << 28,

	SW_BLD_BFM =		1 << 29,

	SW_DMP =			1 << 30,

	SW_SIM_SIZE =		1 << 31,

	SW_BLD_BIOS = SW_BLDR_FILE | SW_KRNL_FILE | SW_KRNL_DATA_FILE | SW_INITTBL_FILE,
};
enum SW_LS : int {
	LS_NONE = 0,
	LS_BIOS = 1 << 1,
	LS_KRNL = 1 << 2,
	LS_INITTBL = 1 << 4,	
	LS_DATA_TBL = 1 << 5,
	LS_NV2A_TBL = 1 << 6,
	LS_KEYS = 1 << 7,
	LS_DUMP_KRNL = 1 << 8,

	LS_OPT = LS_BIOS | LS_KRNL | LS_INITTBL | LS_KEYS
};

struct CMD_TBL
{
	const char* name;
	const char* sw;
	const COMMAND type;
	const int params_req;
	const int params_opt;
	const char* help;

	CMD_TBL() : name(NULL), sw(NULL), type(CMD_NONE), params_req(SW_NONE), params_opt(SW_NONE), help(NULL) { };

	CMD_TBL(char const n[MAX_SWITCH_LEN], const char s[MAX_SWITCH_LEN], const COMMAND c, const int req, const int opt, const char* h) :
		name(n), sw(s), type(c), params_req(req), params_opt(opt), help(h) { };
};

struct PARAM_TBL
{
	enum PARAM_TYPE { NONE, STR, INT, BOOL, FLAG };

	const char* sw;
	const SWITCH swType;
	void* var;
	PARAM_TYPE cmdType;
	const char* help;
	const int val;

	PARAM_TBL() : sw(NULL), swType(SW_NONE), var(NULL), cmdType(NONE), help(NULL), val(0) { };
	PARAM_TBL(const char s[MAX_SWITCH_LEN], const SWITCH t, void* v, PARAM_TYPE c, const char* h, const int val = NULL) :
		sw(s), swType(t), var(v), cmdType(c), help(h), val(val) { };
};

#endif // XB_CLI_TBL_H
