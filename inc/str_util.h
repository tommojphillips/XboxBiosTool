// str_util.h: implements various utility functions for manipulating strings.

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

#ifndef STR_UTIL_H
#define STR_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

// trim left side of string.
void ltrim(char** str);

// trim right side of string.
void rtrim(char** str);

// pad right side of string.
void rpad(char* str, const int buffSize, const char pad);

#ifdef __cplusplus
};
#endif

#endif // !STR_UTIL_H
