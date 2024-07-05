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
void printData(UCHAR* data, const int len, bool newLine)
{
    if (data == NULL)
    {
		error("Error: data is NULL\n");
		return;
	}
    if (len <= 0)
    {
		error("Error: len is less than or equal to 0\n");
		return;
	}

    for (int i = 0; i < len; i++)
    {
		print("%02X ", data[i]);
	}

    if (newLine)
    {
		print("\n");
	}
}

void print_f(const char* format, ...)
{
	// find {1}, {2}, etc and replace with the corresponding argument
	const char START_TOKEN = '{';
	const char END_TOKEN = '}';
	const int BUF_SIZE = 8;

	char msg[BUF_SIZE] = { 0 };
	int len = strlen(format);

	char* p = (char*)format; // set pointer to the start of the format string

	char* startToken = NULL;
	char* endToken = NULL;

	char* argPtr = NULL;

	int curIter = -1;
	while (++curIter < BUF_SIZE - 1)
	{
		if (*p == '\0')
		{
			break;
		}

		if (BUF_SIZE - 1 <= curIter)
		{
			break;
		}

		// check for token
		if (*p == START_TOKEN)
		{
			startToken = p;
			while (*p != END_TOKEN)
			{
				if (*p == '\0')
				{
					break;
				}

				p++;
			}

			if (*p != END_TOKEN)
			{
				break;
			}

			endToken = p;

			// replace token with argument

			// get the number between the tokens
			int num = 0;
			char* token = startToken + 1;
			int tokenLen = endToken - token;
			char tokenStr[10] = { "\0" };

			strncpy(tokenStr, token, tokenLen);
			tokenStr[9] = '\0';

			num = atoi(tokenStr);

			// get the argument based on the number

			char* arg = NULL;
			int argLen = 0;
			va_list ap;

			if (argPtr == NULL)
			{
				va_start(ap, format);
				for (int i = 0; i < num; ++i)
				{
					if (i == num - 1)
					{
						arg = va_arg(ap, char*);
						argLen = strlen(arg);
						break;
					}
				}
				va_end(ap);
			}
			else
			{
				arg = argPtr;
				argLen = strlen(arg);
			}

			if (argLen > 0)
			{
				if (argLen > BUF_SIZE - curIter - 1)
				{
					argLen = BUF_SIZE - curIter - 1;
					argPtr = arg + argLen;
					msg[curIter] = '\0';
					p = startToken - 1;
				}
				else
				{
					argPtr = NULL;
				}

				xb_cpy(msg + curIter, arg, argLen);

				curIter += argLen;
			}
		}
		else
		{
			// copy the character
			msg[curIter] = *p;
		}

		// move to the next character
		p++;

		// print the message
		print(msg);

		if (*p == '\0')
		{
			break;
		}

		curIter = -1;
		xb_zero(msg, BUF_SIZE);
	}
}

void format(char* buffer, const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
}

void print(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
}

void error(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
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

void print(const CON_COL col, const char* format, ...)
{
	setForegroundColor(col);

	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);

	setConsoleColor(0);
}
void error(const CON_COL col, const char* format, ...)
{
	setForegroundColor(col);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);

	setConsoleColor(0);
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

void getTimestamp(unsigned int timestamp, char* timestamp_str)
{
	if (timestamp_str == NULL)
	{
		//error("Error: timestamp_str is NULL\n");
		return;
	}
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
