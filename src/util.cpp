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
#include <memory.h>

// user incl
#include "util.h"
#include "type_defs.h"

static int console_util_color = 0;
static int _g_error_code = XB_ERROR_CODES::ERROR_SUCCESS;

int getErrorCode()
{
	return _g_error_code;
}
void setErrorCode(const int code)
{
	_g_error_code = code;
}

void setConsoleColor(const int col)
{
	if (col != 0)
	{
		console_util_color = 0;
		printf("\033[0m");
	}

	if (col < 0)
	{
		return;
	}

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
		printf("%02X ", data[i]);
	}

    if (newLine)
    {
		printf("\n");
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
			
			if (tokenLen + 1 > sizeof(tokenStr))
			{
				setErrorCode(ERROR_BUFFER_OVERFLOW);
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
					setErrorCode(ERROR_BUFFER_OVERFLOW);
					break;
				}
				memcpy(bufferPtr, arg, argLen);
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
					setErrorCode(ERROR_BUFFER_OVERFLOW);
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

void print(const CON_COL col, const char* format, ...)
{
	setForegroundColor(col);

	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);

	setConsoleColor(0);
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

	while (*str == ' ' || *str == '\t' || *str == '\n')
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
	while (end > str && (*end == ' ' || *end == '\t' || *end == '\n'))
	{
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
