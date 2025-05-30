// mem_tracking.c

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

#if !__APPLE__
#include <malloc.h>
#endif
#include <cstdio>
#include <map>

//#define MEM_TRACKING_PRINT

static long memtrack_allocatedBytes = 0;
static int memtrack_allocations = 0;

static std::map<void*, size_t> allocation_map;

extern "C" void* memtrack_malloc(size_t size)
{
	void* ptr = malloc(size);
	if (ptr == NULL) {
#ifdef MEM_TRACKING_PRINT
		printf("Error: could not allocate %d bytes\n", size);
#endif
		return NULL;
	}

	allocation_map[ptr] = size;
	memtrack_allocations++;
	memtrack_allocatedBytes += size;

#ifdef MEM_TRACKING_PRINT
	printf("allocated %d bytes\n", size);
#endif
	return ptr;
}

extern "C" void memtrack_free(void* ptr)
{
#ifdef MEM_TRACKING_PRINT
	if (ptr == NULL) {
		printf("Error: attempting to free NULL pointer\n");
		return;
	}
#endif

	auto entry = allocation_map.find(ptr);
	if (entry == allocation_map.end()) {
		printf("Error untracked free 0x%p.\n", ptr);
	}
	size_t size = entry->second;
	free(ptr);
	allocation_map.erase(entry);

	memtrack_allocations--;
	memtrack_allocatedBytes -= size;

	if (memtrack_allocations < 0) {
		printf("Error neg allocations.\n");
	}

	if (memtrack_allocatedBytes < 0) {
		printf("Error neg allocated bytes.\n");
	}

#ifdef MEM_TRACKING_PRINT
	printf("freed %d bytes\n", size);
#endif
}

extern "C" void* memtrack_realloc(void* ptr, size_t size)
{
	auto entry = allocation_map.find(ptr);
	if (entry == allocation_map.end()) {
		printf("Error untracked free 0x%p.\n", ptr);
	}
	size_t oldSize = entry->second;
	void* newPtr = realloc(ptr, size);
	allocation_map.erase(entry);
	allocation_map.insert(std::make_pair(newPtr, size));

	if (newPtr == NULL)	{
#ifdef MEM_TRACKING_PRINT
		printf("Error: could not reallocate %d bytes\n", size);
#endif
		return NULL;
	}

	memtrack_allocatedBytes -= oldSize;
	memtrack_allocatedBytes += size;

#ifdef MEM_TRACKING_PRINT
	printf("reallocated %u -> %u ( %d bytes )\n", oldSize, size, (size - oldSize));
#endif

	return newPtr;
}

extern "C" void* memtrack_calloc(size_t count, size_t size)
{
	if (count == 0 || size == 0)
		return NULL;

	void* ptr = calloc(count, size);
	if (ptr == NULL) {
#ifdef MEM_TRACKING_PRINT
		printf("Error: could not allocate %d bytes\n", size);
#endif
		return NULL;
	}

	allocation_map[ptr] = size;
	memtrack_allocations++;
	memtrack_allocatedBytes += size;

#ifdef MEM_TRACKING_PRINT
	printf("allocated %d bytes.\n", size);
#endif

	return ptr;
}

extern "C" int memtrack_report()
{
	if (memtrack_allocatedBytes != 0 || memtrack_allocations != 0) {
		printf("\nLEAK DETECTED: %ld bytes in %d allocations\n", memtrack_allocatedBytes, memtrack_allocations);
		return 1;
	}

#ifdef MEM_TRACKING_PRINT
	printf("\nno leaks detected\n");
#endif

	return 0;
}
