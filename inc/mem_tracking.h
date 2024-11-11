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

#define MEM_TRACKING
#ifdef NO_MEM_TRACKING
#undef MEM_TRACKING
#endif

#ifndef MEM_TRACKING_H
#define MEM_TRACKING_H
#ifdef MEM_TRACKING

#include <stdio.h>

extern int memtrack_allocations;
extern long memtrack_allocatedBytes;

#ifdef __cplusplus
extern "C" {
#endif

	void* memtrack_malloc(size_t size);
	void* memtrack_realloc(void* ptr, size_t size);
	void* memtrack_calloc(size_t count, size_t size);
	void memtrack_free(void* ptr);
	int memtrack_report();

#ifdef __cplusplus
};
#endif

#define malloc(size) memtrack_malloc(size)
#define free(ptr) memtrack_free(ptr)
#define realloc(ptr, size) memtrack_realloc(ptr, size)
#define calloc(count, size) memtrack_calloc(count, size)

#endif // !MEM_TRACKING
#endif // !MEM_TRACKING_H
