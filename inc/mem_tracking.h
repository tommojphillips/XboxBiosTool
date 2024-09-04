// mem_tracking.h

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
 * along with this program.If not, see < https://www.gnu.org/licenses/>. */

 // Author: tommojphillips
 // GitHub: https:\\github.com\tommojphillips

#ifdef NO_MEM_TRACKING
#undef MEM_TRACKING
#else
#ifndef MEM_TRACKING
#define MEM_TRACKING
#endif
#endif

#ifndef MEM_TRACKING_H
#define MEM_TRACKING_H
#ifdef MEM_TRACKING

#include <malloc.h>

class MemTracking
{
public:
	static MemTracking& getInstance() {
		static MemTracking instance;
		return instance;
	};
	void* memtrack_malloc(size_t size) {
		void* ptr = malloc(size);
		if (ptr == NULL)
		{
			printf("Error: Could not allocate %d bytes\n", size);
			return NULL;
		}

		memtrack_allocations++;
		memtrack_allocatedBytes += (long)size;

		if (memtrack_isPrinting) printf("Allocated %d bytes. total alloc: %ld bytes\n", size, memtrack_allocatedBytes);

		return ptr;
	};
	void memtrack_free(void* ptr) {
		if (ptr == NULL)
			return;

		size_t size = _msize(ptr);
		free(ptr);

		memtrack_allocations--;
		memtrack_allocatedBytes -= size;
		if (memtrack_isPrinting) printf("Freed %d bytes. total alloc: %ld bytes\n", size, memtrack_allocatedBytes);
	};
	void* memtrack_realloc(void* ptr, size_t size) {
		size_t oldSize = _msize(ptr);
		void* newPtr = realloc(ptr, size);
		if (newPtr == NULL)
		{
			printf("Error: Could not reallocate %d bytes\n", size);
			return NULL;
		}

		memtrack_allocatedBytes -= oldSize;
		memtrack_allocatedBytes += size;
		if (memtrack_isPrinting) printf("Reallocated %d bytes. total alloc: %ld bytes\n", size, memtrack_allocatedBytes);

		return newPtr;
	};
	void* memtrack_calloc(size_t count, size_t size)
	{
		if (count == 0 || size == 0)
			return NULL;

		void* ptr = calloc(count, size);
		if (ptr == NULL)
		{
			printf("Error: Could not allocate %d bytes\n", size);
			return NULL;
		}

		memtrack_allocations++;
		memtrack_allocatedBytes += size;
		if (memtrack_isPrinting) printf("Allocated %d bytes. total alloc: %ld bytes\n", size, memtrack_allocatedBytes);

		return ptr;
	}
	int memtrack_report() {
		if (memtrack_allocatedBytes != 0 || memtrack_allocations != 0)
		{
			memtrack_isPrinting = true;
			printf("\nLEAK DETECTED: %ld bytes in %d allocations\n", memtrack_allocatedBytes, memtrack_allocations);
			return 1;
		}

		if (memtrack_isPrinting) printf("\nNo leaks detected\n");
		return 0;
	}
private:
	MemTracking() { };
	~MemTracking() { };

	long memtrack_allocatedBytes = 0;
	int memtrack_allocations = 0;
	bool memtrack_isPrinting = false;
};

#define malloc(size) MemTracking::getInstance().memtrack_malloc(size)
#define free(ptr) MemTracking::getInstance().memtrack_free(ptr)
#define realloc(ptr, size) MemTracking::getInstance().memtrack_realloc(ptr, size)
#define calloc(count, size) MemTracking::getInstance().memtrack_calloc(count, size)
#define mem_report() MemTracking::getInstance().memtrack_report()

#else

#define mem_report()

#endif // !MEM_TRACKING
#endif // !MEMORY_TRACKING_H
