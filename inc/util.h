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

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

// pointer-of-struct bounds check
#define IN_BOUNDS(struc, buff, size) ((uint8_t*)struc >= (uint8_t*)buff && (uint8_t*)struc + sizeof(*struc) < (uint8_t*)buff + size)

// pointer-of-block bounds check
#define IN_BOUNDS_BLOCK(block, blockSize, buff, buffSize) ((uint8_t*)block >= (uint8_t*)buff && (uint8_t*)block + blockSize < (uint8_t*)buff + buffSize)

// console colors
#define CONSOLE_COLOR_ERR 0
#define CONSOLE_COLOR_OK 1
#define CONSOLE_COLOR_MSG 3
#define CONSOLE_COLOR_BLACK 30
#define CONSOLE_COLOR_RED 31
#define CONSOLE_COLOR_RED 31
#define CONSOLE_COLOR_GREEN 32
#define CONSOLE_COLOR_YELLOW 33
#define CONSOLE_COLOR_BLUE 34
#define CONSOLE_COLOR_MAGENTA 35
#define CONSOLE_COLOR_CYAN 36
#define CONSOLE_COLOR_WHITE 37

#define ERROR_SUCCESS 0
#define ERROR_FAILED 1
#define ERROR_BUFFER_OVERFLOW 4
#define ERROR_OUT_OF_MEMORY 5
#define ERROR_INVALID_DATA 6

#ifdef __cplusplus
extern "C" {
#endif

// set console color
void util_setConsoleColor(const int col);
void util_setForegroundColor(const int col);

// get timestamp string.
void util_getTimestampStr(const size_t timestamp, char* timestamp_str);

// std print to console with color
void uprintc(const int col, const char* format, ...);

// print to buffer. format. {1} = arg1, {2} = arg2, etc.
void uprintf(char* data, const size_t size, const char* format, ...);

// print a byte array as hex; single line.
// data: buffer
// size: buffer size
// new_line: output new line
void uprinth(const uint8_t* data, const size_t size);

// print byte array as ascii; single line.
// data: buffer
// size: buffer size
// new_line: output new line
void uprinta(const uint8_t* data, const size_t size, const int new_line);

// print byte array as hex; multi line
// data: buffer
// size: buffer size
// per_line: num of hex digits per line. eg 16. -> xx xx xx xx ... x16
void uprinthl(const uint8_t* data, const size_t size, uint32_t per_line, const char* prefix, const int ascii);

#ifdef __cplusplus
};
#endif

#endif //UTIL_H
