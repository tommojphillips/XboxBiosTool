// xbmem.cpp: defines functions for memory allocation and manipulation

// Author: tommojphillips
// GitHub: https:\\github.com\tommojphillips

// std incl
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
		error(CON_COL::RED, "LEAK DETECTED: %ld bytes in %d allocations\n", _curAllocatedBytes, _curAllocations);
		return 1;
	}
	else if (_isPrinting)
	{
		print("No leaks detected\n");
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
		error("Error: Could not allocate %d bytes\n", size);
		return NULL;
	}

	// Zero out the memory
	xb_zero(ptr, size);

	_curAllocations++;
	_curAllocatedBytes += size;

if (_isPrinting)
	print("Allocated %d bytes. total alloc: %ld bytes\n", size, _curAllocatedBytes);

	return ptr;
}
void* XbMem::xb_realloc(void* ptr, UINT size)
{
	if (ptr == NULL)
	{
		error("Error: ptr is NULL\n");
		return NULL;
	}

	UINT oldSize = _msize(ptr);

	void* newPtr = realloc(ptr, size);
	if (newPtr == NULL)
	{
		error("Error: Could not reallocate %d bytes\n", size);
		return NULL;
	}

	_curAllocatedBytes -= oldSize;
	_curAllocatedBytes += size;

if (_isPrinting)
	print("Reallocated %d bytes. total alloc: %ld bytes\n", size, _curAllocatedBytes);

	return newPtr;
}
void XbMem::xb_free(void* ptr)
{
	if (ptr == NULL)
	{
		error("Error: ptr is NULL\n");
		return;
	}

	UINT size = _msize(ptr);

	free(ptr);
	ptr = NULL; // clean up null ptr

	_curAllocations--;
	_curAllocatedBytes -= size;

if (_isPrinting)
	print("Freed %d bytes. total alloc: %ld bytes\n", size, _curAllocatedBytes);
}
void XbMem::xb_zero(void* ptr, UINT size)
{
	memset(ptr, 0, size);
}
void XbMem::xb_set(void* ptr, int val, UINT size)
{
	memset(ptr, val, size);
}
void XbMem::xb_cpy(void* dest, const void* src, UINT size)
{
	memcpy(dest, src, size);
}
void XbMem::xb_mov(void* dest, void* src, UINT size)
{
	memmove(dest, src, size);
}
int XbMem::xb_cmp(const void* ptr1, const void* ptr2, UINT size)
{
	return memcmp(ptr1, ptr2, size);
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
void xb_zero(void* ptr, UINT size)
{
	XbMem::getInstance().xb_zero(ptr, size);
}
void xb_set(void* ptr, int val, UINT size)
{
	XbMem::getInstance().xb_set(ptr, val, size);
}
void xb_cpy(void* dest, const void* src, UINT size)
{
	XbMem::getInstance().xb_cpy(dest, src, size);
}
void xb_mov(void* dest, void* src, UINT size)
{
	XbMem::getInstance().xb_mov(dest, src, size);
}
int xb_cmp(const void* ptr1, const void* ptr2, UINT size)
{
	return XbMem::getInstance().xb_cmp(ptr1, ptr2, size);
}
int xb_leaks()
{
	return XbMem::getInstance().checkForLeaks();
}