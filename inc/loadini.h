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

#ifndef LOAD_INI_H
#define LOAD_INI_H

#include <stdint.h>
#include <stdio.h>

typedef enum {
	LOADINI_SETTING_TYPE_STR,
	LOADINI_SETTING_TYPE_BOOL
} LOADINI_SETTING_TYPE;

enum {
	LOADINI_ERROR_SUCCESS = 0,
	LOADINI_ERROR_INVALID_DATA = 1,
	LOADINI_ERROR_INVALID_KEY = 2
};

typedef struct {
	const char* key;
	const LOADINI_SETTING_TYPE type;
} LOADINI_SETTING;

typedef struct {
	const LOADINI_SETTING* s;
	void* var;
} LOADINI_SETTING_MAP;

typedef struct {
	const LOADINI_SETTING* s;
	const uint32_t size;
	const uint32_t count;
} LOADINI_RETURN_MAP;

#ifdef __cplusplus
extern "C" {
#endif

// load ini file
// stream: file stream
// settings_map: map of settings
// map_size: size of the map
// returns LOADINI_ERROR_CODE
int loadini(FILE* stream, const LOADINI_SETTING_MAP* settings_map, uint32_t map_size);

#ifdef __cplusplus
};
#endif

#endif // !LOAD_INI_H
