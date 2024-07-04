// file.h

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

#ifndef XB_FILE_H
#define XB_FILE_H

// std incl
#include <stdio.h>

// user incl
#include "type_defs.h"

// read a file. allocates memory for the buffer.
// filename: the absolute path to the file.
// bytesRead: if not NULL, will store the number of bytes read.
// expectedSize: if not -1, will check the file size against this value and return NULL if they don't match.
// returns the buffer if successful, NULL otherwise.
UCHAR* readFile(const char* filename, UINT* bytesRead, const int expectedSize = -1);

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