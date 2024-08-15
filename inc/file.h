// file.h

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

#ifndef XB_FILE_H
#define XB_FILE_H

// std incl
#include <cstdio>

// user incl
#include "type_defs.h"

// read a file. allocates memory for the buffer.
// filename: the absolute path to the file.
// bytesRead: if not NULL, will store the number of bytes read.
// expectedSize: if not -1, will check the file size against this value and return NULL if they don't match.
// returns the buffer if successful, NULL otherwise.
UCHAR* readFile(const char* filename, UINT* bytesRead, const UINT expectedSize = 0);

// write to a file.
// filename: the absolute path to the file.
// ptr: the buffer to write from.
// bytesToWrite: the number of bytes to write to the file.
// returns 0 if successful, 1 otherwise.
int writeFile(const char* filename, void* ptr, const UINT bytesToWrite);

// deletes a file.
// filename: the absolute path to the file.
// returns 0 if successful, 1 otherwise.
int deleteFile(const char* filename);

// gets the size of a file.
// file: the file to get the size of.
// fileSize: if not NULL, will store the file size.
// returns 0 if successful, 1 otherwise.
int getFileSize(FILE* file, UINT* fileSize);

#endif