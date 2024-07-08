// util.h

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

#ifndef XB_UTIL_H
#define XB_UTIL_H

// std incl
#include <stdio.h>

// user incl
#include "type_defs.h"

#define SUCCESS(x) (x == 0)

enum CON_COL : int { ERR = 0, OK = 1, MSG = 3, BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

void setConsoleColor(const int col = -1);

void format(char* buffer, const char* format, ...);

void print(FILE* stream, const char* format, ...);			// print to stream (stdout, stderr, FILE*)
void print(const char* format, ...);						// std print to console
void print(const CON_COL col, const char* format, ...);		// std print to console with color
void print_f(const char* format, ...);						// print to console. format. {1} = arg1, {2} = arg2, etc.
void printData(UCHAR* data, int len, bool newLine = true);	// print a char array

void error(const char* format, ...);						// stderr print

int checkSize(const UINT& size);						// check rom size.

void getTimestamp(long long timestamp, char* timestamp_str);	// print timestamp to string.

#endif //XBT_UTIL_H