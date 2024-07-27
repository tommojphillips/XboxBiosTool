#ifndef XB_LOADINI_H
#define XB_LOADINI_H

// std incl
#include <stdio.h>

#include "type_defs.h"


enum LOADINI_SETTING_TYPE : int { STR, BOOL };

enum LOADINI_ERROR_CODE : int {
	LOADINI_ERROR_SUCCESS = 0,
	LOADINI_ERROR_INVALID_DATA = 1,
	LOADINI_ERROR_INVALID_KEY = 2
};

typedef struct {
	const char* key;
	const LOADINI_SETTING_TYPE type;
	void* setting;
} LOADINI_SETTING_MAP;

// load ini file
// stream: file stream
// settings_map: map of settings
// map_size: size of the map
// returns LOADINI_ERROR_CODE
int loadini(FILE* stream, LOADINI_SETTING_MAP* settings_map, UINT map_size);

#endif // !XB_LOADINI_H
