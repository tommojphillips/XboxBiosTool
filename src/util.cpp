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
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

// user incl
#include "util.h"
#include "type_defs.h"
#include "xbmem.h"

static int console_util_color = 0;

void setConsoleColor(const int col)
{
	if (col != 0) // reset col.
	{
		console_util_color = 0;
		printf("\033[0m");
	}

	if (col < 0) // don't set col if less than 0.
	{
		return;
	}

	// set col.
	console_util_color = col;
	printf("\x1B[%dm", col);
}
void setForegroundColor(const CON_COL col)
{
	int con_col = col;

	if (col >= 0 && col < CON_COL::BLACK)
	{
		con_col += 31;
	}

	setConsoleColor((CON_COL)con_col);
}

void printData(UCHAR* data, const int len, bool newLine)
{
	if (data == NULL || len <= 0)
		return;

    for (int i = 0; i < len; i++)
    {
		print("%02X ", data[i]);
	}

    if (newLine)
    {
		print("\n");
	}
}

void print_f(char* const buffer, const int bufferSize, const char* format, ...)
{
	// find {1}, {2}, etc and replace with the corresponding argument

	const char START_TOKEN = '{';
	const char END_TOKEN = '}';

	const char* formatPtr = format;	
	char* bufferPtr = buffer;

	const char* startToken = NULL;
	const char* endToken = NULL;
		
	UINT argLen = 0;
	
	if (buffer == NULL || format == NULL || bufferSize < 1 || strlen(format) < 1)
		return;

	while (true)
	{
		if (*formatPtr == '\0') // check if we are done
			break;

		// check for token
		if (*formatPtr == START_TOKEN)
		{
			startToken = formatPtr;
			while (*formatPtr != END_TOKEN)
			{
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
			char tokenStr[10] = {0};
			xb_set(tokenStr, 0xCC, sizeof(tokenStr));

			if (tokenLen + 1 > sizeof(tokenStr))
			{
				break;
			}
			strncpy(tokenStr, startToken + 1, tokenLen);
			tokenStr[tokenLen] = '\0';

			num = atoi(tokenStr);
			if (num < 1)
			{
				break;
			}

			// get the argument based on the number

			char* arg = NULL;
			va_list ap;

			va_start(ap, format);
			for (int i = 0; i < num; ++i)
			{
				arg = va_arg(ap, char*);
				if (arg == NULL)
				{
					break;
				}
				argLen = strlen(arg);
			}
			va_end(ap);			

			if (argLen > 0)
			{
				if (bufferPtr + argLen >= buffer + bufferSize)
				{
					break;
				}				
				xb_cpy(bufferPtr, arg, argLen);
				bufferPtr += argLen;
			}
		}
		else
		{
			// go find next '{'
			startToken = formatPtr;
			while (*formatPtr != '\0')
			{
				if (*formatPtr == START_TOKEN)
				{
					break;
				}
				formatPtr++;
			}		

			// copy up to '{'
			argLen = formatPtr - startToken;
			if (argLen > 0)
			{
				if (bufferPtr + argLen >= buffer + bufferSize)
				{
					break;
				}
				xb_cpy(bufferPtr, startToken, argLen);
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

	// null terminate the buffer
	bufferPtr[0] = '\0';
}
void format(char* buffer, const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
}

void print(FILE* stream, const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stream, format, args);
	va_end(args);
}
void print(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
}
void print(const CON_COL col, const char* format, ...)
{
	setForegroundColor(col);

	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);

	setConsoleColor(0);
}
void error(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

int checkSize(const UINT& size)
{
	switch (size)
	{
		case 0x40000U:
		case 0x80000U:
		case 0x100000U:
			return 0;
	}
	return 1;
}

void getTimestamp(UINT timestamp, char* timestamp_str)
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
		return;
	}
	time_str[strlen(time_str) - 1] = '\0';
	strcpy(timestamp_str, time_str);
}

void ltrim(char*& str)
{
	if (str == NULL)
		return;

	while (*str == ' ' || *str == '\t')
	{
		str++;
	}
}
void rtrim(char*& str)
{
	if (str == NULL)
		return;

	int len = strlen(str);
	if (len == 0)
		return;

	char* end = str + len - 1;
	while (end > str && (*end == ' ' || *end == '\t'))
	{
		end--;
	}
	*(end + 1) = '\0';
}
void lpad(char buff[], const UINT buffSize, const char pad)
{
	if (buff == NULL)
		return;

	// pad left
	UINT slen = strlen(buff);
	if (slen >= buffSize)
		return;

	// shift right
	for (UINT i = buffSize - 1; i >= slen; i--)
	{
		buff[i] = buff[i - buffSize + slen];
	}

	// pad left
	for (UINT i = 0; i < buffSize - slen; i++)
	{
		buff[i] = pad;
	}
}
void rpad(char buff[], const UINT buffSize, const char pad)
{
	if (buff == NULL)
		return;

	UINT slen = strlen(buff);
	xb_set(buff + slen, ' ', buffSize - slen - 1);
}

void errorExit(const int code, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	exit(code);
}
