// util.h: implements various utility functions for manipulating strings, printing to console, changing console foreground color, size checking, etc.

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

// check if a struct is within bounds of a buffer. the struct needs to live within the buffer.
#define IN_BOUNDS(_struct, _buff, _buff_size) ((UCHAR*)_struct >= (UCHAR*)_buff && (UCHAR*)_struct + sizeof(*_struct) < (UCHAR*)_buff + _buff_size)

// console colors
enum CON_COL : int { ERR = 0, OK = 1, MSG = 3, BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

// set console color
void setConsoleColor(const int col = -1);

// format string
void format(char* buffer, const char* format, ...);

// print to stream (stdout, stderr, FILE*)
void print(FILE* stream, const char* format, ...);

// std print to console			
void print(const char* format, ...);

// std print to console with color
void print(const CON_COL col, const char* format, ...);

// print to buffer. format. {1} = arg1, {2} = arg2, etc.
void print_f(char* buffer, const int bufferSize, const char* format, ...);

// print a char array
void printData(UCHAR* data, int len, bool newLine = true);

// stderr print
void error(const char* format, ...);

// check rom size.
int checkSize(const UINT& size);						

// get timestamp string.
void getTimestamp(UINT timestamp, char* timestamp_str);

// trim left side of string.
void ltrim(char*& str);

// trim right side of string.
void rtrim(char*& str);

// pad left side of string.
void lpad(char buff[], const UINT buffSize, const char pad);

// pad right side of string.
void rpad(char buff[], const UINT buffSize, const char pad);

// error exit
void errorExit(const int code, const char* format, ...);

#endif //XBT_UTIL_H
