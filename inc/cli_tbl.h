// cli_tbl.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_CLI_TBL_H
#define XB_CLI_TBL_H

// the max length of a switch
const int MAX_SWITCH_LEN = 20;

enum COMMAND {	
	CMD_NONE, CMD_HELP, CMD_EXTR, CMD_LIST, CMD_SPLIT, CMD_COMBINE, CMD_BLD_BIOS, CMD_XCODE_SIM, CMD_KRNL_DECOMPRESS, CMD_XCODE_DECODE
};

enum SWITCH : UINT {
	SW_NONE = 1 << 0, SW_HELP = 1 << 1,
	SW_BIOS_FILE = 1 << 2, SW_ROMSIZE = 1 << 3,
	SW_KEY_KRNL = 1 << 4, SW_KEY_BLDR = 1 << 5,

	SW_OUT = 1 << 6,

	SW_BANK1 = 1 << 7, SW_BANK2 = 1 << 8, SW_BANK3 = 1 << 9, SW_BANK4 = 1 << 10,

	SW_BLDR = 1 << 11, SW_KRNL = 1 << 12, SW_INITTBL = 1 << 13,
	SW_ENC_BLDR = 1 << 14, SW_ENC_KRNL = 1 << 15,

	SW_LS_XCODES = 1 << 17, SW_LS_NV2A_TBL = 1 << 18, SW_LS_BIOS = 1 << 19, SW_LS_KRNL = 1 << 20,
	SW_LS_BLDR = 1 << 21, SW_LS_INITTBL = 1 << 22, SW_LS_DATA_TBL = 1 << 23,

	SW_MCPX = 1 << 24,

	SW_PUB_KEY_FILE = 1 << 25,
	SW_PATCH_PUB_KEY = 1 << 26,

	SW_BLD_BIOS = SW_BLDR | SW_KRNL | SW_INITTBL | SW_ROMSIZE,
	SW_BANKS = SW_BANK1 | SW_BANK2 | SW_BANK3 | SW_BANK4,
	SW_LS_OPT = SW_LS_BIOS | SW_LS_NV2A_TBL | SW_LS_KRNL | SW_LS_BLDR | SW_LS_INITTBL /*| SW_LS_XCODES | SW_LS_DATA_TBL*/
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
};

#endif // XB_CLI_TBL_H
