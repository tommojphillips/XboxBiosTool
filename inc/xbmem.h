// xbmem.h

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

#ifndef XBT_MEM_H
#define XBT_MEM_H

#include "type_defs.h"

// if defined, prints memory allocation info
//#define XBT_MEM_ALLOC_PRINT 1

// Memory allocation class
class XbMem
{
public:
	static XbMem& getInstance()
	{
		static XbMem instance;
		return instance;
	}
	int checkForLeaks();
	
	long getAllocated() const;
	int getAllocations() const;
	
	void* xb_alloc(UINT size);
	void* xb_realloc(void* ptr, UINT size);	
	void xb_free(void* ptr);

private:
	XbMem()  { };
	~XbMem() { };

	// current allocated bytes
	long _curAllocatedBytes = 0;
	// current number of allocations
	int _curAllocations = 0;

#ifdef XBT_MEM_ALLOC_PRINT
	bool _isPrinting = true;	// start off printing
#else
	bool _isPrinting = false;	// start off not printing
#endif
};

// allocate memory
void* xb_alloc(UINT size);
// reallocate memory. if ptr canot be reallocated, NULL is returned
void* xb_realloc(void* ptr, UINT size);
// free memory allocated by xb_alloc
void xb_free(void* ptr);

// check for memory leaks
int xb_leaks();

#endif //XBT_MEM_H

