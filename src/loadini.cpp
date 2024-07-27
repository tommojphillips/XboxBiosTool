// loadini.cpp: implements the loadini functions for loading settings from a settings file.

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
#include <cstring>

// user incl
#include "loadini.h"
#include "type_defs.h"
#include "util.h"
#include "xbmem.h"

// set setting value helper function
void setSettingValue(char** setting, const char* value, UINT len);

void setSettingValue(char** setting, const char* value, UINT len)
{
	if (*setting != NULL)  // setting already set, skip
		return;

	*setting = (char*)xb_alloc(len + 1);
	if (*setting == NULL)
		return;

	strcpy(*setting, value);
}

int loadini(FILE* stream, LOADINI_SETTING_MAP* settings_map, UINT map_size)
{
	UINT i = 0;
	UINT len = 0;

	const char delim[] = "=";
	
	char buf[128] = { 0 };
	char* line_ptr = NULL;
	char* key = NULL;
	char* value = NULL;

	// parse next line
	while (fgets(buf, sizeof(buf), stream) != NULL)
	{
		line_ptr = buf;
		ltrim(line_ptr);

		// line-format: key=value

		// ignore invalid characters
		if (line_ptr[0] == ';')
			continue;

		// get the key.
		key = strtok(line_ptr, delim);
		if (key == NULL)
			continue;

		// get the value
		value = strtok(NULL, delim);
		if (value == NULL)
			continue;
		len = strlen(value);
		ltrim(value);
		if (value[len - 1] == '\n')
			value[len - 1] = '\0';
		rtrim(value);

		// cmp key and set values.

		strcpy(buf, key);

		for (i = 0; i < map_size / sizeof(LOADINI_SETTING_MAP); i++)
		{
			if (strcmp(key, settings_map[i].key) != 0)
				continue;

			switch (settings_map[i].type)
			{
			case LOADINI_SETTING_TYPE::STR:
				setSettingValue((char**)settings_map[i].setting, value, len);
				break;

			case LOADINI_SETTING_TYPE::BOOL:
				if (strcmp(value, "true") == 0)
				{
					*(bool*)settings_map[i].setting = true;
				}
				else if (strcmp(value, "false") == 0)
				{
					*(bool*)settings_map[i].setting = false;
				}
				else
				{
					print("Error: invalid value for key: '%s'. value: '%s'\n", buf, value);
					return LOADINI_ERROR_INVALID_DATA;
				}
				break;
			}
			break;
		}

		if (i == map_size / sizeof(LOADINI_SETTING_MAP)) // key not found
		{
			print("Error: unknown entry: '%s'\n", buf);
			return LOADINI_ERROR_INVALID_KEY;
		}
	}

	return LOADINI_ERROR_SUCCESS;
}
