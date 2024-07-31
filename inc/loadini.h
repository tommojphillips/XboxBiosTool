// loadini.h

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
