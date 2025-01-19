// loadini.c: implements the loadini functions for loading settings from a settings file.

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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <malloc.h>

// user incl
#include "loadini.h"
#include "str_util.h"

#ifdef MEM_TRACKING
#include "mem_tracking.h"
#endif

#define LOADINI_MAX_LINE_SIZE 128
#define LOADINI_DELIM "="

static void set_setting_value(char** setting, const char* value, uint32_t len);

int loadini(FILE* stream, const LOADINI_SETTING_MAP* settings_map, uint32_t map_size) {
	uint32_t i = 0;
	uint32_t len = 0;
		
	char buf[LOADINI_MAX_LINE_SIZE] = { 0 };
	char* line_ptr = NULL;
	char* key = NULL;
	char* value = NULL;

	// parse next line
	while (fgets(buf, LOADINI_MAX_LINE_SIZE, stream) != NULL) {
		
		line_ptr = buf;
		ltrim(&line_ptr);

		if (line_ptr[0] == ';') // comment
			continue;

		// line-format: key=value

		// get the key.
		key = strtok(line_ptr, LOADINI_DELIM);
		if (key == NULL)
			continue;
		ltrim(&key);
		rtrim(&key);

		// get the value
		value = strtok(NULL, LOADINI_DELIM);
		if (value == NULL)
			continue;
		len = strlen(value);
		ltrim(&value);
		rtrim(&value);

		// quotes
		if (value[0] == '\"' || value[0] == '\'') {
			value++;
			len = strlen(value);
			if (value[len - 1] == '\"' || value[len - 1] == '\'') {
				value[len - 1] = '\0';
				len--;
			}
		}

		// cmp key and set values.
		for (i = 0; i < map_size / sizeof(LOADINI_SETTING_MAP); i++) {
			if (strcmp(key, settings_map[i].s->key) != 0)
				continue;

			switch (settings_map[i].s->type) {
			case LOADINI_SETTING_TYPE_STR:
				set_setting_value((char**)settings_map[i].var, value, len);
				break;

			case LOADINI_SETTING_TYPE_BOOL:
				if (strcmp(value, "true") == 0) {
					*(bool*)settings_map[i].var = true;
				}
				else if (strcmp(value, "false") == 0) {
					*(bool*)settings_map[i].var = false;
				}
				break;
			}
			break;
		}

		if (i == map_size / sizeof(LOADINI_SETTING_MAP)) {
			return LOADINI_ERROR_INVALID_KEY;
		}
	}

	return LOADINI_ERROR_SUCCESS;
}

static void set_setting_value(char** setting, const char* value, uint32_t len) {
	if (setting == NULL || *setting != NULL)
		return;
	*setting = (char*)malloc(len + 1);
	if (*setting != NULL) {
		strcpy(*setting, value);
	}
}
