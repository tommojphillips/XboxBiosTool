// util.cpp: utility functions. printing to console, changing console color, etc.

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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <memory.h>

// user incl
#include "util.h"

void util_setConsoleColor(const int col)
{
	static int console_util_color = 0;

	if (col != 0) {
		console_util_color = 0;
		printf("\033[0m");
	}

	if (col < 0) {
		return;
	}

	console_util_color = col;
	printf("\x1B[%dm", col);
}
void util_setForegroundColor(const int col)
{
	int con_col = col;

	if (col >= 0 && col < CONSOLE_COLOR_BLACK) {
		con_col += 31;
	}

	util_setConsoleColor(con_col);
}
void util_getTimestampStr(size_t timestamp, char* timestamp_str)
{
	if (timestamp_str == NULL)
		return;

	time_t rawtime = (time_t)timestamp;
	struct tm* ptm;
	ptm = gmtime(&rawtime);
	char* time_str = asctime(ptm);

	if (time_str == NULL)
	{
		strcpy(timestamp_str, "0");
	}
	else
	{
		time_str[strlen(time_str) - 1] = '\0';
		strcpy(timestamp_str, time_str);
	}
}

void uprintf(char* data, const size_t size, const char* format, ...)
{
	// find {1}, {2}, etc and replace with the corresponding argument

	const char START_TOKEN = '{';
	const char END_TOKEN = '}';
	const char* formatPtr = format;
	char* bufferPtr = data;
	const char* startToken = NULL;
	const char* endToken = NULL;
	size_t argLen = 0;

	if (data == NULL || format == NULL || size < 1 || strlen(format) < 1)
		return;

	while (1) {
		if (*formatPtr == '\0') // check if we are done
			break;

		// check for token
		if (*formatPtr == START_TOKEN) {
			startToken = formatPtr;
			while (*formatPtr != END_TOKEN) {
				if (*formatPtr == '\0')
					break;
				formatPtr++;
			}

			if (*formatPtr != END_TOKEN)
				break;

			// replace token with argument

			endToken = formatPtr;

			// get the number between the tokens
			int num = 0;
			int tokenLen = (endToken - startToken + 1) - 2; // -2 to remove the '{' and '}'
			char tokenStr[10] = { 0 };

			if (tokenLen + 1 > sizeof(tokenStr)) {
				break;
			}
			strncpy(tokenStr, startToken + 1, tokenLen);
			tokenStr[tokenLen] = '\0';

			num = atoi(tokenStr);
			if (num < 1) {
				break;
			}

			// get the argument based on the number

			char* arg = NULL;
			va_list ap;

			va_start(ap, format);
			for (int i = 0; i < num; ++i) {
				arg = va_arg(ap, char*);
				if (arg == NULL) {
					break;
				}
				argLen = strlen(arg);
			}
			va_end(ap);

			if (argLen > 0) {
				if (bufferPtr + argLen >= data + size) {
					break;
				}
				memcpy(bufferPtr, arg, argLen);
				bufferPtr += argLen;
			}
		}
		else {
			// go find next '{'
			startToken = formatPtr;
			while (*formatPtr != '\0') {
				if (*formatPtr == START_TOKEN) {
					break;
				}
				formatPtr++;
			}

			// copy up to '{'
			argLen = formatPtr - startToken;
			if (argLen > 0) {
				if (bufferPtr + argLen >= data + size)
				{
					break;
				}
				memcpy(bufferPtr, startToken, argLen);
				bufferPtr += argLen;
			}

			if (*formatPtr == '\0') // check if we are done
				break;

			if (*formatPtr != START_TOKEN)
				formatPtr += argLen;
		}

		if (*formatPtr != START_TOKEN)
			formatPtr++;
	}

	bufferPtr[0] = '\0';
}
void uprintc(const int col, const char* format, ...)
{
	util_setForegroundColor(col);
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	util_setConsoleColor(0);
}

void uprinth(const uint8_t* data, const size_t size)
{
	// print hex str
	if (data == NULL || size == 0)
		return;

	for (uint32_t i = 0; i < size; i++) {
		printf("%02X ", data[i]);
	}

	printf("\n");
}
void uprinta(const uint8_t* data, const size_t size, const int new_line)
{
	// print ascii str
	if (data == NULL || size == 0)
		return;

	for (uint32_t k = 0; k < size; ++k) {
		if ((data[k] >= 0x30 && data[k] < 0x39) ||
			(data[k] >= 50 && data[k] < 132)) {
			printf("%c", data[k]);
		}
		else {
			printf(".");
		}
	}

	if (new_line)
		printf("\n");
}

void uprinthl(const uint8_t* data, const size_t size, uint32_t per_line, const char* prefix, const int ascii)
{
#define HEX_STR_LEN(x) ((x)*3)
#define MAX_BYTES_PER_LINE		32
#define DEFAULT_BYTES_PER_LINE	8

	char line[((MAX_BYTES_PER_LINE * 3) + 1) + MAX_BYTES_PER_LINE] = { 0 };

	if (per_line == 0 || per_line > MAX_BYTES_PER_LINE - 1)
		per_line = DEFAULT_BYTES_PER_LINE;

	uint32_t j = 0;
	for (uint32_t i = 0; i < size; i += per_line) {
		if (prefix != NULL) {
			printf(prefix);
		}
		for (j = 0; j < per_line; ++j) {
			if (i + j >= size) {
				sprintf(line + HEX_STR_LEN(j), "%*c", HEX_STR_LEN(per_line - j), ' ');
				break;
			}
			sprintf(line + HEX_STR_LEN(j), "%02X ", data[i + j]);
		}
		printf("%s", line);

		if (ascii)
			uprinta(data + i, j, 1);
		else
			printf("\n");
	}
}
