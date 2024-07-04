// util.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_UTIL_H
#define XB_UTIL_H

#include "type_defs.h"

#define SUCCESS(x) (x == 0 ? true : false)

// console colors
enum CON_COL : int { ERR = 0, OK = 1, MSG = 3, BLACK = 30, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };

void setConsoleColor(const int col = -1);						// set console foreground color.
void print_f(const char* format, ...);							// print to console. format. {1} = arg1, {2} = arg2, etc.
void format(char* buffer, const char* format, ...);				// format a string. same as sprintf.
void printData(UCHAR* data, int len, bool newLine = true);		// print a char array

void print(const char* format, ...);						// std print to console
void error(const char* format, ...);						// error print to console

void print(const CON_COL col, const char* format, ...);		// std print to console with color
void error(const CON_COL col, const char* format, ...);		// error print to console with color

int checkSize(const UINT& size);							// check rom size.

void getTimestamp(unsigned int timestamp, char* timestamp_str);	// print timestamp to string.

#endif //XBT_UTIL_H