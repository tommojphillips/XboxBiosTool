// str_util.c: implements various utility functions for manipulating strings.

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
#include <string.h>

// user incl
#include "str_util.h"

void ltrim(char** str)
{
	if (str == NULL)
		return;

	while ((*str)[0] == ' ' || (*str)[0] == '\t' || (*str)[0] == '\n') {
		str++;
	}
}

void rtrim(char** str)
{
	if (str == NULL)
		return;

	int len = strlen(*str);
	if (len == 0)
		return;

	char* end = *str + len - 1;
	while (end > *str && (end[0] == ' ' || end[0] == '\t' || end[0] == '\n')) {
		end--;
	}
	*(end + 1) = '\0';
}

void rpad(char* str, const int buffSize, const char pad)
{
	if (str == NULL)
		return;

	int slen = strlen(str);
	memset(str + slen, pad, buffSize - slen - 1);
	str[buffSize - 1] = '\0';
}
