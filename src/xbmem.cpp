// xbmem.cpp: defines functions for memory allocation and manipulation

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
#include <cstdio>
#include <malloc.h>
#include <memory.h>

// user incl
#include "util.h"
#include "xbmem.h"

int XbMem::checkForLeaks()
{
	// should be invoked at the end of the program when all memory has been freed
	// if there are any leaks, print the total allocated bytes and allocations
	// otherwise, print "No leaks detected"

	if (_curAllocatedBytes != 0 || _curAllocations != 0)
	{
		_isPrinting = true;
		print(CON_COL::RED, "LEAK DETECTED: %ld bytes in %d allocations\n", _curAllocatedBytes, _curAllocations);
		return 1;
	}
	else if (_isPrinting)
	{
		printf("No leaks detected\n");
	}
	return 0;
}

long XbMem::getAllocated() const { return _curAllocatedBytes; };
int XbMem::getAllocations() const { return _curAllocations; };

void* XbMem::xb_alloc(UINT size)
{
	void* ptr = malloc(size);
	if (ptr == NULL)
	{
		printf("Error: Could not allocate %d bytes\n", size);
		return NULL;
	}

	memset(ptr, 0, size);

	_curAllocations++;
	_curAllocatedBytes += size;

if (_isPrinting)
	printf("Allocated %d bytes. total alloc: %ld bytes\n", size, _curAllocatedBytes);

	return ptr;
}
void* XbMem::xb_realloc(void* ptr, UINT size)
{
	UINT oldSize = _msize(ptr);

	void* newPtr = realloc(ptr, size);
	if (newPtr == NULL)
	{
		printf("Error: Could not reallocate %d bytes\n", size);
		return NULL;
	}

	_curAllocatedBytes -= oldSize;
	_curAllocatedBytes += size;

if (_isPrinting)
	printf("Reallocated %d bytes. total alloc: %ld bytes\n", size, _curAllocatedBytes);

	return newPtr;
}
void XbMem::xb_free(void* ptr)
{
	UINT size = _msize(ptr);

	free(ptr);
	ptr = NULL; // clean up null ptr

	_curAllocations--;
	_curAllocatedBytes -= size;

if (_isPrinting)
	printf("Freed %d bytes. total alloc: %ld bytes\n", size, _curAllocatedBytes);
}

// static functions for memory allocation. wraps global xbmem instance
void* xb_alloc(UINT size)
{
	return XbMem::getInstance().xb_alloc(size);
}
void* xb_realloc(void* ptr, UINT size)
{
	return XbMem::getInstance().xb_realloc(ptr, size);
}
void xb_free(void* ptr) 
{
	XbMem::getInstance().xb_free(ptr);
}

int xb_leaks()
{
	return XbMem::getInstance().checkForLeaks();
}
